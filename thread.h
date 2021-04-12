/* $Id: thread.h 5946 2006-08-19 10:00:30Z truelight $ */

#ifndef THREAD_H
#define THREAD_H

typedef struct OTTDThread OTTDThread;

typedef void* (*OTTDThreadFunc)(void*);

OTTDThread* OTTDCreateThread(OTTDThreadFunc, void*);
void*       OTTDJoinThread(OTTDThread*);
void        OTTDExitThread(void);

#endif /* THREAD_H */
