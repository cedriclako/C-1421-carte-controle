/* 
 * File:   ControlBridge.h
 * Author: crichard
 *
 * Created on October 14, 2022, 11:09 AM
 */

#ifndef CONTROLBRIDGE_H
#define	CONTROLBRIDGE_H

#ifdef	__cplusplus
extern "C" {
#endif
    #include "MeasureParticles.h"

    void bridgeDataRDY(void);
    void ControlBridgeInitialize(void);
    void ControlBridgeProcess(void);
    void controlBridge_update(SMeasureParticlesObject* mOBJ);
    bool isBridgeRDY(void);
    


#ifdef	__cplusplus
}
#endif

#endif	/* CONTROLBRIDGE_H */

