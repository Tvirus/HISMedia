#ifndef _HISMEDIA_CACHE_H_
#define _HISMEDIA_CACHE_H_


typedef enum
{
    H264_NALU_P      = 1,
    H264_NALU_I      = 5,
    H264_NALU_SEI    = 6,
    H264_NALU_SPS    = 7,
    H264_NALU_PPS    = 8,
    H264_NALU_AUD    = 9,
    H264_NALU_PREFIX = 14,
    H264_NALU_MAX
}h264_frame_type_t;

typedef enum
{
    H265_NALU_P   = 1,
    H265_NALU_I   = 19,
    H265_NALU_VPS = 32,
    H265_NALU_SPS = 33,
    H265_NALU_PPS = 34,
    H265_NALU_SEI = 39,
    H265_NALU_MAX
}h265_frame_type_t;

typedef enum
{
    VIDEO_FORMAT_H264 = 0,
    VIDEO_FORMAT_H265,
    VIDEO_FORMAT_JPEG,
    VIDEO_FORMAT_MAX
}video_format_t;

typedef enum
{
    AUDIO_FORMAT_PCM = 0,
    AUDIO_FORMAT_PCMA,
    AUDIO_FORMAT_PCMU,
    AUDIO_FORMAT_AAC,
    AUDIO_FORMAT_MAX
}audio_format_t;

typedef enum
{
    STREAM_ID_V_START = 0,
    STREAM_ID_V_RGB_MAIN = STREAM_ID_V_START,
    STREAM_ID_V_RGB_SUB1,
    STREAM_ID_V_RGB_SUB2,
    STREAM_ID_V_IR_MAIN,
    STREAM_ID_V_IR_SUB1,
    STREAM_ID_V_IR_SUB2,
    STREAM_ID_V_MAX,
    STREAM_ID_A_START = STREAM_ID_V_MAX,
    STREAM_ID_A_MAIN = STREAM_ID_A_START,
    STREAM_ID_A_SUB,
    STREAM_ID_A_MAX,
    STREAM_ID_MAX = STREAM_ID_A_MAX
}stream_id_t;

typedef struct
{
    video_format_t format;
    unsigned int fps;
    unsigned int width;
    unsigned int height;
}video_frame_info_t;

typedef struct
{
    audio_format_t format;
    unsigned int channels;
    unsigned int sample_rate;
}audio_frame_info_t;

typedef struct
{
    stream_id_t stream_id;
    int ref_cnt;
    unsigned long long pts;
    union
    {
        video_frame_info_t video_info;
        audio_frame_info_t audio_info;
    };
    unsigned int size;
    unsigned char buf[0];
}frame_t;

typedef int (*stream_cb_t)(frame_t *frame, void *arg);


extern int hism_cache_init(void);
extern int hism_register_stream_cb(stream_id_t id, stream_cb_t cb, void **cb_handle, void *arg);
extern int hism_delete_stream_cb(void *cb_handle);
extern frame_t* hism_alloc_frame(unsigned int size);
extern int hism_put_stream_frame(frame_t *frame); /* 返回失败后由调用者释放资源 */
extern int hism_release_stream_frame(frame_t *frame);

extern void hism_print_cache_state(void);


#endif
