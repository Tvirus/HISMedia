#ifndef _HISMEDIA_H_
#define _HISMEDIA_H_


#include "hismedia_cache.h"


typedef enum
{
    SINGLE_FRAME_ID_RGB_MAIN = 0,
    SINGLE_FRAME_ID_RGB_SUB,
    SINGLE_FRAME_ID_IR_MAIN,
    SINGLE_FRAME_ID_IR_SUB,
    SINGLE_FRAME_ID_MAX
}single_frame_id_t;
typedef struct
{
    unsigned long long pts;
    unsigned short width;
    unsigned short height;
    unsigned int stride;
    unsigned int size;
    unsigned char *buf;
    void *handle; /* 同时指示buf地址是否是从SDK获取的 */
}single_frame_t;


typedef enum
{
    COLOR_ID_BLACK = 0,
    COLOR_ID_WHITE,
    COLOR_ID_RED,
    COLOR_ID_GREEN,
    COLOR_ID_BLUE,
    COLOR_ID_YELLOW,
    COLOR_ID_MAX
}color_id_t;
typedef struct
{
    int x;
    int y;
    int w;
    int h;
}rectangle_t;
/* 相对矩形坐标，以1024为最大范围，[0,1024] */
#define REL_COORD_MAX 1024
typedef struct
{
    int x0;  /* 左上角 */
    int y0;
    int x1;  /* 右下角 */
    int y1;
}relative_rect_t;
typedef struct
{
    int x0;  /* 左上角 */
    int y0;
    int x1;  /* 右下角 */
    int y1;
    color_id_t color;
}graphic_rel_rect_t;

typedef struct
{
    unsigned int crop_x;
    unsigned int crop_y;
    unsigned int crop_w;
    unsigned int crop_h;
    unsigned int output_w;
    unsigned int output_h;
}resize_t;




typedef struct
{
    stream_id_t stream_id;
    video_format_t format;
    unsigned int width;
    unsigned int height;
    unsigned int fps;
}video_stream_info_t;
typedef struct
{
    stream_id_t stream_id;
    audio_format_t format;
    unsigned int channels;
    unsigned int sample_rate;
}audio_stream_info_t;


typedef enum
{
    OSD_ID_TIME = 0,
    OSD_ID_TITLE,
    OSD_ID_MAX
}osd_id_t;
typedef enum
{
    OSD_FORMAT_BIT2 = 0,
    OSD_FORMAT_BIT4,
    OSD_FORMAT_ARGB_1555,
    OSD_FORMAT_MAX
}osd_pixel_format_t;
typedef struct
{
    stream_id_t stream_id;
    osd_id_t osd_id;
    osd_pixel_format_t format;
    unsigned int width;
    unsigned int height;
}osd_info_t;




typedef int (*algo_motion_det_cb_t)(void);

typedef enum
{
    OBJECT_TYPE_UNKNOW = -1,
    OBJECT_TYPE_PERSON = 0,
    OBJECT_TYPE_CAR,
    OBJECT_TYPE_BUS,
    OBJECT_TYPE_TRUCK,
    OBJECT_TYPE_CAT,
    OBJECT_TYPE_DOG,
    OBJECT_TYPE_MAX
}algo_object_type_t;
typedef struct
{
    algo_object_type_t type;
    int track_id;
    int confidence; /* 0-100 */
    relative_rect_t box;
}algo_object_info_t;
typedef int (*algo_object_det_cb_t)(const algo_object_info_t *info, unsigned int count);

typedef enum
{
    ALGO_ID_MOTION_DETECTION = 0,
    ALGO_ID_OBJECT_DETECTION,
    ALGO_ID_MAX
}algo_id_t;




extern void hism_set_debug(unsigned char level);
extern int hism_init(void);
extern int hism_exit(void);
extern int hism_start_stream(stream_id_t id);
extern int hism_stop_stream(stream_id_t id);
extern int hism_get_video_stream_info(stream_id_t id, video_stream_info_t *info);
extern int hism_get_audio_stream_info(stream_id_t id, audio_stream_info_t *info);
extern int hism_play_audio_frame(frame_t *frame);
extern int hism_start_isp_tool(void);
extern int hism_stop_isp_tool(void);

extern int hism_get_single_frame(single_frame_id_t id, single_frame_t *frame);
/* resize会返回实际对齐后的尺寸 */
extern int hism_resize_frame(const single_frame_t *in_frame, resize_t *resize, single_frame_t *out_frame);
/* crop会返回实际对齐后的区域，只有裁剪没有缩放 */
extern int hism_encode_jpeg(const single_frame_t *frame, resize_t *crop, unsigned char *buf, unsigned int size);
extern int hism_release_single_frame(single_frame_t *frame);

extern int hism_get_osd_info(stream_id_t stream_id, osd_id_t osd_id, osd_info_t *info);
extern int hism_write_osd(stream_id_t stream_id, osd_id_t osd_id, unsigned char *buf);
extern int hism_draw_rect(stream_id_t stream_id, const graphic_rel_rect_t *rect, unsigned int count);
extern int hism_draw_line(stream_id_t stream_id, const graphic_rel_rect_t *line, unsigned int count);

extern int hism_register_algo_motion_det_cb(algo_motion_det_cb_t cb);
extern int hism_suspend_algo_motion_det(unsigned int s);
extern int hism_register_algo_object_det_cb(algo_object_det_cb_t cb);


#endif
