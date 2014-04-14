
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
	
	pthread_attr_setdetachstate(&(t->attr), PTHREAD_CREATE_DETACHED);
	
	return pthread_create(&(t->thread), &(t->attr), start_routine, arg);
}

int simple_threads_release_thread(THREAD t) {
	
	pthread_join(t.thread, NULL);

	pthread_attr_destroy(&(t.attr));

	return 1;
}

#endif
