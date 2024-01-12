#include "mythread.h"

static sem_t tinfo_list_sync;
static _mythread_info* tinfo_list[MYTHREAD_MAXTHREADNUM] = { NULL };
static size_t tinfo_list_size = 0;
/*  Thread localc storage - current thread info  */
static __thread _mythread_info* tls_tinfo;

static _mythread_info* init_thread(mythread_t *tid, mythread_attr_t *attr, mythread_routine routine, void *arg);
static int finalize_thread(_mythread_info* thread);
#ifdef USE_MAP_FOR_STACK
static int garbage_collector();
#endif

static int routine_wrapper(void *arg);
static mythread_t get_awaiable_thread_space();
static int initialize_mythread();

static int is_mythread_initialized = 0;
int mythread_create(mythread_t *tid, mythread_attr_t *attr, mythread_routine routine, void *arg)
{
    int err;

    if (!is_mythread_initialized)
    {
        err = initialize_mythread();
        if (err == -1)
        {
            fprintf(stderr, "mythread_create failed: in initialize_mythread call\n");
            return -1;
        }
    }

#ifdef USE_MAP_FOR_STACK
    garbage_collector();
#endif

    _mythread_info* new_thread = init_thread(tid, attr, routine, arg);
    if (new_thread == _INIT_THREAD_ERROR)
    {
        fprintf(stderr, "mythread_create failed: in init_thread call\n");
        return -1;
    }

    err = clone(routine_wrapper, new_thread->stack + MYTHREAD_STACK_SIZE,
                    CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND | CLONE_THREAD,
                    new_thread);
    if (err == -1)
    {
        fprintf(stderr, "mythread_create failed: in clone call: %s\n", strerror(errno));
        err = finalize_thread(new_thread);
        if (err == -1)
        {
            fprintf(stderr, "mythread_create failed: in finalize_thread call\n");
        }
        return -1;
    }

    return 0;
}

int mythread_join(mythread_t tid, void **retval)
{
    if (tid > MYTHREAD_MAXTHREADNUM || tid < 0)
    {
        fprintf(stderr, "mythread_join failed: wrong tid\n");
        return -1;
    }
    if (tinfo_list[tid] == NULL)
    {
        fprintf(stderr, "mythread_join failed: no such thread\n");
        return -1;
    }

    int err;
    _mythread_info* thread = tinfo_list[tid];

    if (!thread->is_joinable)
    {
        fprintf(stderr, "mythread_join failed: thread is not joinable\n");
        return -1;
    }

    err = sem_wait(&thread->is_completed);
    if (err == -1)
    {
        fprintf(stderr, "mythread_join failed: sem_wait\n");
        return -1;
    }

    *retval = thread->retval;

    err = sem_post(&thread->is_joined);
    if (err == -1)
    {
        fprintf(stderr, "mythread_join failed: sem_post\n");
        return -1;
    }

#ifdef USE_MAP_FOR_STACK
    garbage_collector();
#endif

    return 0;
}

mythread_t mythread_self()
{
#ifdef USE_MAP_FOR_STACK
    garbage_collector();
#endif

    if (tls_tinfo == _MAIN_THREAD)
        return (mythread_t)MYTHREAD_MAXTHREADNUM;

    return tls_tinfo->tid;
}

int mythread_attr_init(mythread_attr_t *attr)
{
#ifdef USE_MAP_FOR_STACK
    garbage_collector();
#endif

    attr->is_joinable = 1;

    return 0;
}

int mythread_attr_setdetachstate(mythread_attr_t *attr, int detachstate)
{
#ifdef USE_MAP_FOR_STACK
    garbage_collector();
#endif

    if (detachstate != MYTHREAD_CREATE_DETACHED && detachstate != MYTHREAD_CREATE_JOINABLE)
    {
        fprintf(stderr, "mythread_attr_setdetachstate failed: wrong detachstate\n");
        return -1;
    }

    attr->is_joinable = detachstate;

    return 0;
}

_mythread_info *init_thread(mythread_t *tid, mythread_attr_t *attr, mythread_routine routine, void *arg)
{
    _mythread_info* thread;
    mythread_t awaiable_tid = get_awaiable_thread_space();
    if (awaiable_tid == -1)
    {
        fprintf(stderr, "init_thread failed: no tids awaiable\n");
        return _INIT_THREAD_ERROR;
    }
    thread = tinfo_list[awaiable_tid];

    thread->tid = awaiable_tid;
    thread->arg = arg;
    /*  Change malloc to mmap  */
#ifdef USE_MAP_FOR_STACK
    thread->stack = mmap(NULL, MYTHREAD_STACK_SIZE, PROT_READ | PROT_WRITE,
                        MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK,
                        -1, 0);
    if (thread->stack == MAP_FAILED)
    {
        fprintf(stderr, "init_thread failed: can not map memory for stack\n");
        return _INIT_THREAD_ERROR;
    }
#elif defined(USE_HEAP_FOR_STACK)
    thread->stack = malloc(MYTHREAD_STACK_SIZE);
    if (thread->stack == NULL)
    {
        fprintf(stderr, "init_thread failed: can not allocate memory for stack\n");
        return _INIT_THREAD_ERROR;
    }
#endif
    thread->retval = NULL;
    thread->routine = routine;
    if (attr != NULL)
    {
        thread->is_joinable = attr->is_joinable;
    }
    else
    {
        thread->is_joinable = 1;
    }

    int err;
    err = sem_init(&thread->is_completed, 0, 0);
    if (err == -1)
    {
        fprintf(stderr, "init_thread failed: sem_init\n");
        return _INIT_THREAD_ERROR;
    }
    if (thread->is_joinable)
    {
        err = sem_init(&thread->is_joined, 0, 0);
        if (err == -1)
        {
        fprintf(stderr, "init_thread failed: sem_init\n");
            return _INIT_THREAD_ERROR;
        }
    }

    return thread;
}

int finalize_thread(_mythread_info *thread)
{
    if (thread == NULL)
    {
        fprintf(stderr, "finalize_thread failed: thread is NULL pointer\n");
        return -1;
    }

    int err;
    err = sem_destroy(&thread->is_completed);
    if (err == -1)
    {
        fprintf(stderr, "finalize_thread failed: sem_destroy\n");
        return -1;
    }
    if (thread->is_joinable)
    {
        err = sem_destroy(&thread->is_joined);
        if (err == -1)
        {
        fprintf(stderr, "finalize_thread failed: sem_destroy\n");
            return -1;
        }
    }

    void* stack = thread->stack;
    mythread_t tid = thread->tid;
    free(tinfo_list[tid]);
    tinfo_list[tid] = NULL;
#ifdef USE_MAP_FOR_STACK
    err = munmap(stack, MYTHREAD_STACK_SIZE);
    if (err == -1)
    {
        fprintf(stderr, "finalize_thread failed: can not unmap stack\n");
        return -1;
    }
#elif defined(USE_HEAP_FOR_STACK)
    free(stack);
#endif
    return 0;
}

#ifdef USE_MAP_FOR_STACK
int garbage_collector()
{
    int err;
    err = sem_wait(&tinfo_list_sync);
    if (err == -1)
    {
        fprintf(stderr, "garbage_collector failed: sem_wait\n");
        abort();
    }

    size_t old_size = tinfo_list_size;
    for (int i = 0; i < old_size; i++)
    {
        if (tinfo_list[i] == NULL)
            continue;

        _mythread_info* thread = tinfo_list[i];
        if (thread->is_joinable)
        {
            int is_joined;
            err = sem_getvalue(&thread->is_joined, &is_joined);
            if (err == -1)
            {
                fprintf(stderr, "garbage_collector failed: sem_getvalue\n");
                abort();
            }
            if (is_joined)
            {
                err = finalize_thread(thread);
                if (err == -1)
                {
                    fprintf(stderr, "garbage_collector failed: in finalize_thread call\n");
                    abort();
                }
                tinfo_list_size = (i > tinfo_list_size - 1) ? (i + 1) : tinfo_list_size;
            }
        }
        else
        {
            int is_completed;
            err = sem_getvalue(&thread->is_completed, &is_completed);
            if (err == -1)
            {
                fprintf(stderr, "garbage_collector failed: sem_getvalue\n");
                abort();
            }
            if (is_completed)
            {
                err = finalize_thread(thread);
                if (err == -1)
                {
                    fprintf(stderr, "garbage_collector failed: in finalize_thread call\n");
                    abort();
                }
                tinfo_list_size = (i > tinfo_list_size - 1) ? (i + 1) : tinfo_list_size;
            }
        }
    }

    err = sem_post(&tinfo_list_sync);
    if (err == -1)
    {
        fprintf(stderr, "garbage_collector failed: sem_post\n");
        abort();
    }

    return 0;
}
#endif

int routine_wrapper(void *arg)
{
    _mythread_info* thread = (_mythread_info*)arg;
    tls_tinfo = thread;
    thread->retval = thread->routine(thread->arg);

    int err;
    err = sem_post(&thread->is_completed);
    if (err == -1)
    {
        fprintf(stderr, "routine_wrapper failed: sem_post\n");
        return -1;
    }

    if (thread->is_joinable)
    {
        err = sem_wait(&thread->is_joined);
        if (err == -1)
        {
            fprintf(stderr, "routine_wrapper failed: sem_wait\n");
            return -1;
        }
    }

#ifdef USE_HEAP_FOR_STACK
    err = finalize_thread(thread);
    if (err == -1)
    {
        fprintf(stderr, "routine_wrapper failed: in finalize_thread call\n");
        return -1;
    }
#endif
    return 0;
}

mythread_t get_awaiable_thread_space()
{
    int err;
    err = sem_wait(&tinfo_list_sync);
    if (err == -1)
    {
        fprintf(stderr, "get_awaiable_thread_space failed: sem_wait\n");
        return -1;
    }

    for (int i = 0; i < MYTHREAD_MAXTHREADNUM; i++)
    {
        if (tinfo_list[i] == NULL)
        {
            tinfo_list[i] = (_mythread_info*)malloc(sizeof(_mythread_info));
            sem_post(&tinfo_list_sync);
            if (tinfo_list[i] == NULL)
            {
                fprintf(stderr, "get_awaiable_thread_space failed: can not allocate space for thread info\n");
                return -1;
            }

            if (i > tinfo_list_size - 1)
            {
                tinfo_list_size = i + 1;
            }
            return i;
        }
    }

    sem_post(&tinfo_list_sync);
    fprintf(stderr, "get_awaiable_thread_space failed: maximum number of threads reached\n");
    return -1;
}

int initialize_mythread()
{
    int err = sem_init(&tinfo_list_sync, 0, 1);
    if (err == -1)
    {
        fprintf(stderr, "initialize_mythread failed: sem_init\n");
        return -1;
    }

    tls_tinfo = _MAIN_THREAD;
}
