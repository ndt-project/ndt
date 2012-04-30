/*
 * This file contains the definitions and function declarations needed
 * to handle the Admin page. This page allows a remote user to view
 * the usage statistics via a web page.
 *
 * Jakub S³awiñski 2006-07-14
 * jeremian@poczta.fm
 */

#ifndef _JS_WEB100_ADMIN_H
#define _JS_WEB100_ADMIN_H

#define ADMINFILE "admin.html" 

void view_init(int refresh);
int calculate(char now[32], int SumRTT, int CountRTT, int CongestionSignals, int PktsOut,
    int DupAcksIn, int AckPktsIn, int CurrentMSS, int SndLimTimeRwin,
    int SndLimTimeCwnd, int SndLimTimeSender, int MaxRwinRcvd, int CurrentCwnd,
    int Sndbuf, int DataBytesOut, int mismatch, int bad_cable, int c2sspd, int s2cspd,
    int c2sdata, int s2cack, int view_flag);
void gen_html(int c2sspd, int s2cspd, int MinRTT, int PktsRetrans, int Timeouts, int Sndbuf,
    int MaxRwinRcvd, int CurrentCwnd, int mismatch, int bad_cable, int totalcnt, int refresh);

#endif
