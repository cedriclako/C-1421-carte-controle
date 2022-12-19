#include "algo.h"
#include "air_input.h"
#include "slope.h"
#include "cmsis_os.h"
#include "main.h"
#include "DebugPort.h"
#include <stdlib.h>
#include <stdio.h>
#include "Pid.h"
#include "ProdTest.h"
#include "MotorManager.h"
#include "Hmi.h"


typedef struct MotorOpeningsParam{
	int16_t MaxWaiting; //max environ 32000 dixieme de f
	int16_t MinWaiting;
	int16_t MaxReload;
	int16_t MinReload;
	int16_t MaxTempRise;
	int16_t MinTempRise;
	int16_t MaxCombLow;
	int16_t MinCombLow;
	int16_t MaxCombSuperLow;
	int16_t MinCombSuperLow;
	int16_t MaxCombHigh;
	int16_t MinCombHigh;
	int16_t MaxCoalHigh;
	int16_t MinCoalHigh;
	int16_t MaxCoalLow;
	int16_t MinCoalLow;
}MotorOpeningsParam_t;

typedef struct CombTempParam{
	int16_t WaitingToIgnition; //max environ 32000dixieme de f
	int16_t IgnitionToTrise;
	int16_t TriseTargetLow;
	int16_t TriseTargetHigh;
	int16_t CombLowTarget;
	int16_t CombHighTarget;
	int16_t CombLowtoSuperLow;
	int16_t FlameLoss;
	int16_t FlameLossDelta;
	int16_t CoalCrossOverRearLow;
	int16_t CoalCrossOverRearHigh;
	int16_t CoalDeltaTemp;
	int16_t CoalStoveTemp;
	int16_t OverheatPlenum;
	int16_t OverheatPlenumExit;
	int16_t OverheatBaffle;
	int16_t OverheatChamber;
}CombTempParam_t;

typedef struct ParticlesParam{
	int16_t TempRiseMax;
	int16_t CombLowMax;
	int16_t CombSuperLowMax;
	int16_t FlameLossMax;
	int16_t CoalMax;

}ParticlesParam_t;



static const CombTempParam_t TemperatureParam =
{
	//UFEC23 - Test du 2022-11-29 NOUVEAU PCB (une seule carte)               //tenth of F
	.WaitingToIgnition = 1000,
	.IgnitionToTrise = 5250,
	.TriseTargetLow = 6500,
	.TriseTargetHigh = 6900,
	.CombLowTarget = 6600,
	.CombHighTarget = 7000,
	.CombLowtoSuperLow = 7000,
	.FlameLoss = 7500,
	.FlameLossDelta = 1750,
	.CoalCrossOverRearLow = 8000,
	.CoalCrossOverRearHigh = 7000,
	.CoalDeltaTemp = 2500,
	.CoalStoveTemp = 900,
	.OverheatPlenum = 2200,
	.OverheatPlenumExit = 2100,
	.OverheatBaffle = 15000,
	.OverheatChamber = 15000,


};

#define ColdStoveTemp 900 //90F

//Flame loss parameters:
#define  RFlameLossB  500 // 50F Loss of temperature per R time to go in flame loss
#define  RFlameLossR	 1400 // 140F Loss of temperature per R time to go in flame loss
#define TRFlameLoss	   1 // 3 minutes time in minutes of data acquisition for R calculation

//Maximum Primary and grate opening by state
//all values are express in step 0.9degrees.


static const MotorOpeningsParam_t PrimaryMotorParam =
{
	.MaxWaiting = 6,
	.MinWaiting = 6,
	.MaxReload = 97,
	.MinReload = 58,
	.MaxTempRise = 85,
	.MinTempRise = 17,
	.MaxCombHigh = 70,
	.MinCombHigh = 14,
	.MaxCombLow = 39,
	.MinCombLow = 0,
	.MaxCombSuperLow = 25,
	.MinCombSuperLow = 0,
	.MaxCoalHigh = 0,
	.MinCoalHigh = 0,
	.MaxCoalLow = 0,
	.MinCoalLow = 0,

};

static const MotorOpeningsParam_t GrillMotorParam =
{
	.MaxWaiting = 0,
	.MinWaiting = 0,
	.MaxReload = 97,
	.MinReload = 0,
	.MaxTempRise = 30,
	.MinTempRise = 0,
	.MaxCombHigh = 0,
	.MinCombHigh = 0,
	.MaxCombLow = 0,
	.MinCombLow = 0,
	.MaxCombSuperLow = 0,
	.MinCombSuperLow = 0,
	.MaxCoalHigh = 97,
	.MinCoalHigh = 97,
	.MaxCoalLow = 24,
	.MinCoalLow = 24,


};

static const MotorOpeningsParam_t SecondaryMotorParam =
{//Added for current PCB model (parameters must be adjusted by user)
	.MaxWaiting = 6,
	.MinWaiting = 6,
	.MaxReload = 97,
	.MinReload = 97,
	.MaxTempRise = 58,
	.MinTempRise = 58,
	.MaxCombHigh = 97,
	.MinCombHigh = 97,
	.MaxCombLow = 97,
	.MinCombLow = 97,
	.MaxCombSuperLow = 25,
	.MinCombSuperLow = 25,
	.MaxCoalHigh = 50,
	.MinCoalHigh = 50,
	.MaxCoalLow = 10,
	.MinCoalLow = 10,

};


//state machine variable and initial values
static State currentState = ZEROING_STEPPER;
static bool reloadingEvent = false;
static bool errorFlag = false;
bool fanPauseRequired = false;
static AirInput primary = AirInput_init(PRIMARY_MINIMUM_OPENING, PRIMARY_FULL_OPEN);
static AirInput grill = AirInput_init(GRILL_MINIMUM_OPENING, GRILL_FULL_OPEN);
static AirInput secondary = AirInput_init(SECONDARY_MINIMUM_OPENING, SECONDARY_FULL_OPEN);
static bool fixed_sec = false;
static int16_t sec_aperture;

static Algo_DELState delLoadingEnd = ALGO_DEL_OFF;
static Algo_DELState delFermeturePorte = ALGO_DEL_OFF;

static int baffleTemperature = 0;   // [Tenth *F]
static int rearTemperature = 0; // [Tenth *F]
static int plenumTemp = 0;  // [Tenth *F]
static bool thermostatRequest = false;
static bool interlockRequest = false;

//Data for derivative computation
#define NB_DATA  300  // 5 minutes max si une mesure par sec
static int frontTempDataStore[NB_DATA]; //Data for slope calculation

static bool simulatorMode = false;
static Slope slopeBaffleTemp;
float Algo_Simulator_slopeBaffleTemp = 0.0;
float Algo_slopeBaffleTemp = 0.0;
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
void Algo_init();
static int computeAjustement( int tempTarget_tenthF, float dTempAvant_FperS);
void StateEntryControlAdjustment(const uint8_t MinPrimary, const uint8_t MaxPrimary,
		const uint8_t MinGrill, const uint8_t MaxGrill,
		const uint8_t MinSecondary, const uint8_t MaxSecondary);
void AirAdjustment(int adjustement, const uint32_t secondPerStep,
		const uint8_t MinPrimary, const uint8_t MaxPrimary,
		const uint8_t MinGrill, const uint8_t MaxGrill,
		const uint8_t MinSecondary, const uint8_t MaxSecondary);

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
  AirInput_forceAperture(&primary, PRIMARY_CLOSED);
  AirInput_forceAperture(&grill, GRILL_CLOSED);
  AirInput_forceAperture(&secondary, SECONDARY_CLOSED);  //CR TODO: Choose what to do with Secondary
  baffleTemperature = 0;
  rearTemperature = 0;
  thermostatRequest = false;
  delLoadingEnd = ALGO_DEL_OFF;
  delFermeturePorte = ALGO_DEL_OFF;
  Slope_init(&slopeBaffleTemp, frontTempDataStore, NB_DATA, SAMPLING_RATE);
}

static void manageStateMachine(uint32_t currentTime_ms) {

	  State nextState = currentState;
	  float dTavant;
	  int adjustement;

	  static State historyState = ZEROING_STEPPER;

	  static uint32_t stateChangeTimeRef = 0;
	  static uint32_t timeRefAutoMode = 0;
	  static int targetTemperature = 0;
	  static uint32_t Safetydebounce_ms = 0;

	  //flameloss variable
	  static uint32_t timer_flameloss = 0;
	  static int TFlameLossArrayB[4] = {0};
	  static int TFlameLossArrayR[4] = {0};
	  static int R_flamelossB = 0;
	  static int R_flamelossR = 0;


	  const uint32_t SEC_PER_STEP_TEMP_RISE = 3;
	  const uint32_t SEC_PER_STEP_COMB_LOW = 10;
	  const uint32_t SEC_PER_STEP_COMB_HIGH = 6;
	  const uint32_t SEC_PER_STEP_COAL_HIGH = 12;


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
  dTavant = computeSlopeBaffleTemp(2); //�tait 300, selon ce que Novika utilise test du 2019-12-04.
  	  	  	  	  	  	  	  	  	  // la d�riv� risque d'�tre sketch, une mesure de temp�rature /5 secondes si on
  int deltaTemperature = 0;
  /* Perform state's actions. */
  switch (currentState) {

    default:
    case ZEROING_STEPPER:
		AirInput_forceAperture(&primary, PrimaryMotorParam.MinWaiting);
		AirInput_forceAperture(&grill, GrillMotorParam.MinWaiting);
		if(!fixed_sec)
		{
			AirInput_forceAperture(&secondary, SecondaryMotorParam.MinWaiting);
		}else
		{
			AirInput_forceAperture(&secondary,sec_aperture);
		}

		AllMotorToZero(); //set all motors to zero
		while(!AirInput_InPosition(&grill) || !AirInput_InPosition(&primary) || !AirInput_InPosition(&secondary))
		{
		};
		nextState = WAITING;
		break;
    case WAITING:

    	AirInput_forceAperture(&primary, PrimaryMotorParam.MaxWaiting);// PRIMARY_CLOSED_SECONDARY_FULL_OPEN);
    	AirInput_forceAperture(&grill, GrillMotorParam.MaxWaiting);// GRILL_CLOSED);
		if(!fixed_sec)
		{
			AirInput_forceAperture(&secondary, SecondaryMotorParam.MaxWaiting);
		}else
		{
			AirInput_forceAperture(&secondary,sec_aperture);
		}


    	delLoadingEnd = ALGO_DEL_OFF;
    	delFermeturePorte = ALGO_DEL_OFF;



		if(baffleTemperature > 8000 && rearTemperature > 7000 && (!Algo_getInterlockRequest()))
		{
		  nextState = TEMPERATURE_RISE; //the only way this can happen is if we lost power we don't want to go back in reload/temprise
		  reloadingEvent = false;
		}
		else if ((baffleTemperature > TemperatureParam.WaitingToIgnition || reloadingEvent) && (!Algo_getInterlockRequest()) ) { //at 95F, someone is starting a fire
		  nextState = RELOAD_IGNITION;
		  reloadingEvent = false;
		  initPID(&TemperaturePID,Ki,Kd,Kp,20,-20); // pas utilisé
		}

		break;

    case RELOAD_IGNITION:

		AirInput_forceAperture(&primary, PrimaryMotorParam.MaxReload);// PRIMARY_SECONDARY_FULL_OPEN);
		AirInput_forceAperture(&grill, GrillMotorParam.MaxReload);// 39); //2020-03-20 28 //2020-03-18 100

		if(!fixed_sec)
		{
			AirInput_forceAperture(&secondary, SecondaryMotorParam.MaxReload);
		}else
		{
			AirInput_forceAperture(&secondary,sec_aperture);
		}

		if (((baffleTemperature > TemperatureParam.IgnitionToTrise) && (timeSinceStateEntry >= MINUTES(1))) || (baffleTemperature > 10000)) {
		nextState = TEMPERATURE_RISE;
		reloadingEvent = false;
		AirInput_forceAperture(&grill, GRILL_CLOSED);
		}
		if(timeSinceStateEntry >= MINUTES(20))
		{
		  //ignition fail or coal was hot enough to make us enter in Ignition we go back to WAITING
		  nextState = ZEROING_STEPPER;
		}
      break;

    case TEMPERATURE_RISE:

		targetTemperature = thermostatRequest ? TemperatureParam.TriseTargetHigh : TemperatureParam.TriseTargetLow;

		if(historyState != currentState){
		  AirInput_forceAperture(&primary, PrimaryMotorParam.MaxTempRise);
		  AirInput_forceAperture(&grill, GrillMotorParam.MaxTempRise);


			if(!fixed_sec)
			{
				AirInput_forceAperture(&secondary, SecondaryMotorParam.MaxTempRise);
			}else
			{
				AirInput_forceAperture(&secondary,sec_aperture);
			}
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

			if (TimeForStep >= (1 * SEC_PER_STEP_TEMP_RISE * 1000)) { // changer de 3 a 2 2021-12-03
			  timeRefAutoMode = currentTime_ms;

			  adjustement = computeAjustement(targetTemperature, dTavant);
				if(!fixed_sec)
				{
					AirAdjustment(adjustement, SEC_PER_STEP_TEMP_RISE,
												  PrimaryMotorParam.MinTempRise, PrimaryMotorParam.MaxTempRise,
												  GrillMotorParam.MinTempRise,GrillMotorParam.MaxTempRise,
												  SecondaryMotorParam.MinTempRise,SecondaryMotorParam.MaxTempRise);
				}else
				{
					AirAdjustment(adjustement, SEC_PER_STEP_TEMP_RISE,
												  PrimaryMotorParam.MinTempRise, PrimaryMotorParam.MaxTempRise,
												  GrillMotorParam.MinTempRise,GrillMotorParam.MaxTempRise,
												  sec_aperture,sec_aperture);
				}

			}
			timeInTemperatureRise = thermostatRequest ? MINUTES(10):MINUTES(7);
			if ( timeSinceStateEntry > timeInTemperatureRise && (baffleTemperature > targetTemperature))
			{
			  nextState = thermostatRequest ? COMBUSTION_HIGH : COMBUSTION_LOW;
			}


#endif
		if(reloadingEvent || (baffleTemperature < 3000)) {// changé pour 300 au lieu de 460 2022-03-04
			nextState = ZEROING_STEPPER;
		}
		else if(timeSinceStateEntry > MINUTES(30))

		{
			nextState = thermostatRequest ? COMBUSTION_HIGH : COMBUSTION_LOW;
		}

      break;

	case COMBUSTION_HIGH:
			if(historyState != currentState){

            	AirInput_forceAperture(&primary,  AirInput_getAperture(&primary));  //Pier-Luc a ajouté ce paramètre le 2022-10-25
				AirInput_forceAperture(&grill,  AirInput_getAperture(&grill));
				AirInput_forceAperture(&secondary,  AirInput_getAperture(&secondary));

				if(!fixed_sec)
				{
					StateEntryControlAdjustment(PrimaryMotorParam.MinCombHigh, PrimaryMotorParam.MaxCombHigh,
											GrillMotorParam.MinCombHigh,GrillMotorParam.MaxCombHigh,
											SecondaryMotorParam.MinCombHigh,SecondaryMotorParam.MaxCombHigh);
				}else
				{
					StateEntryControlAdjustment(PrimaryMotorParam.MinCombHigh, PrimaryMotorParam.MaxCombHigh,
																GrillMotorParam.MinCombHigh,GrillMotorParam.MaxCombHigh,
																sec_aperture,sec_aperture);
				}
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
              adjustement = computeAjustement(TemperatureParam.CombHighTarget, dTavant);

				if(!fixed_sec)
				{
					AirAdjustment(adjustement, SEC_PER_STEP_COMB_HIGH,
																PrimaryMotorParam.MinCombHigh, PrimaryMotorParam.MaxCombHigh,
												  				GrillMotorParam.MinCombHigh,GrillMotorParam.MaxCombHigh,
																SecondaryMotorParam.MinCombHigh,SecondaryMotorParam.MaxCombHigh);
				}else
				{
					AirAdjustment(adjustement, SEC_PER_STEP_COMB_HIGH,
																PrimaryMotorParam.MinCombHigh, PrimaryMotorParam.MaxCombHigh,
												  				GrillMotorParam.MinCombHigh,GrillMotorParam.MaxCombHigh,
																sec_aperture,sec_aperture);
				}

            }
#endif
            if ( ((baffleTemperature) >= (rearTemperature-TemperatureParam.CoalDeltaTemp)) // changement de <= à >= UFEC 23 2021-11-23
            		&& (TemperatureParam.CoalCrossOverRearHigh > rearTemperature) ) //détection de l'état coal/braise
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
		if(historyState != currentState){
			if (TimeSinceEntryInCombLow == 0) {
				TimeSinceEntryInCombLow = currentTime_ms;
			}

			AirInput_forceAperture(&primary,  AirInput_getAperture(&primary));
			AirInput_forceAperture(&grill,  AirInput_getAperture(&grill));
			AirInput_forceAperture(&secondary, AirInput_getAperture(&secondary));

			if(!fixed_sec)
			{
				StateEntryControlAdjustment(PrimaryMotorParam.MinCombLow, PrimaryMotorParam.MaxCombLow,
											GrillMotorParam.MinCombLow,GrillMotorParam.MaxCombLow,
											SecondaryMotorParam.MinCombLow,SecondaryMotorParam.MaxCombLow);
			}else
			{
				StateEntryControlAdjustment(PrimaryMotorParam.MinCombLow, PrimaryMotorParam.MaxCombLow,
											GrillMotorParam.MinCombLow,GrillMotorParam.MaxCombLow,
											sec_aperture,sec_aperture);
			}

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
		if((timeSinceStateEntry >= MINUTES(3)) ||  (baffleTemperature < 7500)) //Si Tbaffle est plus petite que 750, on attent 3 in avant de commencer à réguler GTF 2022-10-20
		{ //  GTF pas certain


			deltaTemperature = abs(rearTemperature - baffleTemperature);

			if (rearTemperature < TemperatureParam.FlameLoss && ( deltaTemperature < TemperatureParam.FlameLossDelta)) { //changement de reartemp pour le flameloss au lieu de baffletemp GTF 2022-08-30
				nextState = FLAME_LOSS;
				AirInput_forceAperture(&grill, GRILL_FULL_OPEN);
			}
		else{
				//we loss the flamme but we are not in coal yet, we reopen the grill
				if (TimeForStep >= (1 * SEC_PER_STEP_COMB_LOW * 1000)
						&& AirInput_InPosition(&grill)
						&& AirInput_InPosition(&primary)
						&& AirInput_InPosition(&secondary)
				  	  /*&& (timeSinceStateEntry >=MINUTES(2))*/ )
				{

					timeRefAutoMode = currentTime_ms;

					adjustement = computeAjustement(TemperatureParam.CombLowTarget, dTavant);
					if ((currentTime_ms >=(TimeSinceEntryInCombLow + MINUTES(30))) && (AirInput_getAperture(&primary) >= (PrimaryMotorParam.MaxCombLow - 1))){
						nextState = COMBUSTION_SUPERLOW;
					}


					if(!fixed_sec)
					{
						AirAdjustment(adjustement, SEC_PER_STEP_COMB_LOW,
															PrimaryMotorParam.MinCombLow, PrimaryMotorParam.MaxCombLow,
															GrillMotorParam.MinCombLow, GrillMotorParam.MaxCombLow,
															SecondaryMotorParam.MinCombLow, SecondaryMotorParam.MaxCombLow);
					}else
					{
						AirAdjustment(adjustement, SEC_PER_STEP_COMB_LOW,
															PrimaryMotorParam.MinCombLow, PrimaryMotorParam.MaxCombLow,
															GrillMotorParam.MinCombLow, GrillMotorParam.MaxCombLow,
															sec_aperture, sec_aperture);
					}
				}
		}

#endif
		if ( ((baffleTemperature) >= (rearTemperature-TemperatureParam.CoalDeltaTemp)) // changement de <= à >= UFEC 23 2021-11-23
				            		&& (rearTemperature < TemperatureParam.CoalCrossOverRearLow) ) //détection de l'état coal/braise
			{
				nextState = COAL_LOW;
		    }

		if ( (baffleTemperature <= (rearTemperature-TemperatureParam.CoalDeltaTemp)) //RETOUR À <= ET CHANGEMENENT POUR 200 POUR LE COALCROSSOVERLOW gtf 2022-08-30
				&& (rearTemperature < TemperatureParam.CoalCrossOverRearLow) )
        {
        	nextState = FLAME_LOSS;
        }
        if (thermostatRequest) {
          nextState = COMBUSTION_HIGH;
        } else if (reloadingEvent) {
          nextState = ZEROING_STEPPER;
        }
		}
      break;
    case COMBUSTION_SUPERLOW:

		if(!fixed_sec)
		{
			StateEntryControlAdjustment(PrimaryMotorParam.MinCombSuperLow,PrimaryMotorParam.MaxCombSuperLow,
												GrillMotorParam.MinCombSuperLow,GrillMotorParam.MaxCombSuperLow,
												SecondaryMotorParam.MinCombSuperLow,SecondaryMotorParam.MaxCombSuperLow);
		}else
		{
			StateEntryControlAdjustment(PrimaryMotorParam.MinCombSuperLow,PrimaryMotorParam.MaxCombSuperLow,
												GrillMotorParam.MinCombSuperLow,GrillMotorParam.MaxCombSuperLow,
												sec_aperture, sec_aperture);
		}
		adjustement = computeAjustement(TemperatureParam.CombLowTarget, dTavant);  // changement pour comblow au lieu de combsuperlow (660 au lieu de 700) GTF-2022-10-20


		if(!fixed_sec)
		{
			AirAdjustment(adjustement, SEC_PER_STEP_COMB_LOW,
										PrimaryMotorParam.MinCombSuperLow,PrimaryMotorParam.MaxCombSuperLow,
										GrillMotorParam.MinCombSuperLow,GrillMotorParam.MaxCombSuperLow,
										SecondaryMotorParam.MinCombSuperLow,SecondaryMotorParam.MaxCombSuperLow);
		}else
		{
			AirAdjustment(adjustement, SEC_PER_STEP_COMB_LOW,
										PrimaryMotorParam.MinCombSuperLow,PrimaryMotorParam.MaxCombSuperLow,
										GrillMotorParam.MinCombSuperLow,GrillMotorParam.MaxCombSuperLow,
										sec_aperture, sec_aperture);
		}

		if ( ((baffleTemperature) >= (rearTemperature-TemperatureParam.CoalDeltaTemp)) // changement de <= à >= UFEC 23 2021-11-23
		            		&& (rearTemperature < TemperatureParam.CoalCrossOverRearLow) ) //détection de l'état coal/braise
		            {
		            	nextState = COAL_LOW;
		            }

		if ( (baffleTemperature <= (rearTemperature-TemperatureParam.CoalDeltaTemp)) //RETOUR À <= ET CHANGEMENENT POUR 200 POUR LE COALCROSSOVERLOW gtf 2022-08-30
						&& (rearTemperature < TemperatureParam.CoalCrossOverRearLow) )
		        {
		        	nextState = FLAME_LOSS;
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
    	AirInput_forceAperture(&primary, PrimaryMotorParam.MaxCoalLow);
    	AirInput_forceAperture(&grill, GrillMotorParam.MaxCoalLow);


		if(!fixed_sec)
		{
			AirInput_forceAperture(&secondary, SecondaryMotorParam.MaxCoalLow);
		}else
		{
			AirInput_forceAperture(&secondary, sec_aperture);
		}

    	if (thermostatRequest) {
    	          nextState = COAL_HIGH;
    	}else if (reloadingEvent) {
            nextState = ZEROING_STEPPER;
        }
    //	if(historyState != currentState){
    	//				StateEntryControlAdjustment(&primary,PrimaryMotorParam.MinCoalLow, PrimaryMotorParam.MaxCoalLow,
    	//											&grill,GRILL_CLOSED,GRILL_CLOSED);
    		//	    historyState = currentState;
    		//	}
    	      //  /* Since the control algo (i.e. computeAjustement) is limited
    	     //      to +/- 3 steps, it whould take 3 * sec per step to complete
    	       //    the mouvement. Reevaluate the control at that maximum period. */
    	      //  if (TimeForStep >= (3 * SEC_PER_STEP_COAL_HIGH * 1000)) {
    	      //  	if(rearTemperature > 8000) // tentative à 800 2021-11-26
    	      //  	{
    	       // 		adjustement = -1; //Si T > 800, on ferme. Sinon on suit le tableau d'ajustement
    	        //	}
    	        //	else
    	        //	{
    	        //		adjustement = computeAjustement( TemperatureParam.CombHighTarget, dTavant);
    	        //	}
    	        //    AirAdjustment(adjustement, SEC_PER_STEP_COMB_HIGH,
    			//			  	  	  &primary, PrimaryMotorParam.MinCoalLow, PrimaryMotorParam.MaxCoalLow,
    			//					  &grill, GrillMotorParam.MinCoalLow, GrillMotorParam.MaxCoalLow);
    	        //}

    	    	//if (thermostatRequest) {
    	    	  //        nextState = COAL_HIGH;
    	    	//}else if (reloadingEvent) {
    	          //  nextState = ZEROING_STEPPER;
    	        //}else if (baffleTemperature > TemperatureParam.CombLowTarget){
    	        	//nextState = COMBUSTION_LOW;
    	        //}
    	break;

    case FLAME_LOSS:
    	AirInput_forceAperture(&grill, GRILL_FULL_OPEN);
    	deltaTemperature = abs(rearTemperature - baffleTemperature);
    	//if( deltaTemperature > TemperatureParam.FlameLossDelta && timeSinceStateEntry >= MINUTES(1))
    	if( rearTemperature > (TemperatureParam.CoalCrossOverRearLow+400) && timeSinceStateEntry >= MINUTES(1)) // ajout + 20 deg GTF 2022-10-20
    	{
    		nextState = historyState;
    		AirInput_forceAperture(&grill, GRILL_CLOSED);
    	}
		if(reloadingEvent) {
			nextState = ZEROING_STEPPER;
		}
		else if((rearTemperature < TemperatureParam.CoalCrossOverRearLow+200) && timeSinceStateEntry >= MINUTES(5))
		{
			nextState = COAL_LOW;
		}
    	break;

    case COAL_HIGH:
		if(historyState != currentState){

			if(!fixed_sec)
			{
				StateEntryControlAdjustment(PrimaryMotorParam.MinCoalHigh, PrimaryMotorParam.MaxCoalHigh,
												GRILL_CLOSED,GRILL_CLOSED,
												SecondaryMotorParam.MinCoalHigh, SecondaryMotorParam.MaxCoalHigh);
			}else
			{
				StateEntryControlAdjustment(PrimaryMotorParam.MinCoalHigh, PrimaryMotorParam.MaxCoalHigh,
														GRILL_CLOSED,GRILL_CLOSED,
														sec_aperture, sec_aperture);
			}

		    historyState = currentState;
		}
        /* Since the control algo (i.e. computeAjustement) is limited
           to +/- 3 steps, it whould take 3 * sec per step to complete
           the mouvement. Reevaluate the control at that maximum period. */
        if (TimeForStep >= (3 * SEC_PER_STEP_COAL_HIGH * 1000)) {
        	if(rearTemperature > 9000) /// C'EST QUOI ÇA GTF 2022-03-11
        	{
        		adjustement = -1; //Si T > 900, on ferme. Sinon on suit le tableau d'ajustement
        	}
        	else
        	{
        		adjustement = computeAjustement( TemperatureParam.CombHighTarget, dTavant);
        	}

            if(!fixed_sec)
			{
            	AirAdjustment(adjustement, SEC_PER_STEP_COMB_HIGH,
            						  	  	  PrimaryMotorParam.MinCoalHigh, PrimaryMotorParam.MaxCoalHigh,
            								  GrillMotorParam.MinCoalHigh, GrillMotorParam.MaxCoalHigh,
            								  SecondaryMotorParam.MinCoalHigh, SecondaryMotorParam.MaxCoalHigh);
			}else
			{
				AirAdjustment(adjustement, SEC_PER_STEP_COMB_HIGH,
									  	  	  PrimaryMotorParam.MinCoalHigh, PrimaryMotorParam.MaxCoalHigh,
											  GrillMotorParam.MinCoalHigh, GrillMotorParam.MaxCoalHigh,
											  sec_aperture, sec_aperture);
			}
        }

    	if (!thermostatRequest) {
    	          nextState = COAL_LOW;
    	}else if (reloadingEvent) {
            nextState = ZEROING_STEPPER;
        }else if (baffleTemperature > TemperatureParam.CombLowTarget){
        	nextState = COMBUSTION_HIGH;
        }
    	break;

    case OVERTEMP:
    case SAFETY:
      AirInput_forceAperture(&grill, GRILL_CLOSED);
      AirInput_forceAperture(&primary, PRIMARY_CLOSED);
      AirInput_forceAperture(&secondary,SECONDARY_CLOSED); //TODO: Choose what to do with secondary


      if ((baffleTemperature < TemperatureParam.OverheatBaffle)
    		  && (rearTemperature < TemperatureParam.OverheatChamber)
			  && (Algo_getPlenumTemp() < TemperatureParam.OverheatPlenumExit)){
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
  		  if ((baffleTemperature > TemperatureParam.OverheatBaffle) || (rearTemperature > TemperatureParam.OverheatChamber) || (Algo_getPlenumTemp()>TemperatureParam.OverheatPlenum)) {
  			  nextState = OVERTEMP;
  		  }
  		  if(currentState == TEMPERATURE_RISE || currentState == COMBUSTION_HIGH || currentState == COMBUSTION_LOW || currentState == COMBUSTION_SUPERLOW)
  		  {
  			  if ((baffleTemperature < ColdStoveTemp) && (rearTemperature < ColdStoveTemp) && timeSinceStateEntry > MINUTES(1)) {
  				  nextState = WAITING;
  			  }
  			  //flameloss decision

  			  if ((currentTime_ms - timer_flameloss) >= MINUTES(TRFlameLoss)){
  				  for (int i = 0; i < 3; i++){
  					  TFlameLossArrayB[i] = TFlameLossArrayB[i+1];
  					  TFlameLossArrayR[i] = TFlameLossArrayR[i+1];
  				  }
  				  TFlameLossArrayB[3] = Algo_getBaffleTemp();
  				  TFlameLossArrayR[3] = Algo_getRearTemp();
  				  R_flamelossB = TFlameLossArrayB[0] - TFlameLossArrayB[3];
  				  R_flamelossR = TFlameLossArrayR[0] - TFlameLossArrayR[3];
  				  timer_flameloss = currentTime_ms;
  				  if ((R_flamelossB > RFlameLossB) || (R_flamelossR > RFlameLossR)){
  					  nextState = FLAME_LOSS;
  				  }
  			  }
  		  }
  		  if (((baffleTemperature > 6500) || (rearTemperature > 9000)) && (nextState == FLAME_LOSS)){
  			  nextState = currentState;
  		  }
    	break;
  }
  if(Algo_getInterlockRequest() && (currentState !=PRODUCTION_TEST) && (nextState != OVERTEMP) && (nextState != SAFETY))
  {
  		nextState = WAITING;
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
    return Algo_slopeBaffleTemp;
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
  baffleTemperature = temp;
  Slope_addData(&slopeBaffleTemp, temp);
}

void Algo_setRearTemp(int temp) {
  rearTemperature = temp;
}
int Algo_getRearTemp() {
  return rearTemperature;
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

  if (baffleTemperature > (tempTarget_tenthF + 50)) {
    line = 0;
  } else if (baffleTemperature >= (tempTarget_tenthF - 50)) {
    line = 1;
  } else {
    line = 2;
  }

  if (dTempAvant_FperS < -6.0) {
    column = 0;
  } else if (dTempAvant_FperS <= 6.0) {
    column = 1;
  } else {
    column = 2;
  }

  return adjustment[line][column];
}

void AirAdjustment(int adjustement, const uint32_t secondPerStep, ////////////////// Insérer la gestion du secondaire dans cette fonction
		const uint8_t MinPrimary, const uint8_t MaxPrimary,
		const uint8_t MinGrill, const uint8_t MaxGrill,
		const uint8_t MinSecondary, const uint8_t MaxSecondary)
{
	if (adjustement > 0)
	{
		if (AirInput_getAperture(&primary) >= MaxPrimary)
		{
			if(AirInput_getAperture(&secondary) < MaxSecondary)
			{
				AirInput_setAjustement(&secondary, adjustement, secondPerStep);
			}else if (AirInput_getAperture(&grill) < MaxGrill)
			{
				AirInput_setAjustement(&grill, adjustement, secondPerStep);
			}
		}
		else
		{
			AirInput_setAjustement(&primary, adjustement, secondPerStep);

		}
	}
	else if (adjustement < 0)
	{
		if (AirInput_getAperture(&grill) > MinGrill)
		{
			AirInput_setAjustement(&grill, adjustement, secondPerStep);
		}
		else if(AirInput_getAperture(&secondary) > MinSecondary)
		{
			AirInput_setAjustement(&secondary, adjustement, secondPerStep);
		}else
		{
			if(AirInput_getAperture(&primary) > MinPrimary)
			{
				AirInput_setAjustement(&primary, adjustement, secondPerStep);
			}
		}
	}
	/*else{do nothing} air setting doesn't need further adjustment*/
}


void StateEntryControlAdjustment(const uint8_t MinPrimary, const uint8_t MaxPrimary, ////////////// Insérer la gestion du secondaire dans cette fonction
		const uint8_t MinGrill, const uint8_t MaxGrill,
		const uint8_t MinSecondary, const uint8_t MaxSecondary)
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

void algo_fixSecondary(int16_t aperture)
{
	if(aperture >= SECONDARY_MINIMUM_OPENING && aperture <= SECONDARY_FULL_OPEN)
	{
		sec_aperture = aperture;
		fixed_sec = true;
	}else
	{
		fixed_sec = false;
	}


}
