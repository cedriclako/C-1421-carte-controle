#ifndef DEBUG_MAN_H
#define	DEBUG_MAN_H

#include "Algo.h"
//public handle
//public function
void DebugManager(Mobj * stove, uint32_t u32time_ms);
void PrintOutput(Mobj * stove, State currentState , State lastState , State nextState);

#endif	/* TEMPERATURE_MAN_H  */
