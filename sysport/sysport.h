#if !defined(H_SYSPOR)
#define H_SYSPOR

/*
	Possible Definitions for POSIX Semaphores:
		UNNAMED_SEMAPHORES
		NAMED_SEMAPHORES
		MY_SEMAPHORES

	This will turn spinlocks into mutexes:
		NSPINLOCKS
		MY_SPINLOCKS

	(Obsolete) if SMP functions are not going to be used nor linked with -lpthread
		MONOTHREAD 
*/

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/


#if defined(MINGW)
	#include <windows.h>
	#if !defined(MVSC)
		#define MVSC
	#endif
#endif

#ifdef _MSC_VER
	#include <windows.h>
	#if !defined(MVSC)
		#define MVSC
	#endif
#else
	#include <unistd.h>
#endif


#if defined(__linux__) || defined(__GNUC__) || defined (__APPLE__)
	#if !defined(GCCLINUX)
		#define GCCLINUX
	#endif
#endif

#if defined(MINGW)
	#undef GCCLINUX
#endif

/*
|
|	To allow multithreaded functions, MULTI_THREADED_INTERFACE should be defined
|
\*--------------------------------------------------------------------------------*/


#if defined(CYGWIN)
	#define USECLOCK
	#define MULTI_THREADED_INTERFACE
	#undef  NT_THREADS
	#define POSIX_THREADS
	#define GCCLINUX_INTEGERS
#elif defined(MINGW)
	#define USEWINCLOCK
	#define MULTI_THREADED_INTERFACE
	#define NT_THREADS
	#undef  POSIX_THREADS
	#define MSWINDOWS_INTEGERS
#elif defined(GCCLINUX)
	#define USELINCLOCK
	#define MULTI_THREADED_INTERFACE
	#undef  NT_THREADS
	#define POSIX_THREADS
	#define GCCLINUX_INTEGERS
#elif defined(MVSC)
	#define USEWINCLOCK 
	#define MULTI_THREADED_INTERFACE
	#define NT_THREADS
	#undef  POSIX_THREADS
	#define MSWINDOWS_INTEGERS
#else
	#error COMPILER NOT DEFINED
#endif

#if defined(__APPLE__)
	#define NSPINLOCKS
#endif

#if defined(MONOTHREAD)
	#undef MULTI_THREADED_INTERFACE
#endif


#if defined(GCCLINUX) || defined(MINGW)
	#define U64(x) (x##ull)
#elif defined(MVSC)
	#define U64(x) (x##ui64)
#else
	#error OS not defined properly
#endif

/*
|
|	TYPES
|
\*--------------------------------------------------------------------------------*/

#if defined(GCCLINUX) || defined(MINGW)

/*
	typedef unsigned long long int	uint64_t;
	typedef long long int			int64_t;
	typedef unsigned char			uint8_t;
	typedef unsigned short int		uint16_t;
	typedef unsigned int			uint32_t;
*/

	#include <stdint.h>

#elif defined(MVSC)

	typedef unsigned char			uint8_t;
	typedef unsigned short int		uint16_t;
	typedef unsigned int			uint32_t;
	typedef unsigned __int64		uint64_t;

	typedef signed char				int8_t;
	typedef short int				int16_t;
	typedef int						int32_t;
	typedef __int64					int64_t;

#else
	#error OS not defined properly for 64 bit integers
#endif


/*-----------------
	PATH NAMES
------------------*/

#if defined(GCCLINUX)
	#define FOLDERSEP "/"
#elif defined(MVSC)
	#define FOLDERSEP "\\"
#else
	#define FOLDERSEP "/"
#endif

/* path names */
extern int isfoldersep (int x);

/*-----------------
	FOPEN MAX
------------------*/

extern int mysys_fopen_max (void);

/*------------ 
	TIMER 
-------------*/

typedef int64_t myclock_t;

extern myclock_t myclock(void);
extern myclock_t ticks_per_sec (void);

#define MYCLOCKS_PER_SEC (ticks_per_sec())
#define GET_TICK (myclock())

/*********************************************************************/
#if defined(MULTI_THREADED_INTERFACE)

/*------------ 
	THREADS
-------------*/

#if defined (POSIX_THREADS)

	#define SPINLOCKS

	#include <pthread.h>
	#include <semaphore.h>

	#define THREAD_CALL
	
	typedef void * 						thread_return_t;
	typedef pthread_t 					mythread_t;
	typedef thread_return_t 			(THREAD_CALL *routine_t) (void *);
	typedef pthread_mutex_t 			mythread_mutex_t;

	#if defined(NSPINLOCKS)
		typedef pthread_mutex_t 		mythread_spinx_t; 
	#elif defined(MY_SPINLOCKS)
		// Implemente spinlocks when they are not present in the pthread library
		// https://idea.popcount.org/2012-09-12-reinventing-spinlocks/
		typedef int 					mythread_spinx_t; 
	#else
		typedef pthread_spinlock_t 		mythread_spinx_t; 
	#endif
	
	#if defined(UNNAMED_SEMAPHORES)

		typedef sem_t					mysem_t;

	#elif defined(NAMED_SEMAPHORES)

		struct mySEM {
			sem_t *psem;
			char name[32];
		};

		typedef struct mySEM			mysem_t;

	#elif defined(MY_SEMAPHORES)

		struct CONDVAR {
			pthread_mutex_t	mtx;
			int				go;
			pthread_cond_t 	key;
		};

		typedef struct CONDVAR 			condvar_t;

		struct mySemaphore {
			unsigned 			waitcnt;
			unsigned 			counter;
			mythread_spinx_t 	door;
			condvar_t			cv;
		};

		typedef struct mySemaphore 		mysem_t;

	#else
		#error Definition of semaphores not present
	#endif


#elif defined(NT_THREADS)

	#define WIN32_LEAN_AND_MEAN

	#include <windows.h>
	#include <process.h>

	#define THREAD_CALL __stdcall

	typedef unsigned 					thread_return_t;
	typedef HANDLE 						mythread_t;
	typedef thread_return_t 			(THREAD_CALL *routine_t) (void *);
	typedef HANDLE						mythread_mutex_t;

	#if defined(NSPINLOCKS)
		typedef HANDLE 					mythread_spinx_t; 
	#else
		typedef CRITICAL_SECTION 		mythread_spinx_t; 
	#endif

	typedef HANDLE						mysem_t;

#else
	#error Definition of threads not present
#endif


extern int /*boolean*/	mythread_create (/*@out@*/ mythread_t *thread, routine_t start_routine, void *arg, /*@out@*/ int *ret_error);
extern int /*boolean*/	mythread_join (mythread_t thread);
extern void 			mythread_exit (void);
extern const char *		mythread_create_error (int err);

extern void 			mythread_mutex_init		(mythread_mutex_t *m);
extern void 			mythread_mutex_destroy	(mythread_mutex_t *m);
extern void 			mythread_mutex_lock     (mythread_mutex_t *m);
extern void 			mythread_mutex_unlock   (mythread_mutex_t *m);

extern void 			mythread_spinx_init		(mythread_spinx_t *m); /**/
extern void 			mythread_spinx_destroy	(mythread_spinx_t *m); /**/
extern void 			mythread_spinx_lock     (mythread_spinx_t *m); /**/
extern void 			mythread_spinx_unlock   (mythread_spinx_t *m); /**/

/* semaphores*/
extern int /*boolean*/	mysem_init		(mysem_t *sem, unsigned int value);
extern int /*boolean*/	mysem_wait		(mysem_t *sem);
extern int /*boolean*/	mysem_post		(mysem_t *sem);
extern int /*boolean*/	mysem_destroy	(mysem_t *sem);

#if (defined(UNNAMED_SEMAPHORES) || defined(MY_SEMAPHORES))
extern int /*boolean*/ 	mysem_getvalue	(mysem_t *sem, int *pval);
#endif

#endif

/* end MULTI_THREADED_INTERFACE*/
#endif


extern void semaphore_system_init(void);
extern void semaphore_system_done(void);


/*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
