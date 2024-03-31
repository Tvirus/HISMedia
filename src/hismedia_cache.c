#include "hismedia_cache.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
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
    int cache_id;
    stream_cb_t cb; /* 用来判断是否在用 */
    void *cb_arg;
    int should_run;
    unsigned int head;
    unsigned int tail; /* 永远指向空节点 */
    frame_t* ring_buf[MAX_CACHE_CNT];
}stream_cb_info_t;

typedef struct
{
    stream_id_t stream_id;
    int used;
    pthread_spinlock_t spin; /* 除receive_stream外，锁定前先拥有media_cache_mutex */
    pthread_mutex_t mutex; /* 条件变量用 */
    pthread_cond_t cond;
    int cb_cnt;
    stream_cb_info_t cb_info[MAX_CALLBACK_CNT];
}stream_cache_t;


extern unsigned char media_debug;
static unsigned int stream_count = 0;
static stream_cache_t *stream_cache_list = NULL;
static pthread_mutex_t media_cache_mutex = PTHREAD_MUTEX_INITIALIZER;
static int media_cache_inited = 0; /* -1:退出失败，0:未初始化，1:已初始化 */




int hism_cache_init(unsigned int max_stream_count)
{
    pthread_condattr_t attr;
    int i;


    if (0 == max_stream_count)
        return -1;

    pthread_mutex_lock(&media_cache_mutex);

    if (0 != media_cache_inited)
    {
        pthread_mutex_unlock(&media_cache_mutex);
        return -1;
    }
    stream_cache_list = calloc(max_stream_count, sizeof(stream_cache_t));
    if (NULL == stream_cache_list)
    {
        ERROR("Failed to calloc %zu !", sizeof(stream_cache_t) * max_stream_count);
        return -1;
    }
    stream_count = max_stream_count;

    pthread_condattr_init(&attr);
    pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
    for (i = 0; i < max_stream_count; i++)
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

int hism_cache_exit(void)
{
    int i, j, t;
    int ret;


    pthread_mutex_lock(&media_cache_mutex);

    if (0 >= media_cache_inited)
    {
        ret = media_cache_inited;
        pthread_mutex_unlock(&media_cache_mutex);
        return ret;
    }

    for (i = 0; i < stream_count; i++)
    {
        if (!stream_cache_list[i].used)
            continue;

        pthread_spin_lock(&stream_cache_list[i].spin);
        for (j = 0; j < MAX_CALLBACK_CNT; j++)
        {
            if (NULL == stream_cache_list[i].cb_info[j].cb)
                continue;
            stream_cache_list[i].cb_info[j].should_run = 0;
        }
        pthread_spin_unlock(&stream_cache_list[i].spin);

    }
    for (t = 0; t < 40; t++)
    {
        usleep(300 * 1000);
        for (i = 0; i < stream_count; i++)
        {
            if (!stream_cache_list[i].used)
                continue;
            for (j = 0; j < MAX_CALLBACK_CNT; j++)
            {
                if (stream_cache_list[i].cb_info[j].cb)
                    break;
            }
            if (MAX_CALLBACK_CNT > j)
                break;
        }
        if (stream_count <= i)
            break;
    }
    if (40 <= t)
    {
        ERROR("media cache exit timeout !");
        media_cache_inited = -1;
        pthread_mutex_unlock(&media_cache_mutex);
        return -1;
    }

    for (i = 0; i < stream_count; i++)
    {
        pthread_spin_destroy(&stream_cache_list[i].spin);
        pthread_mutex_destroy(&stream_cache_list[i].mutex);
        pthread_cond_destroy(&stream_cache_list[i].cond);
    }
    free(stream_cache_list);
    stream_cache_list = NULL;
    media_cache_inited = 0;

    pthread_mutex_unlock(&media_cache_mutex);

    return 0;
}


/* 返回stream_cache_id */
int hism_register_stream_id(stream_id_t id)
{
    int i;

    pthread_mutex_lock(&media_cache_mutex);

    if (0 >= media_cache_inited)
    {
        pthread_mutex_unlock(&media_cache_mutex);
        return -1;
    }

    for (i = 0; i < stream_count; i++)
    {
        if (    (!stream_cache_list[i].used)
             || (!memcmp(&id, &stream_cache_list[i].stream_id, sizeof(stream_id_t))))
        {
            stream_cache_list[i].stream_id = id;
            stream_cache_list[i].used = 1;
            pthread_mutex_unlock(&media_cache_mutex);
            return i;
        }
    }

    pthread_mutex_unlock(&media_cache_mutex);

    return -1;
}


#define COND_TIMEOUT 1
static void* receive_stream(void *arg)
{
    stream_cb_info_t *cb_info = (stream_cb_info_t *)arg;
    unsigned int *head;
    unsigned int *tail;
    stream_cache_t *stream;
    struct timespec time;


    pthread_detach(pthread_self());
    prctl(PR_SET_NAME, (unsigned long)"receive_stream");

    if (NULL == cb_info)
        return NULL;
    head = &cb_info->head;
    tail = &cb_info->tail;
    stream = &stream_cache_list[cb_info->cache_id];

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
            hism_free_frame(cb_info->ring_buf[*head]);

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
    int i, cache_id;


    if (NULL == cb || NULL == cb_handle)
        return -1;

    pthread_mutex_lock(&media_cache_mutex);

    if (0 >= media_cache_inited)
    {
        pthread_mutex_unlock(&media_cache_mutex);
        return -1;
    }

    for (i = 0; i < stream_count; i++)
    {
        if (   stream_cache_list[i].used
            && !memcmp(&id, &stream_cache_list[i].stream_id, sizeof(stream_id_t)))
            break;
    }
    if (stream_count <= i)
    {
        ERROR("stream[%u,%u,%u] is not registered !", id.type, id.major, id.minor);
        pthread_mutex_unlock(&media_cache_mutex);
        return -1;
    }
    cache_id = i;
    stream = &stream_cache_list[i];
    pthread_spin_lock(&stream->spin);
    pthread_mutex_unlock(&media_cache_mutex);

    for (i = 0; i < MAX_CALLBACK_CNT; i++)
    {
        if (NULL == stream->cb_info[i].cb)
            break;
    }
    if (MAX_CALLBACK_CNT <= i)
    {
        ERROR("stream[%u,%u,%u] callback is full, max:%u !", id.type, id.major, id.minor, MAX_CALLBACK_CNT);
        pthread_spin_unlock(&stream->spin);
        return -1;
    }
    cb_info = &stream->cb_info[i];
    cb_info->stream_id = id;
    cb_info->cache_id = cache_id;
    cb_info->cb_arg = arg;
    cb_info->should_run = 1;
    cb_info->head = 0;
    cb_info->tail = 0;
    cb_info->cb = cb;
    stream->cb_cnt++;

    pthread_spin_unlock(&stream->spin);

    pthread_create(&tid, NULL, receive_stream, (void *)cb_info);
    *cb_handle = (void *)cb_info;

    return 0;
}

int hism_delete_stream_cb(void *cb_handle)
{
    stream_cb_info_t *cb_info = (stream_cb_info_t *)cb_handle;
    stream_cache_t *stream;


    if (NULL == cb_info)
        return -1;

    pthread_mutex_lock(&media_cache_mutex);

    if (0 >= media_cache_inited)
    {
        pthread_mutex_unlock(&media_cache_mutex);
        return -1;
    }
    if ((0 > cb_info->cache_id) || (stream_count <= cb_info->cache_id))
    {
        pthread_mutex_unlock(&media_cache_mutex);
        return -1;
    }
    stream = &stream_cache_list[cb_info->cache_id];
    if ((!stream->used) || memcmp(&stream->stream_id, &cb_info->stream_id, sizeof(stream_id_t)))
    {
        pthread_mutex_unlock(&media_cache_mutex);
        return -1;
    }

    pthread_spin_lock(&stream->spin);
    pthread_mutex_unlock(&media_cache_mutex);
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
int hism_free_frame(frame_t *frame)
{
    if (NULL == frame)
        return -1;

    free(frame);

    return 0;
}


/* 返回失败后由调用者释放资源 */
int hism_put_stream_frame(frame_t *frame)
{
    stream_cache_t *stream;
    stream_cb_info_t *cb_info;
    unsigned int head;
    unsigned int *tail;
    int i, cb_cnt;


    if (NULL == frame)
        return -1;

    pthread_mutex_lock(&media_cache_mutex);

    if (0 >= media_cache_inited || 0 > frame->cache_id || stream_count <= frame->cache_id)
    {
        pthread_mutex_unlock(&media_cache_mutex);
        return -1;
    }
    stream = &stream_cache_list[frame->cache_id];
    if (memcmp(&stream->stream_id, &frame->stream_id, sizeof(stream_id_t)))
    {
        pthread_mutex_unlock(&media_cache_mutex);
        return -1;
    }

    pthread_spin_lock(&stream->spin);
    pthread_mutex_unlock(&media_cache_mutex);

    if (0 == stream->cb_cnt)
    {
        pthread_spin_unlock(&stream->spin);
        hism_free_frame(frame);
        return 0;
    }

    frame->ref_cnt = 0;
    cb_cnt = 0;
    for (i = 0; i < MAX_CALLBACK_CNT && cb_cnt < stream->cb_cnt; i++)
    {
        cb_info = &stream->cb_info[i];
        if (NULL == cb_info->cb)
            continue;
        cb_cnt++;

        head = cb_info->head;
        tail = &cb_info->tail;
        if (((*tail) + 1 == head) || ((0 == head) && (MAX_CACHE_CNT - 1 == *tail)))
        {
            //ERROR("stream[%u,%u,%u] cb[%d] is blocked !",
                     //frame->stream_id.type, frame->stream_id.major, frame->stream_id.minor, i);
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
        hism_free_frame(frame);
        pthread_spin_unlock(&stream->spin);
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

    pthread_mutex_lock(&media_cache_mutex);

    if (0 >= media_cache_inited || 0 > frame->cache_id || stream_count <= frame->cache_id)
    {
        pthread_mutex_unlock(&media_cache_mutex);
        return -1;
    }
    stream = &stream_cache_list[frame->cache_id];
    if (memcmp(&stream->stream_id, &frame->stream_id, sizeof(stream_id_t)))
    {
        pthread_mutex_unlock(&media_cache_mutex);
        return -1;
    }

    pthread_spin_lock(&stream->spin);
    pthread_mutex_unlock(&media_cache_mutex);
    frame->ref_cnt--;
    if (0 == frame->ref_cnt)
        hism_free_frame(frame);
    pthread_spin_unlock(&stream->spin);

    return 0;
}


void hism_print_cache_status(void)
{
    stream_cache_t *stream;
    stream_cb_info_t *cb_info;
    unsigned int head, used;
    int i, j;


    pthread_mutex_lock(&media_cache_mutex);

    if (0 >= media_cache_inited)
    {
        pthread_mutex_unlock(&media_cache_mutex);
        return;
    }

    for (i = 0; i < stream_count; i++)
    {
        stream = &stream_cache_list[i];
        if ((!stream->used) || (0 == stream->cb_cnt))
            continue;

        printf("========== stream[%u,%u,%u] cache status ========\n",
                stream->stream_id.type, stream->stream_id.major, stream->stream_id.minor);
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
