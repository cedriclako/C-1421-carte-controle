#ifndef PID_H
#define	PID_H

#define PID_UPDATE_PERIOD  5

typedef struct
{
    int dLastValue;            //  Last position input
    int iError;                //  Integrator state
    int iErrorMax;
	int iErrorMin;  //  Maximum and minimum allowable integrator state
	int previousError;	//for proportional adjustment
	float kp;            //  Proportionnal gain
	float ki;            //  Integral gain
	float kd;            //  Derivative gain
	int PIDPosition;

}PIDtype;

int UpdatePID(PIDtype* pid, int currentTemp, int targetTemp);
void initPID(PIDtype* pid, float ki, float kd, float kp,int ierrorMax, int iErrorMin);

#endif	/* PILOTE_ADC_H */
