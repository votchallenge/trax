
#ifndef SIMPLE_THREADS
#define SIMPLE_THREADS

#if defined(__OS2__) || defined(__WINDOWS__) || defined(WIN32) || defined(WIN64) || defined(_MSC_VER)
#include <windows.h>
#include <process.h>
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

#if defined(__OS2__) || defined(__WINDOWS__) || defined(WIN32) || defined(WIN64) || defined(_MSC_VER)

#define CREATE_THREAD(T, R, P) simple_threads_create_thread(&T, R, P)
#define RELEASE_THREAD(T) simple_threads_release_thread(T)
#define JOIN_THREAD(T) simple_threads_join_thread(T)

#define THREAD_CALLBACK(NAME, ARGUMENT) DWORD WINAPI NAME(void* ARGUMENT)
#define THREAD_IDENTIFIER DWORD
#define THREAD_GET_IDENTIFIER() GetCurrentThreadId()
#define THREAD HANDLE
#define THREAD_MUTEX HANDLE
#define THREAD_COND HANDLE
#define MUTEX_LOCK(M) WaitForSingleObject(M, INFINITE)
#define MUTEX_UNLOCK(M) ReleaseMutex(M)
#define MUTEX_INIT(M) (M = CreateMutex(NULL, FALSE, NULL))
#define MUTEX_DESTROY(M) WaitForSingleObject(M, INFINITE); CloseHandle(M)
#define THREAD_EQUALS(T1, T2) (T1 == T2)
#define THREAD_NONE -1

#define CONDITION_SIGNAL(C) SetEvent(C)
#define CONDITION_DESTROY(C) CloseHandle(C)
#define CONDITION_INIT(C) (C = CreateEvent(NULL, FALSE, FALSE, NULL))
#define CONDITION_TIMEDWAIT(C, M, T) simple_threads_cond_wait(C, M, T)
#define CONDITION_WAIT(C, M) simple_threads_cond_wait(C, M, -1)

int simple_threads_cond_wait(HANDLE h, HANDLE m, long milisec);

int WINAPI simple_threads_create_thread(HANDLE* h, LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter);

void* simple_threads_release_thread(THREAD t);

void* simple_threads_join_thread(THREAD t);

#else

typedef struct pthread_wrapper {
	pthread_t thread;
	pthread_attr_t attr;
} pthread_wrapper;

#define CREATE_THREAD(T, R, P) simple_threads_create_thread(&T, R, P)
#define RELEASE_THREAD(T) simple_threads_release_thread(T)
#define JOIN_THREAD(T) simple_threads_join_thread(T)

#define THREAD_CALLBACK(NAME, ARGUMENT) void* NAME(void* ARGUMENT)
#define THREAD_IDENTIFIER pthread_t
#define THREAD_GET_IDENTIFIER() pthread_self()
#define THREAD pthread_wrapper
#define THREAD_MUTEX pthread_mutex_t
#define THREAD_COND pthread_cond_t
#define SSIZE_T ssize_t
#define MUTEX_LOCK(M) pthread_mutex_lock(&M)
#define MUTEX_UNLOCK(M) pthread_mutex_unlock(&M)
#define MUTEX_INIT(M) pthread_mutex_init(&M, NULL)
#define MUTEX_DESTROY(M) pthread_mutex_destroy(&M)
#define THREAD_EQUALS(T1, T2) (T1 == T2 || (T1 && T2 && pthread_equal(T1, T2)))
#define THREAD_NONE ((pthread_t) 0)

#define CONDITION_SIGNAL(C) pthread_cond_signal(&C)
#define CONDITION_DESTROY(C) pthread_cond_destroy(&C)
#define CONDITION_INIT(C) pthread_cond_init(&C, NULL)
#define CONDITION_TIMEDWAIT(C, M, T) simple_threads_cond_wait(&C, &M, T)
#define CONDITION_WAIT(C, M) simple_threads_cond_wait(&C, &M, -1)


typedef void *LPVOID;

int simple_threads_cond_wait(pthread_cond_t* h, pthread_mutex_t* m, long milisec);

int simple_threads_create_thread(THREAD* t, void *(*start_routine)(void*), void *arg);

void* simple_threads_release_thread(THREAD t);

void* simple_threads_join_thread(THREAD t);

#endif

#ifdef __cplusplus
}

class Synchronized;

class Mutex {
public:

	Mutex();

	virtual ~Mutex();

	void acquire();

	void release();

protected:

	THREAD_MUTEX mutex;

	THREAD_IDENTIFIER owner;

	int counter;

	bool recursive;

private:

	Mutex& operator=(Mutex& p) throw();
    //const Mutex& operator=(const Mutex&);

};

class RecursiveMutex : public Mutex {
public:

	RecursiveMutex();

	RecursiveMutex(const RecursiveMutex& m);

	virtual ~RecursiveMutex();

private:

	RecursiveMutex& operator=(RecursiveMutex& p) throw();
    //const RecursiveMutex& operator=(const RecursiveMutex&);

};

class Lock {
public:

	Lock(Mutex& mutex);

	Lock(Synchronized& mutex);

	~Lock();

	//report the state of locking when used as a boolean
	operator bool () const;

	void lock();

	void unlock();

private:

	Lock& operator=(Lock& p) throw();
	//const Lock& operator=(const Lock&);

	Mutex& mutex;

	bool locked;

};

class Signal {

public:

	Signal();

	~Signal();

	void wait(long timeout = -1);

	void notify();

private:

	THREAD_COND condition;

	THREAD_MUTEX mutex;

};

class Synchronized {
friend class Lock;
public:

	Synchronized();
	virtual ~Synchronized();

protected:

	RecursiveMutex object_mutex;

};

#define OBJECT_SYNCHRONIZE for(Lock M##_lock(object_mutex); M##_lock; M##_lock.unlock())

#define MUTEX_SYNCHRONIZE(mutex) for(Lock M##_lock(mutex); M##_lock; M##_lock.unlock())


#endif

#endif
