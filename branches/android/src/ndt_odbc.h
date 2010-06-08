/*
 * This file contains function declarations to handle database support (ODBC).
 *
 * Jakub S³awiñski 2007-07-23
 * jeremian@poczta.fm
 */

#ifndef _JS_NDT_ODBC_H
#define _JS_NDT_ODBC_H

int initialize_db(int options, char* dsn, char* uin, char* pwd);
int db_insert(char spds[4][256], float runave[], char* cputimelog, char* snaplog, char* c2s_snaplog,
        char* hostName, int testPort,
        char* date, char* rmt_host, int s2c2spd, int s2cspd, int c2sspd, int Timeouts,
        int SumRTT, int CountRTT, int PktsRetrans, int FastRetran, int DataPktsOut,
        int AckPktsOut, int CurrentMSS, int DupAcksIn, int AckPktsIn, int MaxRwinRcvd,
        int Sndbuf, int MaxCwnd, int SndLimTimeRwin, int SndLimTimeCwnd, int SndLimTimeSender,
        int DataBytesOut, int SndLimTransRwin, int SndLimTransCwnd, int SndLimTransSender,
        int MaxSsthresh, int CurrentRTO, int CurrentRwinRcvd, int link, int mismatch,
        int bad_cable, int half_duplex, int congestion, int c2sdata, int c2sack, int s2cdata,
        int s2cack, int CongestionSignals, int PktsOut, int MinRTT, int RcvWinScale,
        int autotune, int CongAvoid, int CongestionOverCount, int MaxRTT, int OtherReductions,
        int CurTimeoutCount, int AbruptTimeouts, int SendStall, int SlowStart,
        int SubsequentTimeouts, int ThruBytesAcked, int minPeaks, int maxPeaks, int peaks);

#endif
