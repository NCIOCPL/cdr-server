/*
 * $Id: CdrLock.h,v 1.2 2001-12-13 20:24:26 bkline Exp $
 *
 * Class for mutex acquisition to take advantage of automatic release
 * when stack frame exits.
 *
 * Usage example:
 *
 *     HANDLE mutex = CreateMutex(0, false, "CdrActiveConnectionsMutex");
 *     if (mutex != 0) {
 *         // Wait 5 seconds.
 *         cdr::Lock lock(mutex, 5000);
 *         if (lock.m) {
 *             ++activeConnections;
 *         }
 *     }
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.1  2001/11/28 20:04:31  bkline
 * Initial revision
 */

#ifndef _CDR_LOCK_H_
#define _CDR_LOCK_H_

#include <windows.h>

/**@#-*/

namespace cdr {

/**@#+*/

    struct Lock {
        HANDLE m;
        Lock(HANDLE mutex, DWORD usecs) : m(0) {

            // Guard against careless use.
            if (!mutex)
                return;

            // Wait for mutually exclusive access to the resource
            switch (WaitForSingleObject(mutex, usecs)) {
            case WAIT_OBJECT_0:
            case WAIT_ABANDONED:
                m = mutex;
                break;
            case WAIT_FAILED:
            case WAIT_TIMEOUT:
            default:
                // Let the caller decide what to do about these.
                break;
            }
        }
            
        ~Lock() { if (m) ReleaseMutex(m); }
    };
}

#endif
