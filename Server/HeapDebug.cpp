/*
 * Instrumentation for tracking down dynamic memory leaks.
 */

#include "HeapDebug.h"
#ifndef _DEBUG
#define _DEBUG 1
#endif
#ifdef  _NDEBUG
#undef  _NDEBUG
#endif
#ifndef _CRTDBG_MAP_ALLOC
#define _CRTDBG_MAP_ALLOC
#endif
#include <stdlib.h>
#include <crtdbg.h>
#include <iostream>
#include <windows.h>

static bool heapDebugging = false;
void setHeapDebugging(bool flag) { heapDebugging = flag; }

static _CrtMemState memState1, memState2, memState3;

void memStart() {
    if (heapDebugging) {
        static bool first = true;
        if (first) {
            _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_CHECK_CRT_DF);
            first = false;
        }
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
        //Sleep(3000);
        //std::cerr << where << ": " << heapUsed() << std::endl;
        _CrtMemState cms;
        _CrtMemCheckpoint(&cms);
        std::cout << where << ": " << (long)cms.lSizes[_NORMAL_BLOCK]
                           <<  "+" << (long)cms.lSizes[_CRT_BLOCK]
                           <<  "+" << (long)cms.lSizes[_IGNORE_BLOCK]
                           <<  "+" << (long)cms.lSizes[_CLIENT_BLOCK]
                           <<  "=" << (long)cms.lSizes[_NORMAL_BLOCK] +
                                      (long)cms.lSizes[_CRT_BLOCK]    +
                                      (long)cms.lSizes[_IGNORE_BLOCK] +
                                      (long)cms.lSizes[_CLIENT_BLOCK]
                                   << std::endl;
    }
}

void dumpHeapLeaks()
{
    _CrtDumpMemoryLeaks();
}
