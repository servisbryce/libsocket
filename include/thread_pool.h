#ifndef THREAD_POOL_H_
#define THREAD_POOL_H

#include <pthread.h>
#include <stdbool.h>

typedef struct thread_work {

    struct thread_work *next;
    void *routine_vargs;
    void *routine;

} thread_work_t;

typedef struct thread_pool {

    pthread_mutex_t thread_work_mutex;
    pthread_cond_t thread_working_condition;
    pthread_cond_t thread_work_condition;
    thread_work_t *thread_work_head;
    thread_work_t *thread_work_tail;
    size_t thread_working_condition_count;
    size_t thread_work_condition_count;
    bool halt;

} thread_pool_t;

#endif