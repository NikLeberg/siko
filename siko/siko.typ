
TYPE
	sikoCan_typ : 	STRUCT  (*CANopen Einstellungen und Kommunikationsobjekte*)
		sync : CANopenEnableCyclicSync;
		syncOnce : CANopenSendSync;
		nmt : CANopenNMT;
		bootup : CANopenRecvBootupDev;
		pdoWrite : CANopenPDOWrite8;
		pdoRead : CANopenPDORead8;
		sdoWrite : CANopenSDOWrite8;
		sdoRead : CANopenSDORead8;
		pdoRegister : CANopenRegisterCOBID;
		interface : STRING[12]; (*CANopen-Schnittstelle e.g. IF7*)
	END_STRUCT;
	sikoSlaveParameter_typ : 	STRUCT  (*Linked-List von Slave Parametern als ServiceDataObject*)
		parameter : STRING[80]; (*Parameter formatiert als String e.g.: 0F:01:00:00*)
		status : UDINT; (*Parameterstatus (0 == Parameter ohne Fehler parametriert) *)
		next : REFERENCE TO sikoSlaveParameter_typ; (*nächster Parameter*)
	END_STRUCT;
	sikoSDO_typ : 	STRUCT  (*Sktruktur einer SDO Parameters*)
		index : UINT; (*Hauptindex des Objektes*)
		subindex : USINT; (*Subindex*)
		value : UDINT; (*Wert*)
		length : USINT; (*Länge der zu schreibenden Daten*)
	END_STRUCT;
	sikoSlave_typ : 	STRUCT 
		statusStateMachine : USINT;
		statusSiko : UINT; (*Status des Slaves gemäss Statemachine*)
		statusInternal : USINT; (*Interner Bibliotheksstatus*)
		statusCan : USINT; (*CANopen Status*)
		machine : USINT; (*Maschinenzugehörigkeit*)
		position : USINT; (*Positionsnummer innerhalb der Maschine*)
		type : STRING[5]; (*Slavetyp (aktuell unterstützt: ag05, ap05)*)
		hysteresis : DINT; (*Schleifenlänge der Mechanik*)
		min : DINT; (*Minimalwert*)
		max : DINT; (*Maximalwert*)
		actual : INT; (*Istwert*)
		setpoint : INT; (*Sollwert*)
		timestamp : TIME; (*Zeitpunkt der letzten Status-Aktualisierung*)
		canPDO : {REDUND_UNREPLICABLE} CANopenPDORead8; (*Slaveeigenes CAN Kommunikationsobjekt*)
		canNMT : {REDUND_UNREPLICABLE} CANopenNMT; (*Slaveeigenes CAN Managementobjekt*)
		parameters : REFERENCE TO sikoSlaveParameter_typ; (*Funktionspointer zur Slaveinitialisierung*)
	END_STRUCT;
	sikoJob_typ : 	STRUCT 
		slave : USINT;
		step : USINT;
		error : UDINT;
		prevError : UDINT; (*Fehler des vorhergehenden Jobs*)
		parameter : UDINT; (*optionaler Parameter für run-Funktion*)
		prevResult : UDINT; (*Ergebnis der vorhergehenden Jobs*)
		run : UDINT;
		next : REFERENCE TO sikoJob_typ; (*Pointer zum nächsten Auftrag*)
	END_STRUCT;
	siko_typ : 	STRUCT  (*SIKO-Systemobjekt*)
		data0 : USINT;
		data1 : USINT;
		data2 : USINT;
		data3 : USINT;
		datalen : USINT;
		lastUpdate : TIME; (*Zeitpunkt der letzten Instposition-Aktualisierung*)
		slaves : ARRAY[0..127]OF sikoSlave_typ; (*Abbildung aller Slaves*)
		job : REFERENCE TO sikoJob_typ; (*aktueller CANopen-Auftrag*)
		cycleTime : UINT; (*Zykluszeit des Tasks*)
		numJobs : UINT; (*Anzahl offener Jobs*)
		can : sikoCan_typ; (*CANopen Objekte*)
	END_STRUCT;
	sikoSLAVE_STATUS : 
		(
		sikoSLAVE_OK := 0,
		sikoSLAVE_INIT_RESET := 1,
		sikoSLAVE_INIT_BOOTUP := 2,
		sikoSLAVE_INIT_CONFIG_BASIC := 3,
		sikoSLAVE_INIT_ENABLE_SYNC := 4,
		sikoSLAVE_INIT_REGISTER_COBID := 5,
		sikoSLAVE_INIT_OPERATIONAL := 6,
		sikoSLAVE_INIT_WAIT_STATUS := 7,
		sikoSLAVE_INIT_SHUTDOWN := 8,
		sikoSLAVE_INIT_SWITCH_ON := 9,
		sikoSLAVE_INIT_DONE := 10,
		sikoSLAVE_NO_CONFIG := 11,
		sikoSLAVE_CONFIG_LOADED := 12,
		sikoSLAVE_MISSING := 13,
		sikoSLAVE_FAULT := 14
		);
	sikoCONTROL_WORD : 
		(
		sikoCW_SWITCH_ON := 2#0000000000000001,
		sikoCW_DISABLE_VOLTAGE := 2#0000000000000010,
		sikoCW_QUICK_STOP := 2#0000000000000100,
		sikoCW_ENABLE_OPERATION := 2#0000000000001000,
		sikoCW_NEW_SETPOINT := 2#0000000000010000,
		sikoCW_FAULT_RESET := 2#0000000010000000,
		sikoCW_HALT := 2#0000000100000000,
		sikoCW_KEY_LOCK := 2#0000100000000000,
		sikoCW_TIPP1 := 2#0010000000000000,
		sikoCW_TIPP2_POSITIVE := 2#0100000000000000,
		sikoCW_TIPP2_NEGATIVE := 2#1000000000000000
		);
	sikoAG06_COMMANDS : 
		( (*Abb. 11, S. 58 im CANopen Handbuch*)
		sikoAG06_SHUTDOWN := 2#0000000100000110,
		sikoAG06_SWITCH_ON := 2#0000000100000111,
		sikoAG06_ENABLE_OPERATION := 2#0000000100001111,
		sikoAG06_QUICK_STOP := 2#0000000000000010,
		sikoAG06_DISABLE_VOLTAGE := 2#0000000000000000,
		sikoAG06_DISABLE_OPERATION := 2#0000000100000111,
		sikoAG06_FAULT_RESET := 2#0000000010000000,
		sikoAG06_ABORT := 2#0000000000000111,
		sikoAG06_HALT := 2#0000000100001111,
		sikoAG06_CONTINUE := 2#0000000000001111,
		sikoAG06_TIPP_OFF := 2#0000000000001111,
		sikoAG06_TIPP_1 := 2#0010000000001111,
		sikoAG06_TIPP_2_POSITIVE := 2#0100000000001111,
		sikoAG06_GO_TO := 2#0000000000011111,
		sikoAG06_TIPP_2_NEGATIVE := 2#1000000000001111
		);
	sikoERROR : 
		( (*Anzupassen, User Error Bereich zwischen 50000 - 59999*)
		sikoERR_OK := 0,
		sikoERR_INIT_CAN := 1,
		sikoERR_INIT_SLAVES := 2,
		sikoERR_SLAVE_CONFIG_VALUE := 3,
		sikoERR_SLAVE_CONFIG_TYPE := 4,
		sikoERR_CAN := 5,
		sikoERR_TIMEOUT := 6,
		sikoERR_MEMORY := 7,
		sikoERR_NULL := 8,
		sikoERR_STATE := 9,
		sikoERR_PARAMETER := 10,
		sikoERR_PREV := 10
		);
END_TYPE
