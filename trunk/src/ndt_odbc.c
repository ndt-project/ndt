/*
 * This file contains functions needed to handle database support (ODBC).
 *
 * Jakub S³awiñski 2007-07-23
 * jeremian@poczta.fm
 */

#include "../config.h"

#include <stdio.h>
#include <string.h>
#if defined(HAVE_ODBC) && defined(DATABASE_ENABLED) && defined(HAVE_SQL_H)
#include <sql.h>
#include <sqlext.h>
#endif

#include "ndt_odbc.h"
#include "logging.h"

#if defined(HAVE_ODBC) && defined(DATABASE_ENABLED) && defined(HAVE_SQL_H)
SQLHENV env;
SQLHDBC dbc;
SQLHSTMT stmt = NULL;
char* ctStmt_1 = "CREATE TABLE ndt_test_results ("
                          "spds1 TEXT,"
                          "spds2 TEXT,"
                          "spds3 TEXT,"
                          "spds4 TEXT,"
                          "runave1 FLOAT,"
                          "runave2 FLOAT,"
                          "runave3 FLOAT,"
                          "runave4 FLOAT,"
                          "cputimelog TEXT,"
                          "snaplog TEXT,"
                          "c2s_snaplog TEXT,"
                          "hostName TEXT,"
                          "testPort INT,"
                          "date TEXT,"
                          "rmt_host TEXT,";
char* ctStmt_2 = "s2c2spd INT,"
                          "s2cspd INT,"
                          "c2sspd INT,"
                          "Timeouts INT,"
                          "SumRTT INT,"
                          "CountRTT INT,"
                          "PktsRetrans INT,"
                          "FastRetran INT,"
                          "DataPktsOut INT,"
                          "AckPktsOut INT,"
                          "CurrentMSS INT,"
                          "DupAcksIn INT,"
                          "AckPktsIn INT,"
                          "MaxRwinRcvd INT,"
                          "Sndbuf INT,"
                          "MaxCwnd INT,";
char* ctStmt_3 = "SndLimTimeRwin INT,"
                          "SndLimTimeCwnd INT,"
                          "SndLimTimeSender INT,"
                          "DataBytesOut INT,"
                          "SndLimTransRwin INT,"
                          "SndLimTransCwnd INT,"
                          "SndLimTransSender INT,"
                          "MaxSsthresh INT,"
                          "CurrentRTO INT,"
                          "CurrentRwinRcvd INT,"
                          "link INT,"
                          "mismatch INT,"
                          "bad_cable INT,"
                          "half_duplex INT,"
                          "congestion INT,"
                          "c2sdata INT,";
char* ctStmt_4 = "c2sack INT,"
                          "s2cdata INT,"
                          "s2cack INT,"
                          "CongestionSignals INT,"
                          "PktsOut INT,"
                          "MinRTT INT,"
                          "RcvWinScale INT,"
                          "autotune INT,"
                          "CongAvoid INT,"
                          "CongestionOverCount INT,"
                          "MaxRTT INT,"
                          "OtherReductions INT,"
                          "CurTimeoutCount INT,"
                          "AbruptTimeouts INT,"
                          "SendStall INT,"
                          "SlowStart INT,"
                          "SubsequentTimeouts INT,"
                          "ThruBytesAcked INT,"
                          "minPeak INT,"
                          "maxPeak INT,"
                          "peaks INT"
                          ");";
char createTableStmt[2048];

static void
extract_error(char *fn, SQLHANDLE handle, SQLSMALLINT type)
{
    SQLINTEGER   i = 0;
    SQLINTEGER   native;
    SQLCHAR  state[ 7 ];
    SQLCHAR  text[256];
    SQLSMALLINT  len;
    SQLRETURN    ret;

    log_println(2, "\nThe driver reported the following diagnostics whilst running %s:\n", fn);
    do
    {
        ret = SQLGetDiagRec(type, handle, ++i, state, &native, text,
                            sizeof(text), &len );
        if (SQL_SUCCEEDED(ret))
            log_println(2, "%s:%ld:%ld:%s", state, i, native, text);
    }
    while( ret == SQL_SUCCESS );
}
#endif

int
initialize_db(int options, char* dsn, char* uid, char* pwd)
{
#if defined(HAVE_ODBC) && defined(DATABASE_ENABLED) && defined(HAVE_SQL_H)
    if (options) {
        SQLRETURN ret; /* ODBC API return status */
        SQLSMALLINT columns; /* number of columns in result-set */
        SQLCHAR outstr[1024];
        SQLSMALLINT outstrlen;
        char loginstring[1024];

        log_println(1, "Initializing DB with DSN='%s', UID='%s', PWD=%s", dsn, uid, pwd ? "yes" : "no");
        sprintf(createTableStmt, "%s%s%s%s", ctStmt_1, ctStmt_2, ctStmt_3, ctStmt_4);

        /* Allocate an environment handle */
        SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env);
        /* We want ODBC 3 support */
        SQLSetEnvAttr(env, SQL_ATTR_ODBC_VERSION, (void *) SQL_OV_ODBC3, 0);
        /* Allocate a connection handle */
        SQLAllocHandle(SQL_HANDLE_DBC, env, &dbc);
        /* Connect to the DSN */
        memset(loginstring, 0, 1024);
        snprintf(loginstring, 256, "DSN=%s;", dsn);
        if (uid) {
            strcat(loginstring, "UID=");
            strncat(loginstring, uid, 256);
            strcat(loginstring, ";");
        }
        if (pwd) {
            strcat(loginstring, "PWD=");
            strncat(loginstring, pwd, 256);
        }
        ret = SQLDriverConnect(dbc, NULL, (unsigned char*) loginstring, SQL_NTS,
                outstr, sizeof(outstr), &outstrlen, SQL_DRIVER_NOPROMPT);
        if (SQL_SUCCEEDED(ret)) {
            log_println(2, "  Connected");
            log_println(2, "  Returned connection string was:\n\t%s", outstr);
            if (ret == SQL_SUCCESS_WITH_INFO) {
                log_println(2, "Driver reported the following diagnostics");
                extract_error("SQLDriverConnect", dbc, SQL_HANDLE_DBC);
            }
        } else {
            log_println(0, "  Failed to connect to the DSN\n Continuing without DB logging");
            extract_error("SQLDriverConnect", dbc, SQL_HANDLE_DBC);
            return 1;
        }
        /* Allocate a statement handle */
        ret = SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);
        if (!SQL_SUCCEEDED(ret)) {
            log_println(0, "  Failed to alloc statement handle\n Continuing without DB logging");
            extract_error("SQLAllocHandle", dbc, SQL_HANDLE_DBC);
            return 1;
        }
        /* Retrieve a list of tables */
        ret = SQLTables(stmt, NULL, 0, NULL, 0, NULL, 0, (unsigned char*) "TABLE", SQL_NTS);
        if (!SQL_SUCCEEDED(ret)) {
            log_println(0, "  Failed to fetch table info\n Continuing without DB logging");
            extract_error("SQLTables", dbc, SQL_HANDLE_DBC);
            return 1;
        }
        /* How many columns are there */
        SQLNumResultCols(stmt, &columns);

        /* Loop through the rows in the result-set */
        while (SQL_SUCCEEDED(ret = SQLFetch(stmt))) {
            SQLUSMALLINT i;
            /* Loop through the columns */
            for (i = 2; i <= columns; i++) {
                SQLINTEGER indicator;
                char buf[512];
                /* retrieve column data as a string */
                ret = SQLGetData(stmt, i, SQL_C_CHAR,
                        buf, sizeof(buf), &indicator);
                if (SQL_SUCCEEDED(ret)) {
                    /* Handle null columns */
                    if (indicator == SQL_NULL_DATA) strcpy(buf, "NULL");
                    if (strcmp(buf, "ndt_test_results") == 0) {
                        /* the table exists - do nothing */
                        SQLFreeStmt(stmt, SQL_CLOSE);
                        return 0;
                    }
                }
            }
        }
        /* the table doesn't exist - create the one */
        SQLFreeStmt(stmt, SQL_CLOSE);
        log_print(1, "The table 'ndt_test_results' doesn't exist, creating...");
        ret = SQLExecDirect(stmt, (unsigned char*) createTableStmt, strlen(createTableStmt));
        if (!SQL_SUCCEEDED(ret)) {
            log_println(0, "  Failed to create table\n Continuing without DB logging");
            extract_error("SQLExecDirect", dbc, SQL_HANDLE_DBC);
            stmt = NULL;
            return 1;
        }
        log_println(1, " SUCCESS!");
    }
    return 0;
#else
    return 1;
#endif
}

int
db_insert(char spds[4][256], float runave[], char* cputimelog, char* snaplog, char* c2s_snaplog,
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
        int SubsequentTimeouts, int ThruBytesAcked, int minPeak, int maxPeak, int peaks)
{
#if defined(HAVE_ODBC) && defined(DATABASE_ENABLED) && defined(HAVE_SQL_H)
    SQLRETURN    ret;
    char insertStmt[2048];
    if (!stmt) {
        return 1;
    }
    snprintf(insertStmt, 2040, "INSERT INTO ndt_test_results VALUES ("
            "'%s','%s','%s','%s',%f,%f,%f,%f,"
            "'%s','%s','%s','%s',%d,"
            "'%s','%s',%d,%d,%d,%d,"
            "%d,%d,%d,%d,%d,"
            "%d,%d,%d,%d,%d,"
            "%d,%d,%d,%d,%d,"
            "%d,%d,%d,%d,"
            "%d,%d,%d,%d,%d,"
            "%d,%d,%d,%d,%d,%d,"
            "%d,%d,%d,%d,%d,"
            "%d,%d,%d,%d,%d,"
            "%d,%d,%d,%d,"
            "%d,%d,%d,%d,%d"
            ");",
            spds[0], spds[1], spds[2], spds[3], runave[0], runave[1], runave[2], runave[3],
            cputimelog, snaplog, c2s_snaplog, hostName, testPort,
            date, rmt_host, s2c2spd, s2cspd, c2sspd, Timeouts,
            SumRTT, CountRTT, PktsRetrans, FastRetran, DataPktsOut,
            AckPktsOut, CurrentMSS, DupAcksIn, AckPktsIn, MaxRwinRcvd,
            Sndbuf, MaxCwnd, SndLimTimeRwin, SndLimTimeCwnd, SndLimTimeSender,
            DataBytesOut, SndLimTransRwin, SndLimTransCwnd, SndLimTransSender,
            MaxSsthresh, CurrentRTO, CurrentRwinRcvd, link, mismatch,
            bad_cable, half_duplex, congestion, c2sdata, c2sack, s2cdata,
            s2cack, CongestionSignals, PktsOut, MinRTT, RcvWinScale,
            autotune, CongAvoid, CongestionOverCount, MaxRTT, OtherReductions,
            CurTimeoutCount, AbruptTimeouts, SendStall, SlowStart,
            SubsequentTimeouts, ThruBytesAcked, minPeak, maxPeak, peaks
            );
        ret = SQLExecDirect(stmt, (unsigned char*) insertStmt, strlen(insertStmt));
        if (!SQL_SUCCEEDED(ret)) {
            log_println(0, "  Failed to insert test results into the table\n Continuing without DB logging");
            extract_error("SQLExecDirect", dbc, SQL_HANDLE_DBC);
            stmt = NULL;
            return 1;
        }

    return 0;
#else
    return 1;
#endif
}
