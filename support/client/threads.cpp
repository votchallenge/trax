
#include "threads.h"

#ifdef WIN32

int simple_threads_cond_wait(THREAD h, HANDLE m, long milisec) {
	if (milisec < 0)
		return WaitForSingleObject(h, INFINITE);

	return WaitForSingleObject(h, milisec);
}

int WINAPI simple_threads_create_thread(THREAD* h, LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter) {

	HANDLE hn = CreateThread(NULL, 0, lpStartAddress, lpParameter, 0, NULL);

	if (hn == 0)
		return -1;
	*h = hn;

	return 0;
}

void* simple_threads_release_thread(THREAD t) {

	void* result = simple_threads_join_thread(t);

	CloseHandle(t);

	return result;
}

void* simple_threads_join_thread(THREAD t) {

	WaitForSingleObject(t, INFINITE);

	return NULL; // TODO: also get thread exit status

}

#else

int simple_threads_cond_wait(pthread_cond_t * h, pthread_mutex_t *m, long milisec) {
	struct timespec waittime;
	struct timeval curtime;
	int retcode;

	if (milisec < 0) {
		return pthread_cond_wait(h, m);

	}

	/*
	 * We calculate the delay time (representing the desired frame
	 * rate).  This delay time is in *nanoseconds*.
	 * We will wait 0.5 seconds which gives a practical minimum
	 * framerate of 2 which is desired for the motion_loop to
	 * function.
	 */
	gettimeofday(&curtime, NULL);
	curtime.tv_usec += milisec * 1000;

	if (curtime.tv_usec > 1000000) {
		curtime.tv_usec -= 1000000;
		curtime.tv_sec++;
	}

	waittime.tv_sec = curtime.tv_sec;
	waittime.tv_nsec = 1000L * curtime.tv_usec;

	do {
		retcode = pthread_cond_timedwait(h, m, &waittime);
	} while (retcode == EINTR);

	return retcode;
}

int simple_threads_create_thread(THREAD* t, void *(*start_routine)(void*), void *arg) {

	pthread_attr_init(&(t->attr));

	//pthread_attr_setdetachstate(&(t->attr), PTHREAD_CREATE_DETACHED);

	return pthread_create(&(t->thread), &(t->attr), start_routine, arg);
}

void* simple_threads_release_thread(THREAD t) {

	void* result = simple_threads_join_thread(t);

	pthread_attr_destroy(&(t.attr));

	return result;
}

void* simple_threads_join_thread(THREAD t) {

	void* result;

	pthread_join(t.thread, &result);

	return result;

}

#endif

class MutexState {
public:

	MutexState() {

		MUTEX_INIT(mutex);

		owner = -1;

		counter = 0;

		recursive = false;

	}

	~MutexState() {

		MUTEX_DESTROY(mutex);

	}


	THREAD_MUTEX mutex;

	THREAD_IDENTIFIER owner;

	int counter;

	bool recursive;

};


MutexState* Mutex::create() {

	return new MutexState();

}

void Mutex::destroy(MutexState* data) {

	delete data;

}

Mutex::Mutex() : Base() {
	increase_reference_counter();
}

Mutex::Mutex(const Mutex& m) : Base(m) {
	increase_reference_counter();
}

Mutex::~Mutex() {
	decrease_reference_counter();
}

Mutex& Mutex::operator=(Mutex& lhs) throw() {
	swap(lhs);
	return *this;
}

void Mutex::acquire() {

	if (!data->recursive) {
    	MUTEX_LOCK(data->mutex);		
	} else {
		THREAD_IDENTIFIER tid = THREAD_GET_IDENTIFIER();

	    if (tid != data->owner) {
	    	MUTEX_LOCK(data->mutex);
	    	data->owner = tid;
	    	data->counter = 0;
	    }

	    data->counter++;

	}

}

void Mutex::release() {

	if (!data->recursive) {
    	MUTEX_UNLOCK(data->mutex);
	} else {

		THREAD_IDENTIFIER tid = THREAD_GET_IDENTIFIER();

		if (tid == data->owner) {
			data->counter--;

			if (data->counter < 1) {
				data->owner = -1;
	    		MUTEX_UNLOCK(data->mutex);
	    	}
	    }
	}


}

RecursiveMutex::RecursiveMutex() : Mutex() {
	data->recursive = true;
}

RecursiveMutex::RecursiveMutex(const RecursiveMutex& m) : Mutex(m) {

}

RecursiveMutex::~RecursiveMutex() {

}

RecursiveMutex& RecursiveMutex::operator=(RecursiveMutex& lhs) throw() {
	swap(lhs);
	return *this;
}

Lock::Lock(Mutex& mutex) : mutex(mutex), locked(false)
{
	lock();
}

Lock::Lock(Synchronized& mutex) : Lock(mutex.object_mutex) {

}

Lock::~Lock()
{
	unlock();
}


Lock::operator bool () const
{
	return locked;
}

void Lock::lock() {
	if (!locked) {
		mutex.acquire();
		locked = true;
	}
}

void Lock::unlock() {
	if (locked) {
		locked = false;
		mutex.release();

	}
}


Signal::Signal() {

	CONDITION_INIT(condition);
	MUTEX_INIT(mutex);
}

Signal::~Signal() {

	CONDITION_DESTROY(condition);
	MUTEX_DESTROY(mutex);
}

void Signal::wait(long timeout) {
	MUTEX_LOCK(mutex);

	if (timeout > 0) {
		CONDITION_TIMEDWAIT(condition, mutex, timeout);
	} else {
		CONDITION_WAIT(condition, mutex);
	}
	MUTEX_UNLOCK(mutex);
}

void Signal::notify() {
	
	MUTEX_LOCK(mutex);
	CONDITION_SIGNAL(condition);
	MUTEX_UNLOCK(mutex);

}


Synchronized::Synchronized() {

}

Synchronized::~Synchronized() {

}
