
FUNCTION sikoConfig : UDINT (*Konfiguriere SIKO System*)
	VAR_INPUT
		s : siko_typ;
		interface : STRING[80];
	END_VAR
END_FUNCTION

FUNCTION sikoAG06stateGet : USINT
	VAR_INPUT
		statusWord : USINT;
	END_VAR
END_FUNCTION

FUNCTION sikoSlavePositionJob : UDINT (*Im Slave mittels SDO Übertragungsart auf Sync konfigurieren*)
	VAR_INPUT
		s : siko_typ;
		j : sikoJob_typ;
	END_VAR
END_FUNCTION

FUNCTION sikoSlavePosition : UDINT (*Im Slave mittels SDO Übertragungsart auf Sync konfigurieren*)
	VAR_INPUT
		s : siko_typ;
		slave : USINT;
		position : DINT;
	END_VAR
END_FUNCTION

FUNCTION sikoSlaveUpdate : UDINT (*Im Slave mittels SDO Übertragungsart auf Sync konfigurieren*)
	VAR_INPUT
		s : siko_typ;
		slave : USINT;
	END_VAR
END_FUNCTION

FUNCTION sikoSlaveControl : UDINT (*Slave Control Word setzen*)
	VAR_INPUT
		s : siko_typ;
		j : sikoJob_typ;
	END_VAR
END_FUNCTION

FUNCTION sikoSlaveInitJob : UDINT
	VAR_INPUT
		s : siko_typ;
		j : sikoJob_typ;
	END_VAR
END_FUNCTION

FUNCTION sikoSlaveInit : UDINT (*Initialisiere Slave*)
	VAR_INPUT
		s : siko_typ;
		slave : USINT;
	END_VAR
END_FUNCTION

FUNCTION sikoSlaveConfig : UDINT (*Slave konfiguration speichern*)
	VAR_INPUT
		s : siko_typ;
		address : USINT;
		slave : sikoSlave_typ;
	END_VAR
END_FUNCTION

FUNCTION sikoCanRPDO : UDINT (*CANopen-RPDO*)
	VAR_INPUT
		s : siko_typ;
		j : sikoJob_typ;
	END_VAR
END_FUNCTION

FUNCTION sikoCanFormatSDO : UDINT
	VAR_INPUT
		sdo : sikoSDO_typ;
		str : STRING[80];
	END_VAR
END_FUNCTION

FUNCTION sikoCanSDOread : UDINT (*Slave per SDO parametrieren*)
	VAR_INPUT
		s : siko_typ;
		j : sikoJob_typ;
	END_VAR
END_FUNCTION

FUNCTION sikoCanSDOwrite : UDINT (*Slave per SDO parametrieren*)
	VAR_INPUT
		s : siko_typ;
		j : sikoJob_typ;
	END_VAR
END_FUNCTION

FUNCTION sikoAG06stateSet : UDINT
	VAR_INPUT
		s : siko_typ;
		j : sikoJob_typ;
	END_VAR
END_FUNCTION

FUNCTION sikoCanRegister : UDINT (*Cob-ID eines Slaves im Master registrieren*)
	VAR_INPUT
		s : siko_typ;
		j : sikoJob_typ;
	END_VAR
END_FUNCTION

FUNCTION sikoCanNMT : UDINT (*CANopen-NMT*)
	VAR_INPUT
		s : siko_typ;
		j : sikoJob_typ;
	END_VAR
END_FUNCTION

FUNCTION sikoCanInit : UDINT (*Initialisiere CAN-Objekte*)
	VAR_INPUT
		s : siko_typ;
	END_VAR
END_FUNCTION

FUNCTION sikoCanConfig : UDINT (*Konfiguriere CAN-Parameter*)
	VAR_INPUT
		s : siko_typ;
		interface : STRING[80];
	END_VAR
END_FUNCTION

FUNCTION sikoJobSleep : UDINT (*Wartet eine einstellbare Zeit*)
	VAR_INPUT
		s : siko_typ;
		j : sikoJob_typ;
	END_VAR
END_FUNCTION

FUNCTION sikoJobDequeue : UDINT (*Job löschen*)
	VAR_INPUT
		s : siko_typ;
	END_VAR
END_FUNCTION

FUNCTION sikoJobPrequeue : UDINT (*Neuen Job vor allen anderen registrieren*)
	VAR_INPUT
		s : siko_typ;
		slave : USINT;
		jobFunction : UDINT;
		parameter : UDINT;
	END_VAR
END_FUNCTION

FUNCTION sikoJobEnqueue : UDINT (*Neuen Job registrieren*)
	VAR_INPUT
		s : siko_typ;
		slave : USINT;
		jobFunction : UDINT;
		parameter : UDINT;
	END_VAR
END_FUNCTION

FUNCTION sikoJobFree : UDINT (*Speicher von Job freigeben*)
	VAR_INPUT
		j : sikoJob_typ;
	END_VAR
END_FUNCTION

FUNCTION sikoJobAlloc : UDINT (*Speicher für neuen Job bereitstellen*)
	VAR_INPUT
		j : REFERENCE TO sikoJob_typ;
	END_VAR
END_FUNCTION

FUNCTION sikoJobRun : UDINT (*Jobs abarbeiten*)
	VAR_INPUT
		s : siko_typ;
	END_VAR
END_FUNCTION

FUNCTION sikoInit : UDINT (*Initialisiere SIKO System*)
	VAR_INPUT
		s : siko_typ;
	END_VAR
END_FUNCTION

FUNCTION sikoRun : UDINT (*Bearbeite SIKO System*)
	VAR_INPUT
		s : siko_typ;
	END_VAR
END_FUNCTION

FUNCTION sikoCanBootupAwait : UDINT (*CANopen-Bootup*)
	VAR_INPUT
		s : siko_typ;
		j : sikoJob_typ;
	END_VAR
END_FUNCTION

FUNCTION sikoCanBootup : UDINT (*CANopen-Bootup*)
	VAR_INPUT
		s : siko_typ;
		j : sikoJob_typ;
	END_VAR
END_FUNCTION

FUNCTION sikoAG06keyLock : UDINT
	VAR_INPUT
		s : siko_typ;
		j : sikoJob_typ;
	END_VAR
END_FUNCTION
