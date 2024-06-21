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
#include "DebugPort.h"
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <math.h>
#include <WhiteBox.h>
#include "message_buffer.h"
#include "GPIOManager.h"
#include "FlashMap.h"
#include "FanManager.h"
#include "ParticlesManager.h"
#include "EspBridge.h"
#include "TemperatureManager.h"
#include "DebugManager.h"
#include "Algo.h"
#include "GitCommit.h"

#define TAG "Algo"
#define LEN(x)  (sizeof(x) / sizeof((x)[0]))

typedef struct
{
  uint8_t u8prim_pos;
  uint8_t u8sec_pos;
  uint8_t u8grill_pos;

  uint32_t u32StateEntryMem_ms;
  State sPreviousState;

  Fan_Speed_t eFanDistSpeed;
  Fan_Speed_t eFanLowSpeed;
}State_mem_t;



extern MessageBufferHandle_t MotorControlsHandle;
extern QueueHandle_t MotorInPlaceHandle;
static bool motors_ready_for_req = false;
static bool bStepperAdjustmentNeeded = false;
static bool bStateExitConditionMet = false;
static bool tRiseEntry = false;
static bool hot_reload = false;
static bool tStat_just_changed = false;
bool tstat_status = false;
static int temp_coal_entry;
static int smoke_history[] = {0,0,0,0,0,0,0,0,0,0};
static int grill_history[] = {0,0,0,0,0,0,0,0,0,0};
static int particles_zero_counter = 0 ;


static State currentState = ZEROING_STEPPER;
static State lastState = ZEROING_STEPPER;
static State nextState = ZEROING_STEPPER;
static State_mem_t sBoostConfMem;
static State_mem_t sManualConfMem;
uint32_t time_of_door_open = 0;
bool previous_door_status = false;


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
    HELPERMACRO_DEFSTRING(MANUAL_CONTROL),
    HELPERMACRO_DEFSTRING(BOOST)
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
static SmokeStruct sSmoke;

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
static void Algo_boost_action(Mobj* stove, uint32_t u32CurrentTime_ms);

static void Algo_waiting_entry(Mobj *stove);
static void Algo_reload_entry(Mobj *stove);
static void Algo_zeroing_entry(Mobj *stove);
static void Algo_tempRise_entry(Mobj *stove);
static void Algo_combLow_entry(Mobj *stove);
static void Algo_combHigh_entry(Mobj *stove);
static void Algo_coalLow_entry(Mobj *stove);
static void Algo_coalHigh_entry(Mobj *stove);
static void Algo_boost_entry(Mobj *stove);
static void Algo_manual_entry(Mobj *stove);

//static void Algo_waiting_exit(Mobj *stove);
//static void Algo_reload_exit(Mobj *stove);
//static void Algo_zeroing_exit(Mobj *stove);
static void Algo_tempRise_exit(Mobj *stove);
static void Algo_combLow_exit(Mobj *stove);
static void Algo_combHigh_exit(Mobj *stove);
static void Algo_boost_exit(Mobj *stove);
static void Algo_manual_exit(Mobj *stove);
//static void Algo_coalLow_exit(Mobj *stove);
//static void Algo_coalHigh_exit(Mobj *stove);

static void smoke_init(SmokeStruct *sSmoke);
static void Algo_task(Mobj *stove, uint32_t u32CurrentTime_ms);
static bool Algo_adjust_steppers_position(Mobj *stove);
static void Algo_update_steppers_inPlace_flag(void);
static void Algo_stoveInit(Mobj *stove);

static void debug_printer(char str[], _Bool debugisOn);
static void array_setter(int array[], int l, int newval);
static void array_shifter(int array[], int l,int newval);
static void array_printer(int array[],int l);

bool get_motors_ready_status(){return motors_ready_for_req;}

static void aperture_adjust(Mobj* stove,
    PF_CombustionParam_t *sParam,
    int change_var,
    int w_part_dev_change_speed,
    int w_part_change_speed,
    int no_part_change_speed ,
    bool add_condition1,
    bool add_condition2);

static void comb_temperature_control(Mobj* stove, PF_CombustionParam_t* sParam,  uint32_t u32CurrentTime_ms);
static int Algo_smoke_action(Mobj* stove, uint32_t u32CurrentTime_ms,int wait_cycle_time);



void Algo_Init(void const * argument)
{
  LOG(TAG, "\r\n\r\n");
  LOG(TAG, "--------------------------------");
#ifdef DEBUG
  LOG(TAG, "BOOTING APP-BIN (DEBUG)");
#else
  LOG(TAG, "BOOTING APP-BIN (RELEASE)");
#endif

  // TODO: s'Assurer que l'information de la version du code stm32 sorte ici mais surtout dans la page web du esp32. (à faire: Cedric voir page system info, section Server (STM32/Stove) information )
  // s'appelle dans esp : pMemBlock->sSrvFWInfo.sVersion.u8Major

  LOG(TAG, "Version: %d.%d.%d", (int)BM_g_sMarker.u8Versions[0], (int)BM_g_sMarker.u8Versions[1], (int)BM_g_sMarker.u8Versions[2]);
  LOG(TAG, "Git commit ID: %s, branch: '%s', dirty: %s", BM_g_sMarker.u8GitCommitID, BM_g_sMarker.u8GitBranch, (BM_g_sMarker.u8GitIsDirty ? "TRUE" : "FALSE"));
  LOG(TAG, "Compile: %s %s", BM_g_sMarker.u8CompileDate, BM_g_sMarker.u8CompileTime);
  LOG(TAG, "Internal flash: %" PRIu32" KB", (uint32_t)(FMAP_INTERNALFLASH_SIZE/1024));
  FMAP_Init(); // App size and CRC32 get calculated there
  const FMAP_SFileInfo* pFileInfo = FMAP_GetAppFileInfo();
  // If the CRC32 cannot be calculated, it means something is wrong with the trailing sequence.
  // fix in into the linker file.
  if (pFileInfo->u32Size > 0)
    LOG(TAG, "Firmware, size: %" PRIu32 ", crc32: 0x%" PRIx32, pFileInfo->u32Size, pFileInfo->u32CRC32);
  else
    LOG(TAG, "ERROR: Cannot calculate CRC32 and size, this need to be addressed");
  LOG(TAG, "--------------------------------");
#ifndef DEBUG
  LOG(TAG, "WARNING: In release, parameter are not saved");
#endif

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

  smoke_init(&sSmoke);

  Fan_Init();
  // We want to be sure the system is ready before accepting to answer to any commands
  ESPMANAGER_SetReady();
  LOG(TAG, "Ready");

#if WHITEBOX_SANITY_LED != 0
  bool bSanity = false;
  uint32_t u32SanityTicks = osKernelSysTick();
#endif

  for(;;)
  {
    GPIOManager(&UFEC23,osKernelSysTick());
    TemperatureManager(&UFEC23,osKernelSysTick());
    DebugManager(&UFEC23,osKernelSysTick());
    ESPMANAGER_Run();
#if WHITEBOX_SANITY_LED == 0
    ParticlesManager(osKernelSysTick());
#endif
    Algo_task(&UFEC23, osKernelSysTick());
    osDelay(1);

    //TODO: enable le watchdog pour prod (Maxime Carrier)
#if WHITEBOX_DISABLE_WATCHDOG == 0
    HAL_IWDG_Refresh(&hiwdg);
#endif
    Fan_Process(&UFEC23);
    //osDelay(1);

#if WHITEBOX_SANITY_LED != 0
    if ( (osKernelSysTick() - u32SanityTicks) > pdMS_TO_TICKS(100) )
    {
      HAL_GPIO_WritePin(Reset_Particles_Sensor_GPIO_Port,Reset_Particles_Sensor_Pin, (bSanity ? GPIO_PIN_SET : GPIO_PIN_RESET));
      u32SanityTicks = osKernelSysTick();
      bSanity = !bSanity;
    }
#endif
  }
}

static void Algo_task(Mobj *stove, uint32_t u32CurrentTime_ms)

/*
 * @Algo_task
 *
 * This function acts as the main engine of the state machine. It checks different parameters for status changes :
 * (RmtParams for the remote control, over-temperature, manual control, thermostat status change, open door,
 * or any other that are relevant to be checked at the beginning of or before a new state)
 * It then goes into either a state entry, state action or state exit function depending
 * on if the state just changed, if the state has timed out or if the exit conditions have been met.
 * Then it checks if a change of the steppers position has been requested and sends it to MotorManager if needed.
 *
 * The current state in which we find ourselves is determined in the states themselves,
 * but in general, the order would be :
 *
 * 1. The machine starts, the main application object is initialized
 * 2. The steppers do a homing, we go in "ZEROING_STEPPER" and confirm that the homing is complete and
 *    motors_ready_for_req.
 *
 * 3. The state changes to WAITING and we wait for the user to generate bReloadRequested by pressing the start button.
 * 4. The machine goes to RELOAD_IGNITION, the temperature rises until a predefined level and goes to
 *    TEMPERATURE_RISE.
 *
 * 5. Then we start closing the apertures and once we reach a certain level we go to combustion high or low
 *    depending on the thermostat state.
 *
 *
 *  Talk about the following :
 *
 *  paramfile.c
 *  temperaturemanager.c
 *  motormanager.c
 *  particlesmanager.c
 *
 *
 *
 *
 *
 * Operation*/
{
  const PF_UsrParam* UsrParam =  PB_GetUserParam();
  const PF_OverHeat_Thresholds_t* OvrhtParams = PB_GetOverheatParams();
  const PF_RemoteParams_t* RmtParams = PB_GetRemoteParams();
  bool min_time_in_state_met = 0;
  bool stateTimeout = 0;
  const int door_open_time_limit = 20;
  static uint32_t time_stove_zero = 0;

  Algo_update_steppers_inPlace_flag();


  stateTimeout = ((sStateParams[currentState]->i32MaximumTimeInStateMinutes != 0)
      && ((u32CurrentTime_ms - stove->u32TimeOfStateEntry_ms)
          > MINUTES(sStateParams[currentState]->i32MaximumTimeInStateMinutes)));

  min_time_in_state_met = (u32CurrentTime_ms - stove->u32TimeOfStateEntry_ms)
	              > MINUTES(sStateParams[currentState]->i32MinimumTimeInStateMinutes);


  // update tstat_status with either remote param or GPIO
  tstat_status = RmtParams->bThermostat || stove->bThermostatOn;




  // Record door open as major correction event
  //  so that we don't make any drastic moves 30 seconds after closing door
  if(stove->bDoorOpen){
    stove->u32MajorCorrectionTime_ms = u32CurrentTime_ms;
  }


  // Manage  door open time, door sensor issue ?
  if (!previous_door_status && stove->bDoorOpen)
  {
    time_of_door_open = u32CurrentTime_ms;
  }

  else if(stove->bDoorOpen && (u32CurrentTime_ms - time_of_door_open) >
  MINUTES(door_open_time_limit) && stove->fBaffleTemp > 500 ) // door time limit : 20 min currently
  {
    printf("Inform user that their door has been open for :%i minutes, "
        "there might be a door sensor issue.",door_open_time_limit);
    // TODO: Do we want to go to safety here ? reminder:
    // door open keeps us from doing any move, this is bad if we're coming from reload because everything stays open.
  }

  previous_door_status = stove->bDoorOpen;





  // STATE MACHINE ENGINE

  // Starting with alternative states events.

  // Safety event
  if((currentState != SAFETY) && stove->bSafetyOn)
  {
    debug_printer("switching to safety", print_debug_setup);
    nextState = SAFETY;
  }

  // Manual request
  else if((currentState != MANUAL_CONTROL) && (UsrParam->s32ManualOverride == 1))
  {
    // recording of time of entry in manual mode
    sManualConfMem.u32StateEntryMem_ms = stove->u32TimeOfStateEntry_ms;
    nextState = MANUAL_CONTROL;
  }

  // BOOST request
  else if((currentState != MANUAL_CONTROL)
      && (currentState != BOOST)
      && (currentState != ZEROING_STEPPER)
      && (RmtParams->bBoostReq == 1))
  {
    sBoostConfMem.u32StateEntryMem_ms = stove->u32TimeOfStateEntry_ms;
    nextState = BOOST;
  }

  // Overtemp event
  else if((currentState != OVERTEMP)
      && (currentState != SAFETY)
      && ((stove->fBaffleTemp > P2F(OvrhtParams->OverheatBaffle))  ||
          (stove->fChamberTemp > P2F(OvrhtParams->OverheatChamber)) ||
          (stove->fPlenumTemp > P2F(OvrhtParams->OverheatPlenum)))
  )
  {
    nextState = OVERTEMP;
  }


  /* State entry actions
   *
   * If none of the special state events are triggered, do a normal state entry */

  else if(stove->bstateJustChanged)
  {
    if((lastState != BOOST) && (lastState != MANUAL_CONTROL))
    {

      // recording of state time of entry
      // if state change is consequence of thermostat change, keep the same time of entry
      stove->u32TimeOfStateEntry_ms = tStat_just_changed
          ? stove->u32TimeOfStateEntry_ms
              : u32CurrentTime_ms;

      // Do state entry actions
      if(AlgoStateEntryAction[currentState] != NULL)
      {
        AlgoStateEntryAction[currentState](stove);
      }

    }
    stove->bstateJustChanged = false;

  }


  // reload request call
  else if(stove->bReloadRequested
      && (currentState == WAITING
          || currentState == COMBUSTION_LOW
          || currentState == COMBUSTION_HIGH
          || currentState == COAL_LOW
          || currentState == COAL_HIGH
          || currentState == TEMPERATURE_RISE))
  {


    if( (currentState == WAITING || currentState == RELOAD_IGNITION)
        && stove->fBaffleTemp < 150.0
        && stove->fBaffleDeltaT < 1
        && stove->sParticles->u16stDev < 10 )
    {
      particles_zero_counter = 0 ;
      Particle_requestZero();
      time_stove_zero = u32CurrentTime_ms;
    }


    nextState = RELOAD_IGNITION;
    stove->bReloadRequested = false;
    stove->bButtonBlinkRequired = true;
    stove->TimeOfReloadRequest = u32CurrentTime_ms;
  }





  // Do a self particles zero while burning
  else if( ((u32CurrentTime_ms - time_stove_zero) > SECONDS(300))
      && stove->fBaffleTemp < 650.0
      && stove->fBaffleDeltaT < 1
      && stove->sParticles->u16stDev < 4
      && fabs(stove->sParticles->fparticles) > 10 // (if normalized particles are off by value outside of +- 10)
      && particles_zero_counter < 3
      && (currentState == COMBUSTION_LOW
          || currentState == COMBUSTION_HIGH
          || currentState == COAL_LOW
          || currentState == COAL_HIGH ))
  {
    particles_zero_counter ++ ;
    Particle_requestZero();
    time_stove_zero = u32CurrentTime_ms;

    // stove->u32MajorCorrectionTime_ms = u32CurrentTime_ms;
    debug_printer("\nSelf Reset Normalized Particles Zero!\n", print_debug_setup);
  }





  // Normal state action. Do a state action loop
  else
  {

    // If the normal delay between state loops has passed, do a state loop, otherwise, skip
    if((u32CurrentTime_ms - stove->u32TimeOfComputation_ms) > UsrParam->s32TimeBetweenComputations_ms)
    {
      // Delta T Average calculation
      Temperature_update_deltaT(stove,(u32CurrentTime_ms - stove->u32TimeOfComputation_ms));
      if(state_entry_delays_skip ||((u32CurrentTime_ms - stove->u32TimeOfStateEntry_ms) > SECONDS(sStateParams[currentState]->i32EntryWaitTimeSeconds)))
      {
        tStat_just_changed = false ;

        if(AlgoComputeAdjustment[currentState] != NULL)
        {
          // If there is a state action for a given state, do action
          AlgoComputeAdjustment[currentState](stove, u32CurrentTime_ms);
        }
      }
      // record the time of the loop completion
      stove->u32TimeOfComputation_ms = u32CurrentTime_ms;

      // Debugger serial printing
      PrintOutput(stove, currentState, nextState, lastState);

    }

    // If set state is manual, do the manual action between loops.
    else if(currentState == MANUAL_CONTROL)
    {
      if(AlgoComputeAdjustment[currentState] != NULL)
      {
        AlgoComputeAdjustment[currentState](stove, u32CurrentTime_ms);
      }
    }




    // NORMAL STATE EXIT ACTION
    // Check if exit conditions are met and if minimum time is met or if state timed out


    if(( bStateExitConditionMet && ( state_entry_delays_skip || min_time_in_state_met ) )
        || stateTimeout )
    {
      // if you don't want to end up in zeroing, you need to put something in your state exit action
      if(AlgoStateExitAction[currentState] != NULL)
      {
        AlgoStateExitAction[currentState](stove);
      }

      else
      {
        nextState = ZEROING_STEPPER ;
      }
      bStateExitConditionMet = false;

    }
  }


  // If an adjustment is requested, send configs to motors
  if(bStepperAdjustmentNeeded)
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


  // Perform state change if requested (Skips bStateExitConditionMet) | @mcaron ajouté le 2024-02-13
  // Not dependent on bStateExitConditionMet, triggered any time the state is changed. V.
  // important to make sure that the min time in state set is what we want for that.

  // added the states that dont need to wait on min time in state

  if((nextState != currentState) && (min_time_in_state_met || tStat_just_changed
      || nextState == RELOAD_IGNITION
      || nextState == BOOST
      || nextState == MANUAL_CONTROL
      || nextState == OVERTEMP
      || nextState == SAFETY))
  {
    lastState = currentState;
    currentState = nextState;
    stove->bstateJustChanged = true;
  }
}


// Init of main application object "*stove"
static void Algo_stoveInit(Mobj *stove)
{
  stove->sParticles = ParticlesGetObject();
  sOverheatParams = PB_GetOverheatParams();

  for(uint8_t i = 0;i < ALGO_NB_OF_STATE;i++)
  {
    sStateParams[i] = PB_GetSuperStateParams(i);
  }

  stove->u32TimeOfStateEntry_ms = 0;
  stove->u32TimeOfAdjustment_ms = 0;
  stove->u32TimeOfComputation_ms = 0;
  stove->bReloadRequested = false;
  stove->bButtonBlinkRequired = false;
  stove->bstateJustChanged = true;
  stove->bSafetyOn = false;
  stove->TimeOfReloadRequest = 0;

  // Homing request of steppers at init
  stove->sPrimary.u8apertureCmdSteps = MOTOR_HOME_CMD;
  stove->sGrill.u8apertureCmdSteps = MOTOR_HOME_CMD;
  stove->sSecondary.u8apertureCmdSteps = MOTOR_HOME_CMD;
  Algo_adjust_steppers_position(stove);
}

static void smoke_init(SmokeStruct *sSmoke )
{
  sSmoke->part_dev_tol = 20;
  sSmoke->particles_target = 0;
  sSmoke->particles_tolerance = 50;
  sSmoke->deltaT_target_pos = 12;
  sSmoke->deltaT_target_neg = -1;
  sSmoke->Tbaffle_target_min = 320;


  // If we are over 1050, we automatically decide events to be smoke hot.
  sSmoke->T_chamber_target_max = 1050;
  // If we are under 600, we automatically decide events to be smoke cold.
  sSmoke->T_chamber_target_min = 600;


  // paramètre de base pour l'initialisation, mais après sont assignés avec paramfile.c dans les entry des états
  sSmoke->g_min = 3;
  sSmoke->g_max = 97;
  sSmoke->p_min = 3;
  sSmoke->p_max = 97;
  sSmoke->s_min = 20;
  sSmoke->s_max = 90;
  sSmoke->early_states = true;
}



///////////////////////// STATE MACHINE  ////////////////////////////


//** STATE: ZEROING STEPPER **//
static void Algo_zeroing_entry(Mobj *stove)
{
  // close everything
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
  /*
   * Zeroing state
   *
   * zeroing is done previously and this state checks that the motors
   * are done before sending the state machine to the next state
   *
   * */

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

  // TODO : Valider qu'on peut enlever le binterlock et remplacer par breload requested
  if(!stove->bInterlockOn) // va tjrs être vrai parce qu'on utilise plus l'interlock, donc la variable est seulement initialisée
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

  particles_zero_counter = 0 ; // added here to reset if stove never goes to waiting state MC 2024-06-18

  stove->sPrimary.u8apertureCmdSteps = sParam->sPrimary.i32Max;
  stove->sGrill.u8apertureCmdSteps = sParam->sGrill.i32Max;
  stove->sSecondary.u8apertureCmdSteps = sParam->sSecondary.i32Max;

  stove->sGrill.fSecPerStep = 0; // force aperture
  stove->sPrimary.fSecPerStep = 0; // force aperture
  stove->sSecondary.fSecPerStep = 0; // force aperture

  bStepperAdjustmentNeeded = true;


  if((stove->fBaffleTemp > P2F(sParam->fTempToSkipReload))) // 530 F
  {
    nextState = TEMPERATURE_RISE;
  }

  hot_reload = (stove->fBaffleTemp > 400) ;

  // CLEARING HISTORY SO THAT WE HAVE A CLEAN SLATE WHEN RELOADING
  // giving another value than zero so that this is not taken as a reference
  array_setter(smoke_history,LEN(smoke_history),-2);
  array_setter(grill_history,LEN(grill_history),48);


  sSmoke.part_dev_tol = 20;
  sSmoke.particles_target = 0;
  sSmoke.particles_tolerance = 50;
  sSmoke.deltaT_target_pos = 12;
  sSmoke.deltaT_target_neg = -1;
  sSmoke.Tbaffle_target_min = 330;
  sSmoke.g_min = sParam->sGrill.i32Min;
  sSmoke.g_max = sParam->sGrill.i32Max;
  sSmoke.p_min = sParam->sPrimary.i32Min;
  sSmoke.p_max = sParam->sPrimary.i32Max;
  sSmoke.s_min = sParam->sSecondary.i32Min;
  sSmoke.s_max = sParam->sSecondary.i32Max;
  sSmoke.early_states = true;
}


static void Algo_reload_action(Mobj* stove, uint32_t u32CurrentTime_ms)
{
  const PF_ReloadParam_t *sParam = PB_GetReloadParams();
  const int wait_cycle_time = 60;

  if((u32CurrentTime_ms - stove->u32TimeOfStateEntry_ms) < SECONDS(wait_cycle_time) && !state_entry_delays_skip)
  {
    if(print_debug_setup){
      printf("\non attend 60 secondes apres l'entree en reload");
      printf("\n%i\n", (int)(u32CurrentTime_ms - stove->u32TimeOfStateEntry_ms) /1000);}
    return;
  }

  if(((stove->fBaffleTemp > P2F(sParam->fTempToQuitReload)) && !hot_reload)|| ((stove->fBaffleTemp > 525) && hot_reload)) // NORMALEMENT 525
  {
    nextState = TEMPERATURE_RISE;
    debug_printer("HOT RELOAD", print_debug_setup && hot_reload);
    debug_printer("COLD RELOAD", print_debug_setup && !hot_reload);
  }

  // WAIT FOR NEXT LOOP
  if((stove->u32MajorCorrectionTime_ms != 0 && ((u32CurrentTime_ms - stove->u32MajorCorrectionTime_ms) < SECONDS(wait_cycle_time))) )
  {
    debug_printer("there was a major correction,  wait: skip to next loop !", print_debug_setup);
    return;
  }

  if(Algo_smoke_action(stove, u32CurrentTime_ms, wait_cycle_time)){return;}

}

/*
static void Algo_reload_exit(Mobj *stove)
{
}
 */
//** END: RELOAD / IGNITION**//


//** STATE: TEMPERATURE RISE **//
static void Algo_tempRise_entry(Mobj* stove)
{
  const PF_TriseParam_t *sParam = PB_GetTRiseParams();

  stove->sGrill.u8apertureCmdSteps = sParam->sGrill.i32Max;
  stove->sPrimary.u8apertureCmdSteps = sParam->sPrimary.i32Max;
  stove->sSecondary.u8apertureCmdSteps = sParam->sSecondary.i32Max;
  stove->sGrill.fSecPerStep = 0; // force aperture
  stove->sPrimary.fSecPerStep = 0; // force aperture
  stove->sSecondary.fSecPerStep = 0; // force aperture
  // --------------------------------------------------------------------------------------------

  bStepperAdjustmentNeeded = true;
  tRiseEntry = true;

  // CLEARING HISTORY SO THAT WE HAVE A CLEAN SLATE WHEN RELOADING
  // giving another value than zero so that this is not taken as a reverence
  array_setter(smoke_history,LEN(smoke_history),-2);
  array_setter(grill_history,LEN(grill_history),48);

  sSmoke.part_dev_tol = sParam->sPartStdev.fTolerance;
  sSmoke.particles_target = sParam->sParticles.fTarget;
  sSmoke.particles_tolerance = sParam->sParticles.fTolerance;
  sSmoke.deltaT_target_pos = P2F1DEC(sParam->sTempSlope.fTarget) + P2F1DEC(sParam->sTempSlope.fTolerance);
  sSmoke.deltaT_target_neg = -1;
  sSmoke.g_min = sParam->sGrill.i32Min;
  sSmoke.g_max = sParam->sGrill.i32Max;
  sSmoke.p_min = sParam->sPrimary.i32Min;
  sSmoke.p_max = sParam->sPrimary.i32Max;
  sSmoke.s_min = sParam->sSecondary.i32Min;
  sSmoke.s_max = sParam->sSecondary.i32Max;
  sSmoke.early_states = false;

}



static void Algo_tempRise_action(Mobj* stove, uint32_t u32CurrentTime_ms)
{

  // TODO : Enlever le hardcoding des critères de delta T pour la fermeture progressive de la grille
  // FAIT : 2024-06-18 MC, faire suivi


  const PF_TriseParam_t *sParam = PB_GetTRiseParams(); // va chercher les paramètres de l'état
  const PF_StepperStepsPerSec_t *sSpeedParams =  PB_SpeedParams(); // aller chercher les paramètres de vitesse de stepper selon l'état

  int hot_reload_smoke_temp_correction = 0;
  const int wait_cycle_time = 20;
  int exit_temp = tstat_status ? sParam->fTempToCombHigh : sParam->fTempToCombLow;
  bool smoke_detected = 0;


  if(((u32CurrentTime_ms - stove->u32TimeOfStateEntry_ms > SECONDS(wait_cycle_time))
      || state_entry_delays_skip)&& tRiseEntry)
  {
    stove->sPrimary.u8apertureCmdSteps = 76;
    stove->sPrimary.fSecPerStep = 0;
    bStepperAdjustmentNeeded = true;
    tRiseEntry = false;
    debug_printer("trise entry : set primary at 76", print_debug_setup);
  }


  // EXIT CONDITIONs TO COMBUSTION
  //if Tbaffle > 710 or 630, or if primary <= 75 go to comb states
  if(stove->fBaffleTemp > exit_temp ||
      stove->sPrimary.u8apertureCmdSteps <= 75 )
  {
    nextState = tstat_status ? COMBUSTION_HIGH : COMBUSTION_LOW;
    debug_printer("Trise exit to combustion (if min time has been met)", print_debug_setup);

    // put the return in comment so it doesn't prevent us from decrementing while we havent met the min time in state
    // return;
  }

  sSmoke.deltaT_target_pos = P2F1DEC(sParam->sTempSlope.fTarget) + P2F1DEC(sParam->sTempSlope.fTolerance) + P2F(hot_reload_smoke_temp_correction);
  smoke_detected = Algo_smoke_action(stove, u32CurrentTime_ms, wait_cycle_time);
  if(smoke_detected){return;}

  // If it has been too long, go to coal
  if((stove->fBaffleTemp < (P2F(sParam->fTempToCombLow) - 90)   &&
      (u32CurrentTime_ms - stove->u32TimeOfStateEntry_ms) > MINUTES(30)) && !smoke_detected)
  {
    nextState = tstat_status ? COAL_HIGH : COAL_LOW;
    debug_printer("never got to comb, exit to coal low", print_debug_setup);
    return;
  }

  // WAIT FOR NEXT LOOP
  // time of correction < 60s
  if((stove->u32MajorCorrectionTime_ms != 0 && ((u32CurrentTime_ms - stove->u32MajorCorrectionTime_ms) < SECONDS(wait_cycle_time))) )
  {
    debug_printer("there was a major correction,  wait: skip to next loop !", print_debug_setup);
    return;
  }


  if(!stove->bDoorOpen
      && motors_ready_for_req
      && ( (stove->fBaffleDeltaT > P2F1DEC(sParam->sTempSlope.fTarget) && stove->fBaffleTemp > sParam->fTempToStartReg)
          ||  stove->fBaffleTemp > sParam->fTempToStartReg + 275 )
  )

  {
    if(stove->sGrill.u8apertureCmdSteps > 15)
    {
      debug_printer(" TRISE : decrement grill \n\n", print_debug_setup);

      // If we are deltaT > 30 -> -3 steps per move
      // If we are deltaT > 20 -> -2 steps per move
      // If we are deltaT < 20 -> -1 steps per move
      // sParam->sTempSlope.fTarget (référence : paramfile.c)


      stove->fBaffleDeltaT > 6 * P2F1DEC(sParam->sTempSlope.fTarget) ?
          stove->sGrill.u8apertureCmdSteps -= 3
          : ( stove->fBaffleDeltaT > 4 * P2F1DEC(sParam->sTempSlope.fTarget) ?
              stove->sGrill.u8apertureCmdSteps -= 2
              : stove->sGrill.u8apertureCmdSteps -- );
    }

    else
    {
      debug_printer(" TRISE : decrement primary ", print_debug_setup);
      stove->sPrimary.u8apertureCmdSteps--;
    }

    stove->sGrill.fSecPerStep = P2F1DEC(sSpeedParams->fFast);
    stove->sPrimary.fSecPerStep = P2F1DEC(sSpeedParams->fFast);
    bStepperAdjustmentNeeded = true;

  }

}


static void Algo_tempRise_exit(Mobj *stove)
{
  if(bStateExitConditionMet)
  {
    //
  }
  else
  {
    // case for timeout or error
    nextState = ZEROING_STEPPER;
  }
}
//** END: TEMPERATURE RISE **//



//** STATE: COMBUSTION LOW **//
static void Algo_combLow_entry(Mobj *stove)
{
  const PF_CombustionParam_t *sParam = PB_GetCombLowParams();

  stove->sPrimary.u8apertureCmdSteps = RANGE(
      sParam->sPrimary.i32Min,
      stove->sPrimary.u8apertureCmdSteps,
      sParam->sPrimary.i32Max);

  if(lastState == TEMPERATURE_RISE)
  {stove->sGrill.u8apertureCmdSteps = sParam->sGrill.i32Min;}

  else  if(lastState == COAL_LOW)
  {
    stove->sGrill.u8apertureCmdSteps = sParam->sGrill.i32Min;
    stove->sPrimary.u8apertureCmdSteps = sParam->sPrimary.i32Max;
    stove->sSecondary.u8apertureCmdSteps = sParam->sSecondary.i32Max;
  }

  stove->sGrill.fSecPerStep = 0;
  stove->sPrimary.fSecPerStep = 0;
  stove->sSecondary.fSecPerStep = 0;
  bStepperAdjustmentNeeded = true;

  sSmoke.part_dev_tol = sParam->sPartStdev.fTolerance;
  sSmoke.particles_target = sParam->sParticles.fTarget;
  sSmoke.particles_tolerance = sParam->sParticles.fTolerance;
  sSmoke.deltaT_target_pos = P2F1DEC(sParam->sTempSlope.fTarget) + P2F1DEC(sParam->sTempSlope.fTolerance);
  sSmoke.deltaT_target_neg = -1;
  sSmoke.Tbaffle_target_min = 530;
  sSmoke.g_min = sParam->sGrill.i32Min;
  sSmoke.g_max = sParam->sGrill.i32Max;
  sSmoke.p_min = sParam->sPrimary.i32Min;
  sSmoke.p_max = sParam->sPrimary.i32Max;
  sSmoke.s_min = sParam->sSecondary.i32Min;
  sSmoke.s_max = sParam->sSecondary.i32Max;
  sSmoke.early_states = false;

}


static void Algo_combLow_action(Mobj* stove, uint32_t u32CurrentTime_ms)
{
  PF_CombustionParam_t *sParam = PB_GetCombLowParams();
  // const PF_StepperStepsPerSec_t *sSpeedParams =  PB_SpeedParams();
  static int smoke_detected = 0;
  const int wait_cycle_time = 30;
  const int temp_to_coal = 535;
  const int timeToCombLow2_minutes = 30; //GTF 2024-06-07
  static bool comb_low_2 = false;



  //THERMOSTAT SWITCHING
  if(tstat_status)
  {
    nextState = tstat_status ? COMBUSTION_HIGH : COMBUSTION_LOW;
    bStateExitConditionMet = true;
    debug_printer("exit to comb high", print_debug_setup);
    tStat_just_changed = true;
    return;
  }

  // SMOKE MANAGEMENT CALL
  smoke_detected = Algo_smoke_action(stove, u32CurrentTime_ms, wait_cycle_time);if(smoke_detected){return;}

  //EXIT CONDITIONS

  // Le delay skipper et le min time est dans l'Engine de la state machine au début dans algo_task.
  // C'est redondant, je l'ai mit en commentaire. Si ça ne cause pas de problème, l'enlever après un moment.
  // MC 2024-05-30
  if( (stove->fBaffleTemp < temp_to_coal)  && !smoke_detected /* && ( state_entry_delays_skip || ( u32CurrentTime_ms - stove->u32TimeOfStateEntry_ms) > MINUTES(30) ) */)
  {
    nextState = tstat_status ? COAL_HIGH : COAL_LOW;
    bStateExitConditionMet = true;
    debug_printer("exit to coal low", print_debug_setup);
    return;
  }

  // WAIT FOR NEXT LOOP
  if( ((stove->bDoorOpen) ||
      (stove->u32MajorCorrectionTime_ms != 0 && (u32CurrentTime_ms - stove->u32MajorCorrectionTime_ms < SECONDS(wait_cycle_time))) ) )
  {
    debug_printer("there was a major correction: wait until next loop", print_debug_setup);
    return;
  }


  /*
   * comb low 2 action ajouté le 2024-06-06 MC
   * if we haven't added fuel since state entry (checking by door open last time and comparing with state entry time)
   * if enough time has passed since state entry (timeToCombLow2_minutes, in beginning of the comb low action function)
   * if we are under the relevant chamber temperature or if we have already entered comb low 2 and
   * are under the higher hysteresis limit.
   *
   * */

  if (    (u32CurrentTime_ms - stove->u32TimeOfStateEntry_ms <= u32CurrentTime_ms - time_of_door_open)
      &&  (u32CurrentTime_ms - stove->u32TimeOfStateEntry_ms >= MINUTES(timeToCombLow2_minutes))
      &&  (stove->fChamberTemp < 835 || (stove->fChamberTemp < 865 && comb_low_2 ) )
      ){

    comb_low_2 = true;
    debug_printer("Comb low 2 active !\n\rNo movement on secondary or primary.",print_debug_setup);
    stove->sGrill.u8apertureCmdSteps = 15;
    stove->sGrill.fSecPerStep = 0;
    bStepperAdjustmentNeeded = true;

    // the return here is what makes this work, it skips the normal temp control.
    // otherwise the normal algo behavior will act
    return;
  }

  else {
    comb_low_2 = false;
  }

  comb_temperature_control( stove, sParam, u32CurrentTime_ms);
}



static void Algo_combLow_exit(Mobj *stove)
{
  if(bStateExitConditionMet)
  {
    // nextState = COAL_LOW;
  }
  else
  {
    // case for timeout or error
    nextState = ZEROING_STEPPER;
  }
}


//** END: COMBUSTION LOW **//

//** STATE: COMBUSTION HIGH **//
static void Algo_combHigh_entry(Mobj *stove)
{
  const PF_CombustionParam_t *sParam = PB_GetCombHighParams();

  if(lastState == TEMPERATURE_RISE)
  {
    stove->sGrill.u8apertureCmdSteps = sParam->sGrill.i32Min;
  }
  else if (lastState == COAL_LOW)
  {
    stove->sGrill.u8apertureCmdSteps = RANGE(sParam->sGrill.i32Min,stove->sGrill.u8apertureCmdSteps,sParam->sGrill.i32Max);
    stove->sPrimary.u8apertureCmdSteps = sParam->sPrimary.i32Max;
    stove->sSecondary.u8apertureCmdSteps = sParam->sSecondary.i32Max;
  }

  stove->sSecondary.u8apertureCmdSteps = RANGE(sParam->sSecondary.i32Min,stove->sSecondary.u8apertureCmdSteps,sParam->sSecondary.i32Max);

  stove->sGrill.fSecPerStep = 0; // force aperture
  stove->sPrimary.fSecPerStep = 0; // force aperture
  stove->sSecondary.fSecPerStep = 0; // force aperture
  bStepperAdjustmentNeeded = true;

  sSmoke.part_dev_tol = sParam->sPartStdev.fTolerance;
  sSmoke.particles_target = sParam->sParticles.fTarget;
  sSmoke.particles_tolerance = sParam->sParticles.fTolerance;
  sSmoke.deltaT_target_pos = P2F1DEC(sParam->sTempSlope.fTarget) + P2F1DEC(sParam->sTempSlope.fTolerance);
  sSmoke.deltaT_target_neg = -1;
  sSmoke.Tbaffle_target_min = 530;
  sSmoke.g_min = sParam->sGrill.i32Min;
  sSmoke.g_max = sParam->sGrill.i32Max;
  sSmoke.p_min = sParam->sPrimary.i32Min;
  sSmoke.p_max = sParam->sPrimary.i32Max;
  sSmoke.s_min = sParam->sSecondary.i32Min;
  sSmoke.s_max = sParam->sSecondary.i32Max;
  sSmoke.early_states = false;

}


static void Algo_combHigh_action(Mobj* stove, uint32_t u32CurrentTime_ms)
{
  PF_CombustionParam_t *sParam = PB_GetCombHighParams();
  //  const PF_StepperStepsPerSec_t *sSpeedParams =  PB_SpeedParams();
  static int smoke_detected = 0;
  const int wait_cycle_time = 30;


  // Combustion state loop conditions

  // THERMOSTAT SWITCHING

  if(!tstat_status)
  {
    nextState = tstat_status ? COMBUSTION_HIGH : COMBUSTION_LOW;
    bStateExitConditionMet = true;
    debug_printer("exit to comb low", print_debug_setup);
    tStat_just_changed = true;
    return;
  }

  // SMOKE MANAGEMENT CALL

  smoke_detected = Algo_smoke_action(stove, u32CurrentTime_ms, wait_cycle_time);if(smoke_detected){return;}

  //EXIT CONDITIONS

  if( (stove->fBaffleTemp < 500)  && !smoke_detected && ( state_entry_delays_skip || ( u32CurrentTime_ms - stove->u32TimeOfStateEntry_ms) > MINUTES(30) ) )
  {
    nextState = tstat_status ? COAL_HIGH : COAL_LOW;
    bStateExitConditionMet = true;
    debug_printer("exit to coal high", print_debug_setup);
    return;
  }

  // WAITING FOR NEXT LOOP
  if( ((stove->bDoorOpen) ||
      (stove->u32MajorCorrectionTime_ms != 0 && (u32CurrentTime_ms - stove->u32MajorCorrectionTime_ms < SECONDS(wait_cycle_time))) ) )
  {
    debug_printer("there was a major correction: wait until next loop", print_debug_setup);
    return;
  }

  comb_temperature_control( stove, sParam,  u32CurrentTime_ms);

}

// END COMB HIGH ACTION

static void Algo_combHigh_exit(Mobj *stove)
{

  if(bStateExitConditionMet)
  {
    // nextState = COAL_HIGH;
  }
  else
  {
    // case for timeout or error
    nextState = ZEROING_STEPPER;
  }
}



//** END: COMBUSTION HIGH **//

//** STATE: COAL LOW **//
static void Algo_coalLow_entry(Mobj *stove)
{
  const PF_CoalParam_t *sParam = PB_GetCoalLowParams();
  temp_coal_entry = stove->fBaffleTemp;

  sSmoke.part_dev_tol = sParam->sPartStdev.fTolerance;
  sSmoke.particles_target = sParam->sParticles.fTarget;
  sSmoke.particles_tolerance = sParam->sParticles.fTolerance;
  sSmoke.deltaT_target_pos = 1;
  sSmoke.deltaT_target_neg = -1;
  sSmoke.Tbaffle_target_min = 300;
  sSmoke.g_min = sParam->sGrill.i32Min;
  sSmoke.g_max = sParam->sGrill.i32Max;
  sSmoke.p_min = sParam->sPrimary.i32Min;
  sSmoke.p_max = sParam->sPrimary.i32Max;
  sSmoke.s_min = sParam->sSecondary.i32Min;
  sSmoke.s_max = sParam->sSecondary.i32Max;
  sSmoke.early_states = false;

}




/*
 *
 * Idée à ajouter
 *
 * rentrer en coal de fac¸on préventive plus tôt, par exemple à 550, changer le comportement,
 * i.e. : en premier ouvrir la grille genre à 25 et checker après 5 min on regarde si on a gagné au moins 25 degrés. Si oui, on repart le timer.
 * Pour les prochaines boucles, on se compare de 25 degrés plus haut que 550.
 *
 * Lorsqu'on rencontre la condition, on peut faire la fermeture progressive : 5 min primaire et 5 min secondaire
 *
 * */



static void Algo_coalLow_action(Mobj* stove, uint32_t u32CurrentTime_ms)
{
  const PF_CoalParam_t *sParam = PB_GetCoalLowParams();
  static int smoke_detected = 0;
  const int wait_cycle_time = 30;
  const int grill_coal = 0 ;


  // THERMOSTAT SWITCHING
  if(tstat_status)
  {
    nextState = tstat_status ? COAL_HIGH : COAL_LOW;
    bStateExitConditionMet = true;
    debug_printer("exit to coal high", print_debug_setup);
    tStat_just_changed = true;
    return;
  }

  // EXIT TO WAITING
  if(u32CurrentTime_ms - stove->u32TimeOfStateEntry_ms > MINUTES(5) &&
      stove->fBaffleTemp < 140 && stove->fBaffleDeltaT < 1)
  {
    nextState = WAITING ;
    bStateExitConditionMet = true;
    debug_printer("EXIT : RETURN TO WAITING", print_debug_setup);
    return;
  }

  // return to reload if we have a temperature rise (fuel has been added, we need more air)
  if(u32CurrentTime_ms - stove->u32TimeOfStateEntry_ms > MINUTES(5) &&
      stove->fBaffleDeltaT > 10)

  {
    nextState = RELOAD_IGNITION ;
    bStateExitConditionMet = true;
    debug_printer("EXIT : RETURN TO Reload", print_debug_setup);
    return;
  }


  // WAITING FOR NEXT LOOP WHEN MAJOR CORRECTION
  if((stove->u32MajorCorrectionTime_ms != 0 && ((u32CurrentTime_ms - stove->u32MajorCorrectionTime_ms) < SECONDS(wait_cycle_time))) )
  {
    debug_printer("there was a major correction,  wait: skip to next loop !", print_debug_setup);
    return;
  }


  // SMOKE ACTION
  if(Algo_smoke_action(stove, u32CurrentTime_ms, wait_cycle_time)){return;}


  // First : close grill after entry and temperature has reached under 450
  if((stove->fBaffleTemp < P2F( sParam->sTemperature.fTarget)) && stove->sGrill.u8apertureCmdSteps != grill_coal && !smoke_detected)
  {
    stove->sGrill.u8apertureCmdSteps = grill_coal;
    stove->sGrill.fSecPerStep = 0; // force aperture
    bStepperAdjustmentNeeded = true;
  }

  // TODO: ajuster params (temps ou temperature), beaucoup de fumée generee par ce move
  // after 1 min, close primary
  if(u32CurrentTime_ms - stove->u32TimeOfAdjustment_ms > MINUTES(sParam->i32TimeBeforeMovingPrim) && (stove->sPrimary.u8apertureCmdSteps != sParam->sPrimary.i32Min))
  {
    stove->sPrimary.u8apertureCmdSteps = sParam->sPrimary.i32Min;
    stove->sPrimary.fSecPerStep = 0; // force aperture
    bStepperAdjustmentNeeded = true;
  }

  // after 5 minutes, close secondary
  if(u32CurrentTime_ms - stove->u32TimeOfAdjustment_ms > MINUTES(sParam->i32TimeBeforeMovingSec) && (stove->sSecondary.u8apertureCmdSteps != sParam->sSecondary.i32Min))
  {
    stove->sSecondary.u8apertureCmdSteps = sParam->sSecondary.i32Min;
    stove->sSecondary.fSecPerStep = 0; // force aperture
    bStepperAdjustmentNeeded = true;
  }

}

static void Algo_coalLow_exit(Mobj *stove)
{

  // We need to leave the exit action here, otherwise we can't transition between low and high thermostat actions
  debug_printer("Coal low exit", print_debug_setup);

  if(bStateExitConditionMet)
  {
    // nothing to do here
  }
  else
  {
    // case for timeout or error (timeout does not validate bStateExitConditionMet)
    nextState = ZEROING_STEPPER;
  }

}
//** END: COAL LOW **//


//** STATE: COAL HIGH **//
static void Algo_coalHigh_entry(Mobj *stove)
{
  const PF_CoalParam_t *sParam = PB_GetCoalHighParams();

  sSmoke.part_dev_tol = sParam->sPartStdev.fTolerance;
  sSmoke.particles_target = sParam->sParticles.fTarget;
  sSmoke.particles_tolerance = sParam->sParticles.fTolerance;

  sSmoke.deltaT_target_pos = 1;
  sSmoke.deltaT_target_neg = -1;

  sSmoke.Tbaffle_target_min = 300;

  sSmoke.g_min = sParam->sGrill.i32Min;
  sSmoke.g_max = sParam->sGrill.i32Max;
  sSmoke.p_min = sParam->sPrimary.i32Min;
  sSmoke.p_max = sParam->sPrimary.i32Max;
  sSmoke.s_min = sParam->sSecondary.i32Min;
  sSmoke.s_max = sParam->sSecondary.i32Max;
  sSmoke.early_states = false;
}

static void Algo_coalHigh_action(Mobj* stove, uint32_t u32CurrentTime_ms)
{
  const PF_CoalParam_t *sParam = PB_GetCoalHighParams();
  const int wait_cycle_time = 30;
  const PF_StepperStepsPerSec_t *sSpeedParams =  PB_SpeedParams();


  // THERMOSTAT SWITCHING
  if(!tstat_status)
  {
    nextState = tstat_status ? COAL_HIGH : COAL_LOW;
    bStateExitConditionMet = true;
    debug_printer("exit to coal low", print_debug_setup);
    tStat_just_changed = true;
    return;
  }


  // TODO:  check that exit temperature is different and lower
  // than waiting quitting temperature (&m_sWaitingParams.fTempToQuitWaiting,     )


  // EXIT TO WAITING
  if(u32CurrentTime_ms - stove->u32TimeOfStateEntry_ms > MINUTES(5) &&
      stove->fBaffleTemp < 140 && stove->fBaffleDeltaT < 1)
  {
    nextState = WAITING ;
    bStateExitConditionMet = true;
    debug_printer("EXIT : RETURN TO WAITING", print_debug_setup);
    return;
  }

  // return to reload if we have a temperature rise (fuel has been added, we need more air)
  if(u32CurrentTime_ms - stove->u32TimeOfStateEntry_ms > MINUTES(5) &&
      stove->fBaffleDeltaT > 10)
  {
    nextState = RELOAD_IGNITION ;
    bStateExitConditionMet = true;
    debug_printer("EXIT : RETURN TO Reload", print_debug_setup);
    return;
  }


  // WAITING FOR NEXT LOOP
  if((stove->u32MajorCorrectionTime_ms != 0 && ((u32CurrentTime_ms - stove->u32MajorCorrectionTime_ms) < SECONDS(wait_cycle_time))) )
  {
    debug_printer("there was a major correction,  wait: skip to next loop !", print_debug_setup);
    return;
  }

  if(Algo_smoke_action(stove, u32CurrentTime_ms, wait_cycle_time)){return;}

  if(stove->fBaffleTemp<550)
  {
    if(motors_ready_for_req)
    {
      stove->sGrill.fSecPerStep = P2F1DEC(sSpeedParams->fSlow);
      stove->sPrimary.fSecPerStep = P2F1DEC(sSpeedParams->fSlow);
      stove->sSecondary.fSecPerStep = P2F1DEC(sSpeedParams->fSlow);

      if(stove->sGrill.u8apertureCmdSteps++ >= sParam->sGrill.i32Max)
      {
        stove->sGrill.u8apertureCmdSteps = sParam->sGrill.i32Max;

        if(stove->sPrimary.u8apertureCmdSteps-- <= sParam->sPrimary.i32Min)
        {
          stove->sPrimary.u8apertureCmdSteps = sParam->sPrimary.i32Min;

          if(stove->sSecondary.u8apertureCmdSteps-- <= sParam->sSecondary.i32Min)
          {
            stove->sSecondary.u8apertureCmdSteps = sParam->sSecondary.i32Min;
          }
        }
      }

      bStepperAdjustmentNeeded = true;
    }
  }
}


static void Algo_coalHigh_exit(Mobj *stove)
{
  // We need to leave the exit action here, otherwise we can't transition between low and high thermostat actions
  debug_printer("Coal High exit", print_debug_setup);

  if(bStateExitConditionMet)
  {
    // nothing to do here
  }
  else
  {
    // case for timeout or error (timeout does not validate bStateExitConditionMet)
    nextState = ZEROING_STEPPER;
  }

}
//** END: COAL HIGH **//


//** STATE: BOOST **//
static void Algo_boost_entry(Mobj *stove)
{
  sBoostConfMem.sPreviousState = lastState;
  sBoostConfMem.u8grill_pos = stove->sGrill.u8aperturePosSteps;
  sBoostConfMem.u8prim_pos = stove->sPrimary.u8aperturePosSteps;
  sBoostConfMem.u8sec_pos = stove->sSecondary.u8aperturePosSteps;

  sBoostConfMem.eFanDistSpeed = PARAMFILE_GetParamValueByKey(PFD_RMT_DISTFAN);
  sBoostConfMem.eFanLowSpeed = PARAMFILE_GetParamValueByKey(PFD_RMT_LOWFAN);

  PARAMFILE_SetParamValueByKey(FSPEED_HIGH,PFD_RMT_DISTFAN);
  PARAMFILE_SetParamValueByKey(FSPEED_HIGH,PFD_RMT_LOWFAN);

  stove->sPrimary.u8apertureCmdSteps = PF_PRIMARY_FULL_OPEN;
  stove->sPrimary.fSecPerStep = 0; // force aperture
  stove->sGrill.u8apertureCmdSteps = PF_GRILL_FULL_OPEN;
  stove->sGrill.fSecPerStep = 0; // force aperture
  stove->sSecondary.u8apertureCmdSteps = PF_SECONDARY_FULL_OPEN;
  stove->sSecondary.fSecPerStep = 0; // force aperture
  bStepperAdjustmentNeeded = true;

}

static void Algo_boost_action(Mobj* stove, uint32_t u32CurrentTime_ms)
{
  const PF_RemoteParams_t* sParam = PB_GetRemoteParams();


  // cette instruction en principe part les fans au fond dans boost, mais c'est pas ce qu'on veut (ref Cedric)
  if((sParam->i32DistribSpeed != FSPEED_HIGH ) || (sParam->i32LowerSpeed != FSPEED_HIGH))
  {
    PARAMFILE_SetParamValueByKey(FSPEED_HIGH,PFD_RMT_DISTFAN);
    PARAMFILE_SetParamValueByKey(FSPEED_HIGH,PFD_RMT_LOWFAN);
  }

  if(u32CurrentTime_ms - stove->u32TimeOfStateEntry_ms > MINUTES(1))
  {
    nextState = sBoostConfMem.sPreviousState;
    bStateExitConditionMet = true;
  }

}

static void Algo_boost_exit(Mobj *stove)
{
  PARAMFILE_SetParamValueByKey(0,PFD_RMT_BOOST);
  stove->u32TimeOfStateEntry_ms = sBoostConfMem.u32StateEntryMem_ms;

  PARAMFILE_SetParamValueByKey(sBoostConfMem.eFanDistSpeed,PFD_RMT_DISTFAN);
  PARAMFILE_SetParamValueByKey(sBoostConfMem.eFanLowSpeed,PFD_RMT_LOWFAN);


  stove->sPrimary.u8apertureCmdSteps = sBoostConfMem.u8prim_pos;
  stove->sSecondary.u8apertureCmdSteps = sBoostConfMem.u8sec_pos;
  stove->sGrill.u8apertureCmdSteps = sBoostConfMem.u8grill_pos;
  bStepperAdjustmentNeeded = true;
}
//** END: BOOST **//

//** STATE: MANUAL **//
static void Algo_manual_entry(Mobj *stove)
{
  sManualConfMem.u8grill_pos = stove->sGrill.u8aperturePosSteps;
  sManualConfMem.u8prim_pos = stove->sPrimary.u8aperturePosSteps;
  sManualConfMem.u8sec_pos = stove->sSecondary.u8aperturePosSteps;

}

static void Algo_manual_action(Mobj* stove, uint32_t u32CurrentTime_ms)
{
  const PF_UsrParam* sManParam = PB_GetUserParam();

  if(!(sManParam->s32ManualOverride == 1)) // TODO: Put this in task function (Charles Richard)
  {
    nextState = lastState;
    if(nextState == ZEROING_STEPPER)
    {
      motors_ready_for_req = false;
    }
    bStateExitConditionMet = true;

    return;
  }

  if(stove->sPrimary.u8apertureCmdSteps != sManParam->s32ManualPrimary ||
      stove->sGrill.u8apertureCmdSteps != sManParam->s32ManualGrill ||
      stove->sSecondary.u8apertureCmdSteps != sManParam->s32ManualSecondary)
  {
    // forces apertures from the web page via sManParam
    stove->sPrimary.u8apertureCmdSteps = sManParam->s32ManualPrimary;
    stove->sPrimary.fSecPerStep = 0; // force aperture
    stove->sGrill.u8apertureCmdSteps = sManParam->s32ManualGrill;
    stove->sGrill.fSecPerStep = 0; // force aperture
    stove->sSecondary.u8apertureCmdSteps = sManParam->s32ManualSecondary;
    stove->sSecondary.fSecPerStep = 0; // force aperture
    bStepperAdjustmentNeeded = true;
  }


}

static void Algo_manual_exit(Mobj *stove)
{
  stove->u32TimeOfStateEntry_ms = sManualConfMem.u32StateEntryMem_ms;

  stove->sPrimary.u8apertureCmdSteps = sManualConfMem.u8prim_pos;
  stove->sSecondary.u8apertureCmdSteps = sManualConfMem.u8sec_pos;
  stove->sGrill.u8apertureCmdSteps = sManualConfMem.u8grill_pos;
  bStepperAdjustmentNeeded = true;

}

//** END: MANUAL **//

static void Algo_safety_action(Mobj* stove, uint32_t u32CurrentTime_ms)
{
  debug_printer("Safety condition activated", print_debug_setup);

  if(!stove->bSafetyOn)
  {
    nextState = lastState;
  }
}

static void Algo_overtemp_action(Mobj* stove, uint32_t u32CurrentTime_ms)
{
  const PF_OverHeat_Thresholds_t* OvrhtParams = PB_GetOverheatParams();

  debug_printer("WARNING : OVERTEMP !", print_debug_setup);

  if((stove->fBaffleTemp < P2F(OvrhtParams->OverheatBaffle))  &&
      (stove->fChamberTemp < P2F(OvrhtParams->OverheatChamber)) &&
      (stove->fPlenumTemp < P2F(OvrhtParams->OverheatPlenumExit)) )
  {
    nextState = lastState;
  }
}
// TODO: la température de plenum 210, correspond aussi à la sortie de overheat.
// Est-ce qu'il y a un glitch avec ça ?

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
  AlgoComputeAdjustment[BOOST] = Algo_boost_action;

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
  AlgoStateEntryAction[MANUAL_CONTROL] = Algo_manual_entry;
  AlgoStateEntryAction[BOOST] = Algo_boost_entry;

  AlgoStateExitAction[ZEROING_STEPPER] = NULL;
  AlgoStateExitAction[WAITING] = NULL;
  AlgoStateExitAction[RELOAD_IGNITION] = NULL;
  AlgoStateExitAction[TEMPERATURE_RISE] = Algo_tempRise_exit;
  AlgoStateExitAction[COMBUSTION_HIGH] = Algo_combHigh_exit;
  AlgoStateExitAction[COMBUSTION_LOW] = Algo_combLow_exit;
  AlgoStateExitAction[COAL_LOW] = Algo_coalLow_exit;
  AlgoStateExitAction[COAL_HIGH] = Algo_coalHigh_exit;
  AlgoStateExitAction[OVERTEMP] = NULL;
  AlgoStateExitAction[SAFETY] = NULL;
  AlgoStateExitAction[MANUAL_CONTROL] = Algo_manual_exit;
  AlgoStateExitAction[BOOST] = Algo_boost_exit;

}

static void Algo_update_steppers_inPlace_flag(void)
{
  //if(!motors_ready_for_req)
  //{
  xQueueReceive(MotorInPlaceHandle,&motors_ready_for_req,5);
  //}
}

static bool Algo_adjust_steppers_position(Mobj *stove)
{
  uint8_t cmd[NUMBER_OF_STEPPER_CMDS] =
  {
      stove->sPrimary.u8apertureCmdSteps,
      //(uint8_t)(stove->sPrimary.fSecPerStep*10),
      (stove->sPrimary.fSecPerStep),

      stove->sGrill.u8apertureCmdSteps,
      //(uint8_t)(stove->sGrill.fSecPerStep*10),
      (stove->sGrill.fSecPerStep),

      stove->sSecondary.u8apertureCmdSteps,
      //(uint8_t)(stove->sSecondary.fSecPerStep*10)
      (stove->sSecondary.fSecPerStep)


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




static int Algo_smoke_action(Mobj* stove, uint32_t u32CurrentTime_ms,int wait_cycle_time)
{
  /*
   * Algo Smoke Action
   *
   * Developped by : Marco Caron
   *
   * See ParticlesManager.c for further explanation.
   *
   * p2f needed on call of the following :
   * particles_target
   * particles_tolerance
   *
   * This function is called in the major operation states of the stove after the exit conditions and wait conditions
   * It generates an event that is either smoke cold, smoke hot or no smoke. The criteria for hot or cold are if
   * you are under or over a specific slope temperature.
   *
   * It also does proportional control on the hot smoke event so that it makes small corrections if the difference between
   * the target and the actual value is small and large corrections if the difference is large.
   *
   * It takes the smoke global object, baffle and chamber temperatures and returns a smoke status to the calling state if the crieteria are met.
   * It records smoke status and grill opening in a smoke history array.
   *
   * HOT SMOKE : When we have particles over (generally) 50 and a temperature slope OVER a certain value depending on where the function is called.
   * As of now, could be between 1 F to 12 F
   *
   * COLD SMOKE : When we have particles over (generally) 50 and a temperature slope UNDER a certain value depending on where the function is called.
   * As of now, could be between 1 F to 12 F
   *
   * PARTICLES : Amount of normalized particles measured by the sensor, used in proportional control.
   * PART DEV : Std Deviation of particles reading, used as a predictor for smoke, used in preventive moves.
   *
   *
   */

  /* APERTURE SMOKE PROPORTIONAL CONTROL
   * (Only applies to Smoke hot for now, not needed for smoke cold)
   *
   * In the early versions of the project, when we had smoke, we did huge moves on the apertures. This would
   * throw us into oscillation, overshooting and then undershooting. From previous experience, it was determined
   * that it would be better if the response would be proportional to the difference with the target.
   *
   * Now what we have is a controlLer that measures the error between the target smoke value and the measured value and
   * changes the value of the aperture proportionally to that error.
   *
   * This is currently done only in the case of "HOT SMOKE" when we have particles.
   *
   *
   * */

  /*  Checking smoke criteria
   *
   *  We also check a "delay_period_passed" because we do not want to do smoke moves too often because our system has a huge inertia.
   *  Using that delay within the function allows us to pass through the function every time the state runs to give us relevant information.
   *  This also serves that we can wait to see if our problem is resolved (especially in smoke cold)
   *  Before going back to the previous aperture opening
   *
   *
   * See algo.h for smoke struct definition
   *
   * Returns 0, 1 or -1 for respectively : No smoke, Hot Smoke or Cold smoke.
   *
   *
   * */

  //int l = LEN(smoke_history);
  //int lgrill = LEN(grill_history);
  // bool delay_period_passed = (u32CurrentTime_ms - stove->u32MajorCorrectionTime_ms) > SECONDS(wait_cycle_time);

  int i = 0;
  int cold_smk_step = 3;

  bool part_var_outofbounds = 0;
  bool part_val_outofbounds = 0;
  static bool smoke_cold_grill_was_reset = false;


  float controller_target = sSmoke.particles_target + sSmoke.particles_tolerance - 20 ;
  float controller_error = stove->sParticles->fparticles - controller_target;
  float controller_output;
  float k = 1.2;



  controller_error = controller_error == 0 ?
      1 :
      controller_error ;

  controller_output = RANGE((0.4), k * controller_target / controller_error, (0.95));


  part_val_outofbounds = stove->sParticles->fparticles > (sSmoke.particles_target + sSmoke.particles_tolerance);
  part_var_outofbounds = stove->sParticles->u16stDev > sSmoke.part_dev_tol;


  // if the smoke event is generated from part_var, do only fixed small changes
  controller_output = part_val_outofbounds ? controller_output : 0.9 ;
  cold_smk_step = part_val_outofbounds ? 10 : 3 ;

  if(print_debug_setup){
    printf("\n smoke control out : %.2f " , controller_output);
    printf(" smoke pos dt target : %.2f ", sSmoke.deltaT_target_pos);
    printf(" smoke neg dt target : %.2f ", sSmoke.deltaT_target_neg);
    printf(" part limit : %.2f ", controller_target + 20);
    printf(" part dev limit : %i \n\r", sSmoke.part_dev_tol);
  }




  // skip smoke checking if there has been a major correction

  if(u32CurrentTime_ms - stove->u32MajorCorrectionTime_ms < SECONDS(wait_cycle_time)||stove->bDoorOpen)
  {
    /* here we return a 1 smoke status but it is not logged in the array since that is done inside the "Algo_smoke_action" function,
     * we could probably return something else since it's not true that we still have smoke,
     * the reality is just that we are waiting for the conditions to be met to do the next loop
     */
    return 1;
  }

  // Checking if there are particles
  if((part_val_outofbounds || part_var_outofbounds))
  {


    // SMOKE HOT
    // Progressively closes all aperture from grill to secondary as long as the smoke conditions are met.

    if((stove->fBaffleDeltaT > sSmoke.deltaT_target_pos && stove->fBaffleTemp > sSmoke.Tbaffle_target_min && (stove->fChamberTemp > sSmoke.T_chamber_target_min)) ||
        (stove->fChamberTemp > sSmoke.T_chamber_target_max) )
    {
      debug_printer("SMOKE HOT", print_debug_setup);

      if (stove->sGrill.u8apertureCmdSteps > 15)
      {
        stove->sGrill.u8apertureCmdSteps *= controller_output;
      }

      else if(stove->sGrill.u8apertureCmdSteps >  sSmoke.g_min)
      {
        stove->sGrill.u8apertureCmdSteps = sSmoke.g_min;
      }

      else if(stove->sPrimary.u8apertureCmdSteps> 76 ) // 2024-02-07 MC valider qu'on pouvait faire ça, semblait redondant
      {
        stove->sGrill.u8apertureCmdSteps = sSmoke.g_min;
        stove->sPrimary.u8apertureCmdSteps = 76;
      }

      else if( stove->sPrimary.u8apertureCmdSteps > sSmoke.p_min && !sSmoke.early_states)
      {
        stove->sGrill.u8apertureCmdSteps = sSmoke.g_min;
        stove->sPrimary.u8apertureCmdSteps  = RANGE(sSmoke.p_min,stove->sPrimary.u8apertureCmdSteps * controller_output ,sSmoke.p_max); // 20-10 MC, Changed, validate effect
      }

      else {
        stove->sGrill.u8apertureCmdSteps = sSmoke.g_min;
        stove->sPrimary.u8apertureCmdSteps  = sSmoke.p_min;
        stove->sSecondary.u8apertureCmdSteps = RANGE(sSmoke.s_min, stove->sSecondary.u8apertureCmdSteps-5,sSmoke.s_max);
      }

      stove->sGrill.fSecPerStep = 0;
      stove->sPrimary.fSecPerStep = 0;
      stove->sSecondary.fSecPerStep = 0;
      bStepperAdjustmentNeeded = true;
      stove->u32MajorCorrectionTime_ms = u32CurrentTime_ms;

      array_shifter(smoke_history,LEN(smoke_history),1);// setting value 1 for smoke hot
      array_shifter(grill_history,LEN(grill_history),stove->sGrill.u8apertureCmdSteps);

      if(print_debug_setup)
      {
        array_printer(smoke_history,LEN(smoke_history));
        array_printer(grill_history,LEN(grill_history));
      }
      return 1 ;
    }



    /* SMOKE COLD,
     *
     * In smoke cold, we have particles when we are loosing our combustion, we want to inject a lot of air quickly
     *
     * We do it in reverse order from smoke hot : check that secondary is fully open
     * and also then check if primary is not smaller than 15.
     *
     * If those two are verified, inject air from grill
     */

    if((stove->fBaffleDeltaT <= sSmoke.deltaT_target_neg && stove->fChamberTemp < sSmoke.T_chamber_target_max)
        || stove->fChamberTemp < sSmoke.T_chamber_target_min){

      debug_printer("\n SMOKE COLD !", print_debug_setup);
      smoke_cold_grill_was_reset = false;

      //if(part_val_outofbounds || part_var_outofbounds){

      if(stove->sSecondary.u8apertureCmdSteps < sSmoke.s_max)
      {
        stove->sSecondary.u8apertureCmdSteps = sSmoke.s_max;
        debug_printer("smoke && secondary < s_max : set secondary to s_max", print_debug_setup);
      }

      if( stove->sPrimary.u8apertureCmdSteps < 15 )
      {
        stove->sPrimary.u8apertureCmdSteps = 15;
        debug_printer("smoke && primary < 15 : set primary to 15", print_debug_setup);
      }

      else if(stove->sPrimary.u8apertureCmdSteps < 50  && !part_val_outofbounds)
      {
        stove->sPrimary.u8apertureCmdSteps += cold_smk_step ;
        debug_printer("smoke VAR only && primary < 50 primary +3", print_debug_setup);
      }

      else if(stove->sGrill.u8apertureCmdSteps < 10 )
      {
        stove->sGrill.u8apertureCmdSteps = 10 ;
        debug_printer("smoke && grill < 10: grill = 10 ", print_debug_setup);
      }
      else
      {
        stove->sGrill.u8apertureCmdSteps  = RANGE(sSmoke.g_min, stove->sGrill.u8apertureCmdSteps + cold_smk_step, sSmoke.g_max);
        debug_printer("smoke && grill > 10 : grill + cold_smk_step ", print_debug_setup);
      }

      //}
      /*
      // Case where we have smoke "part var" aka smoke prediction : same as before but do smaller moves

      else if (!part_val_outofbounds && part_var_outofbounds){

        if(stove->sSecondary.u8apertureCmdSteps  < sSmoke.s_max)
        {
          stove->sSecondary.u8apertureCmdSteps = sSmoke.s_max ;
          debug_printer("smoke VAR only && secondary < 90 : set secondary +3 ", print_debug_setup);
        }

        else if(stove->sPrimary.u8apertureCmdSteps + 3 < 50 )
        {
          stove->sPrimary.u8apertureCmdSteps += 3 ;
          debug_printer("smoke VAR only && primary < 50 primary +3", print_debug_setup);
        }

        else if(stove->sGrill.u8apertureCmdSteps < 10 )
        {
          stove->sGrill.u8apertureCmdSteps = 10 ;
          debug_printer("smoke VAR && grill < 10: grill = 10 ", print_debug_setup);
        }

        else if(stove->sGrill.u8apertureCmdSteps  + 3 < 40 )
        {
          stove->sGrill.u8apertureCmdSteps += 3 ;
          debug_printer("smoke VAR && grill < 40 : grill + 3 ", print_debug_setup);
        }
      }
       */
      stove->sGrill.fSecPerStep = 0;
      stove->sPrimary.fSecPerStep = 0;
      stove->sSecondary.fSecPerStep = 0;
      bStepperAdjustmentNeeded = true;
      stove->u32MajorCorrectionTime_ms = u32CurrentTime_ms;

      //}

      // recording of smoke status
      array_shifter(smoke_history,LEN(smoke_history),-1); // setting value -1 in history for smoke cold
      array_shifter(grill_history,LEN(grill_history),stove->sGrill.u8apertureCmdSteps);

      if(print_debug_setup)
      {
        array_printer(smoke_history,LEN(smoke_history));
        array_printer(grill_history,LEN(grill_history));
      }

      return 1 ; // 1 means we have smoke.
    }
  }
  // -----------------------------------------
  // We get to this line if there is currently no smoke




  // COLD SMOKE HISTORY CHECKER
  // Correction of the grill value after air was injected for a cold smoke event
  // find the last good grill value before smoke event


  // going from the current value of smoke status until the first value in the history,
  // we search for previous values where we did not have COLD smoke to go back to for grill setting.

  // we do this because, generally, for a cold smoke event, we need to re-inject air for the fire to take and then we're good.
  // we do not want it to take off too aggressively because then we might go over the combustion power we're able to manage in terms of particles


  if(!smoke_cold_grill_was_reset){

  debug_printer("\nCold smoke history checker : Looking for last good setting before cold smoke", print_debug_setup);

  for (i = LEN(smoke_history) ; i >= LEN(smoke_history); i--)
  {

    if(smoke_history[i] == 1)
    {
      // here we've detected a hot smoke event between our cold smoke event and our no smoke event.
      // We don't want to go before that so we stop the loop
      break;
    }
    else if (smoke_history[i] == 0 && smoke_history[i-1] == 0 && smoke_history[i+1] == -1 )
    {
      if(print_debug_setup) {printf("\nUsing previous setting  of grill :%i at position : %i", grill_history[i], i);}

      stove->sGrill.u8apertureCmdSteps  = RANGE(sSmoke.g_min, grill_history[i], sSmoke.g_max);
      stove->sGrill.fSecPerStep = 0;
      bStepperAdjustmentNeeded = true;
      smoke_cold_grill_was_reset = true;

      // We break because we have found what we're looking for
      break;
    }
  }
  }



  // recording of smoke status
  array_shifter(smoke_history,LEN(smoke_history),0);
  array_shifter(grill_history,LEN(grill_history),stove->sGrill.u8apertureCmdSteps);

  if(print_debug_setup){
    printf("\n normal action, no smoke !");
    array_printer(smoke_history,LEN(smoke_history));
    array_printer(grill_history,LEN(grill_history)); }

  // }

  return 0 ; // returns : "no smoke status"
}




static void comb_temperature_control(Mobj* stove, PF_CombustionParam_t* sParam,  uint32_t u32CurrentTime_ms)
/*
 * comb_temperature_control
 *
 * author : Marco Caron
 *
 * Main algo for getting to temperature setpoints set in paramfile. Currently we have 2 setpoints (High and Low),
 * but we could use this to reach any number of set points
 *
 * using :
 *
 * sParam (temperature and slope targets from paramfile.c relative to states where function is called )
 * */


/*
 *
 * Comb low 2 :
 *
 * à ajouter ici, enregistrer le temps d'entrée en comb, et si ça fait un certain moment,
 * là où on commencerait à ouvrir parce qu'on est en perte de temperature, à la place on ferme.
 *
 * Valider avec les plages de temperature de t avant ou t baffle avec guillaume.
 *
 * normalement ça devrait pas baisser la temperature de faire ça.
 *
 * si on ouvre la grille on devrait avoir un petit boost, si c'est le cas, on peut retourner en combustion normale.
 *
 * */






{
  const PF_StepperStepsPerSec_t *sSpeedParams =  PB_SpeedParams();


  // close grill of tstat is low
  if(!tstat_status && stove->sGrill.u8apertureCmdSteps > sParam->sGrill.i32Min)
  {
    stove->sGrill.u8apertureCmdSteps  = sParam->sGrill.i32Min;
    stove->sGrill.fSecPerStep = 0;
    bStepperAdjustmentNeeded = true;
  }

  // APERTURE CONTROL --------------------

  // TEMPERATURE <<<<< TARGET
  if(tstat_status // only used in High mode
      && (stove->fBaffleTemp < (P2F(sParam->sTemperature.fTarget) - 2 * P2F(sParam->sTemperature.fAbsMaxDiff))) )
  {

    if(print_debug_setup){
      printf("\n TBaffle is VERY under target (%.2f)",
          P2F(sParam->sTemperature.fTarget) - 2 * P2F(sParam->sTemperature.fAbsMaxDiff));}

    // deltaT < -5
    if(stove->fBaffleDeltaT < (P2F1DEC(sParam->sTempSlope.fTarget) -  P2F1DEC(sParam->sTempSlope.fAbsMaxDiff)))
    {
      debug_printer("\n dtbaffle < -5 : changing fast", print_debug_setup);

      if(stove->sPrimary.u8apertureCmdSteps >= sParam->sPrimary.i32Max && tstat_status){
        stove->sGrill.u8apertureCmdSteps = RANGE(sParam->sGrill.i32Min,stove->sGrill.u8apertureCmdSteps + 5 , sParam->sGrill.i32Max);}
      else{
        stove->sPrimary.u8apertureCmdSteps = RANGE(sParam->sPrimary.i32Min,stove->sPrimary.u8apertureCmdSteps * 1.5, sParam->sPrimary.i32Max);
        stove->sSecondary.u8apertureCmdSteps = RANGE(sParam->sSecondary.i32Min,stove->sSecondary.u8apertureCmdSteps * 1.2, sParam->sSecondary.i32Max);

      }
      stove->sGrill.fSecPerStep = 0;
      stove->sPrimary.fSecPerStep = 0; // force aperture
      stove->sSecondary.fSecPerStep = 0;
      bStepperAdjustmentNeeded = true;
      stove->u32MajorCorrectionTime_ms = u32CurrentTime_ms;
    }

    // deltaT > -5
    else
    {
      debug_printer("\n dtbaffle > -5 : slow incrementing \n\n", print_debug_setup);
      aperture_adjust(stove, sParam, 1, -1, -1,
          P2F1DEC(sSpeedParams->fNormal) ,
          stove->sPrimary.fSecPerStep == P2F1DEC(sSpeedParams->fVerySlow),0);
    }

  }


  // TEMPERATURE < TARGET
  else if(stove->fBaffleTemp < (P2F(sParam->sTemperature.fTarget) - P2F(sParam->sTemperature.fTolerance)))
  {
    if(print_debug_setup){
      printf("\n TBaffle is under target (%.2f)",P2F(sParam->sTemperature.fTarget) -  P2F(sParam->sTemperature.fTolerance));}


    // deltaT < -5
    if(stove->fBaffleDeltaT < (P2F1DEC(sParam->sTempSlope.fTarget) - P2F1DEC(sParam->sTempSlope.fAbsMaxDiff)))
    {
      if(print_debug_setup){
        printf("\n Eta 10 B \n under slope target (%0.2f) && TBaffle under %f : changing fast (*1.5)!",
            P2F1DEC(sParam->sTempSlope.fTarget) - P2F1DEC(sParam->sTempSlope.fAbsMaxDiff),
            P2F(sParam->sTemperature.fTarget) -  P2F(sParam->sTemperature.fTolerance));}

      if(stove->sPrimary.u8apertureCmdSteps >= sParam->sPrimary.i32Max && tstat_status){
        stove->sGrill.u8apertureCmdSteps = RANGE(sParam->sGrill.i32Min,stove->sGrill.u8apertureCmdSteps + 5 , sParam->sGrill.i32Max);}
      else{
        stove->sPrimary.u8apertureCmdSteps = RANGE(sParam->sPrimary.i32Min,stove->sPrimary.u8apertureCmdSteps * 1.5, sParam->sPrimary.i32Max);
        stove->sSecondary.u8apertureCmdSteps = RANGE(sParam->sSecondary.i32Min,stove->sSecondary.u8apertureCmdSteps * 1.2, sParam->sSecondary.i32Max);}
      stove->sGrill.fSecPerStep = 0;
      stove->sPrimary.fSecPerStep = 0;
      stove->sSecondary.fSecPerStep = 0;
      bStepperAdjustmentNeeded = true;
      stove->u32MajorCorrectionTime_ms = u32CurrentTime_ms;
    }


    //(-5 < delta T  < -2 )
    else if(stove->fBaffleDeltaT > ( - P2F(5) ) && stove->fBaffleDeltaT < P2F(-2)  )
    {
      debug_printer("(-5 < delta T baffle < -2 ", print_debug_setup);
      aperture_adjust(stove, sParam, 1, -1, -1,
          P2F1DEC(sSpeedParams->fFast) ,
          stove->sPrimary.fSecPerStep == P2F1DEC(sSpeedParams->fVerySlow),0);
    }


    // deltaT +-2
    else if(fabs(stove->fBaffleDeltaT) <= 2  )
    {
      debug_printer("deltaTBaffle [ +- 2]) ", print_debug_setup);

      if(tstat_status){ // only opening on high setting
        aperture_adjust(stove, sParam, 1,
            P2F1DEC(sSpeedParams->fNormal),
            -1, P2F1DEC(sSpeedParams->fSlow) , 0,0);}
    }

  }


  // TEMPERATURE in RANGE
  else if(stove->fBaffleTemp < (P2F(sParam->sTemperature.fTarget) + P2F(sParam->sTemperature.fAbsMaxDiff) ))
  {
    if(print_debug_setup){
      printf(" Temp in range (T < %.2f) \n\r",
          P2F(sParam->sTemperature.fTarget) + P2F(sParam->sTemperature.fAbsMaxDiff));}

    // only doing something if tstat is in low position
    if(!tstat_status){

      // deltaT +- 1
      if(fabs(stove->fBaffleDeltaT) <
          (P2F1DEC(sParam->sTempSlope.fTarget) + P2F1DEC(sParam->sTempSlope.fTolerance)))
      {
        if(print_debug_setup){
          printf("baffleDeltaT  +- %.2f \n\r",
              P2F1DEC(sParam->sTempSlope.fTarget) + P2F1DEC(sParam->sTempSlope.fTolerance));}

        aperture_adjust(stove, sParam,-1,
            P2F1DEC(sSpeedParams->fNormal), // with part dev
            P2F1DEC(sSpeedParams->fNormal), // with part normalized
            P2F1DEC(sSpeedParams->fSlow) , 0,0); // no part (normal case)
      }

      // deltaT > 5
      else if(stove->fBaffleDeltaT > (P2F1DEC(sParam->sTempSlope.fTarget) + P2F1DEC(sParam->sTempSlope.fAbsMaxDiff)))
      {
        if(print_debug_setup){
          printf("baffleDeltaT  > %.2f && baffle Temp < %.2f \n\r",
              (P2F1DEC(sParam->sTempSlope.fTarget) + P2F1DEC(sParam->sTempSlope.fAbsMaxDiff)),
              P2F(sParam->sTemperature.fTarget) + P2F(sParam->sTemperature.fAbsMaxDiff));}

        aperture_adjust(stove,sParam,-1,-1,-1,
            P2F1DEC(sSpeedParams->fFast),
            stove->sPrimary.fSecPerStep == P2F1DEC(sSpeedParams->fVerySlow),
            stove->sPrimary.fSecPerStep == P2F1DEC(sSpeedParams->fSlow));
      }

    }
  }


  // TEMPERATURE IS OVER TARGET
  else if(stove->fBaffleTemp < (P2F(sParam->sTemperature.fTarget) + 3 * P2F(sParam->sTemperature.fAbsMaxDiff) ))
  {
    debug_printer("Tbaffle over target range ", print_debug_setup);

    // deltaT +- 5
    if (fabs(stove->fBaffleDeltaT) <
        (P2F1DEC(sParam->sTempSlope.fTarget) + P2F1DEC(sParam->sTempSlope.fAbsMaxDiff))) {

      debug_printer(" -5 < baffleDeltaT  < 5, closing fast if particles ", print_debug_setup);

      aperture_adjust(stove,sParam,-1,
          P2F1DEC(sSpeedParams->fFast),
          P2F1DEC(sSpeedParams->fFast),
          P2F1DEC(sSpeedParams->fSlow),
          stove->sPrimary.fSecPerStep == P2F1DEC(sSpeedParams->fVerySlow),0);
    }


    // delta T > 5
    else if(stove->fBaffleDeltaT > (P2F1DEC(sParam->sTempSlope.fTarget) + P2F1DEC(sParam->sTempSlope.fAbsMaxDiff)))
    {
      debug_printer(" delta T > 5, closing fast", print_debug_setup);

      aperture_adjust(stove,sParam,-1,-1,-1,
          P2F1DEC(sSpeedParams->fFast),
          stove->sPrimary.fSecPerStep == P2F1DEC(sSpeedParams->fVerySlow),
          stove->sPrimary.fSecPerStep == P2F1DEC(sSpeedParams->fSlow));
    }

  }


  // TEMPERATURE IS very OVER TARGET
  else
  {
    debug_printer("Tbaffle 3 * abs max diff over target range ", print_debug_setup);

    // delta T > 5
    if(stove->fBaffleDeltaT > (P2F1DEC(sParam->sTempSlope.fTarget) + P2F1DEC(sParam->sTempSlope.fAbsMaxDiff)))
    {
      debug_printer(" delta T > 5, closing VERY fast", print_debug_setup);

      aperture_adjust(stove,sParam,-1,-1,-1,
          P2F1DEC(sSpeedParams->fVeryFast),
          stove->sPrimary.fSecPerStep == P2F1DEC(sSpeedParams->fVerySlow),
          stove->sPrimary.fSecPerStep == P2F1DEC(sSpeedParams->fSlow));
    }

    // -10 < delta T < 5
    else if(stove->fBaffleDeltaT > (P2F1DEC(sParam->sTempSlope.fTarget) - 2 * P2F1DEC(sParam->sTempSlope.fAbsMaxDiff)))
    {
      debug_printer(" -10 < baffleDeltaT  < 5, closing fast ", print_debug_setup);

      aperture_adjust(stove,sParam,-1,-1,0,
          P2F1DEC(sSpeedParams->fFast),
          stove->sPrimary.fSecPerStep == P2F1DEC(sSpeedParams->fVerySlow),0);
    }
  }


}



static void aperture_adjust(Mobj* stove, PF_CombustionParam_t *sParam, int change_var, int w_part_dev_change_speed, int w_part_change_speed, int no_part_change_speed , bool add_condition1, bool add_condition2)
{
  /*
   * aperture_adjust
   *
   * Developped by : Marco Caron
   *
   *  Function that adjust stepper apertures if following args   :
   *
   *  add_condition1,  add_condition2 or motors_ready_for_req are met
   *
   *  attention : P2F1 conversion of speeds is done OUTSIDE func, you need to put it inside of call
   *
   *  will set aperture positions and speeds according to defined algo
   *
   *  set part speeds to -1 if not in use
   *
   *
   *
   * aperture_adjust(Mobj* stove,
   * PF_CombustionParam_t *sParam,
   * int change_var, (positive or negative for incrementing or decrementing)
   * int w_part_dev_change_speed, (speed if we have particles std deviation criteria/ var) (set to -1 if not used)
   * int w_part_change_speed, (speed if we have normalized particles criteria) (set to -1 if not used)
   * int no_part_change_speed , (speed if no particles (normal case))
   * bool add_condition1, (alternate speed condition 1 for validation if motors are not ready for req. i.e. moving really slow)
   * bool add_condition2, (alternate speed condition 1 for validation if motors are not ready for req. i.e. moving really really slow)
   *
   *
   *
   * */


  int  apertureSpeedOutput = -1;



  if(motors_ready_for_req || add_condition1 || add_condition2 )
  {


    if(change_var == -1)
    {
      debug_printer("\nDecrementing!", print_debug_setup);

      if(stove->sGrill.u8apertureCmdSteps > sParam->sGrill.i32Min)
      {
        stove->sGrill.u8apertureCmdSteps  = RANGE(sParam->sGrill.i32Min,
            stove->sGrill.u8apertureCmdSteps + change_var,sParam->sGrill.i32Max);
      }
      else if(stove->sPrimary.u8apertureCmdSteps  > sParam->sPrimary.i32Min)
      {
        stove->sPrimary.u8apertureCmdSteps  = RANGE(sParam->sPrimary.i32Min,
            stove->sPrimary.u8apertureCmdSteps + change_var,sParam->sPrimary.i32Max);
      }

      else if(stove->sSecondary.u8apertureCmdSteps > sParam->sSecondary.i32Min)
      {
        stove->sSecondary.u8apertureCmdSteps  = RANGE(sParam->sSecondary.i32Min,
            stove->sSecondary.u8apertureCmdSteps + change_var,sParam->sSecondary.i32Max);
      }
    }


    // Incrementing apertures in order : sec-> prim-> grill
    else if(change_var == 1)
    {
      debug_printer("\nIncrementing!", print_debug_setup);

      if(stove->sSecondary.u8apertureCmdSteps < sParam->sSecondary.i32Max)
      {
        stove->sSecondary.u8apertureCmdSteps  = RANGE(sParam->sSecondary.i32Min,
            stove->sSecondary.u8apertureCmdSteps + 3*change_var,sParam->sSecondary.i32Max);
      }
      else if(stove->sPrimary.u8apertureCmdSteps < sParam->sPrimary.i32Max)
      {
        stove->sPrimary.u8apertureCmdSteps  = RANGE(sParam->sPrimary.i32Min,
            stove->sPrimary.u8apertureCmdSteps + change_var,sParam->sPrimary.i32Max);
      }
      else if((stove->sGrill.u8apertureCmdSteps < sParam->sGrill.i32Max) && tstat_status )
      {
        stove->sGrill.u8apertureCmdSteps  = RANGE(sParam->sGrill.i32Min,
            stove->sGrill.u8apertureCmdSteps + change_var,sParam->sGrill.i32Max);
      }
    }


    if(!(w_part_dev_change_speed == -1)&&(stove->sParticles->u16stDev > sParam->sPartStdev.fTolerance))
    {
      debug_printer(" particulate deviation speed", print_debug_setup);

      stove->sGrill.fSecPerStep = tstat_status ?  w_part_dev_change_speed : stove->sGrill.fSecPerStep ;
      stove->sPrimary.fSecPerStep = w_part_dev_change_speed ;
      stove->sSecondary.fSecPerStep = w_part_dev_change_speed ;
      apertureSpeedOutput = w_part_dev_change_speed ;
    }
    else if(!(w_part_change_speed == -1)&&(stove->sParticles->fparticles > (P2F(sParam->sParticles.fTarget) + P2F(sParam->sParticles.fTolerance))))
    {
      debug_printer(" particulate target speed", print_debug_setup);
      stove->sGrill.fSecPerStep = tstat_status ?  w_part_change_speed : stove->sGrill.fSecPerStep ;
      stove->sPrimary.fSecPerStep = w_part_change_speed;
      stove->sSecondary.fSecPerStep = w_part_change_speed;
      apertureSpeedOutput = w_part_change_speed;
    }
    else if (!(no_part_change_speed == -1))
    {
      debug_printer(" no particulate target speed", print_debug_setup);
      stove->sGrill.fSecPerStep = tstat_status ?  no_part_change_speed : stove->sGrill.fSecPerStep ;
      stove->sPrimary.fSecPerStep = no_part_change_speed;
      stove->sSecondary.fSecPerStep = no_part_change_speed;
      apertureSpeedOutput = no_part_change_speed;
    }


    bStepperAdjustmentNeeded = true;

    if(print_debug_setup){printf("\r\n speed set by slow adjust func is : %i\n\r",apertureSpeedOutput);}
    return;

  }

  // if we get to this line, we were not ready for a motor adjustment.
}



static void debug_printer(char str[], _Bool debugisOn) {

  if(debugisOn){
    printf("\n\r%s \n\r", str);
  }

}

static void array_shifter(int array[], int l, int newval){


  for (int i = 0; i <l; i++ )
  {
    if (i+1 <l)
    {
      array[i] =array[i+1];
    }
    else
    {
      array[i] = newval;
      break;
    }
  }

}

static void array_setter(int array[], int l, int newval){

  for (int i = 0; i <l; i++ )
  {
    array[i] = newval;
  }

}


static void array_printer(int array[],int l)
{
  printf("\n\r");
  for(int loop = 0; loop < l; loop++)
  {
    printf("%d ", array[loop]);
  }
  printf("\n\r");
}



