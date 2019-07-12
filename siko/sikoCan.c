/*********************************************************************************
 * Filename:    sikoCan.c
 * Description:	Schnittstelle zwischen Bibliothek und den SIKO Geräten über die
 *              libAsCANopen von B&R.
 * Author:      Niklaus Leuenberger (@NikLeberg)
 * Version:     0.1
 * Date:        2019-07-12
 *********************************************************************************/

#include <bur/plctypes.h>
#include "siko.h"

/* Konfiguriere CAN-Parameter */
unsigned long sikoCanConfig(siko_typ* s, plcstring* interface) {
	// Interface
	brsstrcpy((unsigned long) s->can.interface, (unsigned long) interface);
	// Beende
	return sikoERR_OK;
}

/* Initialisiere CAN-Objekte */
unsigned long sikoCanInit(siko_typ* s) {
	// Objekte vorbelegen
	s->can.nmt.enable = 1;
	s->can.nmt.pDevice = (unsigned long) s->can.interface;
	s->can.bootup.enable = 1;
	s->can.bootup.pDevice = (unsigned long) s->can.interface;
	s->can.pdoWrite.enable = 1;
	s->can.pdoWrite.pDevice = (unsigned long) s->can.interface;
	s->can.pdoRegister.enable = 1;
	s->can.pdoRegister.pDevice = (unsigned long) s->can.interface;
	s->can.pdoRead.enable = 1;
	s->can.pdoRead.pDevice = (unsigned long) s->can.interface;
	s->can.sdoRead.pDevice = (unsigned long) s->can.interface;
	s->can.sdoWrite.pDevice = (unsigned long) s->can.interface;
	s->can.sync.enable = 1;
	s->can.sync.pDevice = (unsigned long) s->can.interface;
	// Zyklischen Sync deaktivieren
	sikoJobEnqueue(s, 0, (unsigned long) &sikoCanSync, 0);
	// Beende
	return sikoERR_OK;
}

/* Zyklischer Sync aktivieren */
unsigned long sikoCanSync(siko_typ* s, sikoJob_typ* j) {
	// bei 0 zyklischer Sync deaktivieren, sonst Intervallzeit des Syncs
	s->can.sync.synctime = j->parameter;
	s->can.sync.syncmode = (j->parameter != 0);
	// zyklischer Sync
	CANopenEnableCyclicSync(&s->can.sync);
	// Status zurückmelden
	return s->can.sync.status;
}

/* CANopen-NMT */
unsigned long sikoCanNMT(siko_typ* s, sikoJob_typ* j) {
	// Init
	s->can.nmt.node = j->slave;
	s->can.nmt.state = j->parameter;
	// Ausführen
	CANopenNMT(&s->can.nmt);
	// Status zurückmelden, laufender Job wird aufgerufen solange status == ERR_FUB_BUSY
	return s->can.nmt.status;
}

/* CANopen-Bootup */
unsigned long sikoCanBootup(siko_typ* s, sikoJob_typ* j) {
	// Init
	s->can.bootup.node = j->slave;
	// Ausführen
	CANopenRecvBootupDev(&s->can.bootup);
	// Status speichern + zurückmelden, laufender Job wird aufgerufen solange error == ERR_FUB_BUSY
	j->parameter = s->can.bootup.recv;
	return s->can.bootup.status;
}

/* Cob-ID eines Slaves im Master registrieren */
unsigned long sikoCanRegister(siko_typ* s, sikoJob_typ* j) {
	// Parameter setzen
	s->can.pdoRegister.cobid = j->parameter + j->slave;
	s->can.pdoRegister.inscribe = 1;
	// Ausführen
	CANopenRegisterCOBID(&s->can.pdoRegister);
	// Status zurückmelden
	return s->can.pdoRegister.status;
}

/* SDO-Parameterstring einlesen */
unsigned long sikoCanFormatSDO(sikoSDO_typ* sdo, plcstring* str) {

	// Beende
	return sikoERR_OK;
}

/* Slave per SDO parametrieren */
unsigned long sikoCanSDOwrite(siko_typ* s, sikoJob_typ* j) {
	// Parameter ist Pointer zu String mit Hex-Zahlen im Format: Index:Subindex=Wert e.g. 1802:02=fe, Wert muss mindestens 8 Bits lang sein
	plcstring* str = j->parameter;
	if (!str || str[4] != ':' || str[7] != '=') return sikoERR_NULL;
	// Index einlesen
	unsigned char i = 0, digit = 0;
	unsigned short index = 0;
	for (; i < 4; ++i) {
		digit = str[i] - '0';
		if (digit > 9) digit = str[i] - 'A' + 10;
		if (digit > 15) digit = str[i] - 'a' + 10;
		if (digit > 15) return sikoERR_PARAMETER;
		index = index * 16 + digit;
	}
	// Subindex einlesen
	digit = 0;
	unsigned char subindex = 0;
	for (++i; i < 7; ++i) {
		digit = str[i] - '0';
		if (digit > 9) digit = str[i] - 'A' + 10;
		if (digit > 15) digit = str[i] - 'a' + 10;
		if (digit > 15) return sikoERR_PARAMETER;
		subindex = subindex * 16 + digit;
	}
	// Parameterwert einlesen
	digit = 0;
	unsigned long long value = 0;
	for (++i; str[i] != '\0'; ++i) {
		digit = str[i] - '0';
		if (digit > 9) digit = str[i] - 'A' + 10;
		if (digit > 15) digit = str[i] - 'a' + 10;
		if (digit > 15) return sikoERR_PARAMETER;
		value = value * 16 + digit;
	}
	// Datenlänge berechnen
	unsigned char length = (i - 8) / 2;
	if (!length) return sikoERR_PARAMETER;
	// SDOwrite ausführen
	s->can.sdoWrite.enable = 1;
	s->can.sdoWrite.node = j->slave;
	s->can.sdoWrite.index = index;
	s->can.sdoWrite.subindex = subindex;
	s->can.sdoWrite.data0 = (unsigned char) value;
	s->can.sdoWrite.data1 = (unsigned char) (value >> 8);
	s->can.sdoWrite.data2 = (unsigned char) (value >> 16);
	s->can.sdoWrite.data3 = (unsigned char) (value >> 24);
	s->can.sdoWrite.data4 = (unsigned char) (value >> 32);
	s->can.sdoWrite.data5 = (unsigned char) (value >> 40);
	s->can.sdoWrite.data6 = (unsigned char) (value >> 48);
	s->can.sdoWrite.data7 = (unsigned char) (value >> 56);
	s->can.sdoWrite.datalen = length;
	// Ausführen
	CANopenSDOWrite8(&s->can.sdoWrite);
	if (s->can.sdoWrite.status == ERR_FUB_BUSY) return ERR_FUB_BUSY;
	// SDO Kanal freigeben
	s->can.sdoWrite.enable = 0;
	CANopenSDOWrite8(&s->can.sdoWrite);
	return sikoERR_OK;
}

/* Slave per SDO auslesen */
unsigned long sikoCanSDOread(siko_typ* s, sikoJob_typ* j) {
	// Parameter ist Pointer zu String mit Hex-Zahlen im Format: Index:Subindex e.g. 1802:02
	plcstring* str = j->parameter;
	if (!str || str[4] != ':') return sikoERR_NULL;
	// Index einlesen
	unsigned char i = 0, digit = 0;
	unsigned short index = 0;
	for (; i < 4; ++i) {
		digit = str[i] - '0';
		if (digit > 9) digit = str[i] - 'A' + 10;
		if (digit > 15) digit = str[i] - 'a' + 10;
		if (digit > 15) return sikoERR_PARAMETER;
		index = index * 16 + digit;
	}
	// Subindex einlesen
	digit = 0;
	unsigned char subindex = 0;
	for (++i; i < 7; ++i) {
		digit = str[i] - '0';
		if (digit > 9) digit = str[i] - 'A' + 10;
		if (digit > 15) digit = str[i] - 'a' + 10;
		if (digit > 15) return sikoERR_PARAMETER;
		subindex = subindex * 16 + digit;
	}
	// SDOread ausführen
	s->can.sdoRead.enable = 1;
	s->can.sdoRead.node = j->slave;
	s->can.sdoRead.index = index;
	s->can.sdoRead.subindex = subindex;
	// Ausführen
	CANopenSDORead8(&s->can.sdoRead);
	if (s->can.sdoRead.status == ERR_FUB_BUSY) return ERR_FUB_BUSY;
	// Daten speichern
	s->data0 = s->can.sdoRead.data0;
	s->data1 = s->can.sdoRead.data1;
	s->data2 = s->can.sdoRead.data2;
	s->data3 = s->can.sdoRead.data3;
	s->datalen = s->can.sdoRead.datalen;
	// SDO Kanal freigeben
	s->can.sdoRead.enable = 0;
	CANopenSDORead8(&s->can.sdoRead);
	return sikoERR_OK;
}

/* RPDO senden */
unsigned long sikoCanRPDO(siko_typ* s, sikoJob_typ* j) {
	// Parameter setzen
	s->can.pdoWrite.cobid = 0x502 + j->slave; // RPDO4
	s->can.pdoWrite.datalen = 2; // Control Word ist 16 Bit weit // 1403:01=cobid
	s->can.pdoWrite.data0 = (unsigned char) j->parameter;
	s->can.pdoWrite.data1 = (unsigned char) (j->parameter >> 8);
	// Ausführen
	CANopenPDOWrite8(&s->can.pdoWrite);
	// Status zurückmelden
	return s->can.pdoWrite.status;
}
