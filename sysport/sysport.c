#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include "sysport.h"


/**** CLOCK *************************************************************************/

#if defined(USECLOCK)

	#include <sys/time.h>
	extern myclock_t myclock(void) {return (myclock_t)clock();}
	extern myclock_t ticks_per_sec (void) {return CLOCKS_PER_SEC;}

#elif defined(USEWINCLOCK)

	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
	extern myclock_t myclock(void) {return (myclock_t)GetTickCount();}
	extern myclock_t ticks_per_sec (void) {return 1000;}


#elif defined(USELINCLOCK)

	#include <sys/time.h>
	extern myclock_t myclock(void) 
	{
		struct timeval tv;
		gettimeofday(&tv, NULL);
		return (myclock_t)tv.tv_sec * 1000 + (myclock_t)tv.tv_usec/1000;
	}
	extern myclock_t ticks_per_sec (void) {return 1000;}

#else

	#error No Clock specified in compilation

#endif

/**** PATH NAMES *************************************************************************/

#if defined(GCCLINUX)
	extern int isfoldersep (int x) { return x == '/';}
#elif defined(MVSC)
	extern int isfoldersep (int x) { return x == '\\' || x == ':';}
#else
	extern int isfoldersep (int x) { return x == '/' || x == '\\' || x == ':';}
#endif

/**** Maximum Files Open *****************************************************************/

#if defined(GCCLINUX)
	#include <sys/resource.h>
	#if 0	
	struct rlimit {
		rlim_t rlim_cur;  /* Soft limit */
		rlim_t rlim_max;  /* Hard limit (ceiling for rlim_cur) */
	};
	#endif
	extern int mysys_fopen_max (void) 
	{ 
		int ok;
		struct rlimit rl;
		ok = 0 == getrlimit(RLIMIT_NOFILE, &rl);
		if (ok)
			return (int)rl.rlim_cur;
		else
			return FOPEN_MAX;
	}
#elif defined(MVSC)
	extern int mysys_fopen_max (void) { return FOPEN_MAX;}
#else
	extern int mysys_fopen_max (void) { return FOPEN_MAX;}
#endif



#if defined(MULTI_THREADED_INTERFACE)
/**** THREADS ****************************************************************************/

/*
|
|	POSIX
|
\*-------------------------*/
#if defined (POSIX_THREADS)

#include <pthread.h>

extern int /* boolean */
mythread_create (/*@out@*/ mythread_t *thread, routine_t start_routine, void *arg, /*@out@*/ int *ret_error)
{
	// create joinable threads
	pthread_attr_t attr; /* attributes */
	int ret;
	pthread_attr_init(&attr); // for portability
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE); // for portability
	ret = pthread_create (thread, &attr, start_routine, arg);
	pthread_attr_destroy(&attr); 	/* Free attribute */
	*ret_error = ret;
	return 0 == ret;
}

extern int /* boolean */
mythread_join (mythread_t thread)
{
	void *p; /* value return from pthread_exit, not used */
	int ret = pthread_join (thread, &p);
	return 0 == ret;
}

extern void 		
mythread_exit (void)
{
	pthread_exit (NULL);
}


extern const char *
mythread_create_error (int err)
{
	const char *s;
	switch (err) {
		case 0     : s = "Success"; break;
		case EAGAIN: s = "EAGAIN" ; break;
		case EINVAL: s = "EINVAL" ; break; 
		case EPERM : s = "EPERM"  ; break;
		default    : s = "Unknown error"; break;
	}
	return s;
}

extern void mythread_mutex_init	(mythread_mutex_t *m) { pthread_mutex_init   (m,NULL);}
extern void mythread_mutex_destroy	(mythread_mutex_t *m) { pthread_mutex_destroy(m)     ;}
extern void mythread_mutex_lock	(mythread_mutex_t *m) { pthread_mutex_lock   (m)     ;}
extern void mythread_mutex_unlock	(mythread_mutex_t *m) { pthread_mutex_unlock (m)     ;}

#if defined(NSPINLOCKS)
extern void mythread_spinx_init	(mythread_spinx_t *m) { pthread_mutex_init   (m,NULL);} /**/
extern void mythread_spinx_destroy	(mythread_spinx_t *m) { pthread_mutex_destroy(m)     ;} /**/
extern void mythread_spinx_lock	(mythread_spinx_t *m) { pthread_mutex_lock   (m)     ;} /**/
extern void mythread_spinx_unlock	(mythread_spinx_t *m) { pthread_mutex_unlock (m)     ;} /**/

#elif defined(MY_SPINLOCKS)
	// Implemented spinlocks when they are not present in the pthread library
	// Important to port to Mac OS X
	// Idea adopted and modified from
	// https://idea.popcount.org/2012-09-12-reinventing-spinlocks/

//#warning Own implementation of spinlocks

void
mythread_spinx_init (mythread_spinx_t *lock)
{
	__asm__ __volatile__ ("" ::: "memory");
	*lock = 0;
}

void mythread_spinx_destroy (mythread_spinx_t *lock) {*lock = 0;}

void 
mythread_spinx_lock (mythread_spinx_t *lock) 
{
	int acquired;
	unsigned i = 0;
	do {
		if ((++i & 0xffffu) == 0) sched_yield();
		acquired = __sync_bool_compare_and_swap(lock, 0, 1);
	} while (!acquired); 
}

void 
mythread_spinx_unlock (mythread_spinx_t *lock) 
{
	__asm__ __volatile__ ("" ::: "memory");
	*lock = 0;
}


#else
extern void mythread_spinx_init	(mythread_spinx_t *m) { pthread_spin_init   (m,0);} /**/
extern void mythread_spinx_destroy	(mythread_spinx_t *m) { pthread_spin_destroy(m)  ;} /**/
extern void mythread_spinx_lock	(mythread_spinx_t *m) { pthread_spin_lock   (m)  ;} /**/
extern void mythread_spinx_unlock	(mythread_spinx_t *m) { pthread_spin_unlock (m)  ;} /**/
#endif

#if defined(UNNAMED_SEMAPHORES)

/* semaphores unnamed */

void semaphore_system_init(void) {return;}
void semaphore_system_done(void) {return;}

extern int /* boolean */ 
mysem_init	(mysem_t *sem, unsigned int value)
	{ return -1 != sem_init (sem, 0 /*not shared with processes*/, value);}

extern int /* boolean */ 
mysem_wait	(mysem_t *sem)
	{ return  0 == sem_wait (sem);}

extern int /* boolean */ 
mysem_post	(mysem_t *sem)
	{ return  0 == sem_post (sem);}
 
extern int /* boolean */ 
mysem_destroy	(mysem_t *sem)
	{ return  0 == sem_destroy (sem);}

extern int /* boolean */ 
mysem_getvalue	(mysem_t *sem, *pval)
	{ return  0 == sem_getvalue (sem, pval);}


#elif defined(NAMED_SEMAPHORES)

/* semaphores named */

#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */
#include <semaphore.h>

static mythread_spinx_t Semcounterlock;
static unsigned Semaphores_count = 0;

void semaphore_system_init(void) {mythread_spinx_init    (&Semcounterlock); return;}
void semaphore_system_done(void) {mythread_spinx_destroy (&Semcounterlock); return;}

extern int /* boolean */ 
mysem_init	(mysem_t *sem, unsigned int value)
{ 
	unsigned getsemid;
	mythread_spinx_lock	(&Semcounterlock);
	getsemid = Semaphores_count++; 
	mythread_spinx_unlock (&Semcounterlock);
	sprintf(sem->name, "/s%u", getsemid); //set semaphore name 
	sem->psem = sem_open(sem->name, O_CREAT, 0644, value);
	return SEM_FAILED != sem->psem;
}

extern int /* boolean */ 
mysem_wait	(mysem_t *sem) {return  0 == sem_wait (sem->psem);}

extern int /* boolean */ 
mysem_post	(mysem_t *sem) {return  0 == sem_post (sem->psem);}
 
extern int /* boolean */ 
mysem_destroy	(mysem_t *sem) {return 0 == sem_close(sem->psem) && 0 == sem_unlink(sem->name);}


#elif defined(MY_SEMAPHORES)

static void
condvar_init (condvar_t *pcv)
{
	pcv->go = 0;
	pthread_cond_init (&pcv->key, NULL);
	pthread_mutex_init(&pcv->mtx, NULL);
}

static void
condvar_done (condvar_t *pcv)
{
	pcv->go = 0;
	pthread_cond_destroy  (&pcv->key);
	pthread_mutex_destroy (&pcv->mtx);
}

static void
condvar_release (condvar_t *pcv)
{
	pthread_mutex_lock	(&pcv->mtx);
    pcv->go++;
    pthread_cond_signal	(&pcv->key); 
    pthread_mutex_unlock(&pcv->mtx);
}

static void
condvar_wait (condvar_t *pcv)
{
												//printf ("        con before mtx %d\n",th); fflush(stdout);
	pthread_mutex_lock  (&pcv->mtx);
												//printf ("        con inside mtx %d\n",th); fflush(stdout);
	while (pcv->go == 0) {
												//printf ("        con wait       %d\n",th); fflush(stdout);
		pthread_cond_wait(&pcv->key, &pcv->mtx);

	}
												//printf ("        con wait over  %d\n",th); fflush(stdout);
    // Reset the predicate and release the mutex.
	pcv->go--;
    pthread_mutex_unlock(&pcv->mtx);
												//printf ("        con out-of mtx %d\n",th); fflush(stdout);
}


void semaphore_system_init(void) {return;}
void semaphore_system_done(void) {return;}

extern int /* boolean */
mysem_init (mysem_t *psem, unsigned int value)
{
	psem->waitcnt = 0;
	psem->counter = value;
	mythread_spinx_init	(&psem->door);
	condvar_init (&psem->cv);
	return 1;
}

extern int /*boolean*/	
mysem_destroy	(mysem_t *psem)
{
	psem->waitcnt = 0;
	psem->counter = 0;
	mythread_spinx_destroy (&psem->door); 
	condvar_done (&psem->cv);
	return 1;
}

extern int /* boolean */ 
mysem_post	(mysem_t *psem)
{
	int flag_unlockwait;
										//printf ("sem++ prev spin lock\n");fflush(stdout);
	mythread_spinx_lock	  (&psem->door);
		psem->counter++;
										//printf ("sem++ = c:%u\n", psem->counter); fflush(stdout);
		flag_unlockwait = psem->waitcnt > 0;
		if (psem->waitcnt > 0) {
			psem->waitcnt--;
										//printf ("sem++ = waitcnt: %u\n", psem->waitcnt); fflush(stdout);
		} 
	mythread_spinx_unlock (&psem->door);
										//printf ("sem++ after spin lock\n"); fflush(stdout);
	if (flag_unlockwait){
										//printf ("sem++ = cond: go to release\n"); fflush(stdout);
		condvar_release (&psem->cv);	 
	} else {
										//printf ("sem++ = cond: NO need to release\n"); fflush(stdout);
	}
										//printf ("sem++ = post out\n"); fflush(stdout);
	return 1;
}

extern int /* boolean */ 
mysem_wait	(mysem_t *psem 
			//, int th
			)
{
	int grabbed;
										//printf ("    ~~~~~~~ mysem_wait %d: in\n",th); fflush(stdout);
	do {
										//printf ("    sem prev spin lock %d\n",th); fflush(stdout);
		mythread_spinx_lock   (&psem->door);
			if (psem->counter > 0) {
				psem->counter--;
				grabbed = 1;	
										//printf ("    sem grabbed=yes %d\n",th); fflush(stdout);
			} else {
										//printf ("    sem grabbed=no, will wait %d\n",th); fflush(stdout);
				psem->waitcnt++;
										//printf ("    sem waitcnt increased = %u\n",psem->waitcnt); fflush(stdout);

				grabbed = 0;
			}
		mythread_spinx_unlock (&psem->door);
										//printf ("    sem after spin lock %d\n",th); fflush(stdout);

		if (0 == grabbed) {
										//printf ("    sem wait %d\n",th); fflush(stdout);
			condvar_wait (&psem->cv);
		}

	} while (0 == grabbed);
										//printf ("    sem thru %d\n",th); fflush(stdout);
										//printf ("    ~~~~~~~ mysem_wait %d: out\n",th); fflush(stdout);
	return 1;
}

extern int /*boolean*/ 	
mysem_getvalue	(mysem_t *psem, int *pval)
{
	mythread_spinx_lock   (&psem->door);
	*pval = (int)psem->counter;
	mythread_spinx_unlock (&psem->door);
	return 1;
}

#endif


/*
|
|	NT_THREADS
|
\*-------------------------*/
#elif defined(NT_THREADS)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <process.h>

extern int /* boolean */
mythread_create (/*@out@*/ mythread_t *thread, routine_t start_routine, void *arg, /*@out@*/ int *ret_error)
{
	static unsigned int	thread_id;
	mythread_t t;
	int /* boolean */ is_ok;

	t =	(mythread_t) _beginthreadex (NULL, 0, start_routine, arg, 0, &thread_id );
	is_ok = (t != 0);
	*thread = t;
	*ret_error = is_ok? 0: errno;
	return is_ok;
}

extern int /* boolean */
mythread_join (mythread_t thread)
{
	unsigned long int ret;
	ret = WaitForSingleObject (thread, INFINITE);
	CloseHandle(thread);
	return ret != WAIT_FAILED;
}

extern void 		
mythread_exit (void)
{
	return;
}

extern const char *
mythread_create_error (int err)
{
	const char *s;
	switch (err) {
		case 0     : s = "Success"; break;
		case EAGAIN: s = "EAGAIN" ; break;
		case EINVAL: s = "EINVAL" ; break; 
		case EPERM : s = "EPERM"  ; break;
		default    : s = "Unknown error"; break;
	}
	return s;
}

extern void mythread_mutex_init		(mythread_mutex_t *m) { *m = CreateMutex(0, FALSE, 0)      ;}
extern void mythread_mutex_destroy	(mythread_mutex_t *m) { CloseHandle(*m)                    ;}
extern void mythread_mutex_lock     (mythread_mutex_t *m) { WaitForSingleObject(*m, INFINITE)  ;}
//_Releases_lock_(m)
extern void mythread_mutex_unlock   (mythread_mutex_t *m) { ReleaseMutex(*m)                   ;}
extern void mythread_spinx_init		(mythread_spinx_t *m) { InitializeCriticalSection(m)  ;} /**/
extern void mythread_spinx_destroy	(mythread_spinx_t *m) { DeleteCriticalSection(m)  ;} /**/
//_Acquires_lock_(m)
extern void mythread_spinx_lock     (mythread_spinx_t *m) { EnterCriticalSection (m)  ;} /**/
//_Releases_lock_(m)
extern void mythread_spinx_unlock   (mythread_spinx_t *m) { LeaveCriticalSection (m)  ;} /**/

/* semaphores */
extern int /* boolean */ 
mysem_init	(mysem_t *sem, unsigned int value)
{
	mysem_t h =
	CreateSemaphore(
  		NULL, 		/* cannot be inherited */
		(LONG)value,/* Initial Count */
  		256,		/* Maximum Count */
  		NULL		/* Name --> NULL, not shared among threads */
	);

	if (h != NULL)	*sem = h;

	return h != NULL;
}

extern int /* boolean */ 
mysem_wait	(mysem_t *sem)
{ 
	HANDLE h = *sem;
	return WAIT_FAILED != WaitForSingleObject (h, INFINITE); 
}

extern int /* boolean */ 
mysem_post	(mysem_t *sem)
{ 
	HANDLE h = *sem;
	return 0 != ReleaseSemaphore(h, 1, NULL);	
}

extern int /* boolean */ 
mysem_destroy	(mysem_t *sem)
{ 
	return 0 != CloseHandle( *sem);
}

/**** THREADS ****************************************************************************/
#else
	#error Definition of threads not present
#endif

/* MULTI_THREADED_INTERFACE */
#endif








