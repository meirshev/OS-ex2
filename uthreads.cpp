/*
 * Meir franco ID: 301615746
 * Rozen arbov ID: 302831474
 */





#include "uthreads.h"
#include <vector>
#include <queue>
#include <signal.h>
#include <sys/time.h>
#include <setjmp.h>
#include <iostream>

using namespace std;

#define BLOCKED -2
#define TERMINATED -1
#define ERROR_VALUE -1
#define SUCCESS_VALUE 0




#ifdef __x86_64__


typedef unsigned long address_t;
#define JB_SP 6
#define JB_PC 7

/* A translation is required when using an address of a variable.*/
address_t translate_address(address_t addr)
{
    address_t ret;
    asm volatile("xor    %%fs:0x30,%0\n"
            "rol    $0x11,%0\n"
    : "=g" (ret)
    : "0" (addr));
    return ret;
}

#else


typedef unsigned int address_t;
#define JB_SP 4
#define JB_PC 5

/* A translation is required when using an address of a variable.*/
address_t translate_address(address_t addr)
{
    address_t ret;
    asm volatile("xor    %%gs:0x18,%0\n"
		"rol    $0x9,%0\n"
                 : "=g" (ret)
                 : "0" (addr));
    return ret;
}

#endif
//forward declaration.
class uthreadLibrary;

//intialize a static lib.
static uthreadLibrary* uthreadLib;

//index(in threads vector) of thread currently running.
static int who_is_running;

int total_number_of_quants;

/**
 * this class is represents a thread.
 */
class uthread
{
private:
    int tid;
    address_t sp, pc;
    bool is_sleeping = false;
    bool is_blocked = false;

public:

    sigjmp_buf buf;
    int numberOfQuantsThreadRan = 0;
    int numberOfQuantsToSleep;

    int get_tid()
    {
        return this->tid;
    }

    void set_tid(int tid)
    {
        this->tid = tid;
    }

    address_t get_pc()
    {
        return this->pc;
    }

    void set_pc(address_t pc)
    {
        this->pc = pc;
    }

    address_t get_sp()
    {
        return this->sp;
    }

    void set_sp(address_t sp)
    {
        this->sp = sp;
    }

    void set_is_sleeping(bool is_sleep)
    {
        this->is_sleeping = is_sleep;
    }

    bool get_is_sleeping()
    {
        return this->is_sleeping;
    }

    void set_is_blocked(bool is_block)
    {
        this->is_blocked = is_block;
    }

    bool get_is_blocked()
    {
        return this->is_blocked;
    }

    void set_sleep_time_quants(int quantsToSleep)
    {
        this->numberOfQuantsToSleep = quantsToSleep;
        this->is_sleeping = true;
    }

    char stack1[STACK_SIZE];
};



/**
 * this is a lib of uthreads.
 */
class uthreadLibrary
{
public:

    vector <uthread*> uthreadVector;
    queue <int> readyList;

    //constructor.
    uthreadLibrary()
    {
        uthreadVector.resize(MAX_THREAD_NUM);
    }

    //destructor
    ~uthreadLibrary()
    {
        for (int i = 0; i < MAX_THREAD_NUM; i++)
        {
            if (uthreadLib->uthreadVector[i] != nullptr)
            {
                delete (uthreadLib->uthreadVector[i]);
                uthreadLib->decreaseUthread();
            }
        }
    }
    //this method remove an element from a queue that represents the
    // 'ready threads'.
    void removeItemFromQueue(int elementToRemove)
    {
        int tempInt;
        int readyListSize = this->readyList.size();
        for (int i = 0; i < readyListSize; i++)
        {
            tempInt = this->readyList.front();
            //cout << tempInt << endl;
            this->readyList.pop();
            if (tempInt != elementToRemove)
            {
                this->readyList.push(tempInt);
            }
        }
    }

    void increaseUthread()
    {
        numberOfUthreads++;
    }

    void decreaseUthread()
    {
        numberOfUthreads--;
    }

    int get_number_of_uthreads ()
    {
        return numberOfUthreads;
    }

private:
    int numberOfUthreads = 0;
};


sigset_t *set;

/**
 * This is the scheduler of the threads - most of the time it is being
 * called by
 * the timer obj and basically manages
 * the threads being executed and tracks ad wakes up sleeping threads.
 */
void timer_handler(int sig)
{

    if (sigprocmask(SIG_BLOCK, set,NULL) < 0)
    {
        printf("system error: text\n");
    }


    total_number_of_quants++;


    for (int i = 0; i < MAX_THREAD_NUM; i++)
    {

        if (uthreadLib->uthreadVector[i] != nullptr &&
                uthreadLib->uthreadVector[i]->get_is_sleeping() == true)
        {

            uthreadLib->uthreadVector[i]->numberOfQuantsToSleep--;
            if (uthreadLib->uthreadVector[i]->numberOfQuantsToSleep == 0)
            {
                uthreadLib->uthreadVector[i]->set_is_sleeping(false);
                uthreadLib->uthreadVector[i]->set_is_sleeping(false);
                uthreadLib->readyList.push(uthreadLib->uthreadVector[i]->
                        get_tid());
            }
        }
    }

    if (who_is_running != TERMINATED)
    {
        if(uthreadLib->uthreadVector[who_is_running]->get_is_blocked() == false
           && uthreadLib->uthreadVector[who_is_running]->get_is_sleeping()
              == false)
        {
            uthreadLib->readyList.push(who_is_running);
        }



        if (sigsetjmp(uthreadLib->uthreadVector[who_is_running]->buf, 1) != 0)
        {

            if (sigprocmask(SIG_UNBLOCK, set,NULL) < 0)
            {
                printf("system error: text\n");
            }
            return;
        }
    }


    who_is_running = uthreadLib->readyList.front();
    uthreadLib->readyList.pop();

    uthreadLib->uthreadVector[who_is_running]->numberOfQuantsThreadRan++;

    uthreadLib->removeItemFromQueue(300);

    siglongjmp(uthreadLib->uthreadVector[who_is_running]->buf, 1);


    if (sigprocmask(SIG_UNBLOCK, set,NULL) < 0)
    {
        printf("system error: text\n");
    }
    return;
}

struct sigaction sa;
itimerval* timer = new itimerval;

/*
 * Description: This function initializes the thread library.
 * You may assume that this function is called before any other thread library
 * function, and that it is called exactly once. The input to the function is
 * the length of a quantum in micro-seconds. It is an error to call this
 * function with non-positive quantum_usecs.
 * Return value: On success, return 0. On failure, return -1.
*/
int uthread_init(int quantum_usecs)
{
    sigemptyset(set);
    sigfillset(set);
    if (sigprocmask(SIG_BLOCK, set,NULL) < 0)
    {
        printf("system error: text\n");
    }
    int sec = 0;
    int temp_quantum_usecs = quantum_usecs;
    while (temp_quantum_usecs >= 1000000)
    {
        sec++;
        temp_quantum_usecs = temp_quantum_usecs - 1000000;
    }


    if (quantum_usecs <= 0)
    {
        if (sigprocmask(SIG_UNBLOCK, set,NULL) < 0)
        {
            printf("system error: text\n");
        }
        return ERROR_VALUE;
    }
    uthreadLib = new uthreadLibrary();
    who_is_running = 0;
    total_number_of_quants = 1;
    uthread* newUthread = new uthread();
    newUthread->set_tid(0);
    uthreadLib->increaseUthread();
    uthreadLib->uthreadVector[0] = newUthread;
    uthreadLib->uthreadVector[0]->numberOfQuantsThreadRan = 1;


    sa.sa_handler = &timer_handler;

    timer->it_interval.tv_sec = sec;
    timer->it_interval.tv_usec = temp_quantum_usecs;
    timer->it_value.tv_sec = sec;
    timer->it_value.tv_usec = temp_quantum_usecs;


    if (sigaction(SIGVTALRM, &sa,NULL) < 0)
    {
        printf("system error: text\n");
    }


    if (setitimer (ITIMER_VIRTUAL, timer, NULL))
    {
        printf("system error: text\n");
        exit(1);
    }


    if (sigprocmask(SIG_UNBLOCK, set,NULL) < 0)
    {
        printf("system error: text\n");
    }
    return SUCCESS_VALUE;
}

/*
 * Description: This function creates a new thread, whose entry point is the
 * function f with the signature void f(void). The thread is added to the end
 * of the READY threads list. The uthread_spawn function should fail if it
 * would cause the number of concurrent threads to exceed the limit
 * (MAX_THREAD_NUM). Each thread should be allocated with a stack of size
 * STACK_SIZE bytes.
 * Return value: On success, return the ID of the created thread.
 * On failure, return -1.
*/
int uthread_spawn(void (*f)(void))
{
    if (sigprocmask(SIG_BLOCK, set,NULL) < 0)
    {
        printf("system error: text\n");
    }
    if (uthreadLib->get_number_of_uthreads() == MAX_THREAD_NUM)
    {
        if (sigprocmask(SIG_UNBLOCK, set,NULL) < 0)
        {
            printf("system error: text\n");
        }
        return ERROR_VALUE;
    }

    address_t sp, pc;
    uthreadLib->increaseUthread();

    for (int i = 1; i < MAX_THREAD_NUM; i++)
    {
        if (uthreadLib->uthreadVector[i] == nullptr)
        {

            uthreadLib->uthreadVector[i] = new uthread();
            uthreadLib->uthreadVector[i]->set_tid(i);

            sp = (address_t)uthreadLib->uthreadVector[i]->stack1 +
                 STACK_SIZE - sizeof(address_t);
            pc = (address_t)f;
            sigsetjmp(uthreadLib->uthreadVector[i]->buf, 1);
            (uthreadLib->uthreadVector[i]->buf->__jmpbuf)[JB_SP]
                    = translate_address(sp);
            (uthreadLib->uthreadVector[i]->buf->__jmpbuf)[JB_PC]
                    = translate_address(pc);
            sigemptyset(&uthreadLib->uthreadVector[i]->buf->__saved_mask);

            uthreadLib->readyList.push(uthreadLib->uthreadVector[i]
                                               ->get_tid());

            if (sigprocmask(SIG_UNBLOCK, set,NULL) < 0)
            {
                printf("system error: text\n");
            }
            return i;
        }
    }
    if (sigprocmask(SIG_UNBLOCK, set,NULL) < 0)
    {
        printf("system error: text\n");
    }
    return ERROR_VALUE;
}




/*
 * Description: This function terminates the thread with ID tid and deletes
 * it from all relevant control structures. All the resources allocated by
 * the library for this thread should be released. If no thread with ID tid
 * exists it is considered as an error. Terminating the main thread
 * (tid == 0) will result in the termination of the entire process using
 * exit(0) [after releasing the assigned library memory].
 * Return value: The function returns 0 if the thread was successfully
 * terminated and -1 otherwise. If a thread terminates itself or the main
 * thread is terminated, the function does not return.
*/
int uthread_terminate(int tid)
{

    if (sigprocmask(SIG_BLOCK, set,NULL) < 0)
    {
        printf("system error: text\n");
    }
    if (uthreadLib->uthreadVector[tid] == nullptr ||
            tid >= MAX_THREAD_NUM || tid < 0)
    {
        if (sigprocmask(SIG_UNBLOCK, set,NULL) < 0)
        {
            printf("system error: text\n");
        }
        return ERROR_VALUE;
    }
    //terminate the main procees
    if (tid == 0)
    {
        delete (uthreadLib);
        if (sigprocmask(SIG_UNBLOCK, set,NULL) < 0)
        {
            printf("system error: text\n");
        }
        exit(0);
    }
    //thread terminates itself
    if (tid == who_is_running)
    {
        delete (uthreadLib->uthreadVector[tid]);
        uthreadLib->uthreadVector[tid] = nullptr;
        uthreadLib->decreaseUthread();
        who_is_running = TERMINATED;
        uthreadLib->removeItemFromQueue(tid);

        if (setitimer (ITIMER_VIRTUAL, timer, NULL))
        {
            printf("system error: text\n");
            exit(1);
        }
        if (sigprocmask(SIG_UNBLOCK, set,NULL) < 0)
        {
            printf("system error: text\n");
        }
        timer_handler(1);
    }
    else
    {
        delete(uthreadLib->uthreadVector[tid]);
        uthreadLib->uthreadVector[tid] = nullptr;

        uthreadLib->decreaseUthread();
        uthreadLib->removeItemFromQueue(tid);
        if (sigprocmask(SIG_UNBLOCK, set,NULL) < 0)
        {
            printf("system error: text\n");
        }
        return SUCCESS_VALUE;
    }
}

/*
 * Description: This function blocks the thread with ID tid. The thread may
 * be resumed later using uthread_resume. If no thread with ID tid exists it
 * is considered as an error. In addition, it is an error to try blocking the
 * main thread (tid == 0). If a thread blocks itself, a scheduling decision
 * should be made. Blocking a thread in BLOCKED or SLEEPING states has no
 * effect and is not considered as an error.
 * Return value: On success, return 0. On failure, return -1.
*/
int uthread_block(int tid)
{
    if (sigprocmask(SIG_BLOCK, set,NULL) < 0)
    {
        printf("system error: text\n");
    }
    if (tid <= 0 || uthreadLib->uthreadVector[tid] == nullptr ||
            tid >= MAX_THREAD_NUM )
    {
        if (sigprocmask(SIG_UNBLOCK, set,NULL) < 0)
        {
            printf("system error: text\n");
        }
        return ERROR_VALUE;
    }
    if (uthreadLib->uthreadVector[tid]->get_is_sleeping() == true)
    {
        if (sigprocmask(SIG_UNBLOCK, set,NULL) < 0)
        {
            printf("system error: text\n");
        }
        return SUCCESS_VALUE;
    }
    uthreadLib->removeItemFromQueue(tid);

    uthreadLib->uthreadVector[tid]->set_is_blocked(true);

    if (tid == who_is_running)
    {


        if (setitimer (ITIMER_VIRTUAL, timer, NULL))
        {
            printf("system error: text\n");
            exit(1);
        }
        if (sigprocmask(SIG_UNBLOCK, set,NULL) < 0)
        {
            printf("system error: text\n");
        }
        timer_handler(1);
    }

    if (sigprocmask(SIG_UNBLOCK, set,NULL) < 0)
    {
        printf("system error: text\n");
    }
    return SUCCESS_VALUE;
}


/*
 * Description: This function resumes a blocked thread with ID tid and moves
 * it to the READY state. Resuming a thread in the RUNNING, READY or SLEEPING
 * state has no effect and is not considered as an error. If no thread with
 * ID tid exists it is considered as an error.
 * Return value: On success, return 0. On failure, return -1.
*/
int uthread_resume(int tid)
{
    if (sigprocmask(SIG_BLOCK, set,NULL) < 0)
    {
        printf("system error: text\n");
    }
    if (tid < 0 || uthreadLib->uthreadVector[tid] == nullptr ||
            tid >= MAX_THREAD_NUM )
    {
        if (sigprocmask(SIG_UNBLOCK, set,NULL) < 0)
        {
            printf("system error: text\n");
        }
        return ERROR_VALUE;
    }
    if (uthreadLib->uthreadVector[tid]->get_is_blocked() == true)
    {
        uthreadLib->uthreadVector[tid]->set_is_blocked(false);
        uthreadLib->readyList.push(tid);
    }
    if (sigprocmask(SIG_UNBLOCK, set,NULL) < 0)
    {
        printf("system error: text\n");
    }
    return SUCCESS_VALUE;
}


/*
 * Description: This function puts the RUNNING thread to sleep for a period
 * of num_quantums (not including the current quantum) after which it is moved
 * to the READY state. num_quantums must be a positive number. It is an error
 * to try to put the main thread (tid==0) to sleep. Immediately after a thread
 * transitions to the SLEEPING state a scheduling decision should be made.
 * Return value: On success, return 0. On failure, return -1.
*/
int uthread_sleep(int num_quantums)
{
    if (sigprocmask(SIG_BLOCK, set,NULL) < 0)
    {
        printf("system error: text\n");
    }
    if (num_quantums <= 0 || who_is_running == 0)
    {
        if (sigprocmask(SIG_UNBLOCK, set,NULL) < 0)
        {
            printf("system error: text\n");
        }
        return ERROR_VALUE;
    }

    uthreadLib->uthreadVector[who_is_running]->numberOfQuantsToSleep =
            num_quantums + 1;
    uthreadLib->uthreadVector[who_is_running]->set_is_sleeping(true);


    if (setitimer (ITIMER_VIRTUAL, timer, NULL))
    {
        printf("system error: text\n");
        exit(1);
    }
    timer_handler(1);
    if (sigprocmask(SIG_UNBLOCK, set,NULL) < 0)
    {
        printf("system error: text\n");
    }
    return SUCCESS_VALUE;
}

/*
 * Description: This function returns the number of quantums until the thread
 * with id tid wakes up including the current quantum. If no thread with ID
 * tid exists it is considered as an error. If the thread is not sleeping,
 * the function should return 0.
 * Return value: Number of quantums (including current quantum) until wakeup.
*/
int uthread_get_time_until_wakeup(int tid)
{
    if (sigprocmask(SIG_BLOCK, set,NULL) < 0)
    {
        printf("system error: text\n");
    }
    if (tid < 0 || uthreadLib->uthreadVector[tid] == nullptr ||
            tid >= MAX_THREAD_NUM )
    {
        if (sigprocmask(SIG_UNBLOCK, set,NULL) < 0)
        {
            printf("system error: text\n");
        }
        return ERROR_VALUE;
    }
    if (uthreadLib->uthreadVector[tid]->get_is_sleeping() == true)
    {
        if (sigprocmask(SIG_UNBLOCK, set,NULL) < 0)
        {
            printf("system error: text\n");
        }
        return uthreadLib->uthreadVector[tid]->numberOfQuantsToSleep;
    }
    if (sigprocmask(SIG_UNBLOCK, set,NULL) < 0)
    {
        printf("system error: text\n");
    }
    return SUCCESS_VALUE;
}

/*
 * Description: This function returns the thread ID of the calling thread.
 * Return value: The ID of the calling thread.
*/
int uthread_get_tid()
{

    return who_is_running;
}


/*
 * Description: This function returns the total number of quantums that were
 * started since the library was initialized, including the current quantum.
 * Right after the call to uthread_init, the value should be 1.
 * Each time a new quantum starts, regardless of the reason, this number
 * should be increased by 1.
 * Return value: The total number of quantums.
*/
int uthread_get_total_quantums()
{
    return total_number_of_quants;
}

/*
 * Description: This function returns the number of quantums the thread with
 * ID tid was in RUNNING state. On the first time a thread runs, the function
 * should return 1. Every additional quantum that the thread starts should
 * increase this value by 1 (so if the thread with ID tid is in RUNNING state
 * when this function is called, include also the current quantum). If no
 * thread with ID tid exists it is considered as an error.
 * Return value: On success, return the number of quantums of the thread
 * with ID tid.
 * On failure, return -1.
*/
int uthread_get_quantums(int tid)
{
    if (sigprocmask(SIG_BLOCK, set,NULL) < 0)
    {
        printf("system error: text\n");
    }

    int returnVal = uthreadLib->uthreadVector[tid]->numberOfQuantsThreadRan;

    if (tid < 0 || uthreadLib->uthreadVector[tid] == nullptr ||
            tid >= MAX_THREAD_NUM )
    {
        if (sigprocmask(SIG_UNBLOCK, set,NULL) < 0)
        {
            printf("system error: text\n");
        }
        return ERROR_VALUE;
    }
    if (who_is_running == tid)
    {

        if (sigprocmask(SIG_UNBLOCK, set,NULL) < 0)
        {
            printf("system error: text\n");
        }

        return returnVal;
    }
    if (sigprocmask(SIG_UNBLOCK, set,NULL) < 0)
    {
        printf("system error: text\n");
    }
    return returnVal;

}
