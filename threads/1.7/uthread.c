#include "uthread.h"

static _uthread_info* thread_list[UTHREAD_MAXTHREADNUM] = { NULL };
static int thread_list_size = 1;
static int current_thread = MAIN_THREAD_TID;
static volatile int thread_list_sync = 0;

static int init_uthread();
static _uthread_info* init_thread(uthread_routine routine, void* arg);
static int finalize_thread(_uthread_info* thread);
static int put_thread(_uthread_info* thread, uthread_t* tid);
static void *start_routine(void* arg);
static void schedule();

#if UTHREAD_YIELD_MODE == TIMED
static int init_timed_yielding();
#endif

// static int garbage_collector();

static int is_initialized = 0;
int uthread_create(uthread_t *tid, uthread_routine routine, void *arg)
{
    int err;

    if (!is_initialized)
    {
        err = init_uthread();
        if (err == -1)
        {
            // perror
            abort();
        }
    }

    garbage_collector();

    while (!__sync_bool_compare_and_swap(&thread_list_sync, 0, 1))
    {
        // uthread_yield();
    }

    _uthread_info* thread = init_thread(routine, arg);
    if (thread == _INIT_THREAD_ERROR)
    {
        // perror
        return -1;
    }

    err = put_thread(thread, tid);
    if (err == -1)
    {
        // perror
        return -1;
    }

    __sync_bool_compare_and_swap(&thread_list_sync, 1, 0);

    return 0;
}


int uthread_join(uthread_t tid, void **retval)
{
    if (tid == MAIN_THREAD_TID)
    {
        // perror
        return -1;
    }

    _uthread_info* thread = thread_list[tid];
    while (!thread->is_completed)
    {
        // uthread_yield();
    }

    *retval = thread->retval;

    thread->is_joined = 1;

    garbage_collector();

    return 0;
}

void uthread_yield()
{
#if UTHREAD_YIELD_MODE == TIMED
    // raise(SIGALRM);
#else
    schedule();
#endif
}

uthread_t uthread_self()
{
    return current_thread;
}

int init_uthread()
{
    thread_list[MAIN_THREAD_TID] = (_uthread_info*)malloc(sizeof(_uthread_info));
    if (thread_list[MAIN_THREAD_TID] == NULL)
    {
        // perror
        return -1;
    }

    _uthread_info* main_thread = thread_list[MAIN_THREAD_TID];
    main_thread->tid = MAIN_THREAD_TID;
    int err;
    err = getcontext(&main_thread->ctx);
    if (err == -1)
    {
        // perror
        return -1;
    }

#if UTHREAD_YIELD_MODE == TIMED
    err = init_timed_yielding();
    if (err == -1)
    {
        // perror
        return -1;
    }
#endif

    return 0;
}

#if UTHREAD_YIELD_MODE == TIMED
int init_timed_yielding()
{
    // struct sigaction action;
    // action.sa_sigaction = schedule;
    // sigemptyset(&action.sa_mask);
    // sigaddset(&action.sa_mask, SIGALRM);
    // sigaction(SIGALRM, &action, NULL);

    struct sigaction action;
    memset(&action, 0, sizeof(action));
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGALRM);

    action.sa_mask = set;
    action.sa_handler = schedule;

    sigaction(SIGALRM, &action, NULL);

    struct itimerval interval;
    interval.it_value.tv_sec = 0;
    interval.it_value.tv_usec = UTHREAD_TIMED_YIELDING_DELAY;
    interval.it_interval = interval.it_value;
    setitimer(ITIMER_REAL, &interval, NULL);

    // alarm(1); //!!!!

    return 0;
}
#endif

_uthread_info *init_thread(uthread_routine routine, void *arg)
{
    _uthread_info* thread = (_uthread_info*)malloc(sizeof(_uthread_info));
    if (thread == NULL)
    {
        // perror
        return _INIT_THREAD_ERROR;
    }

    thread->arg = arg;
    thread->routine = routine;
    thread->is_completed = 0;
    thread->is_joined = 0;
    thread->retval = NULL;
    int err;
    err = getcontext(&thread->ctx);
    if (err == -1)
    {
        // perror
        return _INIT_THREAD_ERROR;
    }
    // thread->ctx.uc_link = &thread_list[MAIN_THREAD_TID]->ctx;
    thread->ctx.uc_link = NULL;
    thread->ctx.uc_stack.ss_size = STACK_SIZE;
    thread->ctx.uc_stack.ss_sp = mmap(NULL, STACK_SIZE, PROT_READ | PROT_WRITE,
                                        MAP_ANONYMOUS | MAP_STACK | MAP_PRIVATE, -1, 0);
    if (thread->ctx.uc_stack.ss_sp == MAP_FAILED)
    {
        // perror
        return _INIT_THREAD_ERROR;
    }
    makecontext(&thread->ctx, (void (*)()) start_routine, 1, thread);

    return thread;
}

int finalize_thread(_uthread_info *thread)
{
    int err;
    err = munmap(thread->ctx.uc_stack.ss_sp, STACK_SIZE);
    if (err == -1)
    {
        // perror
        return -1;
    }

    free(thread);

    return 0;
}

int put_thread(_uthread_info *thread, uthread_t *tid)
{
    for (int i = 0; i < UTHREAD_MAXTHREADNUM; i++)
    {
        if (thread_list[i] != NULL)
            continue;

        *tid = i;
        thread->tid = i;
        thread_list[i] = thread;
        return 0;
    }

    // perror (eagain)
    return -1;
}

void *start_routine(void* arg)
{
    _uthread_info* thread = (_uthread_info*)arg;
    thread->retval = thread->routine(thread->arg);

    thread->is_completed = 1;

    while (!thread->is_joined)
    {
        // uthread_yield();
    }

    // return;
    while (1)
    {
        // uthread_yield();
    }

    return NULL;
}

void schedule()
{
    _uthread_info* current = thread_list[current_thread];
    for (int i = 0; i < UTHREAD_MAXTHREADNUM; i++)
    {
        current_thread = (current_thread + 1) % UTHREAD_MAXTHREADNUM;
        if (thread_list[current_thread] != NULL)
            break;
    }
    _uthread_info* next = thread_list[current_thread];
    if (current == next)
    {
        return;
    }

    // alarm(1); //!!!

    int err = swapcontext(&current->ctx, &next->ctx);
    if (err == -1)
    {
        // perror
        abort();
    }

    return;
}

int garbage_collector()
{
    while (!__sync_bool_compare_and_swap(&thread_list_sync, 0, 1))
    {
        // uthread_yield();
    }

    for (int i = 0; i < UTHREAD_MAXTHREADNUM; i++)
    {
        _uthread_info* thread = thread_list[i];
        if (thread == NULL || i == MAIN_THREAD_TID)
            continue;

        if (thread->is_joined)
        {
            int err = finalize_thread(thread);
            if (err == -1)
            {
                // perror
                abort();
            }
            thread_list[i] = NULL;
        }
    }

    __sync_bool_compare_and_swap(&thread_list_sync, 1, 0);

    return 0;
}