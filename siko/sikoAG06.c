/*********************************************************************************
 * Filename:    sikoAG06.c
 * Description:	HAL für SIKO AG06 Stellantriebe.
 * Author:      Niklaus Leuenberger (@NikLeberg)
 * Version:     0.1
 * Date:        2019-07-12
 *********************************************************************************/

#include <bur/plctypes.h>
#include "siko.h"

/* Eruiere Status des Antriebs */
unsigned char sikoAG06stateGet(unsigned char statusWord) {
	// Not Ready To Switch On
	if ((statusWord & 0b01001111) == 0b00000000) return sikoAG06_STATE_N_RDY;
	// Switch On Disabled		
	else if ((statusWord & 0b01001111) == 0b01000000) return sikoAG06_STATE_SW_ON_DIS;
	// Ready to Switch On
	else if ((statusWord & 0b01101111) == 0b00100001) return sikoAG06_STATE_RDY_SW_ON;
	// Switched On
	else if ((statusWord & 0b01101111) == 0b00100011) return sikoAG06_STATE_SW_ON;
	// Operation Enabled
	else if ((statusWord & 0b01101111) == 0b00100111) return sikoAG06_STATE_OP_EN;
	// Quick Stop Active
	else if ((statusWord & 0b01101111) == 0b00000111) return sikoAG06_STATE_Q_ST_ACT;
	// Fault
	else if ((statusWord & 0b01001111) == 0b00001000) return sikoAG06_STATE_FAULT;
	// Unknown
	else return sikoAG06_STATE_UNKNOWN;
}

/*********************************************************************************
 * curr State->	| N_RDY	| SW_ON_DIS	| RDY_SW_ON	| SW_ON | OP_EN | Q_ST_ACT	| FAULT
 * N_RDY		| =		| x			| x			| x		| x		| x			| x
 * SW_ON_DIS	| a		| =			| 7			| 10	| 9		| 12		| 14
 * RDY_SW_ON	| a		| 2			| =			| 6		| 8		| 12		| 14
 * SW_ON		| a		| 2			| 3			| =		| 5		| 12		| 14
 * OP_EN		| a		| 2			| 3			| 4		| 4		| 12		| 14
 * Q_ST_ACT		| a		| 2			| 3			| 4		| 11	| =			| 14
 * FAULT		| x		| x			| x			| x	 	| x		| x			| =
 ****
 * x = Not Allowed
 * a = Automatic Transition
 * = = No Transition needed
 ****
 * 1st Index = requested State
 * 2nd Index = current State
 * Value	 = needed Command
 *********************************************************************************/
unsigned short sikoAG06stateT[7][7] = {
	{sikoAG06_IEQUAL, sikoAG06_NALLOW, sikoAG06_NALLOW, sikoAG06_NALLOW, sikoAG06_NALLOW, sikoAG06_NALLOW, sikoAG06_NALLOW},
	{sikoAG06_AUTOMA, sikoAG06_IEQUAL, sikoAG06_DIVOLT, sikoAG06_DIVOLT, sikoAG06_DIVOLT, sikoAG06_DIVOLT, sikoAG06_FRESET},
	{sikoAG06_AUTOMA, sikoAG06_STDOWN, sikoAG06_IEQUAL, sikoAG06_STDOWN, sikoAG06_STDOWN, sikoAG06_DIVOLT, sikoAG06_FRESET},
	{sikoAG06_AUTOMA, sikoAG06_STDOWN, sikoAG06_SWCHON, sikoAG06_IEQUAL, sikoAG06_DSOPER, sikoAG06_DIVOLT, sikoAG06_FRESET},
	{sikoAG06_AUTOMA, sikoAG06_STDOWN, sikoAG06_SWCHON, sikoAG06_ENOPER, sikoAG06_ENOPER, sikoAG06_DIVOLT, sikoAG06_FRESET},
	{sikoAG06_AUTOMA, sikoAG06_STDOWN, sikoAG06_SWCHON, sikoAG06_ENOPER, sikoAG06_QUIKST, sikoAG06_IEQUAL, sikoAG06_FRESET},
	{sikoAG06_NALLOW, sikoAG06_NALLOW, sikoAG06_NALLOW, sikoAG06_NALLOW, sikoAG06_NALLOW, sikoAG06_NALLOW, sikoAG06_IEQUAL}};

/* StateMachine gemäss CiA 402 ablaufen bis gewünschten State erreicht*/
unsigned long sikoAG06stateSet(siko_typ* s, sikoJob_typ* j) {
	unsigned char state = sikoAG06stateGet(s->slaves[j->slave].statusSiko);
	unsigned char stateRequested = j->parameter;
	if (state == sikoAG06_STATE_UNKNOWN) return sikoAG06_ERR_STATE;
	unsigned short command = sikoAG06stateT[stateRequested][state];
	if (command == sikoAG06_IEQUAL) return sikoERR_OK; // ist == soll, nichts machen
	if (command == sikoAG06_AUTOMA) return sikoERR_OK; // automatisch, nichts machen
	if (command == sikoAG06_NALLOW) return sikoERR_STATE; // übergang nicht möglich / ungültig
	// Maximal 10x versuchen
	++j->step;
	if (j->step == 10) return sikoERR_STATE;
	// State per Command wechseln
	sikoJobPrequeue(s, j->slave, (unsigned long) &sikoSlaveControl, command);
	return ERR_FUB_BUSY;
}

/* Tastensperre aktivieren / deaktivieren */
unsigned long sikoAG06keyLock(siko_typ* s, sikoJob_typ* j) {
	// Schrittkette
	switch (j->step) {
		default:
		case 0:
			// nur in "Operation Enabled" gültig
			if (sikoAG06stateGet(s->slaves[j->slave].statusSiko) != sikoAG06_STATE_OP_EN) return sikoAG06_ERR_STATE;
			// Ausführen, Parameter: 1 = freigeben, 0 = sperren
			sikoJobPrequeue(s, j->slave, (unsigned long) &sikoSlaveControl, sikoAG06_ENOPER | ((plcbit) j->parameter << 11));
			++j->step;
			break;
		case 1:
			// Prüfen
			if (j->prevError) return sikoERR_PREV;
			else return sikoERR_OK;
	}
	return ERR_FUB_BUSY;
}