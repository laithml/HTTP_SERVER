//
// Created by Laith on 26/12/2022.
//

#include <stdlib.h>
#include <stdio.h>
#include "threadpool.h"

threadpool *create_threadpool(int num_threads_in_pool) {
    if (num_threads_in_pool <= 0 || num_threads_in_pool > MAXT_IN_POOL) {
        return NULL;
    }
    threadpool *tp = malloc(sizeof(threadpool));
    if (tp == NULL) {
        perror("malloc threadpool");
        return NULL;
    }
    tp->num_threads = num_threads_in_pool;
    tp->qsize = 0;
    tp->qhead = NULL;
    tp->qtail = NULL;
    tp->shutdown = 0;
    tp->dont_accept = 0;

    tp->threads = malloc(sizeof(pthread_t) * num_threads_in_pool);
    if (tp->threads == NULL) {
        perror("malloc threads");
        free(tp);
        return NULL;
    }

    //initialize the mutex and condition for the thread pool
    if (pthread_cond_init(&(tp->q_empty), NULL) != 0) {
        perror("mutex init failed");
        free(tp->threads);
        free(tp);
        return NULL;
    }
    if (pthread_cond_init(&(tp->q_not_empty), NULL) != 0) {
        perror("mutex init failed");
        free(tp->threads);
        free(tp);
        return NULL;
    }
    if (pthread_mutex_init(&(tp->qlock), NULL) != 0) {
        perror("mutex init failed");
        free(tp->threads);
        free(tp);
        return NULL;
    }

    //create all the threads
    for (int i = 0; i < num_threads_in_pool; i++) {
        if (pthread_create(&(tp->threads[i]), NULL, do_work, tp) != 0) {
            perror("thread create failed");
            destroy_threadpool(tp);
            return NULL;
        }
    }

    return tp;
}

void dispatch(threadpool *from_me, dispatch_fn dispatch_to_here, void *arg) {
    if (from_me->dont_accept == 1) {
        fprintf(stderr, "cant accept more works");
        return;
    }


    //1. create and init work_t element
    //TODO:FREE the allocated memory
    work_t *wt = malloc(sizeof(work_t));
    if (wt == NULL) {
        perror("malloc of work_t failed");
        return;
    }
    wt->routine = dispatch_to_here;
    wt->arg = arg;
    wt->next = NULL;

    //2. lock the mutex

    if (pthread_mutex_lock(&(from_me->qlock)) != 0) {
        perror("mutex lock failed");
        free(wt);
        return;
    }

    //3. add the work_t element to the queue
    if (from_me->qhead == NULL) {
        from_me->qhead = wt;
        from_me->qtail = wt;
    } else {
        from_me->qtail->next = wt;
        from_me->qtail = wt;
    }
    from_me->qsize++;

    //4. unlock mutex
    if (pthread_mutex_unlock(&(from_me->qlock)) != 0) {
        perror("mutex lock failed");
        free(wt);
        return;
    }

    pthread_cond_signal(&(from_me->q_not_empty));


}

void *do_work(void *p) {
    threadpool *tp = (threadpool *) p;
    while (1) {
        //1. lock mutex
        if (pthread_mutex_lock(&(tp->qlock)) != 0) {
            perror("mutex lock failed");
            destroy_threadpool(tp);
            pthread_exit(NULL);
        }

        //2. if the queue is empty, wait

        if(tp->qsize==0) {
            if (pthread_cond_wait(&(tp->q_not_empty), &(tp->qlock))) {
                perror("cond(queue not empty) wait failed");
                pthread_mutex_unlock(&(tp->qlock));
                pthread_exit(NULL);
            }
        }

        //if the pool is shutting exit the thread
        if (tp->shutdown != 0) {
            printf("is shutting\n");
            pthread_mutex_unlock(&(tp->qlock));
            pthread_exit(NULL);
        }

        // 3. take the first element from the queue (work_t)
        work_t *work = tp->qhead;
        tp->qsize--;
        if (tp->qsize == 0)
            tp->qhead = tp->qtail = NULL;
        else
            tp->qhead = work->next;

        //4. unlock mutex

        if (pthread_mutex_unlock(&(tp->qlock)) != 0) {
            perror("mutex unlock failed");
            pthread_exit(NULL);
        }

        //5. call the thread routine
        int status = work->routine(work->arg);

        free(work);
        if (status != 0) {
            fprintf(stderr, "WORK THREAD FAILED\n");
        }
    }
}

void destroy_threadpool(threadpool *destroyme) {
    int i = 0;
    destroyme->shutdown = 1;

    //alert all threads who are currently waiting in the queue
    if (pthread_cond_broadcast(&(destroyme->q_not_empty)) != 0) {
        perror("cond broadcast failed");
    }

    //wait for all threads to finish
    while (i < destroyme->num_threads) {
        if (pthread_join(destroyme->threads[i], NULL) != 0)
            perror("pthread_join failed");
        i++;
    }

    //destroy mutex and conditions
    if (pthread_mutex_destroy(&(destroyme->qlock)) != 0)
        perror("mutex destroy failed");
    if (pthread_cond_destroy(&(destroyme->q_not_empty)) != 0)
        perror("cond destroy failed");
    if (pthread_cond_destroy(&(destroyme->q_empty)) != 0)
        perror("cond destroy failed");

    free(destroyme->threads);
    free(destroyme);

}