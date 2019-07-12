/*********************************************************************************
 * Filename:    sikoSlave.c
 * Description:	Verwaltung der SIKO Geräte.
 * Author:      Niklaus Leuenberger (@NikLeberg)
 * Version:     0.1
 * Date:        2019-07-12
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
	// CAN-Objekte initialisieren
	s->slaves[address].canNMT.enable = 1;
	s->slaves[address].canNMT.pDevice = (unsigned long) s->can.interface;
	s->slaves[address].canNMT.node = address;
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
	if (j->slave < 2 || j->slave > 127) return sikoERR_NULL;
	switch (j->step) {
		default:
		case 0: // Bus zurücksetzen
			if (j->prevError) return j->prevError; // vorhergehender Fehler
			sikoJobPrequeue(s, j->slave, (unsigned long) &sikoCanNMT, coRESET_NODE);
			sikoSlaveStatusSetInternal(s, j->slave, sikoSLAVE_INIT_RESET);
			j->step = 1;
			break;
		case 1: // Bootup prüfen
			if (j->prevError) return j->prevError; // vorhergehender Fehler
			sikoJobPrequeue(s, j->slave, (unsigned long) &sikoCanBootup, 0);
			sikoSlaveStatusSetInternal(s, j->slave, sikoSLAVE_INIT_BOOTUP);
			j->step = 2;
			break;
		case 2: // Auf Werkseinstellungen zurücksetzen
			if (j->prevError) return j->prevError; // vorhergehender Fehler
			if (!j->prevResult) return sikoERR_OK; // vorhergehender Fehler, kein Slave vorhanden
			sikoJobPrequeue(s, j->slave, (unsigned long) &sikoJobSleep, 1000); // 1s warten
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
			sikoSlaveStatusSetInternal(s, j->slave, sikoSLAVE_INIT_ENABLE_SYNC);
			j->step = 7;
			break;
		case 7: // PDO-CobIDs registrieren
			if (j->prevError) return j->prevError; // vorhergehender Fehler
			sikoJobPrequeue(s, j->slave, (unsigned long) &sikoCanRegister, 0x380); // TPDO3, 6 Bytes, Statuswort + Positionswert
			sikoSlaveStatusSetInternal(s, j->slave, sikoSLAVE_INIT_REGISTER_COBID);
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
			sikoSlaveStatusSetInternal(s, j->slave, sikoSLAVE_INIT_OPERATIONAL);
			j->step = 10;
			break;
		case 10: // State Machine in "Ready to Switch On" schalten
			if (j->prevError) return j->prevError; // vorhergehender Fehler
			j->step = 100;
			sikoJobPrequeue(s, j->slave, (unsigned long) &sikoAG06stateSet, sikoAG06_STATE_RDY_SW_ON);
			sikoSlaveStatusSetInternal(s, j->slave, sikoSLAVE_INIT_WAIT_STATUS);
			break;
		case 100: // Initialisierung abgeschlossen
			if (j->prevError) return j->prevError; // vorhergehender Fehler
			sikoSlaveStatusSetInternal(s, j->slave, sikoSLAVE_INIT_DONE);
			return sikoERR_OK;
			break;
	}
	// Job arbeitet noch
	return ERR_FUB_BUSY;
}

/* Setze lokalen Status eines Slaves */
unsigned long sikoSlaveStatusSetInternal(siko_typ* s, unsigned char slave, enum sikoSLAVE_STATUS status) {
	if (slave < 128) {
		s->slaves[slave].statusInternal = status;
		//s->slaves[slave].timestamp = clock_ms();
	}
	return sikoERR_OK;
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
	if (pSlave->statusInternal < sikoSLAVE_INIT_DONE) return sikoERR_OK; // Sensor nicht vorhanden / unkonfiguriert etc.
	// Ausführen
	do {
		// Ausführen
		CANopenPDORead8(&pSlave->canPDO);
		// Fehlerprüfung
		if (pSlave->canPDO.status == ERR_FUB_BUSY) return ERR_FUB_BUSY;
		else if (pSlave->canPDO.status) return sikoERR_CAN;
		// Prüfe auf gültige Daten
		if (pSlave->canPDO.error == coNO_VALID_DATA) return ERR_FUB_BUSY;
		// Status speichern
		pSlave->statusCan = pSlave->canPDO.error;
		pSlave->statusSiko = pSlave->canPDO.data0 + (pSlave->canPDO.data1 << 8);
		pSlave->statusStateMachine = sikoAG06stateGet((unsigned char) pSlave->statusSiko);
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
	// Ist Slave im Status "Operation Enabled"?
	//if ((s->slaves[j->slave].statusSiko & sikoAG06_OPERATION_ENABLED_MASK) != sikoAG06_states[sikoAG06_OPERATION_ENABLED]) return sikoERR_STATE;
	// Parameter setzen
	s->can.pdoWrite.cobid = 0x400 + j->slave; // RPDO3
	s->can.pdoWrite.datalen = 6; // Control Word + Position
	s->can.pdoWrite.data0 = 0b0000000000011111;
	s->can.pdoWrite.data1 = (0b0000000000011111 >> 8);
	s->can.pdoWrite.data2 = j->parameter;
	s->can.pdoWrite.data3 = (j->parameter >> 8);
	s->can.pdoWrite.data4 = (j->parameter >> 16);
	s->can.pdoWrite.data5 = (j->parameter >> 24);
	// Ausführen
	CANopenPDOWrite8(&s->can.pdoWrite);
	// Status zurückmelden
	return s->can.pdoWrite.status;
}
