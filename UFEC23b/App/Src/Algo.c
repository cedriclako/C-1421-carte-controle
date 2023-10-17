/*
 * Algo.c
 *
 *  Created on: Jul 12, 2023
 *      Author: crichard
 */

#include "ParamFile.h"
#include "HelperMacro.h"
#include "cmsis_os.h"
#include "stm32f1xx_hal_iwdg.h"
#include "main.h"
#include "WhiteBox.h"
#include "DebugPort.h"
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <math.h>
#include "message_buffer.h"
#include "GPIOManager.h"
#include "FlashMap.h"
#include "FanManager.h"
//#include "ProdTest.h"
//#include "MotorManager.h"
#include "ParticlesManager.h"
#include "EspBridge.h"
#include "TemperatureManager.h"
#include "DebugManager.h"
#include "Algo.h"
#include "GitCommit.h"

#define TAG "Algo"

extern MessageBufferHandle_t MotorControlsHandle;
extern QueueHandle_t MotorInPlaceHandle;
static bool motors_ready_for_req = false;
static bool bStepperAdjustmentNeeded = false;
static bool bStateExitConditionMet = false;
static bool tRiseEntry = false;
static State currentState = ZEROING_STEPPER;
static State lastState = ZEROING_STEPPER;
static State nextState = ZEROING_STEPPER;

static const char* m_StateStrings[ALGO_NB_OF_STATE] =
{
	HELPERMACRO_DEFSTRING(ZEROING_STEPPER),
	HELPERMACRO_DEFSTRING(WAITING),
	HELPERMACRO_DEFSTRING(RELOAD_IGNITION),
	HELPERMACRO_DEFSTRING(TEMPERATURE_RISE),
	HELPERMACRO_DEFSTRING(COMBUSTION_HIGH),
	HELPERMACRO_DEFSTRING(COMBUSTION_LOW),
	HELPERMACRO_DEFSTRING(COAL_LOW),
	HELPERMACRO_DEFSTRING(COAL_HIGH),
	HELPERMACRO_DEFSTRING(OVERTEMP),
	HELPERMACRO_DEFSTRING(SAFETY),
	HELPERMACRO_DEFSTRING(MANUAL_CONTROL)
};

typedef void (*fnComputeAdjustment)(Mobj *stove, uint32_t u32CurrentTime_ms);
typedef void (*fnSuperStateAction)(Mobj *stove);

// Adjustment functions for each state
static fnComputeAdjustment AlgoComputeAdjustment[ALGO_NB_OF_STATE];
static fnSuperStateAction AlgoStateEntryAction[ALGO_NB_OF_STATE];
static fnSuperStateAction AlgoStateExitAction[ALGO_NB_OF_STATE];
const PF_SuperStateParam_t* sStateParams[ALGO_NB_OF_STATE];
const PF_OverHeat_Thresholds_t *sOverheatParams;

static Mobj UFEC23;

extern IWDG_HandleTypeDef hiwdg;

static void Algo_fill_state_functions(void);
static void Algo_reload_action(Mobj* stove, uint32_t u32CurrentTime_ms);
static void Algo_Waiting_action(Mobj* stove, uint32_t u32CurrentTime_ms);
static void Algo_tempRise_action(Mobj* stove, uint32_t u32CurrentTime_ms);
static void Algo_combHigh_action(Mobj* stove, uint32_t u32CurrentTime_ms);
static void Algo_combLow_action(Mobj* stove, uint32_t u32CurrentTime_ms);
static void Algo_coalHigh_action(Mobj* stove, uint32_t u32CurrentTime_ms);
static void Algo_coalLow_action(Mobj* stove, uint32_t u32CurrentTime_ms);
static void Algo_manual_action(Mobj* stove, uint32_t u32CurrentTime_ms);
static void Algo_zeroing_action(Mobj* stove, uint32_t u32CurrentTime_ms);
static void Algo_safety_action(Mobj* stove, uint32_t u32CurrentTime_ms);
static void Algo_overtemp_action(Mobj* stove, uint32_t u32CurrentTime_ms);

static void Algo_waiting_entry(Mobj *stove);
static void Algo_reload_entry(Mobj *stove);
static void Algo_zeroing_entry(Mobj *stove);
static void Algo_tempRise_entry(Mobj *stove);
static void Algo_combLow_entry(Mobj *stove);
static void Algo_combHigh_entry(Mobj *stove);
static void Algo_coalLow_entry(Mobj *stove);
static void Algo_coalHigh_entry(Mobj *stove);

//static void Algo_waiting_exit(Mobj *stove);
static void Algo_reload_exit(Mobj *stove);
//static void Algo_zeroing_exit(Mobj *stove);
static void Algo_tempRise_exit(Mobj *stove);
static void Algo_combLow_exit(Mobj *stove);
static void Algo_combHigh_exit(Mobj *stove);
//static void Algo_coalLow_exit(Mobj *stove);
//static void Algo_coalHigh_exit(Mobj *stove);

static void Algo_task(Mobj *stove, uint32_t u32CurrentTime_ms);
static bool Algo_adjust_steppers_position(Mobj *stove);
static void Algo_update_steppers_inPlace_flag(void);
static void Algo_stoveInit(Mobj *stove);

int Algo_smoke_action(Mobj* stove, uint32_t u32CurrentTime_ms,int cycle_time, int dev_maxDiff,
		int particles_target,int particles_tolerance, uint32_t *correction_time, int deltaT_target);


//void printDebugStr(char str[], _Bool debugisOn);

//void printDebugStr(char str[], _Bool debugisOn) {
//  if(debugisOn){
//    printf(" %s \n\n", str);
//  }
//}

void Algo_Init(void const * argument)
{
	LOG(TAG, "\r\n\r\n");
	LOG(TAG, "--------------------------------");
	LOG(TAG, "BOOTING APP");
	LOG(TAG, "Git commit ID: %s, branch: '%s', dirty: %s", GITCOMMIT_COMMITID, GITCOMMIT_BRANCH, (GITCOMMIT_ISDIRTY ? "TRUE" : "FALSE"));
	LOG(TAG, "Internal flash: %"PRIu32" KB", (uint32_t)(FMAP_INTERNALFLASH_SIZE/1024));
	LOG(TAG, "--------------------------------");

	PARAMFILE_Init();
	PARAMFILE_Load();
	LOG(TAG, "Parameter file initialized");

	Algo_fill_state_functions();

	Algo_stoveInit(&UFEC23);
	Temperature_Init();
	LOG(TAG, "Temperature initialized");
	ESPMANAGER_Init();
	LOG(TAG, "ESP Manager initialized");
	Particle_Init();
	LOG(TAG, "Particle initialized");

	Fan_Init();
	// We want to be sure the system is ready before accepting to answer to any commands
	ESPMANAGER_SetReady();
	LOG(TAG, "Ready");

    for(;;)
    {
    	GPIOManager(&UFEC23,osKernelSysTick());
    	TemperatureManager(&UFEC23,osKernelSysTick());
    	DebugManager(&UFEC23,osKernelSysTick());
    	ESPMANAGER_Run();
    	ParticlesManager(osKernelSysTick());
    	Algo_task(&UFEC23, osKernelSysTick());
    	osDelay(1);
		#if WHITEBOX_WATCHDOG_ISDEACTIVATED == 0
    	HAL_IWDG_Refresh(&hiwdg);
		#endif
    	Fan_Process(&UFEC23);

    	//osDelay(1);
    }

}

static void Algo_task(Mobj *stove, uint32_t u32CurrentTime_ms)
{
	const PF_UsrParam* UsrParam =  PB_GetUserParam();
	const PF_OverHeat_Thresholds_t* OvrhtParams = PB_GetOverheatParams();

	Algo_update_steppers_inPlace_flag(); // Check if motor task is moving or in place

	if((currentState != SAFETY) && stove->bSafetyOn)
	{
        printDebugStr("switching to safety", print_debug_setup_states);
		nextState = SAFETY;
	}
	else if((currentState != MANUAL_CONTROL) && UsrParam->s32ManualOverride == 1)
	{

		nextState = MANUAL_CONTROL;
	}
	else if((currentState != OVERTEMP) && (currentState != SAFETY) && (
			(stove->fBaffleTemp > P2F(OvrhtParams->OverheatBaffle))  ||
			(stove->fChamberTemp > P2F(OvrhtParams->OverheatChamber)) ||
			(stove->fPlenumTemp > P2F(OvrhtParams->OverheatPlenum))) )
	{
		nextState = OVERTEMP;
	}
	// STATE ENTRY ACTION
	else if(stove->bstateJustChanged) // If first loop in state, perform entry action
	{
		stove->u32TimeOfStateEntry_ms = u32CurrentTime_ms;
		stove->bstateJustChanged = false;

		if(AlgoStateEntryAction[currentState] != NULL)
		{
			AlgoStateEntryAction[currentState](stove);
		}

	}
	else if(stove->bReloadRequested && (currentState == WAITING || currentState == COMBUSTION_LOW || currentState == COMBUSTION_HIGH ||
			currentState == COAL_LOW || currentState == COAL_HIGH || currentState == TEMPERATURE_RISE ))  /////// test condition bouton, MC 25-09-23
	{
		if(!stove->bInterlockOn) // Will need to be changed when we switch back interlock and start button
		{
			if((currentState == WAITING) && (stove->fBaffleTemp < 120.0))
			{
				Particle_requestZero();
			}
			nextState = RELOAD_IGNITION;
			stove->bReloadRequested = false;
			stove->bButtonBlinkRequired = true;
			stove->TimeOfReloadRequest = u32CurrentTime_ms;
		}
	}
	// STATE EXECUTION
	else // When we get here, check if it's time to compute an adjustment // to perform a state loop action
	{
		if((u32CurrentTime_ms - stove->u32TimeOfComputation_ms) > UsrParam->s32TimeBetweenComputations_ms)
		{
			Temperature_update_deltaT(stove,(u32CurrentTime_ms - stove->u32TimeOfComputation_ms));
			if((u32CurrentTime_ms - stove->u32TimeOfStateEntry_ms) > SECONDS(sStateParams[currentState]->i32EntryWaitTimeSeconds))
			{
				if(AlgoComputeAdjustment[currentState] != NULL)
				{
					AlgoComputeAdjustment[currentState](stove, u32CurrentTime_ms);
				}
			}
			stove->u32TimeOfComputation_ms = u32CurrentTime_ms;
			PrintOutput(stove, currentState, nextState, lastState);
		}

		else if(currentState == MANUAL_CONTROL) // If in manual control, we don't wait the computation time
		{										// But we still loop in the first 'if' once per computation period (to print output)
			if(AlgoComputeAdjustment[currentState] != NULL)
			{
				AlgoComputeAdjustment[currentState](stove, u32CurrentTime_ms);
			}
		}
		// STATE EXIT ACTION
		// Check if state timed out or if exit conditions are met
		if((((u32CurrentTime_ms - stove->u32TimeOfStateEntry_ms) > MINUTES(sStateParams[currentState]->i32MinimumTimeInStateMinutes)) && bStateExitConditionMet) ||
				((sStateParams[currentState]->i32MaximumTimeInStateMinutes != 0) && ((u32CurrentTime_ms - stove->u32TimeOfStateEntry_ms) > MINUTES(sStateParams[currentState]->i32MaximumTimeInStateMinutes))))
		{

			if(AlgoStateExitAction[currentState] != NULL)
			{
				AlgoStateExitAction[currentState](stove);
			}else
			{
				nextState = ZEROING_STEPPER;
			}
			bStateExitConditionMet = false;
		}
	}

	if(bStepperAdjustmentNeeded) // If an adjustment is requested, send configs to motors
	{

		if(Algo_adjust_steppers_position(stove))
		{
			stove->sPrimary.u8aperturePosSteps = RANGE(PF_PRIMARY_MINIMUM_OPENING,stove->sPrimary.u8apertureCmdSteps,PF_PRIMARY_FULL_OPEN);
			stove->sGrill.u8aperturePosSteps = RANGE(PF_GRILL_MINIMUM_OPENING,stove->sGrill.u8apertureCmdSteps,PF_GRILL_FULL_OPEN);
			stove->sSecondary.u8aperturePosSteps = RANGE(PF_SECONDARY_MINIMUM_OPENING,stove->sSecondary.u8apertureCmdSteps,PF_SECONDARY_FULL_OPEN);

			bStepperAdjustmentNeeded = false;
			stove->u32TimeOfAdjustment_ms = u32CurrentTime_ms;
		}
	}


	if(nextState != currentState) // Perform state change if requested
	{
		lastState = currentState;
		currentState = nextState;
		stove->bstateJustChanged = true;

		if((nextState == COMBUSTION_HIGH || nextState == COMBUSTION_LOW) && stove->u32TimeSinceCombEntry_ms == 0)
		{
			stove->u32TimeSinceCombEntry_ms = u32CurrentTime_ms;
		}
	}

}

static void Algo_stoveInit(Mobj *stove)
{
	stove->sParticles = ParticlesGetObject(); // Get pointer to particles Structure
	sOverheatParams = PB_GetOverheatParams();

	for(uint8_t i = 0;i < ALGO_NB_OF_STATE;i++)
	{
		sStateParams[i] = PB_GetSuperStateParams(i);
	}

	stove->u32TimeOfStateEntry_ms = 0;
	stove->u32TimeOfAdjustment_ms = 0;
	stove->u32TimeOfComputation_ms = 0;
	stove->u32TimeSinceCombEntry_ms = 0;
	stove->bReloadRequested = false;
	stove->bButtonBlinkRequired = false;
	stove->bstateJustChanged = true;
	stove->bSafetyOn = false;
	stove->TimeOfReloadRequest = 0;

	stove->sPrimary.u8apertureCmdSteps = MOTOR_HOME_CMD;
	stove->sGrill.u8apertureCmdSteps = MOTOR_HOME_CMD;
	stove->sSecondary.u8apertureCmdSteps = MOTOR_HOME_CMD;

	Algo_adjust_steppers_position(stove);
}

///////////////////////// STATE MACHINE  //////////////////////////////////////////////////////////////////////


//** STATE: ZEROING STEPPER **//
static void Algo_zeroing_entry(Mobj *stove)
{
	stove->sPrimary.u8apertureCmdSteps = 0;
	stove->sPrimary.fSecPerStep = 0;
	stove->sGrill.u8apertureCmdSteps = 0;
	stove->sGrill.fSecPerStep = 0;
	stove->sSecondary.u8apertureCmdSteps = 0;
	stove->sSecondary.fSecPerStep = 0;
	bStepperAdjustmentNeeded = true;
}

static void Algo_zeroing_action(Mobj* stove, uint32_t u32CurrentTime_ms)
{

	if(motors_ready_for_req)
	{
		nextState = WAITING;
	}
}

//static void Algo_zeroing_exit(Mobj *stove)
//{

//}
//** END: ZEROING STEPPER **//

//** STATE: WAITING **//
static void Algo_waiting_entry(Mobj *stove)
{
	const PF_WaitingParam_t *sParam = PB_GetWaitingParams();

	if((stove->fBaffleTemp > P2F(sParam->fTempToSkipWaiting)))
	{
		nextState = TEMPERATURE_RISE;
	}

}

static void Algo_Waiting_action(Mobj* stove, uint32_t u32CurrentTime_ms)
{
	const PF_WaitingParam_t *sParam = PB_GetWaitingParams();

	if(!stove->bInterlockOn)
	{
		if((stove->fBaffleTemp > P2F(sParam->fTempToQuitWaiting)))
		{
			stove->bReloadRequested = true;
		}
	}

}

//static void Algo_waiting_exit(Mobj *stove)
//{

//}

//** END: ZEROING STEPPER **//

//** STATE: RELOAD / IGNITION **//
static void Algo_reload_entry(Mobj* stove)
{
	const PF_ReloadParam_t *sParam = PB_GetReloadParams();
	stove->u32TimeSinceCombEntry_ms = 0;

	stove->sPrimary.u8apertureCmdSteps = sParam->sPrimary.i32Max;
	stove->sPrimary.fSecPerStep = 0; // force aperture
	stove->sGrill.u8apertureCmdSteps = sParam->sGrill.i32Max;
	stove->sGrill.fSecPerStep = 0; // force aperture
	stove->sSecondary.u8apertureCmdSteps = sParam->sSecondary.i32Max;
	stove->sSecondary.fSecPerStep = 0; // force aperture
	bStepperAdjustmentNeeded = true;


	if((stove->fBaffleTemp > P2F(sParam->fTempToSkipReload))) // NORMALEMENT 1000
	{
		nextState = TEMPERATURE_RISE;
	}

}

static void Algo_reload_action(Mobj* stove, uint32_t u32CurrentTime_ms)
{
	const PF_ReloadParam_t *sParam = PB_GetReloadParams();

	if((stove->fBaffleTemp > P2F(sParam->fTempToQuitReload))) // NORMALEMENT 525
	{
		nextState = TEMPERATURE_RISE;
	}
}

static void Algo_reload_exit(Mobj *stove)
{

}
//** END: RELOAD / IGNITION**//


//** STATE: TEMPERATURE RISE **//
static void Algo_tempRise_entry(Mobj* stove)
{

	// prend les params de moteur specifique à Trise et les met dans le pointer sParam pour les moteurs
	const PF_TriseParam_t *sParam = PB_GetTRiseParams();

	stove->u32TimeSinceCombEntry_ms = 0; // set le temps à zero


	// ----------------- set les vitesses et ouvertures pour les moteurs pour le début de l'état -------------------------------
	stove->sPrimary.u8apertureCmdSteps = sParam->sPrimary.i32Max;
	stove->sPrimary.fSecPerStep = 0; // force aperture
	stove->sGrill.u8apertureCmdSteps = sParam->sGrill.i32Max;
	stove->sGrill.fSecPerStep = 0; // force aperture
	stove->sSecondary.u8apertureCmdSteps = sParam->sSecondary.i32Max;
	stove->sSecondary.fSecPerStep = 0; // force aperture
	// --------------------------------------------------------------------------------------------

	bStepperAdjustmentNeeded = true; // leve le flag, donc la queue des stepper devrait le lire et faire l'action

	tRiseEntry = true;

}
// normalement on devrait à 525 rentrer dans ce cas
static void Algo_tempRise_action(Mobj* stove, uint32_t u32CurrentTime_ms)
{
	//TODO : définir cas où la température est plus basse, genre est-ce qu'on retourne en reload ?

	const PF_TriseParam_t *sParam = PB_GetTRiseParams(); // va chercher les paramètres de l'état
	static uint32_t u32MajorCorrectionTime_ms = 0;
	const PF_StepperStepsPerSec_t *sSpeedParams =  PB_SpeedParams(); // aller chercher les paramètres de vitesse de stepper selon l'état
	static int smoke_detected = 0;
	int cycle_time = 60;

	if((u32CurrentTime_ms - stove->u32TimeOfStateEntry_ms > SECONDS(cycle_time))&& tRiseEntry)
	{
		stove->sPrimary.u8apertureCmdSteps = 76;
		stove->sPrimary.fSecPerStep = 0;
		bStepperAdjustmentNeeded = true;
		tRiseEntry = false;
		printDebugStr("trise entry ! eta 11 A, set primary at 76", print_debug_setup_states);
	}


	// EXIT CONDITIONs TO COMBUSTION
	//if hot enough, or if primary <=75 go to comb states
	if((stove->bThermostatOn && stove->fBaffleTemp > P2F(sParam->fTempToCombHigh)) ||
			(!stove->bThermostatOn && stove->fBaffleTemp > P2F(sParam->fTempToCombLow))||
			stove->sPrimary.i8apertureCmdSteps <= 75 )
	{
		nextState = stove->bThermostatOn ? COMBUSTION_HIGH : COMBUSTION_LOW;
		printDebugStr("Trise Eta 11B exit to combustion", print_debug_setup_states);
		return;
	}

	// WAIT FOR NEXT LOOP
	// delta T baffle < 1 or time of correction < 60s
	if((u32MajorCorrectionTime_ms != 0 && (u32CurrentTime_ms - u32MajorCorrectionTime_ms < SECONDS(cycle_time))) ||
			(stove->fBaffleDeltaT < (1 + P2F1DEC(sParam->sTempSlope.fTarget) -
					P2F1DEC(sParam->sTempSlope.fTolerance))))
	{
		printDebugStr("eta 11C, delta t < 1,  i waited, next loop !", print_debug_setup_states);
		return;
	}


	// Todo : valider si on a besoin de la condition 500 deg ici
	// Section regulating (chg rapides)
	if(stove->fBaffleTemp >P2F(sParam->fTempToStartReg))
	{
		printDebugStr("t is over 500, regulating ", print_debug_setup_states);

/*
		// si particules > 80+50 && baffle delta T > 5 +5
		if((stove->sParticles->fparticles >
	(P2F(sParam->sParticles.fTarget) + P2F(sParam->sParticles.fTolerance))) &&
				((stove->fBaffleDeltaT > (P2F1DEC(sParam->sTempSlope.fTarget) +
						P2F1DEC(sParam->sTempSlope.fTolerance)) )||stove->fChamberTemp>1100))
		{

			if(stove->sGrill.i8apertureCmdSteps > sParam->sGrill.i32Min)
			{
				printDebugStr("etat 11D deltat > 10 && smoke", print_debug_setup_states);

				if (stove->sGrill.i8apertureCmdSteps > 15)
				{
					stove->sGrill.i8apertureCmdSteps /=2;
				}
				else
				{
					stove->sGrill.i8apertureCmdSteps = sParam->sGrill.i32Min;
				}
				stove->sGrill.fSecPerStep = 0;
			}


			else
			{
				printDebugStr("Eta 11 E \n", print_debug_setup_states);
				stove->sPrimary.i8apertureCmdSteps = 76;
				stove->sPrimary.fSecPerStep = 0;
			}

			bStepperAdjustmentNeeded = true;
			u32TimeOfMajorCorr = u32CurrentTime_ms;
			return;
		}
	}
*/

		if(Algo_smoke_action(stove, u32CurrentTime_ms,cycle_time, sParam->sPartStdev.fTolerance,
				sParam->sParticles.fTarget,sParam->sParticles.fTolerance, &u32MajorCorrectionTime_ms,P2F1DEC(sParam->sTempSlope.fTarget) +
				P2F1DEC(sParam->sTempSlope.fTolerance)))
		{
			smoke_detected = 1;
			return;
		}
		else
		{
			smoke_detected = 0;
			printDebugStr("Eta 11 E \n", print_debug_setup_states);

			bStepperAdjustmentNeeded = true;
			u32TimeOfMajorCorr = u32CurrentTime_ms;
			return;
		}
	}

	if(motors_ready_for_req)
	{
		if(stove->sGrill.u8apertureCmdSteps > sParam->sGrill.i32Min)
		{
			printDebugStr("etat 12 A : decrement grill \n\n", print_debug_setup_states);

			stove->sGrill.u8apertureCmdSteps--;
			stove->sGrill.fSecPerStep = P2F1DEC(sSpeedParams->fFast);
		}else
		{
			printDebugStr("eta 12 B : decrement primary ", print_debug_setup_states);

			stove->sPrimary.i8apertureCmdSteps--;

			stove->sPrimary.fSecPerStep = P2F1DEC(sSpeedParams->fFast);
		}

		bStepperAdjustmentNeeded = true;
	}


}

static void Algo_tempRise_exit(Mobj *stove)
{

}
//** END: TEMPERATURE RISE **//


//** STATE: COMBUSTION LOW **//
static void Algo_combLow_entry(Mobj *stove)
{
	const PF_CombustionParam_t *sParam = PB_GetCombLowParams();


	//TODO filtrer les valeurs d'ouverture précédentes pour que ce soit dans les valeurs acceptables
	// pour l'état dans lequel on rentre et si oui, garder les mêmes
	stove->sPrimary.u8apertureCmdSteps = RANGE(sParam->sPrimary.i32Min,stove->sPrimary.i8apertureCmdSteps,sParam->sPrimary.i32Max);
	// stove->sPrimary.i8apertureCmdSteps = sParam->sPrimary.i32Max;
	stove->sPrimary.fSecPerStep = 0; // force aperture

	if(lastState == TEMPERATURE_RISE)
	{
		stove->sGrill.u8apertureCmdSteps = sParam->sGrill.i32Min;
	}
	else
	{
		stove->sGrill.u8apertureCmdSteps = RANGE(sParam->sGrill.i32Min,stove->sGrill.i8apertureCmdSteps,sParam->sGrill.i32Max);
	}

	stove->sGrill.u8apertureCmdSteps = sParam->sGrill.i32Min;
	stove->sGrill.fSecPerStep = 0; // force aperture

	stove->sSecondary.i8apertureCmdSteps = RANGE(sParam->sSecondary.i32Min,stove->sSecondary.i8apertureCmdSteps,sParam->sSecondary.i32Max);
	//stove->sSecondary.i8apertureCmdSteps = sParam->sSecondary.i32Max;
	stove->sSecondary.fSecPerStep = 0; // force aperture
	bStepperAdjustmentNeeded = true;
}

/*
TODO : le cas de fumée doit être uniformisé pour chaque état principal de la machine
*/
static void Algo_combLow_action(Mobj* stove, uint32_t u32CurrentTime_ms)
{
	const PF_CombustionParam_t *sParam = PB_GetCombLowParams();

	static uint32_t u32MajorCorrectionTime_ms = 0;
	const PF_StepperStepsPerSec_t *sSpeedParams =  PB_SpeedParams();
	static int smoke_detected = 0;
	int cycle_time = 30;



	//EXIT CONDITIONS TODO : Vérifier avec Guillaume
	if((stove->fBaffleTemp < 500 && (u32CurrentTime_ms - stove->u32TimeOfStateEntry_ms) > MINUTES(30)) || // partie suivante ajoutée par charles, valider pertinence
			(stove->fBaffleTemp < (P2F(sParam->sTemperature.fTarget) - 2 * P2F(sParam->sTemperature.fAbsMaxDiff)) && !smoke_detected))
	{

		nextState = stove->bThermostatOn ? COAL_HIGH : COAL_LOW;
		bStateExitConditionMet = true;
		printDebugStr("exit to coal hi or low", print_debug_setup_states);
		return;
	}

	// WAITING FOR NEXT LOOP
	// TODO : mettre les bonnes variables
	if( (u32MajorCorrectionTime_ms != 0 && (u32CurrentTime_ms - u32MajorCorrectionTime_ms < SECONDS(cycle_time))) ||
			(stove->fBaffleTemp < (P2F(sParam->sTemperature.fTarget) - P2F(sParam->sTemperature.fTolerance)) &&
					fabs(stove->fBaffleDeltaT) <= 1  ) )
	{
		printDebugStr("cas 10 A wait until next loop", print_debug_setup_states);
		return;
	}

	// addition mc 11-10-2023, gestion generale des particules, mit également la déviation dans ce cas



	if(Algo_smoke_action(stove, u32CurrentTime_ms,cycle_time, sParam->sPartStdev.fTolerance,
			sParam->sParticles.fTarget,sParam->sParticles.fTolerance, &u32MajorCorrectionTime_ms,5))
	{
		smoke_detected = 1;
		printDebugStr("SMOKE ACTION\n\n", print_debug_setup_states);
		return;}

	else {
		smoke_detected = 0;}


	// if temperature is under 620 (630-10)
	if(stove->fBaffleTemp < (P2F(sParam->sTemperature.fTarget) - P2F(sParam->sTemperature.fTolerance)))
	{
		printDebugStr("TBaffle under 620", print_debug_setup_states);

		// si baffle deltaT < -5
		if(stove->fBaffleDeltaT < (P2F1DEC(sParam->sTempSlope.fTarget) - P2F1DEC(sParam->sTempSlope.fAbsMaxDiff)))
		{
			printDebugStr("\n Eta 10 B \n under temp slope target (-5)", print_debug_setup_states);

			// TODO setter une variable pour 36 dans ce cas
			stove->sPrimary.i8apertureCmdSteps = RANGE(sParam->sPrimary.i32Min,stove->sPrimary.i8apertureCmdSteps * 2, 36);
			stove->sPrimary.fSecPerStep = 0; // force aperture
			bStepperAdjustmentNeeded = true;
			u32MajorCorrectionTime_ms = u32CurrentTime_ms;
			return;
		}

		// moving slowly
		// cas Comb_Low_eta 10 C: (-5 < delta T baffle < -1 )
		else if(stove->fBaffleDeltaT < (P2F1DEC(sParam->sTempSlope.fTarget) - P2F1DEC(sParam->sTempSlope.fTolerance)))
		{
			printDebugStr("cas Comb_Low_eta 10 C ", print_debug_setup_states);


			if(motors_ready_for_req || stove->sPrimary.fSecPerStep == P2F1DEC(sSpeedParams->fVerySlow))
			{
				if(stove->sPrimary.u8apertureCmdSteps++ > sParam->sPrimary.i32Max)//Open by one step
				{
					stove->sPrimary.u8apertureCmdSteps = sParam->sPrimary.i32Max;
				}

				if(stove->sParticles->u16stDev > sParam->sPartStdev.fTolerance)
				{
					stove->sPrimary.fSecPerStep = P2F1DEC(sSpeedParams->fFast);
				}else
				{
					stove->sPrimary.fSecPerStep = P2F1DEC(sSpeedParams->fNormal);
				}

				bStepperAdjustmentNeeded = true;
				//est-ce qu'on devrait ajouter un write sur time of major correction ici ?
			}

		}
	}

	// if baffle temp is < 630 + 50
	else if(stove->fBaffleTemp < (P2F(sParam->sTemperature.fTarget) + P2F(sParam->sTemperature.fAbsMaxDiff)))
	{
		printDebugStr("baffletemp under 680", print_debug_setup_states);


		// cas 10E : abs(baffledeltaT) < 1
		if(fabs(stove->fBaffleDeltaT) < (P2F1DEC(sParam->sTempSlope.fTarget) + P2F1DEC(sParam->sTempSlope.fTolerance)))
		{

			printDebugStr(" cas 10D", print_debug_setup_states);

			if(motors_ready_for_req)
			{
				if(stove->sPrimary.u8apertureCmdSteps-- < sParam->sPrimary.i32Min)//Close by one step
				{
					stove->sPrimary.u8apertureCmdSteps = sParam->sPrimary.i32Min;
				}

				if(stove->sParticles->u16stDev > sParam->sPartStdev.fTolerance)
				{
					printDebugStr(" decrementing to slow down combustion at particulate deviation speed", print_debug_setup_states);
					stove->sPrimary.fSecPerStep = P2F1DEC(sSpeedParams->fNormal);
				}else if(stove->sParticles->fparticles > P2F(sParam->sParticles.fTarget + sParam->sParticles.fTolerance))
				{
					printDebugStr(" decrementing to slow down combustion at particulate over target speed", print_debug_setup_states);

					stove->sPrimary.fSecPerStep = P2F1DEC(sSpeedParams->fSlow);
				}else
				{
					printDebugStr(" decrementing to slow down combustion with no particulate speed", print_debug_setup_states);

					stove->sPrimary.fSecPerStep = P2F1DEC(sSpeedParams->fVerySlow);
				}


				bStepperAdjustmentNeeded = true;
				//est-ce qu'on devrait ajouter un write sur time of major correction ici ?
			}

		}
	}

	//T baffle > 670 vu qu'on est dans else, on est pas plus petit que 620 ou 680,
	else
	{
		if(stove->fBaffleDeltaT > P2F1DEC(sParam->sTempSlope.fTarget + sParam->sTempSlope.fAbsMaxDiff))
		{

			printDebugStr("Cas 10 E", print_debug_setup_states);


			if(motors_ready_for_req || (stove->sPrimary.fSecPerStep == P2F1DEC(sSpeedParams->fSlow)) || (stove->sPrimary.fSecPerStep == P2F1DEC(sSpeedParams->fVerySlow)))
			{
				if(stove->sPrimary.u8apertureCmdSteps-- < sParam->sPrimary.i32Min)
				{
					stove->sPrimary.u8apertureCmdSteps = sParam->sPrimary.i32Min;
				}

				if(stove->sParticles->u16stDev > sParam->sPartStdev.fTolerance)
				{
					stove->sPrimary.fSecPerStep = P2F1DEC(sSpeedParams->fFast);
					printDebugStr(" decrementing to slow down combustion at particulate deviation speed", print_debug_setup_states);

				}
				else
				{
					stove->sPrimary.fSecPerStep = P2F1DEC(sSpeedParams->fNormal);
				}

				bStepperAdjustmentNeeded = true;
				//est-ce qu'on devrait ajouter un write sur time of major correction ici ?
				return;
			}
		}

		if(stove->fBaffleDeltaT > P2F1DEC(sParam->sTempSlope.fTarget - sParam->sTempSlope.fTolerance))
		{

			printDebugStr("baffleDeltaT > 1, cas 10F ", print_debug_setup_states);

			if(motors_ready_for_req || stove->sPrimary.fSecPerStep == P2F1DEC(sSpeedParams->fVerySlow))
			{
				if(stove->sPrimary.u8apertureCmdSteps-- < sParam->sPrimary.i32Min)
				{
					stove->sPrimary.u8apertureCmdSteps = sParam->sPrimary.i32Min;
				}

				if(stove->sParticles->u16stDev > sParam->sPartStdev.fTolerance)
				{
					stove->sPrimary.fSecPerStep = P2F1DEC(sSpeedParams->fFast);
				}else
				{
					stove->sPrimary.fSecPerStep = P2F1DEC(sSpeedParams->fNormal);
				}

				bStepperAdjustmentNeeded = true;

			}
		}
	}

}

static void Algo_combLow_exit(Mobj *stove)
{
	if(bStateExitConditionMet)
	{
		nextState = COAL_LOW;
	}
	else
	{
		nextState = ZEROING_STEPPER;
	}
}

//** END: COMBUSTION LOW **//

//** STATE: COMBUSTION HIGH **//
static void Algo_combHigh_entry(Mobj *stove)
{
	//const PF_CombustionParam_t *sParam = PB_GetCombHighParams();


}


static void Algo_combHigh_action(Mobj* stove, uint32_t u32CurrentTime_ms)
{
	//const PF_CombustionParam_t *sParam = PB_GetCombHighParams();
}

static void Algo_combHigh_exit(Mobj *stove)
{

}

//** END: COMBUSTION HIGH **//

//** STATE: COAL LOW **//
static void Algo_coalLow_entry(Mobj *stove)
{
	//const PF_CoalParam_t *sParam = PB_GetCoalLowParams();

}

static void Algo_coalLow_action(Mobj* stove, uint32_t u32CurrentTime_ms)
{
	const PF_CoalParam_t *sParam = PB_GetCoalLowParams();

	if(stove->sParticles->u16stDev > sParam->sPartStdev.fTolerance)
	{
		nextState = COMBUSTION_LOW;
		return;
	}


	if((stove->fBaffleTemp < P2F( sParam->sTemperature.fTarget)) && stove->sGrill.u8apertureCmdSteps != sParam->sGrill.i32Max)
	{
		stove->sGrill.u8apertureCmdSteps = sParam->sGrill.i32Max;
		stove->sGrill.fSecPerStep = 0; // force aperture
		bStepperAdjustmentNeeded = true;

	}

		//Ne pas être en coal s'il y a de la fumée (MUST NOT)
	// Timer de combustion (low et high)
		//TODO: Regarder après avoir ouvert la grille, temp et particules++ --> ****(en tout temps pour fumée)si remontée, on retourne en comb
	//Ça pourrait être le entry action, mettre des thresholds temp, parts

	if(u32CurrentTime_ms - stove->u32TimeOfAdjustment_ms > MINUTES(sParam->i32TimeBeforeMovingPrim) && (stove->sPrimary.u8apertureCmdSteps != sParam->sPrimary.i32Min))
	{
		stove->sPrimary.u8apertureCmdSteps = sParam->sPrimary.i32Min;
		stove->sPrimary.fSecPerStep = 0; // force aperture
		bStepperAdjustmentNeeded = true;

	}

	if(u32CurrentTime_ms - stove->u32TimeOfAdjustment_ms > MINUTES(sParam->i32TimeBeforeMovingSec) && (stove->sSecondary.u8apertureCmdSteps != sParam->sSecondary.i32Min))
	{
		stove->sSecondary.u8apertureCmdSteps = sParam->sSecondary.i32Min;
		stove->sSecondary.fSecPerStep = 0; // force aperture
		bStepperAdjustmentNeeded = true;

	}


}

//static void Algo_coalLow_exit(Mobj *stove)
//{

//}
//** END: COAL LOW **//

//** STATE: COAL HIGH **//
static void Algo_coalHigh_entry(Mobj *stove)
{
	//const PF_CoalParam_t *sParams = PB_GetCoalHighParams();
}

static void Algo_coalHigh_action(Mobj* stove, uint32_t u32CurrentTime_ms)
{
	//const PF_CoalParam_t *sParams = PB_GetCoalHighParams();
}

//static void Algo_coalHigh_exit(Mobj *stove)
//{

//}
//** END: COAL HIGH **//

static void Algo_manual_action(Mobj* stove, uint32_t u32CurrentTime_ms)
{
	const PF_UsrParam* sManParam = PB_GetUserParam();

	if(!(sManParam->s32ManualOverride == 1)) // TODO: Put this in task function
	{
		nextState = lastState;

		if(nextState == SAFETY)
		{
			nextState = ZEROING_STEPPER;
		}

		if(nextState == ZEROING_STEPPER)
		{
			motors_ready_for_req = false;
		}

		return;
	}

	if(stove->sPrimary.u8apertureCmdSteps != sManParam->s32ManualPrimary ||
			stove->sGrill.u8apertureCmdSteps != sManParam->s32ManualGrill ||
			stove->sSecondary.u8apertureCmdSteps != sManParam->s32ManualSecondary)
	{
		stove->sPrimary.u8apertureCmdSteps = sManParam->s32ManualPrimary;
		stove->sPrimary.fSecPerStep = 0; // force aperture
		stove->sGrill.u8apertureCmdSteps = sManParam->s32ManualGrill;
		stove->sGrill.fSecPerStep = 0; // force aperture
		stove->sSecondary.u8apertureCmdSteps = sManParam->s32ManualSecondary;
		stove->sSecondary.fSecPerStep = 0; // force aperture
		bStepperAdjustmentNeeded = true;
	}


}

static void Algo_safety_action(Mobj* stove, uint32_t u32CurrentTime_ms)
{
	printDebugStr("Safety condition activated", print_debug_setup_states);

	if(!stove->bSafetyOn)
	{
		nextState = lastState;
	}
}

static void Algo_overtemp_action(Mobj* stove, uint32_t u32CurrentTime_ms)
{
	const PF_OverHeat_Thresholds_t* OvrhtParams = PB_GetOverheatParams();

	if((stove->fBaffleTemp < P2F(OvrhtParams->OverheatBaffle))  &&
				(stove->fChamberTemp < P2F(OvrhtParams->OverheatChamber)) &&
				(stove->fPlenumTemp < P2F(OvrhtParams->OverheatPlenumExit)) )
		{
			nextState = lastState;
		}
}

///////////////// Handle and low level functions /////////////////////////////////////////

void Algo_fill_state_functions(void)
{
	AlgoComputeAdjustment[ZEROING_STEPPER] = Algo_zeroing_action;
	AlgoComputeAdjustment[WAITING] = Algo_Waiting_action;
	AlgoComputeAdjustment[RELOAD_IGNITION] = Algo_reload_action;
	AlgoComputeAdjustment[TEMPERATURE_RISE] = Algo_tempRise_action;
	AlgoComputeAdjustment[COMBUSTION_HIGH] = Algo_combHigh_action;
	AlgoComputeAdjustment[COMBUSTION_LOW] = Algo_combLow_action;
	AlgoComputeAdjustment[COAL_LOW] = Algo_coalLow_action;
	AlgoComputeAdjustment[COAL_HIGH] = Algo_coalHigh_action;
	AlgoComputeAdjustment[OVERTEMP] = Algo_overtemp_action;
	AlgoComputeAdjustment[SAFETY] = Algo_safety_action;
	AlgoComputeAdjustment[MANUAL_CONTROL] = Algo_manual_action;

	AlgoStateEntryAction[ZEROING_STEPPER] = Algo_zeroing_entry;
	AlgoStateEntryAction[WAITING] = Algo_waiting_entry;
	AlgoStateEntryAction[RELOAD_IGNITION] = Algo_reload_entry;
	AlgoStateEntryAction[TEMPERATURE_RISE] = Algo_tempRise_entry;
	AlgoStateEntryAction[COMBUSTION_HIGH] = Algo_combHigh_entry;
	AlgoStateEntryAction[COMBUSTION_LOW] = Algo_combLow_entry;
	AlgoStateEntryAction[COAL_LOW] = Algo_coalLow_entry;
	AlgoStateEntryAction[COAL_HIGH] = Algo_coalHigh_entry;
	AlgoStateEntryAction[OVERTEMP] = Algo_zeroing_entry;
	AlgoStateEntryAction[SAFETY] = Algo_zeroing_entry;
	AlgoStateEntryAction[MANUAL_CONTROL] = NULL;

	AlgoStateExitAction[ZEROING_STEPPER] = NULL;
	AlgoStateExitAction[WAITING] = NULL;
	AlgoStateExitAction[RELOAD_IGNITION] = Algo_reload_exit;
	AlgoStateExitAction[TEMPERATURE_RISE] = Algo_tempRise_exit;
	AlgoStateExitAction[COMBUSTION_HIGH] = Algo_combHigh_exit;
	AlgoStateExitAction[COMBUSTION_LOW] = Algo_combLow_exit;
	AlgoStateExitAction[COAL_LOW] = NULL;
	AlgoStateExitAction[COAL_HIGH] = NULL;
	AlgoStateExitAction[OVERTEMP] = NULL;
	AlgoStateExitAction[SAFETY] = NULL;
	AlgoStateExitAction[MANUAL_CONTROL] = NULL;

}

static void Algo_update_steppers_inPlace_flag(void)
{
	if(!motors_ready_for_req)
	{
		xQueueReceive(MotorInPlaceHandle,&motors_ready_for_req,5);
	}
}

static bool Algo_adjust_steppers_position(Mobj *stove)
{
	uint8_t cmd[NUMBER_OF_STEPPER_CMDS] =
	{
		stove->sPrimary.u8apertureCmdSteps,
		(uint8_t)(stove->sPrimary.fSecPerStep*10),
		stove->sGrill.u8apertureCmdSteps,
		(uint8_t)(stove->sGrill.fSecPerStep*10),
		stove->sSecondary.u8apertureCmdSteps,
		(uint8_t)(stove->sSecondary.fSecPerStep*10)

	};

	if(!xMessageBufferSend(MotorControlsHandle,cmd,NUMBER_OF_STEPPER_CMDS,0))
	{
		return false;
	}
	return true;
}

const Mobj* ALGO_GetObjData()
{
	return &UFEC23;
}

State ALGO_GetCurrentState()
{
	return currentState;
}

const char* ALGO_GetStateString(State state)
{
	if (state >= ALGO_NB_OF_STATE)
		return "-- N/A --";
	return m_StateStrings[currentState];
}


 int Algo_smoke_action(Mobj* stove, uint32_t u32CurrentTime_ms,int cycle_time, int dev_maxDiff, int particles_target,int particles_tolerance,
		 uint32_t* correction_time, int deltaT_target )
{
	const PF_CombustionParam_t *sParam = PB_GetCombLowParams();
	const PF_StepperStepsPerSec_t *sSpeedParams =  PB_SpeedParams();
	bool delay_period_passed = (u32CurrentTime_ms - *correction_time) > SECONDS(cycle_time);

	if (currentState == TEMPERATURE_RISE){const PF_TriseParam_t *sParam = PB_GetTRiseParams();}

	else if(currentState ==  COMBUSTION_LOW){const PF_CombustionParam_t *sParam = PB_GetCombLowParams();}

	else if(currentState ==  COMBUSTION_HIGH){const PF_CombustionParam_t *sParam = PB_GetCombHighParams();}

	else if(currentState ==  COAL_LOW){const PF_CoalParam_t *sParam = PB_GetCoalLowParams();}

	else if(currentState ==  COAL_HIGH){const PF_CoalParam_t *sParam = PB_GetCoalHighParams();}



	if((stove->sParticles->fparticles > (P2F(particles_target) + P2F(particles_tolerance))) &&	(stove->fBaffleDeltaT > deltaT_target ||stove->fChamberTemp>1100))
	{
		if(delay_period_passed ){
//		if(stove->sGrill.i8apertureCmdSteps > sParam->sGrill.i32Min)
	//	{
			printDebugStr("SMOKE HOT", print_debug_setup_states);

			if (stove->sGrill.i8apertureCmdSteps > 15)
			{
				stove->sGrill.i8apertureCmdSteps /=2;
			}
			else if(stove->sPrimary.i8apertureCmdSteps> 76 && stove->sGrill.i8apertureCmdSteps >  sParam->sGrill.i32Min)
			{
				stove->sGrill.i8apertureCmdSteps = sParam->sGrill.i32Min;
				stove->sPrimary.i8apertureCmdSteps = 76;
			}
			else
			{
				stove->sGrill.i8apertureCmdSteps = sParam->sGrill.i32Min;
				stove->sPrimary.i8apertureCmdSteps /=2;
			}


			stove->sPrimary.fSecPerStep = 0;
			stove->sGrill.fSecPerStep = 0;
		//}
		bStepperAdjustmentNeeded = true;
		*correction_time = u32CurrentTime_ms;

		}
		return 1 ;
	}



	if((stove->sParticles->fparticles > (P2F(particles_target) + P2F(particles_tolerance)) // (P2F1DEC(sParam->sParticles.fTarget) + P2F1DEC(sParam->sParticles.fTolerance))
			|| (stove->sParticles->u16stDev > dev_maxDiff))&& stove->fBaffleDeltaT <= -10	){

		// take action if the 30 seconds have passed
		if(delay_period_passed){
			printDebugStr("\n\nSMOKE COLD", print_debug_setup_states);
		if(stove->sGrill.i8apertureCmdSteps < 30)
		{
			stove->sGrill.i8apertureCmdSteps = 30;
			printDebugStr("smoke && grille < 30 : set grill to 30", print_debug_setup_states);
		}
		else
		{
			stove->sGrill.i8apertureCmdSteps  = RANGE(sParam->sGrill.i32Min, stove->sGrill.i8apertureCmdSteps *2,sParam->sGrill.i32Max);
			printDebugStr("smoke && grille >= 30 : set grill *=2 ", print_debug_setup_states);
		}


		stove->sGrill.fSecPerStep = 0; // force aperture
		bStepperAdjustmentNeeded = true;
		*correction_time = u32CurrentTime_ms;
		}
		return 1 ;
	}

	RANGE()
	return 0; // no smoke return 0

}
