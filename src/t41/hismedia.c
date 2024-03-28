#include "hismedia.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/select.h>
#include <pthread.h>
#include <sys/prctl.h>

#include "imp/imp_system.h"
#include "imp/imp_isp.h"
#include "imp/imp_framesource.h"
#include "imp/imp_encoder.h"
#include "imp/imp_audio.h"
#include "imp/imp_osd.h"

#include "ivs/ivs_common.h"
#include "ivs/ivs_interface.h"
#include "ivs/ivs_inf_move.h"
#include "ivs/ivs_inf_personvehiclepetDet.h"




/*************************************************
    SYS Config
**************************************************/
typedef struct
{
    int isp_osd_pool_size;
    int ipu_osd_pool_size;
}sys_cfg_t;


/*************************************************
    ISP Config
**************************************************/
typedef struct
{
    IMPVI_NUM vi_num;
    IMPSensorInfo sensor_info;
    IMPISPSensorFps fps;
    IMPISPHVFLIPAttr flip_attr;
}isp_cfg_t;


/*************************************************
    FrameSource Config
**************************************************/
typedef struct
{
    int chn_id;
    IMPFSChnAttr attr;
}fs_cfg_t;


/*************************************************
    IVS Config
**************************************************/
typedef struct
{
    int grp_id;
}ivs_grp_cfg_t;

typedef struct
{
    int grp_id;
    int chn_id;
    int is_running; /* 指示线程是否在运行 */
    IMPIVSInterface *handler;
    IMPIVSFuncEnum func;
    union
    {
        move_param_input_t move_param;
        personvehiclepetdet_param_input_t personvehiclepet_param;
    };
}ivs_chn_cfg_t;


/*************************************************
    OSD Config
**************************************************/
typedef struct
{
    osd_id_t osd_id;
    unsigned int width;
    unsigned int height;
    int chn;
    int handle;
}isp_osd_region_cfg_t;

/* osd_grp_cfg_t 和 osd_region_cfg_t 中都有grp与stream的对应关系，有冗余 */
#define MAX_OSD_REGION_COUNT 32
typedef struct
{
    stream_id_t stream_id;
    int grp;
    int rect_rgn_cnt;
    int slash_rgn_cnt;
    unsigned int rect_linewidth;
    unsigned int slash_linewidth;
    IMPRgnHandle rect_rgn_handles[MAX_OSD_REGION_COUNT];
    IMPRgnHandle slash_rgn_handles[MAX_OSD_REGION_COUNT];
    IMPOSDGrpRgnAttr def_grp_attr;
}osd_grp_cfg_t;

typedef struct
{
    stream_id_t stream_id;
    int grp;
    osd_id_t osd_id;
    unsigned int width;
    unsigned int height;
    IMPRgnHandle handle;
    IMPOSDRgnAttr rgn_attr;
    IMPOSDGrpRgnAttr grp_attr;
}osd_region_cfg_t;


/*************************************************
    Encoder Config
**************************************************/
typedef struct
{
    int grp_id;
}enc_grp_cfg_t;

typedef struct
{
    int grp_id;
    int chn_id;
    IMPEncoderProfile profile;
    IMPEncoderRcMode rc_mode;
    uint16_t width;
    uint16_t height;
    uint32_t fps;
    uint32_t gop;
    uint32_t bitrate;
}enc_chn_cfg_t;


/*************************************************
    AI Config
**************************************************/
typedef struct
{
    int dev_id;
    IMPAudioIOAttr attr;
}ai_dev_cfg_t;

typedef struct
{
    int dev_id;
    int chn_id;
    int vol;
    int gain;
    IMPAudioIChnParam param;
}ai_chn_cfg_t;


/*************************************************
    AO Config
**************************************************/
typedef struct
{
    int dev_id;
    IMPAudioIOAttr attr;
}ao_dev_cfg_t;

typedef struct
{
    int dev_id;
    int chn_id;
    int vol;
    int gain;
}ao_chn_cfg_t;


/*************************************************
    Bind Config
**************************************************/
typedef struct
{
    IMPCell src_cell;
    IMPCell dst_cell;
}bind_cfg_t;


/*************************************************
    Video Stream Config
**************************************************/
typedef struct
{
    video_stream_info_t stream_info;
    int enc_chnid;
}video_stream_cfg_t;


/*************************************************
    Audio Stream Config
**************************************************/
typedef struct
{
    audio_stream_info_t stream_info;
    int dev_id;
    int chn_id;
}audio_stream_cfg_t;


/*************************************************
    Audio Play Config
**************************************************/
typedef struct
{
    audio_format_t format;
    unsigned int channels;
    unsigned int sample_rate;
    int dev_id;
    int chn_id;
    unsigned max_size;
}audio_play_cfg_t;


/*************************************************
    Single Frame Config
**************************************************/
typedef struct
{
    single_frame_id_t frame_id;
    int width;
    int height;
    int chn_id;
}single_frame_cfg_t;


/*************************************************
    Config End
**************************************************/


#include "hismedia_config.h"


#define DEBUG(fmt, arg...)  do{if(media_debug)printf("--Media-- " fmt "\n", ##arg);}while(0)
#define ERROR(fmt, arg...)  printf("\e[1;31m--Media-- %s: " fmt "\e[0m\n", __func__, ##arg)


typedef struct
{
    unsigned char should_run;
    unsigned char is_running;
}stream_state_t;

typedef struct
{
    algo_id_t algo_id;
    void *cb;
}algo_cb_info_t;


static pthread_mutex_t media_mutex = PTHREAD_MUTEX_INITIALIZER;
static stream_state_t stream_state_list[STREAM_ID_MAX];
#define MAX_ALGO_CB 7
static algo_cb_info_t algo_cb_list[MAX_ALGO_CB];
static unsigned char ivs_should_run = 0;

static unsigned char media_inited = 0;
static unsigned char media_debug = 1;


void hism_set_debug(unsigned char level)
{
    media_debug = level;
}


static int init_sys(void)
{
    IMPVersion ver;
    int ret;

    IMP_System_GetVersion(&ver);
    DEBUG("%s", ver.aVersion);

    if (0 < sys_cfg.isp_osd_pool_size)
    {
        if (IMP_ISP_Tuning_SetOsdPoolSize(sys_cfg.isp_osd_pool_size))
        {
            ERROR("IMP_ISP_Tuning_SetOsdPoolSize failed, size(%d) !", sys_cfg.isp_osd_pool_size);
            return -1;
        }
    }
    if (0 < sys_cfg.ipu_osd_pool_size)
    {
        ret = IMP_OSD_SetPoolSize(sys_cfg.ipu_osd_pool_size);
        if (ret)
        {
            ERROR("IMP_OSD_SetPoolSize failed, size(%d) err: %x !",
                   sys_cfg.ipu_osd_pool_size, ret);
            return -1;
        }
    }

    if (IMP_System_Init())
    {
        ERROR("IMP_System_Init failed !");
        return -1;
    }

    return 0;
}
static int deinit_sys(void)
{
    IMP_System_Exit();
    return 0;
}


static int init_isp(void)
{
    int sensor_added = 0;
    int sensor_enable = 0;
    unsigned char value;
    IMPISPRunningMode mode;
    int i;


    if (IMP_ISP_Open())
    {
        ERROR("IMP_ISP_Open failed !");
        return -1;
    }
    for (i = 0; i < ISP_CFG_COUNT; i++)
    {
        if (IMP_ISP_AddSensor(isp_cfg[i].vi_num, &isp_cfg[i].sensor_info))
        {
            ERROR("IMP_ISP_AddSensor failed, vi[%d] !", isp_cfg[i].vi_num);
            goto EXIT_ISP;
        }
        sensor_added++;

        if (IMP_ISP_EnableSensor(isp_cfg[i].vi_num, &isp_cfg[i].sensor_info))
        {
            ERROR("IMP_ISP_EnableSensor failed, vi[%d] !", isp_cfg[i].vi_num);
            goto EXIT_ISP;
        }
        sensor_enable++;
    }

    if (IMP_ISP_EnableTuning())
        goto EXIT_ISP;

    value = 128;
    mode = IMPISP_RUNNING_MODE_DAY;
    for (i = 0; i < ISP_CFG_COUNT; i++)
    {
        IMP_ISP_Tuning_SetContrast(isp_cfg[i].vi_num, &value);
        IMP_ISP_Tuning_SetSharpness(isp_cfg[i].vi_num, &value);
        IMP_ISP_Tuning_SetSaturation(isp_cfg[i].vi_num, &value);
        IMP_ISP_Tuning_SetBrightness(isp_cfg[i].vi_num, &value);
        IMP_ISP_Tuning_SetISPRunningMode(isp_cfg[i].vi_num, &mode);
        if (IMP_ISP_Tuning_SetSensorFPS(isp_cfg[i].vi_num, &isp_cfg[i].fps))
        {
            ERROR("IMP_ISP_Tuning_SetSensorFPS failed !");
            goto EXIT_TUNING;
        }
        if (IMP_ISP_Tuning_SetHVFLIP(isp_cfg[i].vi_num, &isp_cfg[i].flip_attr))
        {
            ERROR("IMP_ISP_Tuning_SetHVFLIP failed !");
            goto EXIT_TUNING;
        }
    }

    return 0;


EXIT_TUNING:
    IMP_ISP_DisableTuning();
EXIT_ISP:
    for (i = 0; i < sensor_enable; i++)
        IMP_ISP_DisableSensor(isp_cfg[i].vi_num);
    for (i = 0; i < sensor_added; i++)
        IMP_ISP_DelSensor(isp_cfg[i].vi_num, &isp_cfg[i].sensor_info);
    IMP_ISP_Close();

    return -1;
}
static int deinit_isp(void)
{
    int i;

    IMP_ISP_DisableTuning();
    for (i = 0; i < ISP_CFG_COUNT; i++)
    {
        IMP_ISP_DisableSensor(isp_cfg[i].vi_num);
        IMP_ISP_DelSensor(isp_cfg[i].vi_num, &isp_cfg[i].sensor_info);
    }
    IMP_ISP_Close();

    return 0;
}


static int init_framesource(void)
{
    int chn_created = 0;
    int chn_enabled = 0;
    int ret;
    int i;


    for (i = 0; i < FS_CFG_COUNT; i++)
    {
        ret = IMP_FrameSource_CreateChn(fs_cfg[i].chn_id, &fs_cfg[i].attr);
        if (ret)
        {
            ERROR("IMP_FrameSource_CreateChn failed, err: %x !", ret);
            goto EXIT;
        }
        chn_created++;

        ret = IMP_FrameSource_SetChnAttr(fs_cfg[i].chn_id, &fs_cfg[i].attr);
        if (ret)
        {
            ERROR("IMP_FrameSource_SetChnAttr failed, err: %x !", ret);
            goto EXIT;
        }

        ret = IMP_FrameSource_EnableChn(fs_cfg[i].chn_id);
        if (ret)
        {
            ERROR("IMP_FrameSource_EnableChn failed, err: %x !", ret);
            goto EXIT;
        }
        chn_enabled++;
    }

    return 0;

EXIT:
    for (i = 0; i < chn_enabled; i++)
        IMP_FrameSource_DisableChn(fs_cfg[i].chn_id);
    for (i = 0; i < chn_created; i++)
        IMP_FrameSource_DestroyChn(fs_cfg[i].chn_id);

    return -1;
}
static int deinit_framesource(void)
{
    int i;

    for (i = 0; i < FS_CFG_COUNT; i++)
    {
        IMP_FrameSource_DisableChn(fs_cfg[i].chn_id);
        IMP_FrameSource_DestroyChn(fs_cfg[i].chn_id);
    }

    return 0;
}


static struct timespec md_pause_time = {0};
static void do_move_cb(move_param_output_t *result)
{
    algo_motion_det_cb_t cb;
    struct timespec time;
    int i;


    if (0 == result->ret)
        return;

    clock_gettime(CLOCK_MONOTONIC, &time);
    if (   (md_pause_time.tv_sec > time.tv_sec)
        || ((md_pause_time.tv_sec == time.tv_sec) && (md_pause_time.tv_nsec > time.tv_nsec)))
    {
        return;
    }

    for (i = 0; i < MAX_ALGO_CB; i++)
    {
        cb = (algo_motion_det_cb_t)algo_cb_list[i].cb;
        if (cb && (ALGO_ID_MOTION_DETECTION == algo_cb_list[i].algo_id))
            cb();
    }

    return;
}
#define MAX_OBJECT 30
static void do_personvehiclepet_cb(personvehiclepetdet_param_output_t *result)
{
    static int config_index = -1;
    unsigned int w, h;
    algo_object_info_t object_info_list[MAX_OBJECT] = {0};
    algo_object_det_cb_t cb;
    unsigned int count;
    int i;


    if (0 >= result->count)
    {
        for (i = 0; i < MAX_ALGO_CB; i++)
        {
            cb = (algo_object_det_cb_t)algo_cb_list[i].cb;
            if (cb && (ALGO_ID_OBJECT_DETECTION == algo_cb_list[i].algo_id))
                cb(NULL, 0);
        }
        return;
    }

    if (0 > config_index)
    {
        for (i = 0; i < IVS_CHN_CFG_COUNT; i++)
        {
            if (IVS_PERSONVEHICLEPET_DETECT == ivs_chn_cfg[i].func)
                break;
        }
        if (IVS_CHN_CFG_COUNT <= i)
            return;
        config_index = i;
    }
    w = ivs_chn_cfg[config_index].personvehiclepet_param.frameInfo.width;
    h = ivs_chn_cfg[config_index].personvehiclepet_param.frameInfo.height;

    if (MAX_OBJECT < result->count)
        count = MAX_OBJECT;
    else
        count = result->count;
    for (i = 0; i < count; i++)
    {
        switch (result->personvehiclepet[i].class_id)
        {
            case 0:  object_info_list[i].type = OBJECT_TYPE_PERSON;  break;
            case 1:  object_info_list[i].type = OBJECT_TYPE_CAR;     break;
            case 2:  object_info_list[i].type = OBJECT_TYPE_BUS;     break;
            case 3:  object_info_list[i].type = OBJECT_TYPE_TRUCK;   break;
            case 7:  object_info_list[i].type = OBJECT_TYPE_CAT;     break;
            case 8:  object_info_list[i].type = OBJECT_TYPE_DOG;     break;
            default: object_info_list[i].type = OBJECT_TYPE_UNKNOW;  break;
        }

        object_info_list[i].track_id = result->personvehiclepet[i].track_id;
        object_info_list[i].confidence = (int)(result->personvehiclepet[i].confidence * 100);
        object_info_list[i].box.x0 = result->personvehiclepet[i].show_box.ul.x * REL_COORD_MAX / w;
        object_info_list[i].box.y0 = result->personvehiclepet[i].show_box.ul.y * REL_COORD_MAX / h;
        object_info_list[i].box.x1 = result->personvehiclepet[i].show_box.br.x * REL_COORD_MAX / w;
        object_info_list[i].box.y1 = result->personvehiclepet[i].show_box.br.y * REL_COORD_MAX / h;
    }

    for (i = 0; i < MAX_ALGO_CB; i++)
    {
        cb = (algo_object_det_cb_t)algo_cb_list[i].cb;
        if (cb && (ALGO_ID_OBJECT_DETECTION == algo_cb_list[i].algo_id))
            cb(object_info_list, count);
    }

    return;
}
static void* get_ivs_result(void *arg)
{
    int index = (int)arg;
    IMPIVSFuncEnum func = ivs_chn_cfg[index].func;
    int chn_id;
    void *result;

    pthread_detach(pthread_self());
    if (IVS_MOVE_DETECT == func)
        prctl(PR_SET_NAME, (unsigned long)"get_ivs_result move", 0, 0, 0);
    else if (IVS_PERSONVEHICLEPET_DETECT == func)
        prctl(PR_SET_NAME, (unsigned long)"get_ivs_result personvehiclepet", 0, 0, 0);

    chn_id = ivs_chn_cfg[index].chn_id;

    while (ivs_should_run)
    {
        if (IMP_IVS_PollingResult(chn_id, 300))
            continue;
        if (IMP_IVS_GetResult(chn_id, &result))
            continue;

        if (IVS_MOVE_DETECT == func)
            do_move_cb((move_param_output_t *)result);
        else if (IVS_PERSONVEHICLEPET_DETECT == func)
            do_personvehiclepet_cb((personvehiclepetdet_param_output_t *)result);

        if (IMP_IVS_ReleaseResult(chn_id, result))
            ERROR("IMP_IVS_ReleaseResult failed, chn[%d] !", chn_id);
    }

    ivs_chn_cfg[index].is_running = 0;
    return NULL;
}
static IVSPoint point[6];
static int init_ivs_algo_move(int index)
{
#ifdef ALGO_MOVE
    int grp_id = ivs_chn_cfg[index].grp_id;
    int chn_id = ivs_chn_cfg[index].chn_id;
    int width  = ivs_chn_cfg[index].move_param.frameInfo.width;
    int height = ivs_chn_cfg[index].move_param.frameInfo.height;


    if (MOVE_VERSION_NUM != move_get_version_info())
        ERROR("IVS move head ver(%08x) does not match lib ver(%08x) !",
               MOVE_VERSION_NUM, move_get_version_info());

    point[0].x = 0;
    point[0].y = 0;
    point[1].x = width - 1;
    point[1].y = 0;
    point[2].x = width - 1;
    point[2].y = height - 1;
    point[3].x = 0;
    point[3].y = height - 1;
    ivs_chn_cfg[index].move_param.perms[0].p = point;
    ivs_chn_cfg[index].handler = MoveInterfaceInit(&ivs_chn_cfg[index].move_param);
    if (NULL == ivs_chn_cfg[index].handler)
    {
        ERROR("MoveInterfaceInit failed !");
        goto EXIT;
    }
    if (IMP_IVS_CreateChn(chn_id, ivs_chn_cfg[index].handler))
    {
        ERROR("IMP_IVS_CreateChn failed, chn[%d] !", chn_id);
        goto EXIT_INF;
    }
    if (IMP_IVS_RegisterChn(grp_id, chn_id))
    {
        ERROR("IMP_IVS_RegisterChn failed, grp[%d] chn[%d] !", grp_id, chn_id);
        goto EXIT_CREATE;
    }
    if (IMP_IVS_StartRecvPic(chn_id))
    {
        ERROR("IMP_IVS_StartRecvPic failed, chn[%d] !", chn_id);
        goto EXIT_REGISTER;
    }

    return 0;

EXIT_REGISTER:
    IMP_IVS_UnRegisterChn(chn_id);
EXIT_CREATE:
    IMP_IVS_DestroyChn(chn_id);
EXIT_INF:
    MoveInterfaceExit(ivs_chn_cfg[index].handler);
    ivs_chn_cfg[index].handler = NULL;
EXIT:
    return -1;

#else
    ERROR("motion detection algorithm is not enabled !");
    return -1;
#endif
}
static int deinit_ivs_algo_move(int index)
{
#ifdef ALGO_MOVE
    int chn_id = ivs_chn_cfg[index].chn_id;

    IMP_IVS_StopRecvPic(chn_id);
    IMP_IVS_UnRegisterChn(chn_id);
    IMP_IVS_DestroyChn(chn_id);
    MoveInterfaceExit(ivs_chn_cfg[index].handler);
    ivs_chn_cfg[index].handler = NULL;

    return 0;

#else
    ERROR("motion detection algorithm is not enabled !");
    return -1;
#endif
}

static int init_ivs_algo_personvehiclepet(int index)
{
#ifdef ALGO_PERSONVEHICLEPET
    int grp_id = ivs_chn_cfg[index].grp_id;
    int chn_id = ivs_chn_cfg[index].chn_id;


    if (PERSONVEHICLEPETDET_VERSION_NUM != personvehiclepetdet_get_version_info())
        ERROR("IVS personvehiclepetdet head ver(%08x) does not match lib ver(%08x) !",
               PERSONVEHICLEPETDET_VERSION_NUM, personvehiclepetdet_get_version_info());

    ivs_chn_cfg[index].handler = PersonvehiclepetDetInterfaceInit(&ivs_chn_cfg[index].personvehiclepet_param);
    if (NULL == ivs_chn_cfg[index].handler)
    {
        ERROR("PersonvehiclepetDetInterfaceInit failed !");
        goto EXIT;
    }
    if (IMP_IVS_CreateChn(chn_id, ivs_chn_cfg[index].handler))
    {
        ERROR("IMP_IVS_CreateChn failed, chn[%d] !", chn_id);
        goto EXIT_INF;
    }
    if (IMP_IVS_RegisterChn(grp_id, chn_id))
    {
        ERROR("IMP_IVS_RegisterChn failed, grp[%d] chn[%d] !", grp_id, chn_id);
        goto EXIT_CREATE;
    }
    if (IMP_IVS_StartRecvPic(chn_id))
    {
        ERROR("IMP_IVS_StartRecvPic failed, chn[%d] !", chn_id);
        goto EXIT_REGISTER;
    }

    return 0;

EXIT_REGISTER:
    IMP_IVS_UnRegisterChn(chn_id);
EXIT_CREATE:
    IMP_IVS_DestroyChn(chn_id);
EXIT_INF:
    PersonvehiclepetDetInterfaceExit(ivs_chn_cfg[index].handler);
    ivs_chn_cfg[index].handler = NULL;
EXIT:
    return -1;

#else
    ERROR("personvehiclepet detection algorithm is not enabled !");
    return -1;
#endif
}
static int deinit_ivs_algo_personvehiclepet(int index)
{
#ifdef ALGO_PERSONVEHICLEPET
    int chn_id = ivs_chn_cfg[index].chn_id;

    IMP_IVS_StopRecvPic(chn_id);
    IMP_IVS_UnRegisterChn(chn_id);
    IMP_IVS_DestroyChn(chn_id);
    PersonvehiclepetDetInterfaceExit(ivs_chn_cfg[index].handler);
    ivs_chn_cfg[index].handler = NULL;

    return 0;

#else
    ERROR("personvehiclepet detection algorithm is not enabled !");
    return -1;
#endif
}


static int init_ivs(void)
{
    int grp_created = 0;
    int algo_inited = 0;
    pthread_t tid;
    int i, j;


    for (i = 0; i < IVS_GRP_CFG_COUNT; i++)
    {
        if (IMP_IVS_CreateGroup(ivs_grp_cfg[i].grp_id))
        {
            ERROR("IMP_IVS_CreateGroup failed, grp[%d] !", ivs_grp_cfg[i].grp_id);
            goto EXIT;
        }
        grp_created++;
    }

    ivs_should_run = 1;
    for (i = 0; i < IVS_CHN_CFG_COUNT; i++)
    {
        if (IVS_MOVE_DETECT == ivs_chn_cfg[i].func)
        {
            if (init_ivs_algo_move(i))
                goto EXIT;
        }
        else if (IVS_PERSONVEHICLEPET_DETECT == ivs_chn_cfg[i].func)
        {
            if (init_ivs_algo_personvehiclepet(i))
                goto EXIT;
        }
        else
        {
            ERROR("algo(%d) is not supported !", ivs_chn_cfg[i].func);
            continue;
        }
        ivs_chn_cfg[i].is_running = 1;
        pthread_create(&tid, NULL, get_ivs_result, (void *)i);
        algo_inited++;
    }

    return 0;


EXIT:
    ivs_should_run = 0;
    for (i = 0; i < 7; i++)
    {
        usleep(400 * 1000);
        for (j = 0; j < IVS_CHN_CFG_COUNT; j++)
        {
            if (ivs_chn_cfg[j].is_running)
                break;
        }
        if (IVS_CHN_CFG_COUNT <= j)
            break;
    }

    for (i = 0; i < algo_inited; i++)
    {
        if (IVS_MOVE_DETECT == ivs_chn_cfg[i].func)
        {
            deinit_ivs_algo_move(i);
        }
        else if (IVS_PERSONVEHICLEPET_DETECT == ivs_chn_cfg[i].func)
        {
            deinit_ivs_algo_personvehiclepet(i);
        }
    }
    for (i = 0; i < grp_created; i++)
        IMP_IVS_DestroyGroup(ivs_grp_cfg[i].grp_id);

    return -1;
}
static int deinit_ivs(void)
{
    int i, j;

    ivs_should_run = 0;
    for (i = 0; i < 7; i++)
    {
        usleep(400 * 1000);
        for (j = 0; j < IVS_CHN_CFG_COUNT; j++)
        {
            if (ivs_chn_cfg[j].is_running)
                break;
        }
        if (IVS_CHN_CFG_COUNT <= j)
            break;
    }
    if (7 <= i)
    {
        ERROR("ivs exit timeout !");
    }

    for (i = 0; i < IVS_CHN_CFG_COUNT; i++)
    {
        if (IVS_MOVE_DETECT == ivs_chn_cfg[i].func)
        {
            deinit_ivs_algo_move(i);
        }
    }
    for (i = 0; i < IVS_GRP_CFG_COUNT; i++)
        IMP_IVS_DestroyGroup(ivs_grp_cfg[i].grp_id);

    return 0;
}


static int init_osd(void)
{
    int isp_region_created = 0;
    int group_created = 0;
    int group_started = 0;
    int region_created = 0;
    int region_registered = 0;
    int region_showed = 0;
    int group_rect_inited = 0;
    int group_slash_inited = 0;
    int ret;
    int i, j;


    /* ISP OSD */
    for (i = 0; i < ISP_OSD_REGION_CFG_COUNT; i++)
    {
        isp_osd_region_cfg[i].handle = IMP_ISP_Tuning_CreateOsdRgn(isp_osd_region_cfg[i].chn, NULL);
        if (0 > isp_osd_region_cfg[i].handle)
        {
            ERROR("IMP_ISP_Tuning_CreateOsdRgn failed, chn[%d] !", isp_osd_region_cfg[i].chn);
            goto EXIT;
        }
        isp_region_created++;
    }

    /* OSD Group */
    for (i = 0; i < OSD_GRP_CFG_COUNT; i++)
    {
        ret = IMP_OSD_CreateGroup(osd_grp_cfg[i].grp);
        if (ret)
        {
            ERROR("IMP_OSD_CreateGroup failed, grp[%d] err: %x !", osd_grp_cfg[i].grp, ret);
            goto EXIT;
        }
        group_created++;

        ret = IMP_OSD_Start(osd_grp_cfg[i].grp);
        if (ret)
        {
            ERROR("IMP_OSD_Start failed, grp[%d] err: %x !", osd_grp_cfg[i].grp, ret);
            goto EXIT;
        }
        group_started++;
    }

    /* 显示字的 OSD Region */
    for (i = 0; i < OSD_REGION_CFG_COUNT; i++)
    {
        osd_region_cfg[i].handle = IMP_OSD_CreateRgn(&osd_region_cfg[i].rgn_attr);
        if (0 > osd_region_cfg[i].handle)
        {
            ERROR("IMP_OSD_CreateRgn failed !");
            goto EXIT;
        }
        region_created++;

        ret = IMP_OSD_RegisterRgn(osd_region_cfg[i].handle, osd_region_cfg[i].grp, &osd_region_cfg[i].grp_attr);
        if (ret)
        {
            ERROR("IMP_OSD_RegisterRgn failed, grp[%d] err: %x !", osd_region_cfg[i].grp, ret);
            goto EXIT;
        }
        region_registered++;

        ret = IMP_OSD_ShowRgn(osd_region_cfg[i].handle, osd_region_cfg[i].grp, 1);
        if (ret)
        {
            ERROR("IMP_OSD_ShowRgn failed, err: %x !", ret);
            goto EXIT;
        }
        region_showed++;
    }

    for (i = 0; i < OSD_GRP_CFG_COUNT; i++)
    {
        for (j = 0; j < MAX_OSD_REGION_COUNT; j++)
        {
            osd_grp_cfg[i].rect_rgn_handles[j] = -1;
            osd_grp_cfg[i].slash_rgn_handles[j] = -1;
        }
    }
    /* 画框的 OSD Region */
    for (i = 0; i < OSD_GRP_CFG_COUNT; i++)
    {
        if (MAX_OSD_REGION_COUNT < osd_grp_cfg[i].rect_rgn_cnt)
        {
            ERROR("grp[%d] rect_rgn_cnt(%d) exceeds the max(%d) !",
                   osd_grp_cfg[i].grp, osd_grp_cfg[i].rect_rgn_cnt, MAX_OSD_REGION_COUNT);
            goto EXIT;
        }
        for (j = 0; j < osd_grp_cfg[i].rect_rgn_cnt; j++)
        {
            osd_grp_cfg[i].rect_rgn_handles[j] = IMP_OSD_CreateRgn(NULL);
            if (0 > osd_grp_cfg[i].rect_rgn_handles[j])
            {
                ERROR("IMP_OSD_CreateRgn failed !");
                break;
            }
            ret = IMP_OSD_RegisterRgn(osd_grp_cfg[i].rect_rgn_handles[j],
                                      osd_grp_cfg[i].grp, &osd_grp_cfg[i].def_grp_attr);
            if (ret)
            {
                ERROR("IMP_OSD_RegisterRgn failed, grp[%d] err: %x !", osd_grp_cfg[i].grp, ret);
                IMP_OSD_DestroyRgn(osd_grp_cfg[i].rect_rgn_handles[j]);
                osd_grp_cfg[i].rect_rgn_handles[j] = -1;
                break;
            }
        }
        if (osd_grp_cfg[i].rect_rgn_cnt <= j)
        {
            group_rect_inited++;
            continue;
        }
        /* 撤销当前group初始化的region */
        for (; j > 0; j--)
        {
            IMP_OSD_UnRegisterRgn(osd_grp_cfg[i].rect_rgn_handles[j-1], osd_grp_cfg[i].grp);
            IMP_OSD_DestroyRgn(osd_grp_cfg[i].rect_rgn_handles[j-1]);
            osd_grp_cfg[i].rect_rgn_handles[j-1] = -1;
        }
        goto EXIT;
    }
    /* 画斜线的 OSD Region */
    for (i = 0; i < OSD_GRP_CFG_COUNT; i++)
    {
        if (MAX_OSD_REGION_COUNT < osd_grp_cfg[i].slash_rgn_cnt)
        {
            ERROR("grp[%d] slash_rgn_cnt(%d) exceeds the max(%d) !",
                   osd_grp_cfg[i].grp, osd_grp_cfg[i].slash_rgn_cnt, MAX_OSD_REGION_COUNT);
            goto EXIT;
        }
        for (j = 0; j < osd_grp_cfg[i].slash_rgn_cnt; j++)
        {
            osd_grp_cfg[i].slash_rgn_handles[j] = IMP_OSD_CreateRgn(NULL);
            if (0 > osd_grp_cfg[i].slash_rgn_handles[j])
            {
                ERROR("IMP_OSD_CreateRgn failed !");
                break;
            }
            ret = IMP_OSD_RegisterRgn(osd_grp_cfg[i].slash_rgn_handles[j],
                                      osd_grp_cfg[i].grp, &osd_grp_cfg[i].def_grp_attr);
            if (ret)
            {
                ERROR("IMP_OSD_RegisterRgn failed, grp[%d] err: %x !", osd_grp_cfg[i].grp, ret);
                IMP_OSD_DestroyRgn(osd_grp_cfg[i].slash_rgn_handles[j]);
                osd_grp_cfg[i].slash_rgn_handles[j] = -1;
                break;
            }
        }
        if (osd_grp_cfg[i].slash_rgn_cnt <= j)
        {
            group_slash_inited++;
            continue;
        }
        /* 撤销当前group初始化的region */
        for (; j > 0; j--)
        {
            IMP_OSD_UnRegisterRgn(osd_grp_cfg[i].slash_rgn_handles[j-1], osd_grp_cfg[i].grp);
            IMP_OSD_DestroyRgn(osd_grp_cfg[i].slash_rgn_handles[j-1]);
            osd_grp_cfg[i].slash_rgn_handles[j-1] = -1;
        }
        goto EXIT;
    }

    return 0;


EXIT:
    for (i = 0; i < group_slash_inited; i++)
    {
        for (j = 0; j < osd_grp_cfg[i].slash_rgn_cnt; j++)
        {
            IMP_OSD_UnRegisterRgn(osd_grp_cfg[i].slash_rgn_handles[j], osd_grp_cfg[i].grp);
            IMP_OSD_DestroyRgn(osd_grp_cfg[i].slash_rgn_handles[j]);
            osd_grp_cfg[i].slash_rgn_handles[j] = -1;
        }
    }
    for (i = 0; i < group_rect_inited; i++)
    {
        for (j = 0; j < osd_grp_cfg[i].rect_rgn_cnt; j++)
        {
            IMP_OSD_UnRegisterRgn(osd_grp_cfg[i].rect_rgn_handles[j], osd_grp_cfg[i].grp);
            IMP_OSD_DestroyRgn(osd_grp_cfg[i].rect_rgn_handles[j]);
            osd_grp_cfg[i].rect_rgn_handles[j] = -1;
        }
    }
    for (i = 0; i < region_showed; i++)
        IMP_OSD_ShowRgn(osd_region_cfg[i].handle, osd_region_cfg[i].grp, 0);
    for (i = 0; i < region_registered; i++)
        IMP_OSD_UnRegisterRgn(osd_region_cfg[i].handle, osd_region_cfg[i].grp);
    for (i = 0; i < region_created; i++)
        IMP_OSD_DestroyRgn(osd_region_cfg[i].handle);
    for (i = 0; i < group_started; i++)
        IMP_OSD_Stop(osd_region_cfg[i].grp);
    for (i = 0; i < group_created; i++)
        IMP_OSD_DestroyGroup(osd_region_cfg[i].grp);
    for (i = 0; i < isp_region_created; i++)
        IMP_ISP_Tuning_DestroyOsdRgn(isp_osd_region_cfg[i].chn, isp_osd_region_cfg[i].handle);

    return -1;
}
static int deinit_osd(void)
{
    int i, j;

    for (i = 0; i < OSD_GRP_CFG_COUNT; i++)
    {
        for (j = 0; j < osd_grp_cfg[i].slash_rgn_cnt; j++)
        {
            IMP_OSD_UnRegisterRgn(osd_grp_cfg[i].slash_rgn_handles[j], osd_grp_cfg[i].grp);
            IMP_OSD_DestroyRgn(osd_grp_cfg[i].slash_rgn_handles[j]);
            osd_grp_cfg[i].slash_rgn_handles[j] = -1;
        }
        for (j = 0; j < osd_grp_cfg[i].rect_rgn_cnt; j++)
        {
            IMP_OSD_UnRegisterRgn(osd_grp_cfg[i].rect_rgn_handles[j], osd_grp_cfg[i].grp);
            IMP_OSD_DestroyRgn(osd_grp_cfg[i].rect_rgn_handles[j]);
            osd_grp_cfg[i].rect_rgn_handles[j] = -1;
        }
    }
    for (i = 0; i < OSD_REGION_CFG_COUNT; i++)
    {
        IMP_OSD_ShowRgn(osd_region_cfg[i].handle, osd_region_cfg[i].grp, 0);
        IMP_OSD_UnRegisterRgn(osd_region_cfg[i].handle, osd_region_cfg[i].grp);
        IMP_OSD_DestroyRgn(osd_region_cfg[i].handle);
        osd_region_cfg[i].handle = -1;
    }
    for (i = 0; i < OSD_GRP_CFG_COUNT; i++)
    {
        IMP_OSD_Stop(osd_region_cfg[i].grp);
        IMP_OSD_DestroyGroup(osd_region_cfg[i].grp);
    }
    for (i = 0; i < ISP_OSD_REGION_CFG_COUNT; i++)
        IMP_ISP_Tuning_DestroyOsdRgn(isp_osd_region_cfg[i].chn, isp_osd_region_cfg[i].handle);

    return 0;
}


static int init_encoder(void)
{
    int grp_created = 0;
    int chn_created = 0;
    int chn_registered = 0;
    int chn_started = 0;
    IMPEncoderChnAttr chn_attr;
    int ret;
    int i;

    for (i = 0; i < ENC_GRP_CFG_COUNT; i++)
    {
        ret = IMP_Encoder_CreateGroup(enc_grp_cfg[i].grp_id);
        if (ret)
        {
            ERROR("IMP_Encoder_CreateGroup failed, grp[%d] err: %x !", enc_grp_cfg[i].grp_id, ret);
            goto EXIT;
        }
        grp_created++;
    }

    for (i = 0; i < ENC_CHN_CFG_COUNT; i++)
    {
        ret = IMP_Encoder_SetDefaultParam(&chn_attr,
                                          enc_chn_cfg[i].profile,
                                          enc_chn_cfg[i].rc_mode,
                                          enc_chn_cfg[i].width,
                                          enc_chn_cfg[i].height,
                                          enc_chn_cfg[i].fps,
                                          1,
                                          enc_chn_cfg[i].gop,
                                          2, -1,
                                          enc_chn_cfg[i].bitrate);
        if (ret)
        {
            ERROR("IMP_Encoder_SetDefaultParam failed, err: %x !", ret);
            goto EXIT;
        }
        ret = IMP_Encoder_CreateChn(enc_chn_cfg[i].chn_id, &chn_attr);
        if (ret)
        {
            ERROR("IMP_Encoder_CreateChn failed, chn[%d] err: %x !",
                   enc_chn_cfg[i].chn_id, ret);
            goto EXIT;
        }
        chn_created++;

        ret = IMP_Encoder_RegisterChn(enc_chn_cfg[i].grp_id, enc_chn_cfg[i].chn_id);
        if (ret)
        {
            ERROR("IMP_Encoder_RegisterChn failed, grp[%d] chn[%d] err: %x !",
                   enc_chn_cfg[i].grp_id, enc_chn_cfg[i].chn_id, ret);
            goto EXIT;
        }
        chn_registered++;

        ret = IMP_Encoder_StartRecvPic(enc_chn_cfg[i].chn_id);
        if (ret)
        {
            ERROR("IMP_Encoder_StartRecvPic failed, chn[%d] err: %x !",
                   enc_chn_cfg[i].chn_id, ret);
            goto EXIT;
        }
        chn_started++;
    }

    return 0;


EXIT:
    for (i = 0; i < chn_started; i++)
        IMP_Encoder_StopRecvPic(enc_chn_cfg[i].chn_id);
    for (i = 0; i < chn_registered; i++)
        IMP_Encoder_UnRegisterChn(enc_chn_cfg[i].chn_id);
    for (i = 0; i < chn_created; i++)
        IMP_Encoder_DestroyChn(enc_chn_cfg[i].chn_id);
    for (i = 0; i < grp_created; i++)
        IMP_Encoder_DestroyGroup(enc_grp_cfg[i].grp_id);

    return -1;
}
static int deinit_encoder(void)
{
    int i;

    for (i = 0; i < ENC_CHN_CFG_COUNT; i++)
    {
        IMP_Encoder_StopRecvPic(enc_chn_cfg[i].chn_id);
        IMP_Encoder_UnRegisterChn(enc_chn_cfg[i].chn_id);
        IMP_Encoder_DestroyChn(enc_chn_cfg[i].chn_id);
    }
    for (i = 0; i < ENC_GRP_CFG_COUNT; i++)
        IMP_Encoder_DestroyGroup(enc_grp_cfg[i].grp_id);

    return 0;
}


static int init_ai(void)
{
    int dev_enabled = 0;
    int chn_enabled = 0;
    int algo_enabled = 0;
    int i;

    for (i = 0; i < AI_DEV_CFG_COUNT; i++)
    {
        if (IMP_AI_SetPubAttr(ai_dev_cfg[i].dev_id, &ai_dev_cfg[i].attr))
        {
            ERROR("IMP_AI_SetPubAttr failed, dev[%d] !", ai_dev_cfg[i].dev_id);
            goto EXIT;
        }
        if (IMP_AI_Enable(ai_dev_cfg[i].dev_id))
        {
            ERROR("IMP_AI_Enable failed, dev[%d] !", ai_dev_cfg[i].dev_id);
            goto EXIT;
        }
        dev_enabled++;
    }

    for (i = 0; i < AI_CHN_CFG_COUNT; i++)
    {
        if (IMP_AI_SetChnParam(ai_chn_cfg[i].dev_id, ai_chn_cfg[i].chn_id, &ai_chn_cfg[i].param))
        {
            ERROR("IMP_AI_SetChnParam failed, dev[%d] chn[%d] !",
                   ai_chn_cfg[i].dev_id, ai_chn_cfg[i].chn_id);
            goto EXIT;
        }
        if (IMP_AI_EnableChn(ai_chn_cfg[i].dev_id, ai_chn_cfg[i].chn_id))
        {
            ERROR("IMP_AI_EnableChn failed, dev[%d] chn[%d] !",
                   ai_chn_cfg[i].dev_id, ai_chn_cfg[i].chn_id);
            goto EXIT;
        }
        chn_enabled++;

        IMP_AI_SetVol(ai_chn_cfg[i].dev_id, ai_chn_cfg[i].chn_id, ai_chn_cfg[i].vol);
        IMP_AI_SetGain(ai_chn_cfg[i].dev_id, ai_chn_cfg[i].chn_id, ai_chn_cfg[i].gain);

        if (IMP_AI_EnableAlgo(ai_chn_cfg[i].dev_id, ai_chn_cfg[i].chn_id))
        {
            ERROR("IMP_AI_EnableAlgo failed, dev[%d] chn[%d] !",
                   ai_chn_cfg[i].dev_id, ai_chn_cfg[i].chn_id);
            goto EXIT;
        }
        algo_enabled++;
    }

    return 0;


EXIT:
    for (i = 0; i < algo_enabled; i++)
        IMP_AI_DisableAlgo(ai_chn_cfg[i].dev_id, ai_chn_cfg[i].chn_id);
    for (i = 0; i < chn_enabled; i++)
        IMP_AI_DisableChn(ai_chn_cfg[i].dev_id, ai_chn_cfg[i].chn_id);
    for (i = 0; i < dev_enabled; i++)
        IMP_AI_Disable(ai_dev_cfg[i].dev_id);

    return -1;
}
static int deinit_ai(void)
{
    int i;

    for (i = 0; i < AI_CHN_CFG_COUNT; i++)
    {
        IMP_AI_DisableAlgo(ai_chn_cfg[i].dev_id, ai_chn_cfg[i].chn_id);
        IMP_AI_DisableChn(ai_chn_cfg[i].dev_id, ai_chn_cfg[i].chn_id);
    }
    for (i = 0; i < AI_DEV_CFG_COUNT; i++)
        IMP_AI_Disable(ai_dev_cfg[i].dev_id);

    return 0;
}


static int init_ao(void)
{
    int dev_enabled = 0;
    int chn_enabled = 0;
    int algo_enabled = 0;
    int i;

    for (i = 0; i < AO_DEV_CFG_COUNT; i++)
    {
        if (IMP_AO_SetPubAttr(ao_dev_cfg[i].dev_id, &ao_dev_cfg[i].attr))
        {
            ERROR("IMP_AO_SetPubAttr failed, dev[%d] !", ao_dev_cfg[i].dev_id);
            goto EXIT;
        }
        if (IMP_AO_Enable(ao_dev_cfg[i].dev_id))
        {
            ERROR("IMP_AO_Enable failed, dev[%d] !", ao_dev_cfg[i].dev_id);
            goto EXIT;
        }
        dev_enabled++;
    }

    for (i = 0; i < AO_CHN_CFG_COUNT; i++)
    {
        if (IMP_AO_EnableChn(ao_chn_cfg[i].dev_id, ao_chn_cfg[i].chn_id))
        {
            ERROR("IMP_AO_EnableChn failed, dev[%d] chn[%d] !",
                   ao_chn_cfg[i].dev_id, ao_chn_cfg[i].chn_id);
            goto EXIT;
        }
        chn_enabled++;

        if (IMP_AO_SetVol(ao_chn_cfg[i].dev_id, ao_chn_cfg[i].chn_id, ao_chn_cfg[i].vol))
        {
            ERROR("IMP_AO_SetVol failed, dev[%d] chn[%d] vol(%d) !",
                   ao_chn_cfg[i].dev_id, ao_chn_cfg[i].chn_id, ao_chn_cfg[i].vol);
            goto EXIT;
        }
        if (IMP_AO_SetGain(ao_chn_cfg[i].dev_id, ao_chn_cfg[i].chn_id, ao_chn_cfg[i].gain))
        {
            ERROR("IMP_AO_SetGain failed, dev[%d] chn[%d] gain(%d) !",
                   ao_chn_cfg[i].dev_id, ao_chn_cfg[i].chn_id, ao_chn_cfg[i].gain);
            goto EXIT;
        }
        if (IMP_AO_CacheSwitch(ao_chn_cfg[i].dev_id, ao_chn_cfg[i].chn_id, 0))
        {
            ERROR("IMP_AO_CacheSwitch failed, dev[%d] chn[%d] !",
                   ao_chn_cfg[i].dev_id, ao_chn_cfg[i].chn_id);
            goto EXIT;
        }
        if (IMP_AO_EnableAlgo(ao_chn_cfg[i].dev_id, ao_chn_cfg[i].chn_id))
        {
            ERROR("IMP_AO_EnableAlgo failed, dev[%d] chn[%d] !",
                   ao_chn_cfg[i].dev_id, ao_chn_cfg[i].chn_id);
            goto EXIT;
        }
        algo_enabled++;
    }

    return 0;


EXIT:
    for (i = 0; i < algo_enabled; i++)
        IMP_AO_DisableAlgo(ao_chn_cfg[i].dev_id, ao_chn_cfg[i].chn_id);
    for (i = 0; i < chn_enabled; i++)
        IMP_AO_DisableChn(ao_chn_cfg[i].dev_id, ao_chn_cfg[i].chn_id);
    for (i = 0; i < dev_enabled; i++)
        IMP_AO_Disable(ao_dev_cfg[i].dev_id);

    return -1;
}
static int deinit_ao(void)
{
    int i;

    for (i = 0; i < AO_CHN_CFG_COUNT; i++)
    {
        IMP_AO_DisableAlgo(ao_chn_cfg[i].dev_id, ao_chn_cfg[i].chn_id);
        IMP_AO_DisableChn(ao_chn_cfg[i].dev_id, ao_chn_cfg[i].chn_id);
    }
    for (i = 0; i < AO_DEV_CFG_COUNT; i++)
        IMP_AO_Disable(ao_dev_cfg[i].dev_id);

    return 0;
}


static int bind(void)
{
    int i;

    for (i = 0; i < BIND_CFG_COUNT; i++)
    {
        if (IMP_System_Bind(&bind_cfg[i].src_cell, &bind_cfg[i].dst_cell))
        {
            ERROR("IMP_System_Bind failed, src(%d,%d,%d) dst(%d,%d,%d) !",
                   bind_cfg[i].src_cell.deviceID,
                   bind_cfg[i].src_cell.groupID,
                   bind_cfg[i].src_cell.outputID,
                   bind_cfg[i].dst_cell.deviceID,
                   bind_cfg[i].dst_cell.groupID,
                   bind_cfg[i].dst_cell.outputID);
            goto EXIT;
        }
    }

    return 0;

EXIT:
    for (; i > 0; i--)
        IMP_System_UnBind(&bind_cfg[i-1].src_cell, &bind_cfg[i-1].dst_cell);

    return -1;
}
static int unbind(void)
{
    int i;

    for (i = 0; i < BIND_CFG_COUNT; i++)
        IMP_System_UnBind(&bind_cfg[i].src_cell, &bind_cfg[i].dst_cell);

    return -1;
}


static void* get_video_stream(void *arg)
{
    stream_id_t stream_id = (stream_id_t)arg;
    video_format_t format;
    unsigned int width;
    unsigned int height;
    unsigned int fps;
    int chnid;
    IMPEncoderStream stream;
    frame_t *frame;
    uint32_t rem_size;
    unsigned int total_len;
    int i = 0;


    pthread_detach(pthread_self());
    prctl(PR_SET_NAME, (unsigned long)"get_video_stream", 0, 0, 0);

    for (i = 0; i < VIDEO_STREAM_CFG_COUNT; i++)
    {
        if (stream_id == video_stream_cfg[i].stream_info.stream_id)
        {
            format = video_stream_cfg[i].stream_info.format;
            width  = video_stream_cfg[i].stream_info.width;
            height = video_stream_cfg[i].stream_info.height;
            fps    = video_stream_cfg[i].stream_info.fps;
            chnid  = video_stream_cfg[i].enc_chnid;
            break;
        }
    }

    while (stream_state_list[stream_id].should_run)
    {
        if (0 > IMP_Encoder_PollingStream(chnid, 1000))
        {
            DEBUG("IMP_Encoder_PollingStream timeout, stream_id[%d] chn[%u] !", stream_id, chnid);
            continue;
        }
        if (IMP_Encoder_GetStream(chnid, &stream, 0))
        {
            DEBUG("IMP_Encoder_GetStream failed, stream_id[%d] chn[%u] !", stream_id, chnid);
            continue;
        }
        if (0 == stream.packCount)
        {
            if (IMP_Encoder_ReleaseStream(chnid, &stream))
            {
                ERROR("IMP_Encoder_ReleaseStream failed, stream_id[%d] chn[%u] !",
                       stream_id, chnid);
            }
            continue;
        }

        total_len = 0;
        for (i = 0; i < stream.packCount; i++)
        {
            total_len += stream.pack[i].length;
        }
        frame = hism_alloc_frame(total_len);
        if (NULL == frame)
        {
            ERROR("hism_alloc_frame(%u) failed !", total_len);
            if (IMP_Encoder_ReleaseStream(chnid, &stream))
            {
                ERROR("IMP_Encoder_ReleaseStream failed, stream_id[%d] chn[%u] !",
                       stream_id, chnid);
            }
            continue;
        }

        frame->stream_id = stream_id;
        frame->pts  = stream.pack[0].timestamp;
        frame->video_info.format = format;
        frame->video_info.fps    = fps;
        frame->video_info.width  = width;
        frame->video_info.height = height;
        frame->size = total_len;
        total_len = 0;
        for (i = 0; i < stream.packCount; i++)
        {
            if (0 == stream.pack[i].length)
                continue;

            rem_size = stream.streamSize - stream.pack[i].offset;
            if (rem_size < stream.pack[i].length)
            {
                memcpy(frame->buf + total_len, (void *)(stream.virAddr + stream.pack[i].offset), rem_size);
                total_len += rem_size;
                memcpy(frame->buf + total_len, (void *)(stream.virAddr), stream.pack[i].length - rem_size);
                total_len += stream.pack[i].length - rem_size;
            }
            else
            {
                memcpy(frame->buf + total_len, (void *)(stream.virAddr + stream.pack[i].offset), stream.pack[i].length);
                total_len += stream.pack[i].length;
            }
        }

        if (hism_put_stream_frame(frame))
            free(frame);

        if (IMP_Encoder_ReleaseStream(chnid, &stream))
        {
            ERROR("IMP_Encoder_ReleaseStream failed, stream_id[%d] chn[%u] !", stream_id, chnid);
        }
    }

    stream_state_list[stream_id].is_running = 0;

    return NULL;
}
static void* get_audio_stream(void *arg)
{
    stream_id_t stream_id = (stream_id_t)arg;
    audio_format_t format;
    unsigned int channels;
    unsigned int sample_rate;
    int dev_id;
    int chn_id;
    IMPAudioFrame data;
    frame_t *frame;
    int i;


    pthread_detach(pthread_self());
    prctl(PR_SET_NAME, (unsigned long)"get_audio_stream", 0, 0, 0);

    for (i = 0; i < AUDIO_STREAM_CFG_COUNT; i++)
    {
        if (stream_id == audio_stream_cfg[i].stream_info.stream_id)
        {
            format      = audio_stream_cfg[i].stream_info.format;
            channels    = audio_stream_cfg[i].stream_info.channels;
            sample_rate = audio_stream_cfg[i].stream_info.sample_rate;
            dev_id      = audio_stream_cfg[i].dev_id;
            chn_id      = audio_stream_cfg[i].chn_id;
            break;
        }
    }

    while (stream_state_list[stream_id].should_run)
    {
        if (IMP_AI_PollingFrame(dev_id, chn_id, 1000))
            continue;

        if (IMP_AI_GetFrame(dev_id, chn_id, &data, NOBLOCK))
        {
            DEBUG("IMP_AI_GetFrame failed !");
            continue;
        }
        frame = hism_alloc_frame(data.len);
        if (NULL == frame)
        {
            ERROR("hism_alloc_frame(%u) failed !", data.len);
            IMP_AI_ReleaseFrame(dev_id, chn_id, &data);
            continue;
        }

        frame->stream_id = stream_id;
        frame->pts = data.timeStamp;
        frame->audio_info.format = format;
        frame->audio_info.channels = channels;
        frame->audio_info.sample_rate = sample_rate;
        frame->size = data.len;
        memcpy(frame->buf, data.virAddr, frame->size);
        if (hism_put_stream_frame(frame))
            free(frame);

        IMP_AI_ReleaseFrame(dev_id, chn_id, &data);
    }

    stream_state_list[stream_id].is_running = 0;
    return NULL;
}


int hism_init(void)
{
    pthread_mutex_lock(&media_mutex);
    if (media_inited)
    {
        pthread_mutex_unlock(&media_mutex);
        return 0;
    }

    memset(algo_cb_list, 0, sizeof(algo_cb_list));
    memset(stream_state_list, 0, sizeof(stream_state_list));

    if (hism_cache_init())
        goto EXIT;

    if (init_sys())
        goto EXIT;
    if (init_isp())
        goto EXIT_SYS;
    if (init_framesource())
        goto EXIT_ISP;
    if (init_ivs())
        goto EXIT_FS;
    if (init_osd())
        goto EXIT_IVS;
    if (init_encoder())
        goto EXIT_OSD;
    if (init_ai())
        goto EXIT_ENC;
    if (init_ao())
        goto EXIT_AI;
    if (bind())
        goto EXIT_AO;

    media_inited = 1;
    pthread_mutex_unlock(&media_mutex);

    return 0;


//EXIT_BIND:
    //unbind();
EXIT_AO:
    deinit_ao();
EXIT_AI:
    deinit_ai();
EXIT_ENC:
    deinit_encoder();
EXIT_OSD:
    deinit_osd();
EXIT_IVS:
    deinit_ivs();
EXIT_FS:
    deinit_framesource();
EXIT_ISP:
    deinit_isp();
EXIT_SYS:
    deinit_sys();
EXIT:
    pthread_mutex_unlock(&media_mutex);
    return -1;
}
int hism_exit(void)
{
    int i, j;

    pthread_mutex_lock(&media_mutex);
    if (0 == media_inited)
    {
        pthread_mutex_unlock(&media_mutex);
        return 0;
    }
    media_inited = 0;

    for (i = 0; i < STREAM_ID_MAX; i++)
    {
        stream_state_list[i].should_run = 0;
    }
    for (i = 0; i < 70; i++)
    {
        usleep(40 * 1000);
        for (j = 0; j < STREAM_ID_MAX; j++)
        {
            if (stream_state_list[j].is_running)
                break;
        }
        if (STREAM_ID_MAX <= j)
            break;
    }
    if (70 <= i)
    {
        ERROR("stream exit timeout !");
    }


    unbind();
    deinit_ao();
    deinit_ai();
    deinit_encoder();
    deinit_osd();
    deinit_ivs();
    deinit_framesource();
    deinit_isp();
    deinit_sys();

    pthread_mutex_unlock(&media_mutex);

    return 0;
}


static int start_video_stream(stream_id_t id)
{
    pthread_t tid;
    int i;


    for (i = 0; i < VIDEO_STREAM_CFG_COUNT; i++)
    {
        if (id == video_stream_cfg[i].stream_info.stream_id)
            break;
    }
    if (VIDEO_STREAM_CFG_COUNT <= i)
    {
        ERROR("video stream[%d] is not configured !", id);
        return -1;
    }

    pthread_mutex_lock(&media_mutex);
    if ((0 == media_inited) || (stream_state_list[id].is_running))
    {
        ERROR("start stream[%d] failed, media_inited[%d] running[%d] !",
               id, media_inited, stream_state_list[id].is_running);
        pthread_mutex_unlock(&media_mutex);
        return -1;
    }

    stream_state_list[id].should_run = 1;
    stream_state_list[id].is_running = 1;
    pthread_mutex_unlock(&media_mutex);

    pthread_create(&tid, NULL, get_video_stream, (void *)id);

    return 0;
}
static int start_audio_stream(stream_id_t id)
{
    pthread_t tid;
    int i;


    for (i = 0; i < AUDIO_STREAM_CFG_COUNT; i++)
    {
        if (id == audio_stream_cfg[i].stream_info.stream_id)
            break;
    }
    if (AUDIO_STREAM_CFG_COUNT <= i)
    {
        ERROR("audio stream[%d] is not configured !", id);
        return -1;
    }

    pthread_mutex_lock(&media_mutex);
    if ((0 == media_inited) || (stream_state_list[id].is_running))
    {
        ERROR("start stream[%d] failed, media_inited[%d] running[%d] !",
               id, media_inited, stream_state_list[id].is_running);
        pthread_mutex_unlock(&media_mutex);
        return -1;
    }
    stream_state_list[id].should_run = 1;
    stream_state_list[id].is_running = 1;
    pthread_mutex_unlock(&media_mutex);

    pthread_create(&tid, NULL, get_audio_stream, (void *)id);

    return 0;
}
int hism_start_stream(stream_id_t id)
{
    if ((STREAM_ID_V_START <= id) && (STREAM_ID_V_MAX > id))
        return start_video_stream(id);
    else if ((STREAM_ID_A_START <= id) && (STREAM_ID_A_MAX > id))
        return start_audio_stream(id);
    else
        return -1;
}
int hism_stop_stream(stream_id_t id)
{
    if ((0 > id) || (STREAM_ID_MAX <= id))
        return -1;

    pthread_mutex_lock(&media_mutex);
    if (0 == stream_state_list[id].is_running)
    {
        pthread_mutex_unlock(&media_mutex);
        return -1;
    }
    stream_state_list[id].should_run = 0;
    pthread_mutex_unlock(&media_mutex);

    return 0;
}


int hism_start_isp_tool(void)
{
    return -1;
}
int hism_stop_isp_tool(void)
{
    return -1;
}


int hism_get_single_frame(single_frame_id_t id, single_frame_t *frame)
{
    int width;
    int height;
    int chn_id;
    IMPFrameInfo info;
    int i;

    if ((0 == media_inited) || (NULL == frame))
        return -1;

    for (i = 0; i < SINGLE_FRAME_CFG_COUNT; i++)
    {
        if (id != single_frame_cfg[i].frame_id)
            continue;

        width  = single_frame_cfg[i].width;
        height = single_frame_cfg[i].height;
        chn_id = single_frame_cfg[i].chn_id;
        break;
    }
    if (SINGLE_FRAME_CFG_COUNT <= i)
    {
        ERROR("single frame[%d] is not configured !", id);
        return -1;
    }

    frame->buf = malloc(width * height * 3 / 2);
    if (NULL == frame->buf)
    {
        ERROR("malloc %u failed !", width * height * 3 / 2);
        return -1;
    }
    if (IMP_FrameSource_SnapFrame(chn_id, PIX_FMT_NV12, width, height, (void *)frame->buf, &info))
    {
        ERROR("IMP_FrameSource_SnapFrame failed !");
        free(frame->buf);
        frame->buf = NULL;
        return -1;
    }
    frame->pts    = info.timeStamp;
    frame->width  = info.width;
    frame->height = info.height;
    frame->stride = 0;
    frame->size   = info.size;
    frame->handle = NULL;

    return 0;
}


int hism_resize_frame(const single_frame_t *in_frame, resize_t *resize, single_frame_t *out_frame)
{
    return -1;
}


int hism_encode_jpeg(const single_frame_t *frame, resize_t *crop, unsigned char *buf, unsigned int size)
{
    return -1;
}


int hism_release_single_frame(single_frame_t *frame)
{
    if ((NULL == frame) || (NULL == frame->buf))
        return -1;

    free(frame->buf);
    frame->buf = NULL;

    return 0;
}


int hism_write_osd(stream_id_t stream_id, osd_id_t osd_id, unsigned char *buf)
{
    int i;

    if ((0 == media_inited) || (NULL == buf))
        return -1;

    for (i = 0; i < OSD_REGION_CFG_COUNT; i++)
    {
        if (   (stream_id == osd_region_cfg[i].stream_id)
            && (   osd_id == osd_region_cfg[i].osd_id))
            break;
    }
    if (OSD_REGION_CFG_COUNT <= i)
        return -1;

    osd_region_cfg[i].rgn_attr.data.bitmapData = buf;
    IMP_OSD_UpdateRgnAttrData(osd_region_cfg[i].handle, &osd_region_cfg[0].rgn_attr.data);

    return 0;
}
int hism_draw_rect(stream_id_t stream_id, const graphic_rel_rect_t *rect, unsigned int count)
{
    osd_grp_cfg_t *cfg;
    unsigned int w, h;
    IMPOSDRgnAttr attr;
    int i;


    if (0 == media_inited)
        return -1;

    for (i = 0; i < OSD_GRP_CFG_COUNT; i++)
    {
        if ((stream_id == osd_grp_cfg[i].stream_id) && osd_grp_cfg[i].rect_rgn_cnt)
            break;
    }
    if (OSD_GRP_CFG_COUNT <= i)
        return -1;
    cfg = &osd_grp_cfg[i];

    if (0 == count)
    {
        for (i = 0; i < cfg->rect_rgn_cnt; i++)
        {
            IMP_OSD_ShowRgn(cfg->rect_rgn_handles[i], cfg->grp, 0);
        }

        return 0;
    }

    if (NULL == rect)
        return -1;
    if (cfg->rect_rgn_cnt < count)
        count = cfg->rect_rgn_cnt;

    for (i = 0; i < VIDEO_STREAM_CFG_COUNT; i++)
    {
        if (stream_id == video_stream_cfg[i].stream_info.stream_id)
        {
            w = video_stream_cfg[i].stream_info.width;
            h = video_stream_cfg[i].stream_info.height;
            break;
        }
    }
    if (VIDEO_STREAM_CFG_COUNT <= i)
        return -1;

    memset(&attr, 0, sizeof(attr));
    attr.type = OSD_REG_RECT;
    attr.fmt = PIX_FMT_MONOWHITE;
    attr.data.lineRectData.linewidth = cfg->rect_linewidth;
    attr.data.lineRectData.linelength = 0;
    attr.data.lineRectData.rectlinelength = 0;
    for (i = 0; i < count; i++)
    {
        attr.rect.p0.x = rect[i].x0 * w / REL_COORD_MAX;
        attr.rect.p0.y = rect[i].y0 * h / REL_COORD_MAX;
        attr.rect.p1.x = rect[i].x1 * w / REL_COORD_MAX;
        attr.rect.p1.y = rect[i].y1 * h / REL_COORD_MAX;
        switch (rect[i].color)
        {
            case COLOR_ID_BLACK:  attr.data.lineRectData.color = OSD_BLACK;  break;
            case COLOR_ID_RED:    attr.data.lineRectData.color = OSD_RED;    break;
            case COLOR_ID_GREEN:  attr.data.lineRectData.color = OSD_GREEN;  break;
            case COLOR_ID_YELLOW: attr.data.lineRectData.color = OSD_YELLOW; break;
            default:              attr.data.lineRectData.color = OSD_YELLOW; break;
        }
        IMP_OSD_SetRgnAttr(cfg->rect_rgn_handles[i], &attr);
        IMP_OSD_ShowRgn(cfg->rect_rgn_handles[i], cfg->grp, 1);
    }
    for (; i < cfg->rect_rgn_cnt; i++)
    {
        IMP_OSD_ShowRgn(cfg->rect_rgn_handles[i], cfg->grp, 0);
    }

    return 0;
}
int hism_draw_line(stream_id_t stream_id, const graphic_rel_rect_t *line, unsigned int count)
{
    osd_grp_cfg_t *cfg;
    unsigned int w, h;
    IMPOSDRgnAttr attr;
    int i;


    if (0 == media_inited)
        return -1;

    for (i = 0; i < OSD_GRP_CFG_COUNT; i++)
    {
        if ((stream_id == osd_grp_cfg[i].stream_id) && osd_grp_cfg[i].slash_rgn_cnt)
            break;
    }
    if (OSD_GRP_CFG_COUNT <= i)
        return -1;
    cfg = &osd_grp_cfg[i];

    if (0 == count)
    {
        for (i = 0; i < cfg->slash_rgn_cnt; i++)
        {
            IMP_OSD_ShowRgn(cfg->slash_rgn_handles[i], cfg->grp, 0);
        }

        return 0;
    }

    if (NULL == line)
        return -1;
    if (cfg->slash_rgn_cnt < count)
        count = cfg->slash_rgn_cnt;

    for (i = 0; i < VIDEO_STREAM_CFG_COUNT; i++)
    {
        if (stream_id == video_stream_cfg[i].stream_info.stream_id)
        {
            w = video_stream_cfg[i].stream_info.width;
            h = video_stream_cfg[i].stream_info.height;
            break;
        }
    }
    if (VIDEO_STREAM_CFG_COUNT <= i)
        return -1;

    memset(&attr, 0, sizeof(attr));
    attr.type = OSD_REG_SLASH;
    attr.fmt = PIX_FMT_MONOWHITE;
    attr.data.lineRectData.linewidth = cfg->slash_linewidth;
    attr.data.lineRectData.linelength = 0;
    attr.data.lineRectData.rectlinelength = 0;
    for (i = 0; i < count; i++)
    {
        attr.rect.p0.x = line[i].x0 * w / REL_COORD_MAX;
        attr.rect.p0.y = line[i].y0 * h / REL_COORD_MAX;
        attr.rect.p1.x = line[i].x1 * w / REL_COORD_MAX;
        attr.rect.p1.y = line[i].y1 * h / REL_COORD_MAX;
        switch (line[i].color)
        {
            case COLOR_ID_BLACK: attr.data.lineRectData.color = OSD_IPU_BLACK; break;
            case COLOR_ID_WHITE: attr.data.lineRectData.color = OSD_IPU_WHITE; break;
            case COLOR_ID_RED:   attr.data.lineRectData.color = OSD_IPU_RED;   break;
            case COLOR_ID_GREEN: attr.data.lineRectData.color = OSD_IPU_GREEN; break;
            case COLOR_ID_BLUE:  attr.data.lineRectData.color = OSD_IPU_BLUE;  break;
            default:             attr.data.lineRectData.color = OSD_IPU_GREEN; break;
        }
        IMP_OSD_SetRgnAttr(cfg->slash_rgn_handles[i], &attr);
        IMP_OSD_ShowRgn(cfg->slash_rgn_handles[i], cfg->grp, 1);
    }
    for (; i < cfg->slash_rgn_cnt; i++)
    {
        IMP_OSD_ShowRgn(cfg->slash_rgn_handles[i], cfg->grp, 0);
    }

    return 0;
}


int hism_play_audio_frame(frame_t *frame)
{
    IMPAudioFrame frm;
    unsigned int size;
    unsigned char *p;

    if ((NULL == frame) || (NULL == frame->buf))
        return -1;

    size = frame->size;
    p = frame->buf;
    while (audio_play_cfg.max_size < size)
    {
        frm.virAddr = (unsigned int *)p;
        frm.len = (int)audio_play_cfg.max_size;
        if (IMP_AO_SendFrame(audio_play_cfg.dev_id, audio_play_cfg.chn_id, &frm, BLOCK))
        {
            ERROR("IMP_AO_SendFrame failed, len(%d) !\n", frm.len);
            return -1;
        }
        size -= audio_play_cfg.max_size;
        p += audio_play_cfg.max_size;
    }
    frm.virAddr = (unsigned int *)p;
    frm.len = (int)size;
    if (IMP_AO_SendFrame(audio_play_cfg.dev_id, audio_play_cfg.chn_id, &frm, BLOCK))
    {
        ERROR("IMP_AO_SendFrame failed, len(%d) !\n", frm.len);
        return -1;
    }

    return 0;
}


/* 暂时没有取消注册操作 */
static int register_algo_cb(algo_id_t algo_id, void *cb)
{
    int i;

    if (NULL == cb)
        return -1;

    pthread_mutex_lock(&media_mutex);
    if (0 == media_inited)
    {
        ERROR("media is not inited !");
        pthread_mutex_unlock(&media_mutex);
        return -1;
    }
    for (i = 0; i < MAX_ALGO_CB; i++)
    {
        if (NULL == algo_cb_list[i].cb)
        {
            algo_cb_list[i].algo_id = algo_id;
            algo_cb_list[i].cb = cb;
            pthread_mutex_unlock(&media_mutex);
            return 0;
        }
    }
    pthread_mutex_unlock(&media_mutex);

    ERROR("cb count exceeds the max(%u) !", MAX_ALGO_CB);
    return -1;
}
int hism_register_algo_motion_det_cb(algo_motion_det_cb_t cb)
{
    return register_algo_cb(ALGO_ID_MOTION_DETECTION, (void *)cb);
}
int hism_suspend_algo_motion_det(unsigned int s)
{
    struct timespec time;

    DEBUG("suspend motion detection %us", s);

    clock_gettime(CLOCK_MONOTONIC, &time);
    pthread_mutex_lock(&media_mutex);
    if (   (md_pause_time.tv_sec > time.tv_sec + s)
        || ((md_pause_time.tv_sec == time.tv_sec + s) && (md_pause_time.tv_nsec > time.tv_nsec)))
    {
        pthread_mutex_unlock(&media_mutex);
        return 0;
    }
    md_pause_time.tv_sec  = time.tv_sec + s;
    md_pause_time.tv_nsec = time.tv_nsec;
    pthread_mutex_unlock(&media_mutex);

    return 0;
}


int hism_register_algo_object_det_cb(algo_object_det_cb_t cb)
{
    return register_algo_cb(ALGO_ID_OBJECT_DETECTION, (void *)cb);
}
