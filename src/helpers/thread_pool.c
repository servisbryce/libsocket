#include "../include/thread_pool.h"

int thread_pool_assign_work(thread_pool_t *thread_pool, void (*routine)(void *vargs), void *routine_vargs) {

    if (!thread_pool || !routine || !routine_vargs) {

        return -1;

    }

    thread_work_t *work = (thread_work_t*) malloc(sizeof(thread_work_t));
    work->routine_vargs = routine_vargs;
    work->routine = routine;
    work->next = NULL;

    pthread_mutex_lock(&(thread_pool->thread_work_mutex));
    if (!thread_pool->thread_work_head) {

        thread_pool->thread_work_head = work;
        thread_pool->thread_work_tail = work;

    } else {

        thread_pool->thread_work_tail->next = work;
        thread_pool->thread_work_tail = work;

    }

    pthread_cond_broadcast(&(thread_pool->thread_work_condition));
    pthread_mutex_unlock(&(thread_pool->thread_work_mutex));
    return 0;

} 