Describe variables


|Variable 		|Type		|Description							|Description_EN	|
| ------------- |:--------:	| ------------------------------------:|---------------|
|TavantF		|int32		|Température foyer avant				|Front fire temperature|
|TArriereF		|int32		|Température foyer arrière				|Back fire temperature|
|PlenumF		|int32		|Température Plenum (147F )			|Plenum temperature|
|state			|ESTOVESTATE|État de l'algo						|Algorithm state|
|tStat 			|bool		|État du thermostat					|Thermostat state|
|dTav			|int32		|Pente du changement de température avant|Temperature change slope before|
|FanSpeed		|int32		|Vitesse de fan						|Fan speed|
|Grille			|int32		|Position du moteur de grille			|Grille motor position|
|PIDPos			|int32		|Commande PID							|PID command|
|Prim			|int32		|Position du moteur du primaire		|Primary motor position|
|Sec			|int32		|Position du moteur du secondaire		|Secondary motor position|
|TboardF		|int32		|Température du PCB de contrôle		|Control PCB temperature|
|Door			|bool		|Porte ouverte?						|Door open?|
|PartCH0		|int32		|# de comptes du capteur de particules (CHO)|Particle sensor counts (CHO)|
|PartCH1		|int32		|# de comptes du capteur de particules (CH1)|Particle sensor counts (CH1)|
|PartVar		|int32		|Variance des mesures de CH0			|CH0 measurement variance|
|PartSlope		|int32		|Pente des mesures de CH0				|CH0 measurement slope|
|TPartF			|int32		|Température							|Temperature|
|PartCurr		|int32		|Courant mesuré dans les DELs			|Current measured in LEDs|
|PartLuxON		|int32		|Valeur de Lux mesurée avec DELs allumées|Lux value measured with LEDs on|
|PartLuxOFF		|int32		|Valeur de Lux mesurée avec DELs éteintes|Lux value measured with LEDs off|
|PartTime		|int32		|Millisecondes écoulées depuis l'init	|Milliseconds since init|