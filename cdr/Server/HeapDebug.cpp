/*
 * $Id: HeapDebug.cpp,v 1.4 2002-03-04 21:22:38 bkline Exp $
 *
 * Instrumentation for tracking down dynamic memory leaks.
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.3  2002/03/04 20:52:59  bkline
 * Modified headers to conform with MS advice on heap debugging.
 *
 * Revision 1.2  2001/12/14 18:28:06  bkline
 * Added heapDebugging flag.
 *
 * Revision 1.1  2001/12/14 15:22:20  bkline
 * Initial revision
 *
 */

#include "HeapDebug.h"
#ifndef _DEBUG
#define _DEBUG 1
#endif
#ifndef _CRTDBG_MAP_ALLOC
#define _CRTDBG_MAP_ALLOC
#endif
#include <stdlib.h>
#include <crtdbg.h>
#include <iostream>

static bool heapDebugging = false;
void setHeapDebugging(bool flag) { heapDebugging = flag; }

static _CrtMemState memState1, memState2, memState3;

void memStart() {
    if (heapDebugging) { 
        _CrtMemCheckpoint(&memState1); 
    } 
}

void memReport() {
    if (heapDebugging) {
        static bool first = true;
        if (first) {
            _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
            _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
            first = false;
        }
        _CrtMemCheckpoint(&memState2);
        _CrtMemDifference(&memState3, &memState1, &memState2);
        _CrtMemDumpStatistics(&memState3);
        _CrtMemDumpAllObjectsSince(&memState1);
    }
}

long heapUsed() 
{
    if (heapDebugging) {
        _CrtMemState cms;
        _CrtMemCheckpoint(&cms);
        return (long)cms.lSizes[_NORMAL_BLOCK] +
               (long)cms.lSizes[_CRT_BLOCK]    +
               (long)cms.lSizes[_CLIENT_BLOCK];
    }
    return 0;
}

void showHeapUsed(const char *where)
{
    if (heapDebugging) {
        std::cerr << where << ": " << heapUsed() << std::endl;
    }
}
