/*********************************************************************************
 * Filename:    sikoJob.c
 * Description:	Implementiert eine Warteschlange an Jobs um Zyklusgerecht alle
                Bibliotheksfunktionen zu behandeln.
 * Author:      Niklaus Leuenberger (@NikLeberg)
 * Version:     0.1
 * Date:        2019-07-12
 *********************************************************************************/

#include <bur/plctypes.h>
#include "siko.h"

/* Funktionspointer für Jobs */
typedef unsigned long(*sikoJobFunction)(siko_typ* s, sikoJob_typ* j);

///* Jobs abarbeiten */
//unsigned long sikoJobRun(siko_typ* s) {
//	// pendente Jobs abarbeiten
//	if (!s->job || !s->job->run) return sikoERR_NULL;
//	// Job-Funktion ausführen
//	sikoJob_typ* j = s->job;
//	j->error = ((sikoJobFunction)j->run)(s, j);
//	// Job arbeitet noch
//	if (j->error == ERR_FUB_BUSY) return ERR_FUB_BUSY;
//	// Job fertig (mit oder ohne Fehler), löschen
//	unsigned long error;
//	error = j->error;
//	sikoJobDequeue(s);
//	return error;
//}

/* Jobs abarbeiten */
unsigned long sikoJobRun(siko_typ* s) {
	if (!s) return sikoERR_NULL;
	// solange Jobs abarbeiten bis entweder
	// - Job mittels ERR_FUB_BUSY blockiert
	// - oder Zykluszeit abglaufen ist (10% Sicherheitsfaktor)
	plctime tStart = clock_ms();
	while ((clock_ms() - tStart) < (s->cycleTime * 0.9)) {
		if (!s->job || !s->job->run) return sikoERR_OK;
		// Job ausführen
		sikoJob_typ* j = s->job;
		j->error = ((sikoJobFunction)j->run)(s, j);
		// Job blockiert -> Ende
		if (j->error == ERR_FUB_BUSY) return ERR_FUB_BUSY;
		// Job fertig -> Löschen & nächster Job starten
		sikoJobDequeue(s);
	}
	return sikoERR_OK;
}

/* Speicher für neuen Job bereitstellen */
unsigned long sikoJobAlloc(sikoJob_typ** j) {
	if (TMP_alloc(sizeof(sikoJob_typ), (void**) j)) return sikoERR_MEMORY;
	brsmemset((unsigned long) *j, 0, sizeof(sikoJob_typ));
	return sikoERR_OK;
}

/* Speicher von Job freigeben */
unsigned long sikoJobFree(sikoJob_typ* j) {
	if (TMP_free(sizeof(sikoJob_typ), j)) return sikoERR_MEMORY;
	return sikoERR_OK;
}

/* Neuen Job registrieren */
unsigned long sikoJobEnqueue(siko_typ* s, unsigned char slave, unsigned long jobFunction, unsigned long parameter) {
	// Übergebene Parameter prüfen
	if (!s) return sikoERR_NULL;
	if (!jobFunction) return sikoERR_NULL;
	// neuen Job aufsetzen
	sikoJob_typ* newJob;
	if (sikoJobAlloc(&newJob)) return sikoERR_MEMORY;
	newJob->slave = slave;
	newJob->parameter = parameter;
	newJob->run = jobFunction;
	// Auf leere Warteschlange prüfen
	if (!s->job) s->job = newJob;
	else {
		// Ende der Job-Warteschlange suchen
		sikoJob_typ* lastJob;
		lastJob = s->job;
		while (lastJob->next) lastJob = lastJob->next; // suche bis NULL
		lastJob->next = newJob;
	}
	++s->numJobs;
	return sikoERR_OK;
}

/* Neuen Job vor allen anderen registrieren */
unsigned long sikoJobPrequeue(siko_typ* s, unsigned char slave, unsigned long jobFunction, unsigned long parameter) {
	// Übergebene Parameter prüfen
	if (!s) return sikoERR_NULL;
	if (!jobFunction) return sikoERR_NULL;
	// neuen Job aufsetzen
	sikoJob_typ* newJob;
	if (sikoJobAlloc(&newJob)) return sikoERR_MEMORY;
	newJob->slave = slave;
	newJob->parameter = parameter;
	newJob->run = jobFunction;
	// in Warteschlage auf Position 1 schieben
	newJob->next = s->job;
	s->job = newJob;
	++s->numJobs;
	return sikoERR_OK;
}

/* Job löschen */
unsigned long sikoJobDequeue(siko_typ* s) {
	// Gibt es einen aktuellen Job?
	sikoJob_typ* job;
	job = s->job;
	if (!job) return sikoERR_OK;
	// Job-Pointer umhängen
	s->job = s->job->next;
	// gibt es noch weitere Jobs?
	if (s->job) {
		// Fehler und Parameter weiterleiten (für Prequeue Jobs)
		s->job->prevError = job->error;
		s->job->prevResult = job->parameter;
	}
	// Job löschen
	--s->numJobs;
	return sikoJobFree(job);
}

/* Warteschlange löschen */
unsigned long sikoJobClear(siko_typ* s) {
	while (s->numJobs) sikoJobDequeue(s);
}

/* Wartet eine einstellbare Zeit */
unsigned long sikoJobSleep(siko_typ* s, sikoJob_typ* j) {
	// Warte
	if (clock_ms() < j->parameter) return ERR_FUB_BUSY;
	else return sikoERR_OK;
}