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
enum  PROCESS_STATUS_INT { UNKNOWN, PROCESS_STARTED, PROCESS_ENDED };

int getCurrentTest();
void setCurrentTest(int testId);
char *get_currenttestdesc ();
int getCurrentDirn();
void setCurrentDirn(enum Tx_DIRECTION directionarg);
int getCurrentDirn();
char *get_currentdirndesc ();
char *get_otherdirndesc ();
char *get_procstatusdesc(enum PROCESS_STATUS_INT procstatusarg, char *sprocarg);

#endif /* RUNNINGTEST_H_ */
