#ifndef AUDIT002_THREADS_H
#define AUDIT002_THREADS_H

/* MinGW's C library omits C11 threads.h; SH4ZAM's software matrix backend
 * needs only the thread_local spelling for this single-threaded host probe. */
#define thread_local _Thread_local

#endif
