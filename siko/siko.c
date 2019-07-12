/*********************************************************************************
 * Filename:    siko.c
 * Description:	SIKO System für geführte oder vollautomatische Positionierung
 *              von Formateinstellungen.
 * Author:      Niklaus Leuenberger (@NikLeberg)
 * Version:     0.1
 * Date:        2019-07-12
 *********************************************************************************/

#include <bur/plctypes.h>
#include "siko.h"

/* Konfiguriere SIKO System */
unsigned long sikoConfig(siko_typ* s, plcstring* interface) {
	// Warteschlange vorbereiten
	s->job = 0;
	// CANopen konfigurieren
	sikoCanConfig(s, interface);
	// Zykluszeit ermitteln
	RTInfo_typ rtInfo;
	rtInfo.enable = 1;
	RTInfo(&rtInfo);
	s->cycleTime = (rtInfo.cycle_time / 1000); // us -> ms
	// Slaves müssen einzeln und manuell konfiguriert werden.
	return sikoERR_OK;
}

/* Initialisiere SIKO System */
unsigned long sikoInit(siko_typ* s) {
	// CANopen
	if (sikoCanInit(s)) return sikoERR_INIT_CAN;
	// Slaves
	if (sikoSlaveInit(s, 0)) return sikoERR_INIT_SLAVES;
	// Ende
	return sikoERR_OK;
}

/* Bearbeite SIKO System */
unsigned long sikoRun(siko_typ* s) {
	// Aktualisierung aller Slaves
	sikoSlaveUpdate(s, 0);
	// pendente Jobs abarbeiten
	unsigned long error;
	error = sikoJobRun(s);
	// Sicherheit
	// Not-Stopp auslösen?
	if (error) return error; // Jobs noch busy oder Fehlerhaft
}
