#include "hismedia_cache.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <sys/prctl.h>




#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})

#define DEBUG(fmt, arg...)  do{if(media_debug)printf("--MediaCache-- " fmt "\n", ##arg);}while(0)
#define ERROR(fmt, arg...)  printf("\e[1;31m--MediaCache-- %s: " fmt "\e[0m\n", __func__, ##arg)


#define MAX_CALLBACK_CNT 4
#define MAX_CACHE_CNT 32




typedef struct
{
    stream_id_t stream_id;
    int should_run;
    stream_cb_t cb; /* 用来判断是否在用 */
    void *cb_arg;
    unsigned int head;
    unsigned int tail; /* 永远指向空节点 */
    frame_t* ring_buf[MAX_CACHE_CNT];
}stream_cb_info_t;

typedef struct
{
    pthread_spinlock_t spin;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int cb_cnt;
    stream_cb_info_t cb_info[MAX_CALLBACK_CNT];
}stream_cache_t;


static stream_cache_t stream_cache_list[STREAM_ID_MAX];
static pthread_mutex_t media_cache_mutex = PTHREAD_MUTEX_INITIALIZER;
static int media_cache_inited = 0;




int hism_cache_init(void)
{
    int i;
    pthread_condattr_t attr;


    pthread_mutex_lock(&media_cache_mutex);
    if (media_cache_inited)
    {
        pthread_mutex_unlock(&media_cache_mutex);
        return 0;
    }

    memset(stream_cache_list, 0, sizeof(stream_cache_list));
    pthread_condattr_init(&attr);
    pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
    for (i = 0; i < STREAM_ID_MAX; i++)
    {
        pthread_spin_init(&stream_cache_list[i].spin, PTHREAD_PROCESS_PRIVATE);
        pthread_mutex_init(&stream_cache_list[i].mutex, NULL);
        pthread_cond_init(&stream_cache_list[i].cond, &attr);
    }
    pthread_condattr_destroy(&attr);

    media_cache_inited = 1;
    pthread_mutex_unlock(&media_cache_mutex);
    return 0;
}
/*
没有必要退出。
同时因为注册、放入流等过程中没用media_cache_mutex，所以没法保证资源释放后不再使用
int hism_cache_deinit(void)
{
    return 0;
}
*/


#define COND_TIMEOUT 1
static void* receive_stream(void *arg)
{
    stream_cb_info_t *cb_info;
    unsigned int *head;
    unsigned int *tail;
    stream_cache_t *stream;
    struct timespec time;


    pthread_detach(pthread_self());
    prctl(PR_SET_NAME, (unsigned long)"receive_stream");

    if (NULL == arg)
        return NULL;
    cb_info = (stream_cb_info_t *)arg;
    head = &cb_info->head;
    tail = &cb_info->tail;
    stream = &stream_cache_list[cb_info->stream_id];

    while (cb_info->should_run)
    {
        /* 为了简化实现，条件变量没有用锁，可能造成小概率延后一帧处理数据 */
        if (*head == *tail)
        {
            clock_gettime(CLOCK_MONOTONIC, &time);
            time.tv_sec += COND_TIMEOUT;
            pthread_mutex_lock(&stream->mutex);
            pthread_cond_timedwait(&stream->cond, &stream->mutex, &time);
            pthread_mutex_unlock(&stream->mutex);
            continue;
        }

        cb_info->cb(cb_info->ring_buf[*head], cb_info->cb_arg);
        (*head)++;
        if (MAX_CACHE_CNT <= *head)
            *head = 0;
    }

    pthread_spin_lock(&stream->spin);
    while (*head != *tail)
    {
        cb_info->ring_buf[*head]->ref_cnt--;
        if (0 == cb_info->ring_buf[*head]->ref_cnt)
            free(cb_info->ring_buf[*head]);

        (*head)++;
        if (MAX_CACHE_CNT <= *head)
            *head = 0;
    }
    stream->cb_cnt--;
    cb_info->cb = NULL;
    pthread_spin_unlock(&stream->spin);

    return NULL;
}
int hism_register_stream_cb(stream_id_t id, stream_cb_t cb, void **cb_handle, void *arg)
{
    stream_cache_t *stream;
    stream_cb_info_t *cb_info;
    pthread_t tid;
    int i;

    if (0 > id || STREAM_ID_MAX <= id)
        return -1;
    if (NULL == cb || NULL == cb_handle)
        return -1;

    stream = &stream_cache_list[id];
    pthread_spin_lock(&stream->spin);

    for (i = 0; i < MAX_CALLBACK_CNT; i++)
    {
        if (NULL == stream->cb_info[i].cb)
            break;
    }
    if (MAX_CALLBACK_CNT <= i)
    {
        pthread_spin_unlock(&stream->spin);
        ERROR("stream[%d] callback is full, max:%u !", id, MAX_CALLBACK_CNT);
        return -1;
    }

    cb_info = &stream->cb_info[i];
    cb_info->should_run = 1;
    cb_info->cb_arg = arg;
    cb_info->head = 0;
    cb_info->tail = 0;
    cb_info->cb = cb;
    stream->cb_cnt++;

    pthread_spin_unlock(&stream->spin);

    pthread_create(&tid, NULL, receive_stream, (void *)cb_info);
    *cb_handle = (void *)cb_info;

    return 0;
}

/* 无法彻底防止重复释放 */
int hism_delete_stream_cb(void *cb_handle)
{
    stream_cb_info_t *cb_info;
    stream_cache_t *stream;


    if (NULL == cb_handle)
        return -1;

    cb_info = (stream_cb_info_t *)cb_handle;
    if ((0 > cb_info->stream_id) || (STREAM_ID_MAX <= cb_info->stream_id))
        return -1;
    stream = &stream_cache_list[cb_info->stream_id];

    pthread_spin_lock(&stream->spin);
    if (NULL == cb_info->cb)
    {
        pthread_spin_unlock(&stream->spin);
        return -1;
    }
    cb_info->should_run = 0;
    pthread_spin_unlock(&stream->spin);

    return 0;
}


frame_t* hism_alloc_frame(unsigned int size)
{
    if (0 == size)
        return NULL;

    return (frame_t *)malloc(sizeof(frame_t) + size);
}


/* 返回失败后由调用者释放资源 */
int hism_put_stream_frame(frame_t *frame)
{
    stream_cache_t *stream;
    stream_cb_info_t *cb_info;
    unsigned int head;
    unsigned int *tail;
    int i;


    if (NULL == frame)
        return -1;
    if (0 > frame->stream_id || STREAM_ID_MAX <= frame->stream_id)
        return -1;

    stream = &stream_cache_list[frame->stream_id];
    pthread_spin_lock(&stream->spin);

    if (0 == stream->cb_cnt)
    {
        pthread_spin_unlock(&stream->spin);
        free(frame);
        return 0;
    }

    frame->ref_cnt = 0;
    for (i = 0; i < MAX_CALLBACK_CNT && frame->ref_cnt < stream->cb_cnt; i++)
    {
        cb_info = &stream->cb_info[i];
        if (NULL == cb_info->cb)
            continue;

        head = cb_info->head;
        tail = &cb_info->tail;
        if (((*tail) + 1 == head) || ((0 == head) && (MAX_CACHE_CNT - 1 == *tail)))
        {
            //ERROR("stream[%d] cb[%d] is blocked !", frame->stream_id, i);
            continue;
        }

        cb_info->ring_buf[*tail] = frame;
        __sync_synchronize();
        (*tail)++;
        if (MAX_CACHE_CNT <= *tail)
            *tail = 0;
        frame->ref_cnt++;
    }
    if (0 == frame->ref_cnt)
    {
        pthread_spin_unlock(&stream->spin);
        free(frame);
        return 0;
    }

    pthread_spin_unlock(&stream->spin);
    pthread_cond_broadcast(&stream->cond);

    return 0;
}

/* 无法彻底防止重复释放 */
int hism_release_stream_frame(frame_t *frame)
{
    stream_cache_t *stream;


    if (NULL == frame)
        return -1;
    if ((0 > frame->stream_id) || (STREAM_ID_MAX <= frame->stream_id))
        return -1;

    stream = &stream_cache_list[frame->stream_id];

    pthread_spin_lock(&stream->spin);
    frame->ref_cnt--;
    if (0 == frame->ref_cnt)
        free(frame);
    pthread_spin_unlock(&stream->spin);

    return 0;
}


void hism_print_cache_state(void)
{
    stream_cache_t *stream;
    stream_cb_info_t *cb_info;
    unsigned int head, used;
    int i, j;


    for (i = 0; i < STREAM_ID_MAX; i++)
    {
        stream = &stream_cache_list[i];
        if (0 == stream->cb_cnt)
            continue;

        printf("========== stream[%d] cache state ========\n", i);
        pthread_spin_lock(&stream->spin);
        for (j = 0; j < MAX_CALLBACK_CNT; j++)
        {
            cb_info = &stream->cb_info[j];
            if (NULL == cb_info->cb)
                continue;

            head = cb_info->head;
            if (head <= cb_info->tail)
                used = cb_info->tail - head;
            else
                used = MAX_CACHE_CNT + cb_info->tail - head;
            printf("cb[%d] cached: %u\n", j, used);
        }
        pthread_spin_unlock(&stream->spin);
    }
    printf("\n");
}
