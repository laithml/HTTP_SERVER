#include "threadpool.h"
#include <stdlib.h>

/**
 * create_threadpool creates a fixed-sized thread
 * pool.  If the function succeeds, it returns a (non-NULL)
 * "threadpool", else it returns NULL.
 * this function should:
 * 1. input sanity check
 * 2. initialize the threadpool structure
 * 3. initialized mutex and conditional variables
 * 4. create the threads, the thread init function is do_work and its argument is the initialized threadpool.
 */
threadpool *create_threadpool(int num_threads_in_pool) {
    if (num_threads_in_pool <= 0 || num_threads_in_pool > MAXT_IN_POOL) {
        return NULL;
    }
    threadpool *pool = (threadpool *) malloc(sizeof(threadpool));
    if (pool == NULL) {
        return NULL;
    }
    pool->num_threads = num_threads_in_pool;
    pool->qsize = 0;
    pool->threads = (pthread_t *) malloc(sizeof(pthread_t) * num_threads_in_pool);
    if (pool->threads == NULL) {
        free(pool);
        return NULL;
    }
    pool->qhead = NULL;
    pool->qtail = NULL;
    if (pthread_mutex_init(&pool->qlock, NULL) != 0) {
        free(pool->threads);
        free(pool);
        return NULL;
    }
    if (pthread_cond_init(&pool->q_not_empty, NULL) != 0) {
        pthread_mutex_destroy(&pool->qlock);
        free(pool->threads);
        free(pool);
        return NULL;
    }
    if (pthread_cond_init(&pool->q_empty, NULL) != 0) {
        pthread_mutex_destroy(&pool->qlock);
        pthread_cond_destroy(&pool->q_not_empty);
        free(pool->threads);
        free(pool);
        return NULL;
    }
    pool->shutdown = 0;
    pool->dont_accept = 0;
    for (int i = 0; i < num_threads_in_pool; i++) {
        if (pthread_create(&pool->threads[i], NULL, do_work, pool) != 0) {
            pthread_mutex_destroy(&pool->qlock);
            pthread_cond_destroy(&pool->q_not_empty);
            pthread_cond_destroy(&pool->q_empty);
            free(pool->threads);
            free(pool);
            return NULL;
        }
    }
    return pool;
}
/**
 * dispatch enter a "job" of type work_t into the queue.
 * when an available thread takes a job from the queue, it will
 * call the function "dispatch_to_here" with argument "arg".
 * this function should:
 * 1. create and init work_t element
 * 2. lock the mutex
 * 3. add the work_t element to the queue
 * 4. unlock mutex
 *
 */
void dispatch(threadpool *from_me, dispatch_fn dispatch_to_here, void *arg) {
    //write me
    if (from_me == NULL || dispatch_to_here == NULL) {
        return;
    }
    pthread_mutex_lock(&from_me->qlock);
    if (from_me->dont_accept == 1) {
        pthread_mutex_unlock(&from_me->qlock);
        return;
    }
    work_t *work = (work_t *) malloc(sizeof(work_t));
    if (work == NULL) {
        pthread_mutex_unlock(&from_me->qlock);
        return;
    }
    work->routine = dispatch_to_here;
    work->arg = arg;
    work->next = NULL;
    if (from_me->qsize == 0) {
        from_me->qhead = work;
        from_me->qtail = work;
        pthread_cond_signal(&from_me->q_not_empty);
    } else {
        from_me->qtail->next = work;
        from_me->qtail = work;
    }
    from_me->qsize++;
    pthread_mutex_unlock(&from_me->qlock);
}
/**
 * destroy_threadpool kills the threadpool, causing
 * all threads in it to commit suicide, and then
 * frees all the memory associated with the threadpool.
 */
void destroy_threadpool(threadpool *destroyme) {
    //write me
    if (destroyme == NULL) {
        return;
    }
    pthread_mutex_lock(&destroyme->qlock);
    if (destroyme->dont_accept == 1) {
        pthread_mutex_unlock(&destroyme->qlock);
        return;
    }
    destroyme->dont_accept = 1;
    while (destroyme->qsize != 0) {
        pthread_cond_wait(&destroyme->q_empty, &destroyme->qlock);
    }
    destroyme->shutdown = 1;
    pthread_cond_broadcast(&destroyme->q_not_empty);
    pthread_mutex_unlock(&destroyme->qlock);
    for (int i = 0; i < destroyme->num_threads; i++) {
        pthread_join(destroyme->threads[i], NULL);
    }
    pthread_mutex_destroy(&destroyme->qlock);
    pthread_cond_destroy(&destroyme->q_not_empty);
    pthread_cond_destroy(&destroyme->q_empty);
    free(destroyme->threads);
    work_t *work = destroyme->qhead;
    while (work != NULL) {
        work_t *temp = work;
        work = work->next;
        free(temp);
    }
    free(destroyme);
}
/**
 * The work function of the thread
 * this function should:
 * 1. lock mutex
 * 2. if the queue is empty, wait
 * 3. take the first element from the queue (work_t)
 * 4. unlock mutex
 * 5. call the thread routine
 *
 */
void* do_work(void* p){
    threadpool *pool = (threadpool *) p;
    while (1) {
        pthread_mutex_lock(&pool->qlock);
        while (pool->qsize == 0 && pool->shutdown == 0) {
            pthread_cond_wait(&pool->q_not_empty, &pool->qlock);
        }
        if (pool->shutdown == 1) {
            pthread_mutex_unlock(&pool->qlock);
            pthread_exit(NULL);
        }
        work_t *work = pool->qhead;
        pool->qhead = pool->qhead->next;
        pool->qsize--;
        if (pool->qsize == 0) {
            pool->qtail = NULL;
            pthread_cond_signal(&pool->q_empty);
        }
        pthread_mutex_unlock(&pool->qlock);
        work->routine(work->arg);
        free(work);
    }
}

