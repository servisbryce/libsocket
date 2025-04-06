#ifndef THREAD_POOL_H_
#define THREAD_POOL_H

#include <pthread.h>
#include <stdbool.h>

typedef struct thread_work {

    struct thread_work *next;
    void *routine_vargs;
    void *(*routine)(void *routine_vargs);

} thread_work_t;

typedef struct thread_pool {

    pthread_mutex_t thread_work_mutex;
    pthread_cond_t thread_working_condition;
    pthread_cond_t thread_work_condition;
    thread_work_t *thread_work_head;
    thread_work_t *thread_work_tail;
    size_t thread_working_count;
    size_t thread_worker_count;
    bool halt;

} thread_pool_t;

thread_pool_t *thread_pool_create(size_t threads);
int thread_pool_assign_work(thread_pool_t *thread_pool, void *(*routine)(void *vargs), void *routine_vargs);
int thread_pool_increment_threads(thread_pool_t *thread_pool, size_t threads);
int thread_pool_wait(thread_pool_t *thread_pool);
int thread_pool_destroy(thread_pool_t *thread_pool);

#endif