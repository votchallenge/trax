
#ifndef SIMPLE_THREADS
#define SIMPLE_THREADS

#ifdef WIN32
#include <windows.h>
#include <process.h>
#include <BaseTsd.h>
#else
#include <pthread.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#endif

#include <errno.h>

#undef HAVE_STDLIB_H
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef WIN32

#define CREATE_THREAD(T, R, P) simple_threads_create_thread(&T, R, P)
#define RELEASE_THREAD(T) CloseHandle(T)

#define THREAD_CALLBACK(NAME, ARGUMENT) DWORD WINAPI NAME(void* ARG)
#define THREAD HANDLE
#define THREAD_MUTEX HANDLE
#define THREAD_COND HANDLE
#define MUTEX_LOCK(M) WaitForSingleObject(M, INFINITE)
#define MUTEX_UNLOCK(M) ReleaseMutex(M)
#define MUTEX_INIT(M) (M = CreateMutex(NULL, FALSE, NULL))
#define MUTEX_DESTROY(M) WaitForSingleObject(M, INFINITE); CloseHandle(M)

#define CONDITION_SIGNAL(C) SetEvent(C)
#define CONDITION_DESTROY(C) CloseHandle(C)
#define CONDITION_INIT(C) (C = CreateEvent(NULL, FALSE, FALSE, NULL))
#define CONDITION_TIMEDWAIT(C, M, T) simple_threads_cond_wait(C, M, T)
#define CONDITION_WAIT(C, M) simple_threads_cond_wait(C, M, -1)

int simple_threads_cond_wait(HANDLE h, HANDLE m, long milisec);

int WINAPI simple_threads_create_thread(HANDLE* h, LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter);

#else

typedef struct pthread_wrapper {
	pthread_t thread;
	pthread_attr_t attr;
} pthread_wrapper;

#define CREATE_THREAD(T, R, P) simple_threads_create_thread(&T, R, P)
#define RELEASE_THREAD(T) simple_threads_release_thread(T)

#define THREAD_CALLBACK(NAME, ARGUMENT) void* NAME(void* ARG)
#define THREAD pthread_wrapper
#define THREAD_MUTEX pthread_mutex_t
#define THREAD_COND pthread_cond_t
#define SSIZE_T ssize_t
#define MUTEX_LOCK(M) pthread_mutex_lock(&M)
#define MUTEX_UNLOCK(M) pthread_mutex_unlock(&M)
#define MUTEX_INIT(M) pthread_mutex_init(&M, NULL)
#define MUTEX_DESTROY(M) pthread_mutex_destroy(&M)

#define CONDITION_SIGNAL(C) pthread_cond_signal(&C)
#define CONDITION_DESTROY(C) pthread_cond_destroy(&C)
#define CONDITION_INIT(C) pthread_cond_init(&C, NULL)
#define CONDITION_TIMEDWAIT(C, M, T) simple_threads_cond_wait(&C, &M, T)
#define CONDITION_WAIT(C, M) simple_threads_cond_wait(&C, &M, -1)


typedef void *LPVOID;

int simple_threads_cond_wait(pthread_cond_t* h, pthread_mutex_t* m, long milisec);

int simple_threads_create_thread(THREAD* t, void *(*start_routine)(void*), void *arg);

int simple_threads_release_thread(THREAD t);

#endif

#ifdef __cplusplus
}
#endif

#endif
