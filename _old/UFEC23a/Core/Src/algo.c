#include "ParamFile.h"
#include "air_input.h"
#include "slope.h"
#include "cmsis_os.h"
#include "main.h"
#include "DebugPort.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "Pid.h"
#include "ProdTest.h"
#include "MotorManager.h"
#include "ParticlesManager.h"
#include "Hmi.h"
#include "algo.h"



#define ColdStoveTemp 900 //90F


//Flame loss parameters:
#define  RFlameLossB  500 // 50F Loss of temperature per R time to go in flame loss
#define  RFlameLossR	 1400 // 140F Loss of temperature per R time to go in flame loss
#define TRFlameLoss	   1 // 3 minutes time in minutes of data acquisition for R calculation


//state machine variable and initial values
static State currentState = ZEROING_STEPPER;
static bool reloadingEvent = false;
static bool errorFlag = false;
bool fanPauseRequired = false;
static AirInput primary = AirInput_init(PF_PRIMARY_MINIMUM_OPENING, PF_PRIMARY_FULL_OPEN);
static AirInput grill = AirInput_init(PF_GRILL_MINIMUM_OPENING, PF_GRILL_FULL_OPEN);
static AirInput secondary = AirInput_init(PF_SECONDARY_MINIMUM_OPENING, PF_SECONDARY_FULL_OPEN);

static Algo_DELState delLoadingEnd = ALGO_DEL_OFF;
static Algo_DELState delFermeturePorte = ALGO_DEL_OFF;

static int baffleTemperature = 0;   // [Tenth *F]
static int frontTemperature = 0; // [Tenth *F]
static int plenumTemp = 0;  // [Tenth *F]
static bool thermostatRequest = false;
static bool interlockRequest = false;

//Data for derivative computation
#define NB_DATA  300  // 5 minutes max si une mesure par sec
static int frontTempDataStore[NB_DATA]; //Data for slope calculation
static int baffleTempDataStore[NB_DATA];
static int frontAccelDataStore[NB_DATA]; //Data for slope calculation
static int baffleAccelDataStore[NB_DATA];
static int lastBaffleTemp;


static bool simulatorMode = false;
static Slope slopeBaffleTemp;
static Slope slopeFrontTemp;
static Slope accelBaffleTemp;
static Slope accelFrontTemp;
float Algo_Simulator_slopeBaffleTemp = 0.0;
float Algo_slopeBaffleTemp = 0.0;
float Algo_slopeFrontTemp = 0.0;
static uint32_t timeSinceStateEntry;
static uint32_t TimeOfReloadRequest;
static uint32_t TimeSinceEntryInCombLow = 0;


//Novika parameters Kc=0.500, Ti=0.200, Td = 0.002 (pas utilisé)
//Kp = Kc
//Kd = Kc*Td*det/dt
//Ki = Kc/Ti
#define Kc 0.080
#define Ti 0.200
#define Td 0.040
#define Kp Kc
#define Kd 0.020
#define Ki 0.00
#define PID_CONTROL_ON false
#define PID_UPDATE_PERIOD_MS 5000
static PIDtype TemperaturePID;
int16_t PIDTrapPosition = 0;

//private functions
static inline bool inBetween( int var, int low, int high);
static float computeSlopeBaffleTemp(unsigned int nbData);
static float computeslopeFrontTemp(unsigned int nbData);
void Algo_init();
static int computeAjustement( int tempTarget_tenthF, float dTempAvant_FperS);
static void StateEntryControlAdjustment(uint8_t MinPrimary, uint8_t MaxPrimary,
		uint8_t MinGrill, uint8_t MaxGrill,
		uint8_t MinSecondary, uint8_t MaxSecondary);
static void AirAdjustment(int adjustement, float secondPerStep,
		uint8_t MinPrimary, uint8_t MaxPrimary,
		uint8_t MinGrill, uint8_t MaxGrill,
		uint8_t MinSecondary, uint8_t MaxSecondary);


void Algo_init() {

  if(GPIO_PIN_SET==HAL_GPIO_ReadPin(Button_Input_GPIO_Port,Button_Input_Pin))
  {
	  currentState = PRODUCTION_TEST;
  }
  else
  {
	  currentState = ZEROING_STEPPER;
  }

  reloadingEvent = false;
  AirInput_forceAperture(&primary, PF_PRIMARY_CLOSED);
  AirInput_forceAperture(&grill, PF_GRILL_CLOSED);
  AirInput_forceAperture(&secondary, PF_SECONDARY_CLOSED);
  baffleTemperature = 0;
  frontTemperature = 0;
  thermostatRequest = false;
  delLoadingEnd = ALGO_DEL_OFF;
  delFermeturePorte = ALGO_DEL_OFF;

  Slope_init(&slopeBaffleTemp, baffleTempDataStore, NB_DATA, SAMPLING_RATE);
  Slope_init(&accelBaffleTemp, baffleAccelDataStore, NB_DATA, SAMPLING_RATE);
  Slope_init(&slopeFrontTemp, frontTempDataStore, NB_DATA, SAMPLING_RATE);
  Slope_init(&accelFrontTemp, frontAccelDataStore, NB_DATA, SAMPLING_RATE);


}

static void manageStateMachine(uint32_t currentTime_ms) {

	const PF_MotorOpeningsParam_t* pGrillMotorParam = PB_GetGrillMotorParam();
	const PF_MotorOpeningsParam_t* pPrimaryMotorParam = PB_GetPrimaryMotorParam();
	const PF_MotorOpeningsParam_t* pSecondaryMotorParam = PB_GetSecondaryMotorParam();
	const PF_CombTempParam_t* pTemperatureParam = PB_GetTemperatureParam();
	const PF_UsrParam* pUserParam = PB_GetUserParam();

	  State nextState = currentState;
	  static int adjustement = 0;

	  static State historyState = ZEROING_STEPPER;
	  static ParticleAction action = WAIT;
	  static uint32_t stateChangeTimeRef = 0;
	  static uint32_t timeRefAutoMode = 0;
	  static int targetTemperature = 0;
	  static uint32_t Safetydebounce_ms = 0;
	  static uint32_t partComeback_timer_ms = 0;
	  float dTbaffle;
	  float dTavant;

	  //flameloss variable
	  static uint32_t timer_flameloss = 0;
	  static int TFlameLossArrayB[4] = {0};
	  static int TFlameLossArrayR[4] = {0};
	  static int R_flamelossB = 0;
	  static int R_flamelossR = 0;


	  const uint32_t SEC_PER_STEP_TEMP_RISE = 6;
	  const uint32_t SEC_PER_STEP_COMB_LOW = 10;
	  const uint32_t SEC_PER_STEP_COMB_HIGH = 12;
	  const uint32_t SEC_PER_STEP_COAL_HIGH = 12;
	  float sec_per_step = 0.0;

	  manageFans(Algo_getBaffleTemp(),pUserParam);


	  //calculate time used in the state transition.
	  timeSinceStateEntry = currentTime_ms - stateChangeTimeRef;
	  uint32_t timeInTemperatureRise = 0;
#if PID_CONTROL_ON
	  static uint32_t Pidtimeref = 0;
	  uint32_t TimeSinceLastPIDUpdate = currentTime_ms - Pidtimeref;
#endif
	  uint32_t TimeForStep = currentTime_ms - timeRefAutoMode;

	  // TODO: la periode utilisée pour le calcule de la pente n'est pas définie
  //       dans le document
  dTbaffle = computeSlopeBaffleTemp(6); //�tait 300, selon ce que Novika utilise test du 2019-12-04.

  dTbaffle = (float)(baffleTemperature-lastBaffleTemp)/50;// la d�riv� risque d'�tre sketch, une mesure de temp�rature /5 secondes si on

  dTavant = computeslopeFrontTemp(6);
  int deltaTemperature = 0;
  /* Perform state's actions. */
  switch (currentState) {

    default:
    case ZEROING_STEPPER:
		AirInput_forceAperture(&primary, pPrimaryMotorParam->MinWaiting);
		AirInput_forceAperture(&grill, pGrillMotorParam->MinWaiting);
		AirInput_forceAperture(&secondary, pSecondaryMotorParam->MinWaiting);


		AllMotorToZero(); //set all motors to zero
		while(!AirInput_InPosition(&grill) || !AirInput_InPosition(&primary) || !AirInput_InPosition(&secondary))
		{
		};
		Particle_requestZero();
		nextState = WAITING;
		break;
    case WAITING:

    	AirInput_forceAperture(&primary, pPrimaryMotorParam->MaxWaiting);// PRIMARY_CLOSED_SECONDARY_FULL_OPEN);
    	AirInput_forceAperture(&grill, pGrillMotorParam->MaxWaiting);// GRILL_CLOSED);
		AirInput_forceAperture(&secondary, pSecondaryMotorParam->MaxWaiting);
		osDelay(10);

    	delLoadingEnd = ALGO_DEL_OFF;
    	delFermeturePorte = ALGO_DEL_OFF;

		if(baffleTemperature > 8000 && frontTemperature > 7000 && (!Algo_getInterlockRequest()))
		{
		  nextState = TEMPERATURE_RISE; //the only way this can happen is if we lost power we don't want to go back in reload/temprise
		  reloadingEvent = false;
		}
		else if ((baffleTemperature > pTemperatureParam->WaitingToIgnition) && (!Algo_getInterlockRequest()) ) { //at 95F, someone is starting a fire
		  nextState = RELOAD_IGNITION;
		  reloadingEvent = false;
		  //initPID(&TemperaturePID,Ki,Kd,Kp,20,-20); // pas utilisé
		}else if(reloadingEvent && (!Algo_getInterlockRequest()))
		{
			//Particle_requestZero();
			nextState = RELOAD_IGNITION;
			reloadingEvent = false;
		}

		break;

    case RELOAD_IGNITION:

		AirInput_forceAperture(&primary, pPrimaryMotorParam->MaxReload);// PRIMARY_SECONDARY_FULL_OPEN);
		AirInput_forceAperture(&grill, pGrillMotorParam->MaxReload);// 39); //2020-03-20 28 //2020-03-18 100
		AirInput_forceAperture(&secondary, pSecondaryMotorParam->MaxReload);

		if (((baffleTemperature > pTemperatureParam->IgnitionToTrise) && (timeSinceStateEntry >= MINUTES(1))) || (baffleTemperature > 10000)) {
		nextState = TEMPERATURE_RISE;
		reloadingEvent = false;
		//AirInput_forceAperture(&grill, PF_GRILL_CLOSED);
		}
		if(timeSinceStateEntry >= MINUTES(20))
		{
		  //ignition fail or coal was hot enough to make us enter in Ignition we go back to WAITING
		  nextState = ZEROING_STEPPER;
		}
      break;

    case TEMPERATURE_RISE:

		targetTemperature = thermostatRequest ? pTemperatureParam->TriseTargetHigh : pTemperatureParam->TriseTargetLow;

		if(historyState != currentState){
		  AirInput_forceAperture(&primary, pPrimaryMotorParam->MaxTempRise);
		  AirInput_forceAperture(&grill, pGrillMotorParam->MaxTempRise);
		  AirInput_forceAperture(&secondary, pSecondaryMotorParam->MaxTempRise);

		  historyState = currentState;
		}
#if PID_CONTROL_ON
		if(TimeSinceLastPIDUpdate > PID_UPDATE_PERIOD_MS)
		{
			Pidtimeref = currentTime_ms;
			PIDTrapPosition += UpdatePID(&TemperaturePID, baffleTemperature,targetTemperature);
			PIDTrapPosition = PIDTrapPosition > PRIMARY_SECONDARY_FULL_OPEN?PRIMARY_SECONDARY_FULL_OPEN:PIDTrapPosition<0?0:PIDTrapPosition;

		}
		AirInput_forceAperture(&primary,PIDTrapPosition);
#else
		/* Since the control algo (i.e. computeAjustement) is limited
		   to +/- 3 steps, it should take 3 * sec per step to complete
		   the movement. Reevaluate the control at that maximum period. */
//		if((timeSinceStateEntry >= MINUTES(3)) ||  (baffleTemperature > targetTemperature)) //3minutes // changemenet 2 min 2021-12-03
		//asservie seulement si on est depuis 3 minutes dans Temperature Rise ou qu'on a atteint 650 ou 660

			if (TimeForStep >= (1 * SEC_PER_STEP_TEMP_RISE * 1000) && timeSinceStateEntry > MINUTES(pUserParam->s32StateEntryWaitTime)) { // changer de 3 a 2 2021-12-03
				timeRefAutoMode = currentTime_ms;

				sec_per_step = (float)SEC_PER_STEP_TEMP_RISE;

				if(PM_isPboard_absent())
				{
					adjustement = computeAjustement(targetTemperature, dTbaffle);
					AirAdjustment(adjustement, sec_per_step,
								 (uint8_t)pPrimaryMotorParam->MinTempRise, (uint8_t)pPrimaryMotorParam->MaxTempRise,
								  (uint8_t)pGrillMotorParam->MinTempRise, (uint8_t)pGrillMotorParam->MaxTempRise,
								  (uint8_t)pSecondaryMotorParam->MinTempRise, (uint8_t)pSecondaryMotorParam->MaxTempRise);

				}else
				{
					action =  computeParticleRiseAdjustment(dTbaffle, currentTime_ms, baffleTemperature);
					int grillAperture = Algo_getGrill();

					switch(action)
					{
					case FAST_CLOSE:

						if(grillAperture != 0)
						{
							grillAperture = (int)(grillAperture/2);
							if(grillAperture < 15)
							{
								grillAperture = 0;
							}
							AirInput_forceAperture(&grill, grillAperture);
						}else
						{
							AirInput_forceAperture(&primary, 75);
						}
						break;
					case SLOW_CLOSE:
						if(grillAperture < 15 && grillAperture != 0)
						{
							AirInput_forceAperture(&grill, 0);
							break;
						}
						AirAdjustment(-1, sec_per_step,
									 (uint8_t)pPrimaryMotorParam->MinTempRise, (uint8_t)pPrimaryMotorParam->MaxTempRise,
									  (uint8_t)pGrillMotorParam->MinTempRise, (uint8_t)pGrillMotorParam->MaxTempRise,
									  (uint8_t)pSecondaryMotorParam->MinTempRise, (uint8_t)pSecondaryMotorParam->MaxTempRise);

					case WAIT:
					default:
						break;
					}

				}


			}
			//timeInTemperatureRise = thermostatRequest ? MINUTES(10):MINUTES(7);
			timeInTemperatureRise = MINUTES(1);
			if ( (timeSinceStateEntry > timeInTemperatureRise && (baffleTemperature > targetTemperature)) || (Algo_getPrimary() < 75))
			{
				TimeSinceEntryInCombLow = 0;
				nextState = thermostatRequest ? COMBUSTION_HIGH : COMBUSTION_LOW;
			}


#endif
		if(reloadingEvent || (baffleTemperature < 3000)) {// changé pour 300 au lieu de 460 2022-03-04
			nextState = ZEROING_STEPPER;
		}
		else if(timeSinceStateEntry > MINUTES(30))

		{
			TimeSinceEntryInCombLow = 0;
			nextState = thermostatRequest ? COMBUSTION_HIGH : COMBUSTION_LOW;
		}

      break;

	case COMBUSTION_HIGH:
			if(historyState != currentState){

            	AirInput_forceAperture(&primary,  AirInput_getAperture(&primary));  //Pier-Luc a ajouté ce paramètre le 2022-10-25
				AirInput_forceAperture(&grill,  AirInput_getAperture(&grill));
				AirInput_forceAperture(&secondary,  pSecondaryMotorParam->MaxCombHigh);

				StateEntryControlAdjustment(pPrimaryMotorParam->MinCombHigh, pPrimaryMotorParam->MaxCombHigh,
											pGrillMotorParam->MinCombHigh,pGrillMotorParam->MaxCombHigh,
											pSecondaryMotorParam->MinCombHigh,pSecondaryMotorParam->MaxCombHigh);

				historyState = currentState;
			}
#if PID_CONTROL_ON
            if(TimeSinceLastPIDUpdate > PID_UPDATE_PERIOD_MS)
            {
            	Pidtimeref = currentTime_ms;
            	PIDTrapPosition += UpdatePID(&TemperaturePID, baffleTemperature,targetTemperature);
            	PIDTrapPosition = PIDTrapPosition > PRIMARY_SECONDARY_FULL_OPEN?PRIMARY_SECONDARY_FULL_OPEN:PIDTrapPosition<0?0:PIDTrapPosition;
            }
#else
            /* Since the control algo (i.e. computeAjustement) is limited
               to +/- 3 steps, it whould take 3 * sec per step to complete
               the mouvement. Reevaluate the control at that maximum period. */
            if (TimeForStep >= (3 * SEC_PER_STEP_COMB_HIGH * 1000)&& AirInput_InPosition(&grill) && AirInput_InPosition(&primary) && AirInput_InPosition(&secondary)) {
              timeRefAutoMode = currentTime_ms;
			  
            adjustement = computeAjustement(pTemperatureParam->CombHighTarget, dTbaffle);

			AirAdjustment((int32_t)(adjustement), (float)(SEC_PER_STEP_COMB_HIGH),
							pPrimaryMotorParam->MinCombHigh, pPrimaryMotorParam->MaxCombHigh,
							pGrillMotorParam->MinCombHigh,pGrillMotorParam->MaxCombHigh,
							pSecondaryMotorParam->MinCombHigh,pSecondaryMotorParam->MaxCombHigh);
            }
#endif
            if ( ((baffleTemperature) >= (frontTemperature-pTemperatureParam->CoalDeltaTemp)) // changement de <= à >= UFEC 23 2021-11-23
            		&& (pTemperatureParam->CoalCrossOverRearHigh > frontTemperature) ) //détection de l'état coal/braise
            {
            	nextState = COAL_HIGH;
            }
            if (!thermostatRequest) {

            	TimeSinceEntryInCombLow = 0;
            	nextState = COMBUSTION_LOW;
            } else if (reloadingEvent) {
            	nextState = ZEROING_STEPPER;
            }

          break;

    case COMBUSTION_LOW:
    	//HAL_GPIO_WritePin(SPEED1_COIL_GPIO_Port,SPEED1_COIL_Pin,RESET);//desactive le relai pour activer la carte 2 PLV 15/12/21
		if (TimeSinceEntryInCombLow == 0 || historyState != FLAME_LOSS) {
			TimeSinceEntryInCombLow = currentTime_ms;
		}

    	if(historyState != currentState){


			AirInput_forceAperture(&primary,  AirInput_getAperture(&primary));
			AirInput_forceAperture(&grill,  0);
			AirInput_forceAperture(&secondary, pSecondaryMotorParam->MaxCombLow);

			StateEntryControlAdjustment(pPrimaryMotorParam->MinCombLow, pPrimaryMotorParam->MaxCombLow,
										pGrillMotorParam->MinCombLow, pGrillMotorParam->MaxCombLow,
										pSecondaryMotorParam->MinCombLow,pSecondaryMotorParam->MaxCombLow);

		    historyState = currentState;
		}
#if PID_CONTROL_ON
        if(TimeSinceLastPIDUpdate > PID_UPDATE_PERIOD_MS)
        {
        	Pidtimeref = currentTime_ms;
        	PIDTrapPosition += UpdatePID(&TemperaturePID, baffleTemperature,targetTemperature);
        	PIDTrapPosition = PIDTrapPosition > PRIMARY_SECONDARY_FULL_OPEN?PRIMARY_SECONDARY_FULL_OPEN:PIDTrapPosition<0?0:PIDTrapPosition;

        }
		AirInput_forceAperture(&primary,PIDTrapPosition);
#else

			if (AirInput_InPosition(&grill)
					&& AirInput_InPosition(&primary)
					&& AirInput_InPosition(&secondary)
				  /*&& (timeSinceStateEntry >=MINUTES(2))*/ )
			{

				timeRefAutoMode = currentTime_ms;
				bool stove_too_cold = false; // If all openings are maxed and temperature can't be brought over minimum value, go to comb_superlow


				if(PM_isPboard_absent()){  //2023-05-16 (CR): Permet de contrôler avec l'ancien algo si la carte de particules est boguée ou absente
					adjustement = computeAjustement(pTemperatureParam->CombLowTarget, dTbaffle);
					sec_per_step = (float)(SEC_PER_STEP_COMB_LOW);
				}else{

					bool closeGrill = false;
					bool openGrill = false;
					// Adjustment based on particle value
					closeGrill = computeParticleLowAdjustment(dTavant, &adjustement, &sec_per_step,
							currentTime_ms,baffleTemperature/10,pTemperatureParam->CombLowTarget/10, &openGrill);



					//closeGrill = computeParticleCombLowAdjustment(dTbaffle, currentTime_ms, baffleTemperature, pTemperatureParam->CombLowTarget, &action);
					int grillAperture = Algo_getGrill();

					if(openGrill)
					{
						if(grillAperture < 30)
						{
							grillAperture = 30;
						}else
						{
							grillAperture *= 2;
						}
						AirInput_forceAperture(&grill, grillAperture);
					}

					if((closeGrill) && (grillAperture != 0) && (currentTime_ms > partComeback_timer_ms))
					{
						partComeback_timer_ms = currentTime_ms + 500;
						AirInput_forceAperture(&grill, grillAperture - 1);
					}
/*
					switch(action)
					{
					case WAIT:
						break;
					case VERY_SLOW_CLOSE:
						adjustement = 1;
						sec_per_step = (float)pUserParam->s32VslowClose/10;
						break;
					case SLOW_CLOSE:
						adjustement = 1;
						sec_per_step = (float)pUserParam->s32SlowClose/10;
						break;
					case FAST_CLOSE:
						adjustement = 5;
						sec_per_step = (float)pUserParam->s32FastClose/10;
						break;
					case SLOW_OPEN:
						adjustement = 2;
						sec_per_step = (float)pUserParam->s32SlowOpen/10;
						break;
					case FAST_OPEN:
						adjustement = 5;
						sec_per_step = (float)pUserParam->s32FastOpen/10;
						break;
					case INSTANT_OPEN:
						if(grillAperture < 30)
						{
							grillAperture = 30;
						}else
						{
							grillAperture *= 2;
						}
						AirInput_forceAperture(&grill, grillAperture);
						adjustement = 0;
						break;
					default:
						adjustement = 0;
						break;

					}*/

				}
				// Same adjustment function as in other states, but changed arguments
				AirAdjustment(adjustement, sec_per_step,
							pPrimaryMotorParam->MinCombLow, pPrimaryMotorParam->MaxCombLow,
							pGrillMotorParam->MinCombLow, pGrillMotorParam->MaxCombLow,
							pSecondaryMotorParam->MinCombLow, pSecondaryMotorParam->MaxCombLow);


				if (((currentTime_ms >=(TimeSinceEntryInCombLow + MINUTES(30))) && (AirInput_getAperture(&primary) >= (pPrimaryMotorParam->MaxCombLow - 1))) || stove_too_cold){
					nextState = COMBUSTION_SUPERLOW;
				}


			}

#endif
		/*  NE PAS PASSER EN COAL LOW D'ICI.... CETTE ACTION SE FAIT EN COMB_SUPERLOW CR:2023-05-23
		if ( ((baffleTemperature) >= (frontTemperature-pTemperatureParam->CoalDeltaTemp)) // changement de <= à >= UFEC 23 2021-11-23
				            		&& (frontTemperature < pTemperatureParam->CoalCrossOverRearLow) ) //détection de l'état coal/braise
			{
				nextState = COAL_LOW;
		    }
	     */
		if ( (baffleTemperature <= (frontTemperature-pTemperatureParam->CoalDeltaTemp)) //RETOUR À <= ET CHANGEMENENT POUR 200 POUR LE COALCROSSOVERLOW gtf 2022-08-30
				&& (frontTemperature < pTemperatureParam->CoalCrossOverRearLow) )
        {
        	//nextState = FLAME_LOSS;
        }
        if (thermostatRequest) {
          nextState = COMBUSTION_HIGH;
        } else if (reloadingEvent) {
          nextState = ZEROING_STEPPER;
        }
      break;
    case COMBUSTION_SUPERLOW:

		AirInput_forceAperture(&secondary, pSecondaryMotorParam->MaxCombSuperLow);
		AirInput_forceAperture(&grill, 0);
		StateEntryControlAdjustment(pPrimaryMotorParam->MinCombSuperLow,pPrimaryMotorParam->MaxCombSuperLow,
									pGrillMotorParam->MinCombSuperLow,pGrillMotorParam->MaxCombSuperLow,
									pSecondaryMotorParam->MinCombSuperLow,pSecondaryMotorParam->MaxCombSuperLow);

		bool stove_too_cold = false;

		//if(PM_isPboard_absent()){  //2023-05-16 (CR): Permet de contrôler avec l'ancien algo si la carte de particules est boguée ou absente
			adjustement = computeAjustement(pTemperatureParam->CombLowTarget, dTbaffle);  // changement pour comblow au lieu de combsuperlow (660 au lieu de 700) GTF-2022-10-20

			sec_per_step = (float)SEC_PER_STEP_COMB_LOW;

		//}else{
			// Adjustment based on particle value
		//	stove_too_cold = computeParticleLowAdjustment(dTavant, &adjustement, &sec_per_step,
		//			baffleTemperature/10, currentTime_ms, pTemperatureParam->CombLowtoSuperLow/10);

		//}
		AirAdjustment(adjustement, sec_per_step,
				pPrimaryMotorParam->MinCombSuperLow, pPrimaryMotorParam->MaxCombSuperLow,
				pGrillMotorParam->MinCombSuperLow, pGrillMotorParam->MaxCombSuperLow,
				pSecondaryMotorParam->MinCombSuperLow, pSecondaryMotorParam->MaxCombSuperLow);

		if ( (((baffleTemperature) >= (frontTemperature-pTemperatureParam->CoalDeltaTemp)) // changement de <= à >= UFEC 23 2021-11-23
		            		&& (frontTemperature < pTemperatureParam->CoalCrossOverRearLow)) || stove_too_cold) //détection de l'état coal/braise
		            {
		            	nextState = COAL_LOW;
		            }

		if ( (baffleTemperature <= (frontTemperature-pTemperatureParam->CoalDeltaTemp)) //RETOUR À <= ET CHANGEMENENT POUR 200 POUR LE COALCROSSOVERLOW gtf 2022-08-30
						&& (frontTemperature < pTemperatureParam->CoalCrossOverRearLow) )
		        {
		        	//nextState = FLAME_LOSS;
		        }
		        if (thermostatRequest) {
		          nextState = COMBUSTION_HIGH;
		        } else if (reloadingEvent) {
		          nextState = ZEROING_STEPPER;
		        }
				//  GTF pas certain

		break;
    case COAL_LOW: //remplacement de la logic du low pour la logic du high de la fournaise pour UFEC 23 2021-11-23

    	//HAL_GPIO_WritePin(SPEED1_COIL_GPIO_Port,SPEED1_COIL_Pin,SET);//active le relai pour activer la carte 2 PLV 15/12/21
    	AirInput_forceAperture(&primary, pPrimaryMotorParam->MaxCoalLow);
    	AirInput_forceAperture(&grill, pGrillMotorParam->MaxCoalLow);
    	AirInput_forceAperture(&secondary, pSecondaryMotorParam->MaxCoalLow);

    	if (thermostatRequest) {
    	          nextState = COAL_HIGH;
    	}else if (reloadingEvent) {
            nextState = ZEROING_STEPPER;
        }

    	break;

    case FLAME_LOSS:
    	AirInput_forceAperture(&grill, PF_GRILL_FULL_OPEN);
    	AirInput_forceAperture(&secondary, PF_SECONDARY_FULL_OPEN);
    	deltaTemperature = abs(frontTemperature - baffleTemperature);
    	//if( deltaTemperature > pTemperatureParam->FlameLossDelta && timeSinceStateEntry >= MINUTES(1))
    	if( frontTemperature > (pTemperatureParam->CoalCrossOverRearLow+400) && timeSinceStateEntry >= MINUTES(1)) // ajout + 20 deg GTF 2022-10-20
    	{
    		nextState = historyState;
    		AirInput_forceAperture(&grill, PF_GRILL_CLOSED);
    	}
		if(reloadingEvent) {
			nextState = ZEROING_STEPPER;
		}
		else if((frontTemperature < pTemperatureParam->CoalCrossOverRearLow+200) && timeSinceStateEntry >= MINUTES(5))
		{
			nextState = COAL_LOW;
		}
    	break;

    case COAL_HIGH:
		if(historyState != currentState){

			AirInput_forceAperture(&secondary, pSecondaryMotorParam->MaxCoalHigh);

			StateEntryControlAdjustment(pPrimaryMotorParam->MinCoalHigh, pPrimaryMotorParam->MaxCoalHigh,
										PF_GRILL_CLOSED, PF_GRILL_CLOSED,
										pSecondaryMotorParam->MinCoalHigh, pSecondaryMotorParam->MaxCoalHigh);

		    historyState = currentState;
		}
        /* Since the control algo (i.e. computeAjustement) is limited
           to +/- 3 steps, it whould take 3 * sec per step to complete
           the mouvement. Reevaluate the control at that maximum period. */
        if (TimeForStep >= (3 * SEC_PER_STEP_COAL_HIGH * 1000)) {
        	if(frontTemperature > 9000) /// C'EST QUOI ÇA GTF 2022-03-11
        	{
        		adjustement = -1; //Si T > 900, on ferme. Sinon on suit le tableau d'ajustement
        	}
        	else
        	{
        		adjustement = computeAjustement( pTemperatureParam->CombHighTarget, dTbaffle);
        	}


           	AirAdjustment(adjustement, (float)SEC_PER_STEP_COMB_HIGH,
     			  	  	  pPrimaryMotorParam->MinCoalHigh, pPrimaryMotorParam->MaxCoalHigh,
        				  pGrillMotorParam->MinCoalHigh, pGrillMotorParam->MaxCoalHigh,
						  pSecondaryMotorParam->MinCoalHigh, pSecondaryMotorParam->MaxCoalHigh);
        }

    	if (!thermostatRequest) {
    	          nextState = COAL_LOW;
    	}else if (reloadingEvent) {
            nextState = ZEROING_STEPPER;
        }else if (baffleTemperature > pTemperatureParam->CombLowTarget){
        	nextState = COMBUSTION_HIGH;
        }
    	break;

    case OVERTEMP:
    case SAFETY:
      AirInput_forceAperture(&grill, PF_GRILL_CLOSED);
      AirInput_forceAperture(&primary, PF_PRIMARY_CLOSED);
      AirInput_forceAperture(&secondary, PF_SECONDARY_CLOSED); //TODO: Choose what to do with secondary


      if ((baffleTemperature < pTemperatureParam->OverheatBaffle)
    		  && (frontTemperature < pTemperatureParam->OverheatChamber)
			  && (Algo_getPlenumTemp() < pTemperatureParam->OverheatPlenumExit)){
    	  if(historyState == SAFETY || historyState == OVERTEMP)
    	  {
    		  nextState = ZEROING_STEPPER; //autre façon de fermer les trappes
    	  }
    	  else
    	  {
    		  nextState = COMBUSTION_LOW;
    	  }
      }
      break;

    case PRODUCTION_TEST:
    	TestRunner();
		nextState = currentState;  //assign the current state in the runner
    	break;

    case MANUAL_CONTROL:
        AirInput_forceAperture(&primary, pUserParam->s32ManualPrimary);
        AirInput_forceAperture(&secondary, pUserParam->s32ManualSecondary);
        AirInput_forceAperture(&grill, pUserParam->s32ManualGrill);

    	break;
  }

	if((GPIO_PIN_SET==HAL_GPIO_ReadPin(Safety_ON_GPIO_Port,Safety_ON_Pin)) && (currentState !=PRODUCTION_TEST))
	{
		uint32_t kerneltime = osKernelSysTick();
		if ((Safetydebounce_ms+100) < kerneltime)
		{
			nextState = SAFETY; //force the safety state
		}

		else
		{
			Safetydebounce_ms = osKernelSysTick();
		}
	}
  /* Perform superstate action's */
  switch (currentState) {
  	  case WAITING:
  		  //case RELOAD_IGNITION:
  	  case OVERTEMP:
  	  case SAFETY:
  	  case PRODUCTION_TEST:
  		  /* do nothing */
  		  break;
  	  default:
  		  if ((baffleTemperature > pTemperatureParam->OverheatBaffle) || (frontTemperature > pTemperatureParam->OverheatChamber) || (Algo_getPlenumTemp()>pTemperatureParam->OverheatPlenum)) {
  			  nextState = OVERTEMP;
  		  }
  		  if(currentState == TEMPERATURE_RISE || currentState == COMBUSTION_HIGH || currentState == COMBUSTION_LOW || currentState == COMBUSTION_SUPERLOW)
  		  {
  			  if ((baffleTemperature < ColdStoveTemp) && (frontTemperature < ColdStoveTemp) && timeSinceStateEntry > MINUTES(1)) {
  				  nextState = WAITING;
  			  }
  			  //flameloss decision

  			  if ((currentTime_ms - timer_flameloss) >= MINUTES(TRFlameLoss)){
  				  for (int i = 0; i < 3; i++){
  					  TFlameLossArrayB[i] = TFlameLossArrayB[i+1];
  					  TFlameLossArrayR[i] = TFlameLossArrayR[i+1];
  				  }
  				  TFlameLossArrayB[3] = Algo_getBaffleTemp();
  				  TFlameLossArrayR[3] = Algo_getFrontTemp();
  				  R_flamelossB = TFlameLossArrayB[0] - TFlameLossArrayB[3];
  				  R_flamelossR = TFlameLossArrayR[0] - TFlameLossArrayR[3];
  				  timer_flameloss = currentTime_ms;
  				  if ((R_flamelossB > RFlameLossB) || (R_flamelossR > RFlameLossR)){
  					  //nextState = FLAME_LOSS;
  				  }
  			  }
  		  }
  		  //if (((baffleTemperature > 6500) || (frontTemperature > 9000)) && (nextState == FLAME_LOSS)){
  			//  nextState = currentState;
  		  //}

  		//computeParticleAdjustment(adjustement, &pUserParam->s32APERTURE_OFFSET, &pUserParam->s32SEC_PER_STEP, currentTime_ms);
    	break;
  }
  if(Algo_getInterlockRequest() && (currentState !=PRODUCTION_TEST) && (nextState != OVERTEMP) && (nextState != SAFETY))
  {
  		nextState = WAITING;
  }

  if(pUserParam->s32ManualOverride == 1)
  {
	  nextState = MANUAL_CONTROL;
  }	else if(currentState == MANUAL_CONTROL)
	{
		nextState = COMBUSTION_HIGH;
	}

  if (nextState != currentState) {

	if ((currentState == COMBUSTION_HIGH  && nextState == COMBUSTION_LOW) || (currentState == COMBUSTION_LOW && nextState == COMBUSTION_HIGH))
	{
		//do not update the state stateChangeTimeRef
	}
	else
	{

	    stateChangeTimeRef = currentTime_ms;
	}
	  historyState = currentState;
	  currentState = nextState;
  }

}

void Algo_task(uint32_t currentTime_ms) {

  manageStateMachine(currentTime_ms);
//  managePlenumSpeed(Algo_getPlenumTemp(),Algo_getThermostatRequest(),currentTime_ms);

  if(Algo_getState()!= PRODUCTION_TEST)
  {
	  AirInput_task( &primary, currentTime_ms);
	  AirInput_task( &grill, currentTime_ms);
	  AirInput_task( &secondary, currentTime_ms);
  }
}

static inline bool inBetween( int var, int low, int high) {
  return ((var >= low) && (var <= high));
}

void Algo_setSimulatorMode( bool active) {
  simulatorMode = active;
}

float Algo_getBaffleTempSlope() {
  return Algo_slopeBaffleTemp;
}

/* Returns temperature slope in [*C / s] */
static float computeSlopeBaffleTemp(unsigned int nbData) {
  if (simulatorMode)
  {
    return Algo_Simulator_slopeBaffleTemp;
  }
  else
  {
	Algo_slopeBaffleTemp = Slope_compute(&slopeBaffleTemp, nbData) / 10.0;
	Algo_slopeBaffleTemp = (float)(baffleTemperature-lastBaffleTemp)/500;
    return Algo_slopeBaffleTemp;
  }
}

static float computeslopeFrontTemp(unsigned int nbData) {
  if (simulatorMode)
  {
    return Algo_Simulator_slopeBaffleTemp;
  }
  else
  {
	Algo_slopeFrontTemp = Slope_compute(&slopeFrontTemp, nbData) / 10.0;
    return Algo_slopeFrontTemp;
  }
}

void Algo_setState(State state) {
  //if (simulatorMode) {
    currentState = state;
  //}
}

State Algo_getState() {
  return currentState;
}
uint32_t getStateTime(){
  return timeSinceStateEntry;
}

void Algo_setBaffleTemp(int temp) {
  lastBaffleTemp = baffleTemperature;
  baffleTemperature = temp;
  Slope_addData(&slopeBaffleTemp, temp);
}

void Algo_setFrontTemp(int temp) {
  frontTemperature = temp;
  Slope_addData(&slopeFrontTemp,temp);
}
int Algo_getFrontTemp() {
  return frontTemperature;
}

int Algo_getBaffleTemp() {
  return baffleTemperature;
}

void Algo_setPlenumTemp(int temp) {
  plenumTemp = temp;
}
int Algo_getPlenumTemp() {
  return plenumTemp;
}

int Algo_getPrimary() {
  return AirInput_getAperture(&primary);
}

int Algo_getGrill() {
  return AirInput_getAperture(&grill);
}

int Algo_getSecondary() {
	return AirInput_getAperture(&secondary);
}

int Algo_getPrimarySetPoint(void)
{
	return AirInput_getSetPoint(&primary);
}

int Algo_getGrillSetPoint(void)
{
	return AirInput_getSetPoint(&grill);
}

int Algo_getSecondarySetPoint(void)
{
	return AirInput_getSetPoint(&secondary);
}

Algo_DELState Algo_getStateFinChargemenent() {
  return delLoadingEnd;
}

Algo_DELState Algo_getStateFermeturePorte() {
  return delFermeturePorte;
}

void Algo_setThermostatRequest(bool demand) {
  thermostatRequest = demand;
}
bool Algo_getThermostatRequest() {
  return thermostatRequest;
}
void Algo_setInterlockRequest(bool demand) {
	interlockRequest = demand;
}
bool Algo_getInterlockRequest() {
  return interlockRequest;
}

void Algo_startChargement(uint32_t currentTime_ms) {
  reloadingEvent = true;
  fanPauseRequired = true;
  TimeOfReloadRequest = currentTime_ms;
}
uint32_t Algo_getTimeOfReloadRequest() {
  return TimeOfReloadRequest;
}

bool Algo_IsFanPauseRequested() {
  return fanPauseRequired;
}

void Algo_clearReloadRequest() {
  reloadingEvent = false;
}

static int computeAjustement( int tempTarget_tenthF, float dTempAvant_FperS) {

  //                  [line][column]
  const int adjustment[3][3] = {
    { 0, -2, -3},
    { +1, 0, -1},
    { +3, +2, 0}
  };

  unsigned int line;
  unsigned int column;

  if (baffleTemperature > (tempTarget_tenthF + 500)) {
    line = 0;
  } else if (baffleTemperature >= (tempTarget_tenthF - 500)) {
    line = 1;
  } else {
    line = 2;
  }

  if (dTempAvant_FperS < -0.6) {
    column = 0;
  } else if (dTempAvant_FperS <= 0.6) {
    column = 1;
  } else {
    column = 2;
  }

  return adjustment[line][column];
}




static void AirAdjustment(int adjustment, float secondPerStep, ////////////////// Insérer la gestion du secondaire dans cette fonction
		uint8_t MinPrimary, uint8_t MaxPrimary,
		uint8_t MinGrill, uint8_t MaxGrill,
		uint8_t MinSecondary, uint8_t MaxSecondary)
{
	int adjustment_carriage = 0; // garde la balance de l'ajustement s'il est plus grand que l'angle alloué
	int sec_delta;	// Espace (en pas) disponible pour bouger dans une direction ou l'autre
	int prim_delta;
	int gril_delta;

	if (adjustment > 0)
	{
		sec_delta = MaxSecondary - AirInput_getAperture(&secondary); // On ouvre, donc on regarde on est à combien du max
		prim_delta = MaxPrimary - AirInput_getAperture(&primary);
		gril_delta = MaxGrill - AirInput_getAperture(&grill);


		if (sec_delta != 0) // A-t-on atteint le max?
		{
			if(adjustment > sec_delta) // Si l'ajustement est trop grand, on bouge au max et on garde le restant pour le prochain moteur
			{
				adjustment_carriage = adjustment - sec_delta;
				adjustment = sec_delta;
			}
			AirInput_setAjustement(&secondary, adjustment, secondPerStep);
			if(adjustment_carriage == 0) // Si on n'a pas de pas supplémentaire à faire, on a terminé
			{
				return;
			}
			adjustment = adjustment_carriage;
			adjustment_carriage = 0;
		}

		if (prim_delta != 0)
		{
			if(adjustment > prim_delta)
			{
				adjustment_carriage = adjustment - prim_delta;
				adjustment = prim_delta;
			}
			AirInput_setAjustement(&primary, adjustment, secondPerStep);
			if(adjustment_carriage == 0)
			{
				return;
			}
			adjustment = adjustment_carriage;
			adjustment_carriage = 0;
		}

		if (gril_delta != 0)
		{
			if(adjustment > gril_delta)
			{
				adjustment_carriage = adjustment - gril_delta;
				adjustment = gril_delta;
			}
			AirInput_setAjustement(&grill, adjustment, secondPerStep);
		}

	}
	else if (adjustment < 0)
	{
		sec_delta = MinSecondary - AirInput_getAperture(&secondary);
		prim_delta = MinPrimary - AirInput_getAperture(&primary);
		gril_delta = MinGrill - AirInput_getAperture(&grill);

		if (gril_delta != 0)
		{
			if(adjustment < gril_delta)
			{
				adjustment_carriage = adjustment - gril_delta;
				adjustment = gril_delta;
			}
			AirInput_setAjustement(&grill, adjustment, secondPerStep);

			if(adjustment_carriage == 0)
			{
				return;
			}
			adjustment = adjustment_carriage;
			adjustment_carriage = 0;
		}

		if (prim_delta != 0)
		{
			if(adjustment < prim_delta)
			{
				adjustment_carriage = adjustment - prim_delta;
				adjustment = prim_delta;
			}
			AirInput_setAjustement(&primary, adjustment, secondPerStep);

			if(adjustment_carriage == 0)
			{
				return;
			}
			adjustment = adjustment_carriage;
			adjustment_carriage = 0;
		}

		if (sec_delta != 0)
		{
			if(adjustment < sec_delta)
			{
				adjustment_carriage = adjustment - sec_delta;
				adjustment = sec_delta;
			}
			AirInput_setAjustement(&secondary, adjustment, secondPerStep);
		}

	}
	/*else{do nothing} air setting doesn't need further adjustment*/
}


static void StateEntryControlAdjustment(uint8_t MinPrimary, uint8_t MaxPrimary, ////////////// Insérer la gestion du secondaire dans cette fonction
		uint8_t MinGrill, uint8_t MaxGrill,
		uint8_t MinSecondary, uint8_t MaxSecondary)
{
	int aperture = AirInput_getAperture(&primary);
	int apertureAdjustment = 0;
	if (aperture >= MaxPrimary)
	{
		apertureAdjustment = MaxPrimary - aperture;
		AirAdjustment(apertureAdjustment, 2, MinPrimary,MaxPrimary,
				MinGrill,MaxGrill,
				MinSecondary, MaxSecondary);
	}
	else if (aperture <= MinPrimary)
	{
		apertureAdjustment = MinPrimary - aperture;
		AirAdjustment(apertureAdjustment,2, MinPrimary, MaxPrimary,
				MinGrill,MaxGrill,
				MinSecondary,MaxSecondary);
	}

	aperture = AirInput_getAperture(&grill);
	if (aperture >= MaxGrill)
	{
		apertureAdjustment = MaxGrill - aperture;
		AirAdjustment(apertureAdjustment,1, MinPrimary, MaxPrimary,
				MinGrill, MaxGrill,
				MinSecondary,MaxSecondary);
	}
	else if (aperture <= MinGrill)
	{
		apertureAdjustment = MinGrill - aperture;
		AirAdjustment(apertureAdjustment,1, MinPrimary, MaxPrimary,
				MinGrill, MaxGrill,
				MinSecondary, MaxSecondary);

	}
}

bool IsDoorOpen(void)
{
	return GPIO_PIN_SET == HAL_GPIO_ReadPin(Limit_switch_Door_GPIO_Port,Limit_switch_Door_Pin);
}

void setErrorFlag(uint32_t errorcode, ErrorType type)
{
	errorFlag = true;
	static uint16_t uartCounter = 0;

	switch(type)
	{
	case UART:
		uartCounter++;
		break;
	case I2C:
		break;
	case Particle:
		break;
	default:
		break;
	}
}



