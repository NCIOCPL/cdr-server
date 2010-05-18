/*
 * $Id$
 *
 * Instrumentation for tracking down dynamic memory leaks.
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.2  2002/03/07 12:57:17  bkline
 * Added dumpHeapLeaks() and more conditional macros.
 *
 * Revision 1.1  2001/12/14 18:29:52  bkline
 * Initial revision
 *
 */

#ifndef _HEAPDEBUG_H_
#define _HEAPDEBUG_H_

#ifdef HEAPDEBUG
#define MEM_START() memStart()
#define MEM_REPORT() memReport()
#define SHOW_HEAP_USED(msg) showHeapUsed(msg)
#define SET_HEAP_DEBUGGING(f) setHeapDebugging(f)
#define DUMP_HEAP_LEAKS() dumpHeapLeaks()
#else
#define MEM_START()
#define MEM_REPORT()
#define SHOW_HEAP_USED(msg)
#define SET_HEAP_DEBUGGING(f)
#define DUMP_HEAP_LEAKS()
#endif

void memStart();
void memReport();
void showHeapUsed(const char *);
void dumpHeapLeaks();
void setHeapDebugging(bool);
long heapUsed();

extern "C" {
    void MarkHeap(void);
    void DumpMarkedHeap(void);
}

#endif
