/*
 * $Id: HeapDebug.cpp,v 1.1 2001-12-14 15:22:20 bkline Exp $
 *
 * Instrumentation for tracking down dynamic memory leaks.
 *
 * $Log: not supported by cvs2svn $
 */

#include "HeapDebug.h"
#ifndef _DEBUG
#define _DEBUG 1
#endif
#include <crtdbg.h>
#include <iostream>
#include <malloc.h>

static _CrtMemState memState1, memState2, memState3;
void memStart() { _CrtMemCheckpoint(&memState1); }
void memReport() {
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

long heapUsed() 
{
    _CrtMemState cms;
    _CrtMemCheckpoint(&cms);
    return (long)cms.lSizes[_NORMAL_BLOCK];
}

void showHeapUsed(const char *where)
{
    std::cerr << where << ": " << heapUsed() << std::endl;
}
