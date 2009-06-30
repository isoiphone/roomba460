#ifndef _OS_H_  
#define _OS_H_  
/**
 * \file os.h
 * \brief A simple RTOS interface  
 *
 * \mainpage A Simple RTOS
 * This is a simple RTOS that supports pre-emptive multithreading, and
 * interprocess synchronization using Events and Timed Mutexes.
 * 
 * \b Note: Please don't edit the interface file "os.h".
 *
 * \author Dr. Mantis Cheng
 * \date 18 June 2009
 *
 * \section assumptions GLOBAL ASSUMPTIONS 
 *
 *    (ATMEL specific)
 *  - Counter1 Timer and SWI interrupts are reserved.
 * 
 *  - All runtime exceptions (where assumptions are violated) or other  
 *    unrecoverable errors get handled by calling OS_Abort().  
 *  - Each valid entry in PPP[] must be non-zero except IDLE.
 *  - All unspecified runtime errors have undefined behaviours, 
 *    e.g., stack overflow.
 *  - PPP[] \b cannot be modified once the application starts running.
 *
 * \section policy SCHEDULING POLICY   
 *
 *   There are three scheduling levels: SYSTEM, PERIODIC and BOUNDED_RR.
 *   These levels are prioritized with SYSTEM being the highest, and 
 *   BOUNDED_RR being the lowest.
 *
 *   Preemption occurs immediately. Whenever preemption is feasible, it takes
 *   place instantly. As soon as a higher priority task becomes ready, it
 *   preempts all lower priority tasks.
 *
 * \section system SYSTEM TASKS
 *
 *   SYSTEM (level) tasks are FCFS; they run to completion, i.e., until they
 *   terminate, block or yield. Thus, they are non-preemptible, not even by
 *   other SYSTEM tasks. They should only be used for critical system level
 *   activities, e.g., error or fault recovery. Running too many SYSTEM tasks 
 *   could affect the real time performance of all other low level tasks, 
 *   including PERIODIC tasks; therefore, use SYSTEM tasks sparingly and keep
 *   their execution time short. 
 *
 * \section periodic PERIODIC TASKS
 *
 *   PERIODIC tasks are scheduled based on a fixed cyclic scheduling plan;
 *   they are time-based. (See below about PPP[].) Since they are time-critical,
 *   they are \b NOT allowed to block (i.e., wait on an event).
 *   When a PERIODIC task is created, it is assigned a "name", 
 *   a non-zero user-defined constant between 1 and MAXPROCESS.
 *   This name is fixed and can NEVER be changed again. 
 *   No two PERIODIC tasks have the same name. All names are unique. 
 *   Since tasks may terminate and be created again over time, 
 *   the same name may be reused over time for different PERIODIC task
 *   instance.
 *
 *   The PPP[] array is essentially a linear representation of a Gantt diagram.
 *   It is an array of [Name1, Interval1, Name2, Interval 2, ... ].
 *   The name of every PERIODIC task must appear in PPP[] array at least once,
 *   but may be more than once.
 * 
 *   For example, if we create three PERIODIC tasks with names A, B and 
 *   C out of three functions P(), Q() and R() respectively. Then, 
 *   PPP[] = { A, 2, B, 3, A, 5, C, 1 } means executing A for 2 ticks,
 *   then B for 3 ticks, then A again for 5 ticks, then C for 1 tick,
 *   then A again, and so on. The total cycle time is 2+3+5+1=11 ticks.
 *   That is, within 11 ticks, A executes twice, B once and C once. 
 *   If P() terminates, but the name A is later assigned to another function U(), 
 *   then A will be executed again according to PPP[] order using U(). 
 *   In a sense, PPP[] specifies at least a single execution cycle 
 *   of all PERIODIC tasks. IDLE is a special PERIODIC task name, which means
 *   do nothing during this task's assigned interval.
 *
 *   A PERIODIC task may yield (calling Task_Next()) to relinquish
 *   the processor before its assigned interval. In this case, it has completed
 *   its current execution interval and is waiting for its next interval. The
 *   time between when the current PERIOIDIC yields and when the next PERIODIC
 *   task starts is called the IDLE time.
 * 
 *   It is a runtime error if a PERIODIC task executes longer than the
 *   currently assigned interval. It is important NOT to underestimate
 *   the execution time requirement of a PERIODIC task. Choosing the appropriate
 *   execution order and intervals for all PERIODIC tasks is the responsibility
 *   of the Application Design Engineer(s), not our RTOS. Hence, all timing
 *   violations should be caught and then reported.
 *
 *   By specifying PPP[] and scheduling our PERIODIC tasks accordingly,
 *   we shall know precisely the execution "cycle" time of all such tasks, 
 *   thus their best execution frequency/rate and response time. This is how
 *   we guarantee the predictability of timing and ordering all critical
 *   activities.
 *
 *  \section rr BOUNDED_RR TASKS
 *   
 *   BOUNDED_RR (or BRR for short) tasks are scheduled in a round-robin fashion.
 *   It is different from typical round-robin tasks in that each BRR may specify
 *   its next bounded execution quantum. When a BRR task is created, it is  
 *   assigned an initial quantum. However, when it yields, it may call either
 *   Task_Next() or Task_Next_Quantum(). If it calls Task_Next(), it is
 *   rescheduled with a default quantum. If it calls Task_Next_Quantum(n), it is
 *   rescheduled with a new quantum "n".
 *
 *   BRR tasks are allowed to run \a only when no PERIODIC or SYSTEM tasks
 *   are executing. Any BRR task with a quantum "m" (m >= 1) must fit into the
 *   next IDLE time before the next PERIODIC task; otherwise, it is rescheduled.
 *   That is, if a BRR task's next quantum doesn't fit the current IDLE time,
 *   it is assumed that this BRR task has completed its quantum. Our goal is
 *   to provide a "contiguous" time slot for executing a BRR task.
 *
 *   However, a BRR task may still be interrupted by a SYSTEM level task. As a
 *   result, it may not execute contiguously for its specified bounded quantum.
 *   When a BRR is preempted by a SYSTEM or PERIODIC task, it is assumed that it
 *   has consumed all its quantum. Hence, our BOUNDED ROUND ROBIN scheduling is
 *   a "best effort" policy to meet a BRR task's specified computation time
 *   requirement.
 *
 *   A BRR task may be assigned a quantum size of 0. In that case, it is treated
 *   as a FCFS run-to-completion task. That is, it must voluntarily give up
 *   its share of processor by calling Task_Next() or Task_Next_Quantum().
 *
 *
 *  \section boot OS BOOTING
 *     Our RTOS is compiled together with an application. It doesn't provide
 *     a "main()" function, which is a part of the application. By convention,
 *     the "main()" is the first function to be called by the C runtime code, 
 *     "crt0.S". For our RTOS, we shall change this convention as follows: 
 *     -# OS_Init() is called from crt0.S as the very first C function to be
 *        executed instead of main().
 *     -# Upon completion of OS_Init(), the application's main() is then
 *        created as the first and only SYSTEM level task.
 *     -# In main(), the rest of the application tasks are then created.
 *     -# In order for all other application tasks to run, our main() task
 *        must either terminate or block on an event. (For example, main()
 *        may become a "watchdog" task to reset the entire application.)
 *
 *
 * \section ipc INTERPROCESS COMMUNICATION (Events)
 *
 *    Events are one-way synchronization signals. They don't have any "memory"
 *    (i.e., values); thus, they are NOT semaphores. Any SYSTEM or RR task may
 *    wait on an event; PERIODIC tasks \b MUST \b NOT wait on any event.
 *    However, any task may signal/broadcast an event.
 *    A waiting task is resumed when the associated event is signalled.
 *    All waiting tasks are resumed when the associated event is broadcasted.
 *    When an event is signalled (or broadcasted) but there are no waiting tasks,
 *    this is a \a no-op; hence, events have \b no memory.
 *
 * \section ipc BOUNDED MUTUAL EXCLUSION (Mutexes)
 *
 *    For mutual exclusion, we provided a bounded form of mutexes. Any SYSTEM
 *    or BRR task may lock a bounded mutex, calling Mutex_Lock(m,n) where "m"
 *    is the mutex and "n" is the maximum number of clock ticks allowed in
 *    using the lock. While holding a mutex, the owner is executed as a
 *    highest level SYSTEM task until it releases the mutex. However, this
 *    priority promotion is only temporary until the mutex is released or
 *    "n" ticks has expired. In the latter case, it is a timing violation. The
 *    RTOS is aborted with an error message.
 * 
 *    Only the owner of mutex (the task that locks it) is allowed to unlock
 *    the mutex. A mutex may be locked multiple times by its owner. The number
 *    of times of lock and unlock must be matched.
 */  
  
   
/*==================================================================  
 *             T Y P E S   &   C O N S T A N T S  
 *==================================================================  
 */  

/*================
  *    C O N S T A N T S
  *================
  */
/* limits */

/** max. number of processes supported */  
#define MAXPROCESS    8       

/** max. number of Events supported */
#define MAXEVENT	  8

/** max. number of Mutexes supported */
#define MAXMUTEX	  8

/** workspace size of each process in bytes */  
#define WORKSPACE     256   

/** BRR task quantum sizes, number of TICKs */
#define MAXQUANTUM    5

/** milliseconds, or something close to this value 
 * \sa \ref periodic.
 */
#define TICK          5        

/* scheduling levels */

/** a scheduling level: system tasks with first-come-first-served policy 
 * \sa \ref system, Task_Create().
 */
#define SYSTEM         3 

/** a scheduling level: periodic tasks with predefined intervals 
 * \sa \ref periodic, Task_Create().
 */
#define PERIODIC       2 

/** A scheduling level: first-come-first-served cooperative tasks
 * \sa \ref rr, Task_Create(). 
 */
#define BRR            1      

#ifndef NULL
#define NULL     0   /* undefined */
#endif

#define IDLE     0   /* a well-known PERIODIC task name */

/*================
  *    T Y P E S
  *================
  */
/** An Event descriptor
 * \sa Event_Init().
 */
typedef struct event EVENT;  


/** A Mutex descriptor
 * \sa Mutex_Init().
 */
typedef struct mutex MUTEX;


/*================
  *    G L O B A L S
  *================
  */
/**
 *  A periodic task scheduling plan (read-only, defined by the application).
 *
 *  This is specified by the application. The scheduling ordering and duration
 *  of all PERIODIC tasks are specified by this plan. The total number of
 *  occurrences of all PERIODIC tasks, including IDLE, is given by PT.
 *  e.g., PPP[] = { A, 2, IDLE, 5, B, 1, A, 2, C, 4 };
 *        PT = 5;
 *  \sa \ref periodic.
 */  
extern const unsigned char PPP[];  // N.B. Its length MUST be even.
extern const unsigned int PT;  // should always be sizeof(PPP)/2
  
/*==================================================================  
 *             A C C E S S    P R O C E D U R E S  
 *==================================================================  
 */  

  /*=====  OS   Initialization ===== */

/** Initialize this RTOS; must be called first. 
 * \sa \ref boot.
 */
void OS_Init(void);     

/** Abort the execution of this RTOS due to an unrecoverable erorr.
 * \sa \ref assumptions. 
 */
void OS_Abort(void);  

  /*=====  Task API ===== */

 /**
   * \param f  a parameterless function to be created as a process instance
   * \param arg an integer argument to be assigned to this process instanace
   * \param level assigned scheduling level: SYSTEM, PERIODIC or BRR
   * \param n assigned PERIODIC process name or an initial quantum for a BRR
   *        task where 0 <= n <= MAXQUANTUM. 
   * \return 0 if not successful; otherwise non-zero.
   * \sa Task_GetArg(), PPP[].
   *
   *  A new process is created to execute the parameterless
   *  function \a f with an initial parameter \a arg, which is retrieved
   *  by a call to Task_GetArg().  If a new process cannot be
   *  created, 0 is returned; otherwise, it returns non-zero.
   *  The created process will belong to its scheduling \a level.
   *  If the process is PERIODIC, then its \a name is a user-specified name 
   *  to be used in the PPP[] array. Otherwise, \a name is ignored. 
   * \sa \ref policy
   */
int   Task_Create(void (*f)(void), int arg, unsigned int level, unsigned int n);

/** 
 * Terminate the calling process
 *
 *  When a process returns, i.e., it executes its last instruction in 
 *  the associated function/code, it is automatically terminated.
 */
void Task_Terminate(void);    

/** Voluntarily relinquish the processor. */
void Task_Next(void);

/** Retrieve the assigned parameter. 
  * \sa Task_Create(). 
  */
int Task_GetArg(void);          

  /*=====  Events API ===== */

/**
 * \return a non-NULL Event descriptor if successful; NULL otherwise.
 *
 *  Initialize a new, non-NULL Event descriptor.
 */
EVENT *Event_Init(void);

/**  
  * \param e  an Even descriptor
  *
  * Wait for the next occurrence of a signal on \a e. The calling process always blocks. 
  */
void Event_Wait( EVENT *e );  

/**  
  * \param e an Event descriptor
  *
  * Resume a \b single waiting task on \a e. It is a \a no-op if there is no waiting process.
  * \sa Event_Wait() 
  */
void Event_Signal( EVENT *e );

/**  
  * \param e  an Event descriptor
  *
  * Resume \b ALL waiting tasks on \a e. It is a \a no-op if there is no waiting process. 
  * \sa Event_Wait()
  */
void Event_Broadcast( EVENT *e );

   
/**  
  * \param e  an Event descriptor
  *
  * Resume a waiting task on \a e and at the same time relinquish the processor.
  * This is equivalent to "Event_Signal( e ); Task_Next()" in concept. The
  * fundamental difference is that these two operations are performed as
  * an indivisible unit. So conceptually, the calling task resumes another
  * waiting task and gives up its share of the processor simultaneously.
  * \sa Event_Signal(), Task_Next()
  */
void  Signal_And_Next( EVENT *e );
   
/**  
  * \param e  an Event descriptor
  *
  * Resume \b ALL waiting tasks on \a e and at the same time relinquish the processor.
  * This is equivalent to "Event_Broadcast( e ); Task_Next()" in concept.
  * \sa Event_Broadcast(), Task_Next()
  */
void  Broadcast_And_Next( EVENT *e );
  
  /*=====  !!! NEW !!! API calls ===== */

/**  
  * Returns the number of clock ticks since OS_Init(). Note that this number
  * wraps around after it overflows as an unsigned integer. The arithmetic
  * of 2's complement will take care of this wrap-around behaviour if you use
  * this number correctly.
  * Let  T = Now() and we want to know when Now() reaches T+1000.
  * Now() is always increasing. Even if Now() wraps around, (Now() - T) always
  * >= 0. As long as the duration of interest is less than the wrap-around time,
  * then (Now() - T >= 1000) would mean we have reached T+1000.
  * However, we cannot compare Now() against T directly due to this wrap-around
  * behaviour.
  */
unsigned int Now();


/** Voluntarily relinquish the processor by a BRR task and specify its
  * next quantum required, where 0 <= n <= MAXQUANTUM
  */
void Task_Next_Quantum( unsigned int n );              

  
/**
 * \return a non-NULL Mutex descriptor if successful; NULL otherwise.
 *
 *  Initialize a new, non-NULL Mutex descriptor. The mutex is initially free.
 */
MUTEX *Mutex_Init(void);

/**  
  * \param e  a Mutex descriptor
  *
  * Locks mutex "m" for "n" ticks. That is, if "m" is free, then the 
  * calling task will lock "m" for up to "n" clock ticks. It must release
  * "m" (calling Mutex_Unlock()) before "n" ticks; otherwise, it is a timing
  * violation. While the calling task is holding "m", it is executed as
  * a SYSTEM level task temporarily until it releases the mutex.
  * Furthermore, the calling task is allowed to lock/unlock "m" multiple times.
  * That is, mutexes are "recursive".
  * (N.B. PERIODIC tasks are NOT allowed to use mutexes.)
  */
void Mutex_Lock( MUTEX *m, unsigned int n );  

/**  
  * \param e a Mutex descriptor
  *
  * Unlock a "locked" mutex "m". Only the owner of a mutex (the task that locks
  * "m") is allowed to unlock "m".
  */
void Mutex_Unlock( MUTEX *m );


#endif /* _OS_H_ */
