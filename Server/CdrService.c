/*
 *  MODULE:   CdrService.c
 *
 *  PURPOSE:  Implements the body of the service.
 *            Spawns the servers.  Shuts them down
 *            when notified by the service manager.
 *
 *  FUNCTIONS:
 *            ServiceStart(DWORD dwArgc, LPTSTR *lpszArgv);
 *            ServiceStop( );
 *
 *  COMMENTS: The functions implemented in CdrService.c are
 *            prototyped in service.h
 *
 *
 *  AUTHOR:   Bob Kline - RK Systems
 */


#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <process.h>
#include <tchar.h>
#include <time.h>
#include "service.h"

// this event is signalled when the
// service should end
//
HANDLE  hServerStopEvent = NULL;

#define RECHECK_TIME 60000

#define MAX_SERVERS 32
#define N_SERVERS 1
#define MAX_REGSTR 1024
#define CLEANUP_SECONDS 5
#define SHUTDOWN "d:\\cdr\\bin\\ShutdownCdr.exe"

static char server_pathname[MAX_REGSTR] = "d:\\cdr\\bin\\CdrServer.exe";
static char service_log[MAX_REGSTR] = "d:\\cdr\\log\\CdrService.log";
static char service_account[MAX_REGSTR] = "CdrService";
static char service_password[MAX_REGSTR];

static int nServers = N_SERVERS;
static HANDLE hProcesses[MAX_SERVERS];
static const char *GetCdrServerRegString(const char *name);
static void GetCdrServerRegVariables(void);

#ifndef DEBUG
#define DBGLOG(s,f)
#else
#define DBGLOG(s,f) dbglog(s,f)
static void dbglog(char *s, char *f)
{
    time_t now = time(0);
    FILE *fp = fopen(f, "a");
    char* timeString = ctime(&now);
    char* newline = strchr(timeString, '\n');
    if (newline)
        *newline = 0;
    if (fp) {
        fprintf(fp, "%s *** %s ***\n", timeString, s);
        fclose(fp);
    }
}
#endif

//
//  FUNCTION: ServiceStart
//
//  PURPOSE: Actual code of the service
//           that does the work.
//
//  PARAMETERS:
//    dwArgc   - number of command line arguments
//    lpszArgv - array of command line arguments
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//    Kick off the server processes.  The service
//    stops when hServerStopEvent is signalled
//
VOID ServiceStart (DWORD dwArgc, LPTSTR *lpszArgv)
{
    STARTUPINFO             si;
    BOOL                    bRet;
    char *                  buf;
    int                     i;
    DWORD                   rc;

    DBGLOG("TOP OF SERVICESTART", service_log);

    // Service initialization
    // report the status to the service control manager.
    if (!ReportStatusToSCMgr(SERVICE_START_PENDING, NO_ERROR, 3000))
        return;

    // create the event object. The control handler function signals
    // this event when it receives the "stop" control code.
    hServerStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!hServerStopEvent)
        return;

    DBGLOG("EVENT OBJECT CREATED", service_log);

    // report the status to the service control manager.
    if (!ReportStatusToSCMgr(SERVICE_START_PENDING, NO_ERROR, 3000)) {
        CloseHandle(hServerStopEvent);
        return;
    }

    // create the server processes
    memset(&si, 0, sizeof si);
    si.cb = sizeof si;
    buf = malloc(strlen(server_pathname) + 80);
    sprintf(buf, "%s", server_pathname);

    for (;;)
    {
        for (i = 0; i < nServers; ++i) {

            if (hProcesses[i] != 0) {
                if (!GetExitCodeProcess(hProcesses[i], &rc) ||
                        rc != STILL_ACTIVE) {
                    DBGLOG("SERVER PROCESS EXITTED", service_log);
                    CloseHandle(hProcesses[i]);
                    hProcesses[i] = 0;
                }
            }


            if (hProcesses[i] == 0) {
                PROCESS_INFORMATION pi;
                bRet = CreateProcess(0, buf, 0, 0, FALSE,
                        DETACHED_PROCESS, 0, 0, &si, &pi);
                if (bRet) {
                    DBGLOG("SERVER PROCESS CREATED", service_log);
                    hProcesses[i] = pi.hProcess;
                }
                else {
                    DBGLOG("SERVER PROCESS CREATION FAILURE", service_log);
                    --nServers;
                }

                // report the status to the service control manager.
                if (!ReportStatusToSCMgr(SERVICE_START_PENDING, NO_ERROR, 3000))
                {
                    CloseHandle(hServerStopEvent);
                    ServiceStop();
                    free(buf);
                    return;
                }
            }

            // report the status to the service control manager.
            if (!ReportStatusToSCMgr(SERVICE_RUNNING, NO_ERROR, 3000)) {
                CloseHandle(hServerStopEvent);
                ServiceStop();
                free(buf);
                return;
            }
        }

        //DBGLOG("ENTERING WAITFORSINGLEOBJECT", service_log);
        // Servers are now running, wait for shutdown
        if (WaitForSingleObject(hServerStopEvent, RECHECK_TIME) != WAIT_TIMEOUT)
            break;
        //DBGLOG("BACK FROM WAITFORSINGLEOBJECT", service_log);
    }

    DBGLOG("SHUTTING DOWN", service_log);
    free(buf);
    CloseHandle(hServerStopEvent);
    DBGLOG("HANDLE CLOSED FOR EVENT OBJECT", service_log);
}


//
//  FUNCTION: ServiceStop
//
//  PURPOSE: Stops the service
//
//  PARAMETERS:
//    none
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//    If a ServiceStop procedure is going to
//    take longer than 3 seconds to execute,
//    it should spawn a thread to execute the
//    stop code, and return.  Otherwise, the
//    ServiceControlManager will believe that
//    the service has stopped responding.
//
VOID ServiceStop()
{
    char* key = "cdrservice:";
    size_t keylen = strlen(key);
    char line[MAX_REGSTR];
    FILE* fp = fopen("d:\\etc\\cdrpw", "r");
    DBGLOG("TOP OF SERVICESTOP", service_log);
    if (!fp) {
        DBGLOG("SERVICE STOP FAILURE", service_log);
        return;
    }
    service_password[0] = '\0';
    while (fgets(line, MAX_REGSTR, fp)) {
        if (!memcmp(line, "cdrservice:", keylen)) {
            size_t j = 0;
            size_t i = keylen;
            while (i < MAX_REGSTR && line[i] && line[i] != '\n')
                service_password[j++] = line[i++];
            service_password[j] = '\0';
            break;
        }
    }
    fclose(fp);
    if (!service_password[0]) {
        DBGLOG("SERVICE ACCOUNT NOT FOUND", service_log);
        return;
    }
    _spawnl(_P_NOWAIT, SHUTDOWN, SHUTDOWN, service_account,
            service_password, 0);
    if (!ReportStatusToSCMgr(SERVICE_STOP_PENDING,
                             NO_ERROR,
                             CLEANUP_SECONDS * 1000)) {
        DBGLOG("FAILURE REPORTING STATUS TO SERVICE MONITOR", service_log);
        return;
    }
    Sleep(CLEANUP_SECONDS * 1000);
    DBGLOG("WAKING BACK UP", service_log);
    if (hServerStopEvent) {
        DBGLOG("BEFORE SETEVENT", service_log);
        SetEvent(hServerStopEvent);
        DBGLOG("BACK FROM SETEVENT", service_log);
    }
}

//----------------------------------------------------------------------
// @func Local function to extract all CdrServer settings from the Windows
// NT Registry
//----------------------------------------------------------------------
static void GetCdrServerRegVariables(void)
{
    const char* value;
    if ((value = GetCdrServerRegString("ServerPathname")) != NULL
	    && *value != '\0')
      strcpy(server_pathname, value);
    if ((value = GetCdrServerRegString("ServiceLog")) != NULL
	    && *value != '\0')
      strcpy(service_log, value);
    if ((value = GetCdrServerRegString("ServiceAccount")) != NULL
	    && *value != '\0')
      strcpy(service_account, value);
    if ((value = GetCdrServerRegString("ServicePassword")) != NULL
	    && *value != '\0')
      strcpy(service_password, value);
}

//----------------------------------------------------------------------
// @func Local function to extract CdrServer settings from the Windows
// NT Registry
//----------------------------------------------------------------------
static const char *GetCdrServerRegString(
    const char *name)               // @parm name of value to retrieve
{
    static char buf[MAX_REGSTR] = { 0 };
    HKEY hKey;
    DWORD dwDisposition, len = sizeof buf, type = REG_SZ;
    LONG rc = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                             "SOFTWARE\\CIPS\\CdrServer",
                             0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ,
                             NULL, &hKey, &dwDisposition);
    if (rc == ERROR_SUCCESS)
        rc = RegQueryValueEx(hKey, name, 0, &type,
			     (unsigned char *)buf, &len);

    return (rc == ERROR_SUCCESS) ? buf : NULL;
}
