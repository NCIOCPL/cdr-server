/*
 * $Id: HeapDebug.h,v 1.1 2001-12-14 18:29:52 bkline Exp $
 *
 * Instrumentation for tracking down dynamic memory leaks.
 *
 * $Log: not supported by cvs2svn $
 */

#ifndef _HEAPDEBUG_H_
#define _HEAPDEBUG_H_

#ifdef HEAPDEBUG
#define SHOW_HEAP_USED(msg) showHeapUsed(msg)
#define SET_HEAP_DEBUGGING(f) setHeapDebugging(f)
#else
#define SHOW_HEAP_USED(msg)
#define SET_HEAP_DEBUGGING(f)
#endif

void memStart();
void memReport();
void showHeapUsed(const char *);
void setHeapDebugging(bool);
long heapUsed();

extern "C" {
    void MarkHeap(void);
    void DumpMarkedHeap(void);
}

#endif
