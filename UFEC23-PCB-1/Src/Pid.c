#include "PID.h"

int UpdatePID(PIDtype * pid, int currentTemp, int targetTemp)
{
    int pTerm;
    int dTerm;
	int iTerm;
    int error;

    int currentTempdC = currentTemp/10;
    int TargetTempdC = targetTemp/10;
    error = TargetTempdC - currentTempdC;

    //Proportionnal term
    pTerm = pid->kp * error;

    //Integral state with appropriate limiting
    pid->iError += error;
    if(pid->iError > pid->iErrorMax)
    {
    	pid->iError = pid->iErrorMax;
    }
    else if(pid->iError < pid->iErrorMin)
    {
    	pid->iError = pid->iErrorMin;
    }

    iTerm = pid->ki * pid->iError;

    //Calculate derivate term best on error reduction
    //dTerm = (error - pid->previousError) * pid->kd;
    dTerm = (pid->dLastValue - currentTempdC) * pid->kd;

    pid->dLastValue = currentTempdC;

    return (pTerm + dTerm + iTerm); //iTerm +

}

void initPID(PIDtype* pid, float ki, float kd, float kp,int iErrorMax, int iErrorMin)
{
	pid->dLastValue = 0;
	pid->iErrorMax = iErrorMax;
	pid->iErrorMin = iErrorMin;
	pid->previousError = 0;
	pid->kd = kd;
	pid->ki = ki;
	pid->kp = kp;
	pid->PIDPosition = 0;
}
