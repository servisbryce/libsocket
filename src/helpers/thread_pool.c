#include "../../include/tls.h"
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

void *thread_worker(void *thread_worker_vargs) {

    thread_pool_t *thread_pool = (thread_pool_t*) thread_worker_vargs;
    while (1) {

        pthread_mutex_lock(&(thread_pool->thread_work_mutex));
        while (!thread_pool->halt && thread_pool->thread_work_head == NULL) {

            pthread_cond_wait(&(thread_pool->thread_work_condition), &(thread_pool->thread_work_mutex));

        }

        if (thread_pool->halt) {

            break;

        }

        thread_work_t *thread_work = thread_pool->thread_work_head;
        thread_pool->thread_work_head = thread_work->next;
        thread_pool->thread_working_count++;
        pthread_mutex_unlock(&(thread_pool->thread_work_mutex));
        if (thread_work) {

            tls_worker_vargs_t *tls_worker_vargs = (tls_worker_vargs_t *) thread_work->routine_vargs;
            if (!thread_work->routine((void *) tls_worker_vargs)) {

                SSL_shutdown(tls_worker_vargs->ssl);
                SSL_free(tls_worker_vargs->ssl);
                free(tls_worker_vargs);
                
            }

            free(thread_work);

        }

        pthread_mutex_lock(&(thread_pool->thread_work_mutex));
        thread_pool->thread_working_count--;
        if (!thread_pool->halt && thread_pool->thread_working_count == 0 && !thread_pool->thread_work_head) {

            pthread_cond_signal(&(thread_pool->thread_working_condition));

        }

        pthread_mutex_unlock(&(thread_pool->thread_work_mutex));

    }

    thread_pool->thread_worker_count--;
    pthread_cond_signal(&(thread_pool->thread_working_condition));
    pthread_mutex_unlock(&(thread_pool->thread_work_mutex));
    return NULL;

}

thread_pool_t *thread_pool_create(size_t threads) {

    if (threads <= 0) {

        return NULL;

    }

    thread_pool_t *thread_pool = (thread_pool_t*) malloc(sizeof(thread_pool_t));
    thread_pool->thread_worker_count = threads;
    pthread_mutex_init(&(thread_pool->thread_work_mutex), NULL);
    pthread_cond_init(&(thread_pool->thread_working_condition), NULL);
    pthread_cond_init(&(thread_pool->thread_work_condition), NULL);
    thread_pool->thread_work_head = NULL;
    thread_pool->thread_work_tail = NULL;
    thread_pool->halt = false;

    for (size_t i = 0; i < threads; i++) {

        pthread_t thread;
        pthread_create(&thread, NULL, thread_worker, (void *) thread_pool);
        pthread_detach(thread);

    }

    return thread_pool;

}

int thread_pool_assign_work(thread_pool_t *thread_pool, void *(*routine)(void *vargs), void *routine_vargs) {

    if (!thread_pool || thread_pool->halt) {

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

int thread_pool_wait(thread_pool_t *thread_pool) {

    if (!thread_pool) {

        return -1;

    }

    pthread_mutex_lock(&(thread_pool->thread_work_mutex));
    while (1) {

        if (thread_pool->thread_work_head || (!thread_pool->halt && thread_pool->thread_working_count != 0) || (thread_pool->halt && thread_pool->thread_worker_count != 0)) {

            pthread_cond_wait(&(thread_pool->thread_working_condition), &(thread_pool->thread_work_mutex));

        } else {

            break;

        }

    }

    pthread_mutex_unlock(&(thread_pool->thread_work_mutex));
    return 0;
    
}

int thread_pool_destroy(thread_pool_t *thread_pool) {

    if (!thread_pool) {

        return -1;

    }

    pthread_mutex_lock(&(thread_pool->thread_work_mutex));
    thread_work_t *previous = NULL;
    thread_work_t *current = thread_pool->thread_work_head;
    if (current) {

        while (current->next) {

            previous = current;
            current = current->next;
            free(previous);

        }

    }

    thread_pool->thread_work_head = NULL;
    thread_pool->thread_work_tail = NULL;
    thread_pool->halt = true;
    pthread_cond_broadcast(&(thread_pool->thread_work_condition));
    pthread_mutex_unlock(&(thread_pool->thread_work_mutex));
    thread_pool_wait(thread_pool);
    pthread_mutex_destroy(&(thread_pool->thread_work_mutex));
    pthread_cond_destroy(&(thread_pool->thread_work_condition));
    pthread_cond_destroy(&(thread_pool->thread_working_condition));
    free(thread_pool);
    return 0;

}