/*
 * Instrumentation for tracking down dynamic memory leaks.
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
