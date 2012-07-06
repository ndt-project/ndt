/**
 * runningtest.h
 *
 *  Created on: Sep 29, 2011
 *      Author: kkumar
 */

#ifndef RUNNINGTEST_H_
#define RUNNINGTEST_H_

#define PROCESS_STATUS_DESC_SIZE 17 // suffice to hold status defined below
// indicates the status of process like the web100srv or web100clt
//enum  PROCESS_STATUS_INT { UNKNOWN, PROCESS_STARTED, PROCESS_ENDED };
enum PROCESS_STATUS_INT {
	UNKNOWN, PROCESS_STARTED, PROCESS_ENDED
};

enum PROCESS_TYPE_INT {
	PROCESS_TYPE, CONNECT_TYPE
};

int getCurrentTest();
void setCurrentTest(int testId);
char *get_currenttestdesc();
int getCurrentDirn();
void setCurrentDirn(enum Tx_DIRECTION directionarg);
int getCurrentDirn();
char *get_currentdirndesc();
char *get_otherdirndesc();
char *get_procstatusdesc(enum PROCESS_STATUS_INT procstatusarg, char *sprocarg);
char *get_processtypedesc(enum PROCESS_TYPE_INT procid, char *snamearg);

#endif /* RUNNINGTEST_H_ */
