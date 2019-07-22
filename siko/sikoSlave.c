/*********************************************************************************
 * Filename:    sikoSlave.c
 * Description:	Verwaltung der SIKO Geräte.
 * Author:      Niklaus Leuenberger (@NikLeberg)
 * Version:     0.2
 * Date:        2019-07-22
 *********************************************************************************/

#include <bur/plctypes.h>
#include "siko.h"

/* Slave konfiguration speichern */
unsigned long sikoSlaveConfig(siko_typ* s, unsigned char address, sikoSlave_typ* slave) {
	// Parameter prüfen
	if (address < 2 || address > 127) return sikoERR_SLAVE_CONFIG_VALUE;
	if (!slave) return sikoERR_NULL;
	if (slave->min > 9999) return sikoERR_SLAVE_CONFIG_VALUE;
	if (slave->max > 9999) return sikoERR_SLAVE_CONFIG_VALUE;
	plcbit typeOk = 0;
	for (unsigned short i = 0; i < sikoSlaveSupportedNum; ++i) {
		if (brsstrcmp((unsigned long) slave->type, (unsigned long) sikoSlaveSupported[i]) == 0) {
			typeOk = 1;
			break;
		}
	}
	//if (!typeOk) return sikoERR_SLAVE_CONFIG_TYPE;
	// Speichern
	brsmemcpy(&s->slaves[address], slave, sizeof(sikoSlave_typ));
	// PDO-Objekt initialisieren
	s->slaves[address].canPDO.enable = 1;
	s->slaves[address].canPDO.pDevice = (unsigned long) s->can.interface;
	s->slaves[address].canPDO.cobid = 0x380 + address; // TPDO3
	// Beenden
	return sikoERR_OK;
}

/* Initialisiere Slaves */
unsigned long sikoSlaveInit(siko_typ* s, unsigned char slave) {
    // Wenn alle Slaves initialisiert werden sollen, spawne Job für jeden
	if (slave < 2) {
		// Jobs erstellen
		for (unsigned short i = 2; i < 128; ++i) {
			sikoJobEnqueue(s, i, (unsigned long) &sikoSlaveInitJob, 0);
		}
	} else {
		sikoJobEnqueue(s, slave, (unsigned long) &sikoSlaveInitJob, 0);
	}
	// Job beenden
	return sikoERR_OK;
}

/* Init-Job */
unsigned long sikoSlaveInitJob(siko_typ* s, sikoJob_typ* j) {
	// Schrittweise ein Slave initialisieren
	// 0 -> Busreset
	// 1 -> Bootup prüfen
	// 2 -> Auf Werkseinstellungen zurücksetzen
	// 3 -> Überwachung aktivieren
	// 4 -> Standartparameter hochladen
	// 5 -> Zusätzliche Parameter hochladen
	// 6 -> Übertragungsart des TPDO3 auf Sync einstellen
	// 7 -> PDO-CobIDs registrieren
	// 8 -> Zyklischer Sync aktivieren
	// 9 -> NMT Operational schalten
	// 10 -> State Machine in "Ready to Switch On" schalten
	// 100 -> Initialisierung abgeschlossen    
    if (j->slave == 2) s->data0 = j->step; // DEBUG
    if (j->slave == 3) s->data1 = j->step; // DEBUG
	if (j->slave < 2 || j->slave > 127) return sikoERR_NULL;
	switch (j->step) {
		default:
		case 0: // Bus zurücksetzen
			if (j->prevError) return j->prevError; // vorhergehender Fehler
			sikoJobPrequeue(s, j->slave, (unsigned long) &sikoCanNMT, coRESET_NODE);
			j->step = 1;
			break;
		case 1: // Bootup prüfen
			if (j->prevError) return j->prevError; // vorhergehender Fehler
            sikoJobPrequeue(s, j->slave, (unsigned long) &sikoCanBootup, 0);
            sikoJobPrequeue(s, j->slave, (unsigned long) &sikoJobSleep, 200); // 200ms warten
			j->step = 2;
			break;
		case 2: // Auf Werkseinstellungen zurücksetzen
			if (j->prevError) return j->prevError; // vorhergehender Fehler
			if (!j->prevResult) return sikoERR_OK; // vorhergehender Fehler, kein Slave vorhanden
			//sikoJobPrequeue(s, j->slave, (unsigned long) &sikoJobSleep, 5000); // 1s warten
			sikoJobPrequeue(s, j->slave, (unsigned long) &sikoCanSDOwrite, (unsigned long) "1011:04=64616f6c");
			sikoJobPrequeue(s, j->slave, (unsigned long) &sikoCanSDOwrite, (unsigned long) "1011:05=64616f6c");
			sikoJobPrequeue(s, j->slave, (unsigned long) &sikoCanSDOwrite, (unsigned long) "1011:07=64616f6c");
			j->step = 3;
			break;
		case 3: // Überwachung aktivieren
			if (j->prevError) return j->prevError; // vorhergehender Fehler
			if (!j->prevResult) return sikoERR_OK; // vorhergehender Fehler, kein Slave vorhanden
			j->step = 4;
			break;
		case 4: // Standartparameter hochladen
			if (j->prevError) return j->prevError; // vorhergehender Fehler
			j->step = 5;
			sikoJobPrequeue(s, j->slave, (unsigned long) &sikoCanSDOwrite, (unsigned long) "2400:08=01"); // Tastensperre
			// Inpos-Mode?
			// ToDo
			break;
		case 5: // Zusätzliche Parameter hochladen
			if (j->prevError) return j->prevError; // vorhergehender Fehler
			j->step = 6;
			// ToDo
			break;
		case 6: // Übertragungsart des TPDO3 auf Asynchron einstellen + alle 10s
			if (j->prevError) return j->prevError; // vorhergehender Fehler
			sikoJobPrequeue(s, j->slave, (unsigned long) &sikoCanSDOwrite, (unsigned long) "1802:02=fe"); // asynchron nach Änderung
			sikoJobPrequeue(s, j->slave, (unsigned long) &sikoCanSDOwrite, (unsigned long) "1802:05=2710"); // zyklisch alle 10s Update schicken
			j->step = 7;
			break;
		case 7: // PDO-CobIDs registrieren
			if (j->prevError) return j->prevError; // vorhergehender Fehler
			sikoJobPrequeue(s, j->slave, (unsigned long) &sikoCanRegister, 0x380); // TPDO3, 6 Bytes, Statuswort + Positionswert
			j->step = 8;
			break;
		case 8: // Zyklischer Sync aktivieren
			if (j->prevError) return j->prevError; // vorhergehender Fehler
			j->step = 9;
			// ToDo
			break;
		case 9: // NMT Operational schalten
			if (j->prevError) return j->prevError; // vorhergehender Fehler
			sikoJobPrequeue(s, j->slave, (unsigned long) &sikoCanNMT, coSTART_REMOTE_NODE);
			j->step = 10;
			break;
		case 10: // State Machine in "Ready to Switch On" schalten
			if (j->prevError) return j->prevError; // vorhergehender Fehler
			j->step = 100;
			sikoJobPrequeue(s, j->slave, (unsigned long) &sikoAG06stateSet, sikoAG06_STATE_RDY_SW_ON);
			break;
		case 100: // Initialisierung abgeschlossen
			if (j->prevError) return j->prevError; // vorhergehender Fehler
            s->slaves[j->slave].canPDO.enable = 1; // TPDO aktivieren
			return sikoERR_OK;
			break;
	}
	// Job arbeitet noch
	return ERR_FUB_BUSY;
}

/* Slave Control Word setzen */
unsigned long sikoSlaveControl(siko_typ* s, sikoJob_typ* j) {
	// Parameter setzen
	s->can.pdoWrite.cobid = 0x200 + j->slave; // RPDO1
	s->can.pdoWrite.datalen = 2; // Control Word ist 16 Bit weit
	s->can.pdoWrite.data0 = (unsigned char) j->parameter;
	s->can.pdoWrite.data1 = (unsigned char) (j->parameter >> 8);
	// Ausführen
	CANopenPDOWrite8(&s->can.pdoWrite);
	// Status zurückmelden
	return s->can.pdoWrite.status;
}

/* Aktualisiere Positionswert */
unsigned long sikoSlaveUpdate(siko_typ* s, unsigned char slave) {
	// Parameter prüfen
	if (!s) return sikoERR_NULL;
	if (slave < 2) {
		for (unsigned short i = 2; i < 128; ++i) {
			sikoSlaveUpdate(s, i);
		}
		return sikoERR_OK;
	} else if (slave > 127) return sikoERR_NULL;
	// Variablen
	sikoSlave_typ* pSlave;
	pSlave = &s->slaves[slave];
	// Status prüfen
	if (!pSlave->canPDO.enable) return sikoERR_OK; // PDO nicht aktiviert
	// Ausführen
	do {
		// Ausführen
		CANopenPDORead8(&pSlave->canPDO);
		// Fehlerprüfung
		if (pSlave->canPDO.status == ERR_FUB_BUSY) return ERR_FUB_BUSY;
		else if (pSlave->canPDO.status) return sikoERR_CAN;
		// Prüfe auf gültige Daten
		if (pSlave->canPDO.error) return ERR_FUB_BUSY;
		// Status speichern
		pSlave->statusCan = pSlave->canPDO.error;
		pSlave->statusSiko = pSlave->canPDO.data0 + (pSlave->canPDO.data1 << 8);
		// Position speichern
		pSlave->actual = 
			pSlave->canPDO.data2 +
			(pSlave->canPDO.data3 << 8) +
			(pSlave->canPDO.data4 << 16) +
			(pSlave->canPDO.data5 << 24);
		// Zeitstempel der Aktualisierung speichern
		pSlave->timestamp = clock_ms();
	} while (pSlave->canPDO.status == ERR_OK); // starte erneut nach erfolgreichem Kopieren
	// Beende
	return sikoERR_OK;
}

/* Neuen Positionswert übernehmen und ausführen */
unsigned long sikoSlavePosition(siko_typ* s, unsigned char slave, signed long position) {
	// Parameter prüfen
	if (!s) return sikoERR_NULL;
	if (slave < 2 || slave > 127) return sikoERR_PARAMETER;
    if (position < s->slaves[slave].min || position > s->slaves[slave].max) return sikoERR_PARAMETER;
    // in Bewegung? (Bit14 oder Bit15 == True)
    if (s->slaves[slave].statusSiko >> 14) return sikoERR_STATE;
	// Slave in Status "Operation Enabled" setzen
	sikoJobEnqueue(s, slave, (unsigned long) &sikoAG06stateSet, sikoAG06_STATE_OP_EN);
	// Setpoint speichern
	s->slaves[slave].setpoint = position;
	// Job starten
	return sikoJobEnqueue(s, slave, (unsigned long) &sikoSlavePositionJob, (unsigned long) position);
}

/* Positionier-Job */
unsigned long sikoSlavePositionJob(siko_typ* s, sikoJob_typ* j) {
	// Parameter prüfen
	if (!s || !j) return sikoERR_NULL;
	if (j->slave < 2 || j->slave > 127) return sikoERR_PARAMETER;
	// im Status "Operation Enabled"?
    if (sikoAG06stateGet(s->slaves[j->slave].statusSiko) != sikoAG06_STATE_OP_EN) return sikoERR_STATE;
	// Parameter setzen
	s->can.pdoWrite.cobid = 0x400 + j->slave; // RPDO3
	s->can.pdoWrite.datalen = 6; // Control Word + Position
	s->can.pdoWrite.data0 = 0b00011111; // Befehl "New Setpoint"
	s->can.pdoWrite.data1 = 0b00000000;
	s->can.pdoWrite.data2 = j->parameter;
	s->can.pdoWrite.data3 = (j->parameter >> 8);
	s->can.pdoWrite.data4 = (j->parameter >> 16);
	s->can.pdoWrite.data5 = (j->parameter >> 24);
	// Ausführen
	CANopenPDOWrite8(&s->can.pdoWrite);
	// Status zurückmelden
	return s->can.pdoWrite.status;
}

/* Quick-Stop */
unsigned long sikoSlaveStop(siko_typ* s, unsigned char slave) {
    // alle Jobs abbrechen
    while (s->numJobs) sikoJobDequeue(s);
    // Befehl senden
    return sikoJobEnqueue(s, slave, (unsigned long) &sikoAG06stateSet, sikoAG06_STATE_Q_ST_ACT);
}

///* Iterierhilfe über alle parametrierten Slaves */
//unsigned char sikoSlaveNext(siko_typ* s, unsigned char slave) {
//    for (; slave < 128; ++slave) {
//        if (s->slaves[slave].canPDO.enable) return slave;
//    }
//}
