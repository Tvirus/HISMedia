#include "hismedia.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <pthread.h>
#include <sys/prctl.h>

#include "ss_mpi_sys.h"
#include "ss_mpi_sys_mem.h"
#include "ss_mpi_vb.h"
#include "ot_mipi_rx.h"
#include "ss_mpi_vi.h"
#include "ot_sns_ctrl.h"
#include "ss_mpi_isp.h"
#include "ss_mpi_ae.h"
#include "ss_mpi_awb.h"
#include "ss_mpi_aibnr.h"
#include "ss_mpi_vpss.h"
#include "ss_mpi_venc.h"
#include "ss_mpi_audio.h"
#include "ot_acodec.h"
#include "ss_mpi_sys_bind.h"
#include "ss_mpi_region.h"




/*************************************************
    VB Config
**************************************************/
typedef struct
{
    ot_vb_cfg cfg;
}vb_cfg_t;

typedef struct
{
    ot_vb_pool id;
    ot_vb_pool_cfg cfg;
}user_vb_pool_cfg_t;


/*************************************************
    VI Config
**************************************************/
typedef struct
{
    td_u32 dev;
    combo_dev_attr_t combo_dev_attr;
    ext_data_type_t ext_data_attr;
}vi_mipi_cfg_t;

typedef struct
{
    ot_vi_dev dev;
    ot_vi_dev_attr attr;
    ot_vi_bind_pipe bind_pipe;
}vi_dev_cfg_t;

typedef struct
{
    ot_vi_grp fusion_grp;
    ot_vi_wdr_fusion_grp_attr fusion_grp_attr;
}vi_grp_cfg_t;

typedef struct
{
    ot_vi_pipe vi_pipe;
    td_u32 bnr_bnf_num;
    ot_vi_pipe_attr pipe_attr;
    td_u32 vc_number;
    /* 只有主pipe才需要设置attr_3dnr中的enable为1 !!! */
    ot_3dnr_attr attr_3dnr;
    ot_vb_src vb_src;
    int user_vb_pool_idx;
}vi_pipe_cfg_t;

/* 只有主pipe才需要设置chn !!! */
typedef struct
{
    ot_vi_pipe vi_pipe;
    ot_vi_chn vi_chn;
    ot_vi_chn_attr attr;
    ot_fmu_mode fmu_mode;
}vi_chn_cfg_t;

typedef struct
{
    ot_vi_pipe vi_pipe;
    ot_isp_sns_obj *sns_obj;
    ot_isp_3a_alg_lib ae_lib;
    ot_isp_3a_alg_lib awb_lib;
    ot_isp_sns_commbus sns_bus_info;
    ot_isp_pub_attr isp_pub_attr;
}vi_isp_cfg_t;

typedef struct
{
    ot_vi_pipe vi_pipe;
    const char *model_file;
    td_s32 model_id;
    ot_aibnr_model aibnr_model;
    ot_aibnr_cfg aibnr_cfg;
    ot_aibnr_attr aibnr_attr;
}vi_aibnr_cfg_t;


/*************************************************
    VPSS Config
**************************************************/
typedef struct
{
    ot_vpss_grp grp;
    ot_vpss_grp_attr attr;
}vpss_grp_cfg_t;

typedef struct
{
    ot_vpss_grp grp;
    ot_vpss_chn chn;
     ot_vpss_chn_attr attr;
}vpss_chn_cfg_t;


/*************************************************
    VENC Config
**************************************************/
typedef struct
{
    ot_venc_chn chn;
    ot_venc_chn_attr attr;
    ot_venc_start_param start_param;
}venc_cfg_t;


/*************************************************
    AIO Config
**************************************************/
typedef struct
{
    ot_audio_dev dev;
    ot_aio_attr attr;
}ai_dev_cfg_t;

typedef struct
{
    ot_audio_dev dev;
    ot_ai_chn chn;
    ot_ai_chn_param param;
}ai_chn_cfg_t;

typedef struct
{
    ot_audio_dev dev;
    ot_aio_attr attr;
}ao_dev_cfg_t;

typedef struct
{
    ot_audio_dev dev;
    ot_ao_chn chn;
}ao_chn_cfg_t;

typedef struct
{
    ot_acodec_fs sample_rate;
    ot_acodec_mixer input_mode;
    int input_vol;
}acodec_cfg_t;


/*************************************************
    Bind Config
**************************************************/
typedef struct
{
    ot_mpp_chn src;
    ot_mpp_chn dst;
}bind_cfg_t;


/*************************************************
    Region Config
**************************************************/
typedef struct
{
    ot_rgn_handle handle;
    ot_rgn_attr attr;
}region_cfg_t;

typedef struct
{
    ot_rgn_handle handle;
    int attach_chn;  /* attach to chn or dev */
    ot_mpp_chn chn;
    ot_rgn_chn_attr chn_attr;
}region_attach_cfg_t;


/*************************************************
    Video Stream Config
**************************************************/
typedef struct
{
    video_stream_info_t stream_info;
    int cache_id;
    int venc_chn;
}video_stream_cfg_t;


/*************************************************
    Audio Stream Config
**************************************************/
typedef struct
{
    audio_stream_info_t stream_info;
    int cache_id;
    ot_audio_dev ai_dev;
    ot_ai_chn ai_chn;
}audio_stream_cfg_t;


/*************************************************
    OSD Config
**************************************************/
typedef struct
{
    osd_info_t osd_info;
    ot_rgn_handle handle;
    ot_bmp bmp;
}osd_cfg_t;


/*************************************************
    Config End
**************************************************/


#include "hismedia_config.h"


#define DEBUG(fmt, arg...)  do{if(media_debug)printf("--Media-- " fmt "\n", ##arg);}while(0)
#define ERROR(fmt, arg...)  printf("\e[1;31m--Media-- %s: " fmt "\e[0m\n", __func__, ##arg)

#define MIPI_DEV_NAME "/dev/ot_mipi_rx"


typedef struct
{
    stream_id_t stream_id;
    unsigned char should_run;
    unsigned char is_running;
}stream_state_t;

typedef struct
{
    algo_id_t algo_id;
    void *cb;
}algo_cb_info_t;


static pthread_mutex_t media_mutex = PTHREAD_MUTEX_INITIALIZER;
static stream_state_t stream_state_list[STREAM_COUNT];
#define MAX_ALGO_CB 7
static algo_cb_info_t algo_cb_list[MAX_ALGO_CB];

static unsigned char media_inited = 0; /* -1:退出失败，0:未初始化，1:已初始化 */
unsigned char media_debug = 1;


void hism_set_debug(unsigned char level)
{
    media_debug = level;
}


static int init_sys(void)
{
    td_s32 ret;
    int i;

    ss_mpi_sys_exit();
    ss_mpi_vb_exit();

    ret = ss_mpi_vb_set_cfg(&vb_cfg.cfg);
    if (ret)
    {
        ERROR("ss_mpi_vb_set_cfg failed, err: %x !", ret);
        return -1;
    }

    ret = ss_mpi_vb_init();
    if (ret)
    {
        ERROR("ss_mpi_vb_init failed, err: %x !", ret);
        return -1;
    }

    ret = ss_mpi_sys_init();
    if (ret)
    {
        ERROR("ss_mpi_sys_init failed, err: %x !", ret);
        ss_mpi_vb_exit();
        return -1;
    }

    for (i = 0; i < USER_VB_CFG_COUNT; i++)
    {
        user_vb_pool_cfg[i].id = ss_mpi_vb_create_pool(&user_vb_pool_cfg[i].cfg);
        if (0 > user_vb_pool_cfg[i].id)
        {
            ERROR("ss_mpi_vb_create_pool failed, size(%llu) cnt(%u) err: %x !",
                   user_vb_pool_cfg[i].cfg.blk_size, user_vb_pool_cfg[i].cfg.blk_cnt, ret);
            break;
        }
    }
    if (i < USER_VB_CFG_COUNT)
    {
        for (i--; i >= 0; i--)
        {
            ss_mpi_vb_destroy_pool(user_vb_pool_cfg[i].id);
            user_vb_pool_cfg[i].id = -1;
        }
        ss_mpi_sys_exit();
        ss_mpi_vb_exit();
        return -1;
    }

    return 0;
}
static int deinit_sys(void)
{
    int i;

    for (i = 0; i < USER_VB_CFG_COUNT; i++)
    {
        ss_mpi_vb_destroy_pool(user_vb_pool_cfg[i].id);
        user_vb_pool_cfg[i].id = -1;
    }

    ss_mpi_sys_exit();
    ss_mpi_vb_exit();

    return 0;
}


static void* isp_thread(void *arg)
{
    char name[20];
    td_s32 ret;

    pthread_detach(pthread_self());
    sprintf(name, "isp_%ld", (long)arg);
    prctl(PR_SET_NAME, (unsigned long)name, 0, 0, 0);

    ret = ss_mpi_isp_run((long)arg);
    if (ret)
        ERROR("ss_mpi_isp_run failed, err: %x !", ret);

    return NULL;
}
static int init_vi(void)
{
    int dev_enabled = 0;
    int pipe_bound = 0;
    int pipe_created = 0;
    int pipe_started = 0;
    int chn_enabled = 0;
    int sns_registered = 0;
    int ae_registered = 0;
    int awb_registered = 0;
    int pool_idx;
    ot_isp_sns_obj *sns_obj;
    pthread_t tid;
    td_s32 ret;
    int fd;
    int i, j;


    /* MIPI */
    fd = open(MIPI_DEV_NAME, O_RDWR);
    if (0 > fd)
    {
        ERROR("open %s failed !", MIPI_DEV_NAME);
        return -1;
    }

    if (TD_SUCCESS != ioctl(fd, OT_MIPI_SET_HS_MODE, &lane_divide_mode))
    {
        ERROR("OT_MIPI_SET_HS_MODE failed !");
        goto EXIT;
    }

    for (i = 0; i < VI_MIPI_CFG_COUNT; i++)
    {
        if (ioctl(fd, OT_MIPI_ENABLE_MIPI_CLOCK, &vi_mipi_cfg[i].dev))
        {
            ERROR("OT_MIPI_ENABLE_MIPI_CLOCK failed !");
            goto EXIT;
        }
        if (ioctl(fd, OT_MIPI_RESET_MIPI, &vi_mipi_cfg[i].dev))
        {
            ERROR("OT_MIPI_RESET_MIPI failed !");
            goto EXIT;
        }
        if (ioctl(fd, OT_MIPI_ENABLE_SENSOR_CLOCK, &vi_mipi_cfg[i].dev))
        {
            ERROR("OT_MIPI_ENABLE_SENSOR_CLOCK failed !");
            goto EXIT;
        }
        if (ioctl(fd, OT_MIPI_RESET_SENSOR, &vi_mipi_cfg[i].dev))
        {
            ERROR("OT_MIPI_RESET_SENSOR failed !");
            goto EXIT;
        }
        if (ioctl(fd, OT_MIPI_SET_DEV_ATTR, &vi_mipi_cfg[i].combo_dev_attr))
        {
            ERROR("OT_MIPI_SET_DEV_ATTR failed !");
            goto EXIT;
        }
        if (ioctl(fd, OT_MIPI_SET_EXT_DATA_TYPE, &vi_mipi_cfg[i].ext_data_attr))
        {
            ERROR("OT_MIPI_SET_EXT_DATA_TYPE failed !");
            goto EXIT;
        }
        if (ioctl(fd, OT_MIPI_UNRESET_MIPI, &vi_mipi_cfg[i].ext_data_attr))
        {
            ERROR("OT_MIPI_UNRESET_MIPI failed !");
            goto EXIT;
        }
        if (ioctl(fd, OT_MIPI_UNRESET_SENSOR, &vi_mipi_cfg[i].ext_data_attr))
        {
            ERROR("OT_MIPI_UNRESET_SENSOR failed !");
            goto EXIT;
        }
    }

    /* VI DEV */
    for (i = 0; i < VI_DEV_CFG_COUNT; i++)
    {
        ret = ss_mpi_vi_set_dev_attr(vi_dev_cfg[i].dev, &vi_dev_cfg[i].attr);
        if (ret)
        {
            ERROR("ss_mpi_vi_set_dev_attr failed, dev[%d] err: %x !", vi_dev_cfg[i].dev, ret);
            goto EXIT;
        }
        ret = ss_mpi_vi_enable_dev(vi_dev_cfg[i].dev);
        if (ret)
        {
            ERROR("ss_mpi_vi_enable_dev failed, dev[%d] err: %x !", vi_dev_cfg[i].dev, ret);
            goto EXIT;
        }
        dev_enabled++;

        for (j = 0; j < vi_dev_cfg[i].bind_pipe.pipe_num; j++)
        {
            ret = ss_mpi_vi_bind(vi_dev_cfg[i].dev, vi_dev_cfg[i].bind_pipe.pipe_id[j]);
            if (ret)
            {
                ERROR("ss_mpi_vi_bind failed, dev[%d] pipe[%d] err: %x !",
                       vi_dev_cfg[i].dev, vi_dev_cfg[i].bind_pipe.pipe_id[j], ret);
                break;
            }
        }
        if (j < vi_dev_cfg[i].bind_pipe.pipe_num)
        {
            for (j--; j >= 0; j--)
                ss_mpi_vi_unbind(vi_dev_cfg[i].dev, vi_dev_cfg[i].bind_pipe.pipe_id[j]);
            goto EXIT;
        }
        pipe_bound++;
    }

    /* Group */
    for (i = 0; i < VI_GRP_CFG_COUNT; i++)
    {
        ret = ss_mpi_vi_set_wdr_fusion_grp_attr( vi_grp_cfg[i].fusion_grp,
                                                &vi_grp_cfg[i].fusion_grp_attr);
        if (ret)
        {
            ERROR("ss_mpi_vi_set_wdr_fusion_grp_attr failed, grp[%d] err: %x !",
                   vi_grp_cfg[i].fusion_grp, ret);
            goto EXIT;
        }
    }

    /* PIPE */
    ret = ss_mpi_sys_set_vi_vpss_mode(&vi_vpss_mode_cfg);
    if (ret)
    {
        ERROR("ss_mpi_sys_set_vi_vpss_mode failed, err: %x !", ret);
        goto EXIT;
    }

    for (i = 0; i < VI_PIPE_CFG_COUNT; i++)
    {
        ret = ss_mpi_vi_set_pipe_bnr_buf_num(vi_pipe_cfg[i].vi_pipe,
                                             vi_pipe_cfg[i].bnr_bnf_num);
        if (ret)
        {
            ERROR("ss_mpi_vi_set_pipe_bnr_buf_num failed, pipe[%d] num(%u) err: %x !",
                   vi_pipe_cfg[i].vi_pipe, vi_pipe_cfg[i].bnr_bnf_num, ret);
            goto EXIT;
        }
        ret = ss_mpi_vi_create_pipe(vi_pipe_cfg[i].vi_pipe, &vi_pipe_cfg[i].pipe_attr);
        if (ret)
        {
            ERROR("ss_mpi_vi_create_pipe failed, pipe[%d] err: %x !",
                   vi_pipe_cfg[i].vi_pipe, ret);
            goto EXIT;
        }
        pipe_created++;

        ret = ss_mpi_vi_set_pipe_vc_number(vi_pipe_cfg[i].vi_pipe, vi_pipe_cfg[i].vc_number);
        if (ret)
        {
            ERROR("ss_mpi_vi_set_pipe_vc_number failed, pipe[%d] num(%u) err: %x !",
                   vi_pipe_cfg[i].vi_pipe, vi_pipe_cfg[i].vc_number, ret);
            goto EXIT;
        }
        ret = ss_mpi_vi_set_pipe_vb_src(vi_pipe_cfg[i].vi_pipe, vi_pipe_cfg[i].vb_src);
        if (ret)
        {
            ERROR("ss_mpi_vi_set_pipe_vb_src failed, pipe[%d] src(%u) err: %x !",
                   vi_pipe_cfg[i].vi_pipe, vi_pipe_cfg[i].vb_src, ret);
            goto EXIT;
        }
        pool_idx = vi_pipe_cfg[i].user_vb_pool_idx;
        if ((0 <= pool_idx) && (USER_VB_CFG_COUNT > pool_idx)
            && (0 <= user_vb_pool_cfg[pool_idx].id))
        {
            ret = ss_mpi_vi_attach_pipe_vb_pool(vi_pipe_cfg[i].vi_pipe,
                                                user_vb_pool_cfg[pool_idx].id);
            if (ret)
            {
                ERROR("ss_mpi_vi_attach_pipe_vb_pool failed, pipe[%d] pool[%u] err: %x !",
                       vi_pipe_cfg[i].vi_pipe, user_vb_pool_cfg[pool_idx].id, ret);
                goto EXIT;
            }
        }
        ret = ss_mpi_vi_start_pipe(vi_pipe_cfg[i].vi_pipe);
        if (ret)
        {
            ERROR("ss_mpi_vi_start_pipe failed, pipe[%d] err: %x !",
                   vi_pipe_cfg[i].vi_pipe, ret);
            goto EXIT;
        }
        pipe_started++;
    }

    /* Channel */
    for (i = 0; i < VI_CHN_CFG_COUNT; i++)
    {
        ret = ss_mpi_vi_set_chn_attr(vi_chn_cfg[i].vi_pipe, vi_chn_cfg[i].vi_chn,
                                     &vi_chn_cfg[i].attr);
        if (ret)
        {
            ERROR("ss_mpi_vi_set_chn_attr failed, pipe[%d] chn[%d] err: %x !",
                   vi_chn_cfg[i].vi_pipe, vi_chn_cfg[i].vi_chn, ret);
            goto EXIT;
        }
        ret = ss_mpi_vi_set_chn_fmu_mode(vi_chn_cfg[i].vi_pipe, vi_chn_cfg[i].vi_chn,
                                         vi_chn_cfg[i].fmu_mode);
        if (ret)
        {
            ERROR("ss_mpi_vi_set_chn_fmu_mode failed, pipe[%d] chn[%d] mode(%d) err: %x !",
                   vi_chn_cfg[i].vi_pipe, vi_chn_cfg[i].vi_chn, vi_chn_cfg[i].fmu_mode, ret);
            goto EXIT;
        }
        ret = ss_mpi_vi_enable_chn(vi_chn_cfg[i].vi_pipe, vi_chn_cfg[i].vi_chn);
        if (ret)
        {
            ERROR("ss_mpi_vi_enable_chn failed, pipe[%d] chn[%d] err: %x !",
                   vi_chn_cfg[i].vi_pipe, vi_chn_cfg[i].vi_chn, ret);
            goto EXIT;
        }
        chn_enabled++;
    }
    for (i = 0; i < VI_PIPE_CFG_COUNT; i++)
    {
        ret = ss_mpi_vi_set_pipe_3dnr_attr(vi_pipe_cfg[i].vi_pipe, &vi_pipe_cfg[i].attr_3dnr);
        if (ret)
        {
            ERROR("ss_mpi_vi_set_pipe_3dnr_attr failed, pipe[%d] err: %x !",
                   vi_pipe_cfg[i].vi_pipe, ret);
            goto EXIT;
        }
    }

    /* ISP */
    for (i = 0; i < VI_ISP_CFG_COUNT; i++)
    {
        sns_obj = vi_isp_cfg[i].sns_obj;
        ret = sns_obj->pfn_register_callback( vi_isp_cfg[i].vi_pipe,
                                             &vi_isp_cfg[i].ae_lib,
                                             &vi_isp_cfg[i].awb_lib);
        if (ret)
        {
            ERROR("pfn_register_callback failed, pipe[%d] err: %x !",
                   vi_isp_cfg[i].vi_pipe, ret);
            goto EXIT;
        }
        sns_registered++;

        ret = sns_obj->pfn_set_bus_info(vi_isp_cfg[i].vi_pipe, vi_isp_cfg[i].sns_bus_info);
        if (ret)
        {
            ERROR("pfn_set_bus_info failed, pipe[%d] err: %x !", vi_isp_cfg[i].vi_pipe, ret);
            goto EXIT;
        }

        ret = ss_mpi_ae_register(vi_isp_cfg[i].vi_pipe, &vi_isp_cfg[i].ae_lib);
        if (ret)
        {
            ERROR("ss_mpi_ae_register failed, pipe[%d] err: %x !",
                   vi_isp_cfg[i].vi_pipe, ret);
            goto EXIT;
        }
        ae_registered++;

        ret = ss_mpi_awb_register(vi_isp_cfg[i].vi_pipe, &vi_isp_cfg[i].awb_lib);
        if (ret)
        {
            ERROR("ss_mpi_awb_register failed, pipe[%d] err: %x !",
                   vi_isp_cfg[i].vi_pipe, ret);
            goto EXIT;
        }
        awb_registered++;

        ret = ss_mpi_isp_mem_init(vi_isp_cfg[i].vi_pipe);
        if (ret)
        {
            ERROR("ss_mpi_isp_mem_init failed, pipe[%d] err: %x !",
                   vi_isp_cfg[i].vi_pipe, ret);
            goto EXIT;
        }
        ret = ss_mpi_isp_set_pub_attr(vi_isp_cfg[i].vi_pipe, &vi_isp_cfg[i].isp_pub_attr);
        if (ret)
        {
            ERROR("ss_mpi_isp_set_pub_attr failed, pipe[%d] err: %x !",
                   vi_isp_cfg[i].vi_pipe, ret);
            goto EXIT;
        }
        ret = ss_mpi_isp_init(vi_isp_cfg[i].vi_pipe);
        if (ret)
        {
            ERROR("ss_mpi_isp_init failed, pipe[%d] err: %x !", vi_isp_cfg[i].vi_pipe, ret);
            goto EXIT;
        }
        if (pthread_create(&tid, NULL, isp_thread, (void *)((long)vi_isp_cfg[i].vi_pipe)))
        {
            ERROR("pthread_create isp failed, pipe[%d] err: %x !",
                   vi_isp_cfg[i].vi_pipe, ret);
            goto EXIT;
        }
    }

    close(fd);
    return 0;


EXIT:
    for (i = 0; i < VI_ISP_CFG_COUNT; i++)
        ss_mpi_isp_exit(vi_isp_cfg[i].vi_pipe);
    for (i = 0; i < awb_registered; i++)
        ss_mpi_awb_unregister(vi_isp_cfg[i].vi_pipe, &vi_isp_cfg[i].awb_lib);
    for (i = 0; i < ae_registered; i++)
        ss_mpi_ae_unregister(vi_isp_cfg[i].vi_pipe, &vi_isp_cfg[i].ae_lib);
    for (i = 0; i < sns_registered; i++)
        sns_obj->pfn_un_register_callback( vi_isp_cfg[i].vi_pipe,
                                          &vi_isp_cfg[i].ae_lib,
                                          &vi_isp_cfg[i].awb_lib);
    for (i = 0; i < chn_enabled; i++)
        ss_mpi_vi_disable_chn(vi_chn_cfg[i].vi_pipe, vi_chn_cfg[i].vi_chn);
    for (i = 0; i < pipe_started; i++)
        ss_mpi_vi_stop_pipe(vi_pipe_cfg[i].vi_pipe);
    for (i = 0; i < pipe_created; i++)
    {
        ss_mpi_vi_detach_pipe_vb_pool(vi_pipe_cfg[i].vi_pipe);
        ss_mpi_vi_destroy_pipe(vi_pipe_cfg[i].vi_pipe);
    }
    for (i = 0; i < pipe_bound; i++)
    {
        for (j = 0; j < vi_dev_cfg[i].bind_pipe.pipe_num; j++)
            ss_mpi_vi_unbind(vi_dev_cfg[i].dev, vi_dev_cfg[i].bind_pipe.pipe_id[j]);
    }
    for (i = 0; i < dev_enabled; i++)
        ss_mpi_vi_disable_dev(vi_dev_cfg[i].dev);
    for (i = 0; i < VI_MIPI_CFG_COUNT; i++)
    {
        ioctl(fd, OT_MIPI_RESET_SENSOR, &vi_mipi_cfg[i].dev);
        ioctl(fd, OT_MIPI_DISABLE_SENSOR_CLOCK, &vi_mipi_cfg[i].dev);
        ioctl(fd, OT_MIPI_RESET_MIPI, &vi_mipi_cfg[i].dev);
        ioctl(fd, OT_MIPI_DISABLE_MIPI_CLOCK, &vi_mipi_cfg[i].dev);
    }

    close(fd);
    return -1;
}
static int deinit_vi(void)
{
    ot_isp_sns_obj *sns_obj;
    int fd;
    int i, j;


    for (i = 0; i < VI_ISP_CFG_COUNT; i++)
    {
        sns_obj = vi_isp_cfg[i].sns_obj;
        ss_mpi_isp_exit(vi_isp_cfg[i].vi_pipe);
        ss_mpi_awb_unregister(vi_isp_cfg[i].vi_pipe, &vi_isp_cfg[i].awb_lib);
        ss_mpi_ae_unregister(vi_isp_cfg[i].vi_pipe, &vi_isp_cfg[i].ae_lib);
        sns_obj->pfn_un_register_callback( vi_isp_cfg[i].vi_pipe,
                                          &vi_isp_cfg[i].ae_lib,
                                          &vi_isp_cfg[i].awb_lib);
    }
    for (i = 0; i < VI_CHN_CFG_COUNT; i++)
        ss_mpi_vi_disable_chn(vi_chn_cfg[i].vi_pipe, vi_chn_cfg[i].vi_chn);
    for (i = 0; i < VI_PIPE_CFG_COUNT; i++)
    {
        ss_mpi_vi_stop_pipe(vi_pipe_cfg[i].vi_pipe);
        ss_mpi_vi_detach_pipe_vb_pool(vi_pipe_cfg[i].vi_pipe);
        ss_mpi_vi_destroy_pipe(vi_pipe_cfg[i].vi_pipe);
    }
    for (i = 0; i < VI_DEV_CFG_COUNT; i++)
    {
        for (j = 0; j < vi_dev_cfg[i].bind_pipe.pipe_num; j++)
            ss_mpi_vi_unbind(vi_dev_cfg[i].dev, vi_dev_cfg[i].bind_pipe.pipe_id[j]);
        ss_mpi_vi_disable_dev(vi_dev_cfg[i].dev);
    }

    fd = open(MIPI_DEV_NAME, O_RDWR);
    if (0 > fd)
    {
        ERROR("open %s failed !", MIPI_DEV_NAME);
        return -1;
    }
    for (i = 0; i < VI_MIPI_CFG_COUNT; i++)
    {
        ioctl(fd, OT_MIPI_RESET_SENSOR, &vi_mipi_cfg[i].dev);
        ioctl(fd, OT_MIPI_DISABLE_SENSOR_CLOCK, &vi_mipi_cfg[i].dev);
        ioctl(fd, OT_MIPI_RESET_MIPI, &vi_mipi_cfg[i].dev);
        ioctl(fd, OT_MIPI_DISABLE_MIPI_CLOCK, &vi_mipi_cfg[i].dev);
    }

    close(fd);
    return 0;
}


static int aiisp_load_mem(ot_aiisp_mem_info *info, const char *file)
{
    FILE *fp = NULL;
    td_s32 ret;


    info->phys_addr = 0;
    info->virt_addr = NULL;

    fp = fopen(file, "rb");
    if (NULL == fp)
    {
        ERROR("fopen failed, \"%s\" !", file);
        return -1;
    }

    if (fseek(fp, 0L, SEEK_END))
    {
        ERROR("fseek end failed !");
        goto EXIT;
    }

    info->size = ftell(fp);
    if (0 >= info->size)
    {
        ERROR("ftell failed !");
        goto EXIT;
    }

    if (fseek(fp, 0L, SEEK_SET))
    {
        ERROR("fseek set failed !");
        goto EXIT;
    }

    ret = ss_mpi_sys_mmz_alloc(&(info->phys_addr), &(info->virt_addr), file, NULL, info->size);
    if (ret || (0 == info->phys_addr) || (NULL == info->virt_addr))
    {
        ERROR("ss_mpi_sys_mmz_alloc failed, size(%d) err: %x !", info->size, ret);
        goto EXIT;
    }

    if (1 != fread(info->virt_addr, info->size, 1, fp))
    {
        ERROR("fread failed !");
        goto EXIT;
    }

    fclose(fp);
    return 0;


EXIT:
    if (info->phys_addr)
        ss_mpi_sys_mmz_free(info->phys_addr, info->virt_addr);
    info->phys_addr = 0;
    info->virt_addr = NULL;
    if (fp)
        fclose(fp);

    return -1;
}
static int init_aiisp(void)
{
    int aibnr_inited = 0;
    int aibnr_loaded = 0;
    int aibnr_enabled = 0;
    td_s32 ret;
    int i;


    if (0 == VI_AIBNR_CFG_COUNT)
        return 0;

    if (VI_AIBNR_CFG_COUNT)
    {
        ret = ss_mpi_aibnr_init();
        if (ret)
        {
            ERROR("ss_mpi_aibnr_init failed, err: %x !", ret);
            goto EXIT;
        }
        aibnr_inited = 1;
    }
    for (i = 0; i < VI_AIBNR_CFG_COUNT; i++)
    {
        if (aiisp_load_mem(&vi_aibnr_cfg[i].aibnr_model.model.mem_info,
                            vi_aibnr_cfg[i].model_file))
            goto EXIT;
        ret = ss_mpi_aibnr_load_model(&vi_aibnr_cfg[i].aibnr_model, &vi_aibnr_cfg[i].model_id);
        if (ret)
        {
            ERROR("ss_mpi_aibnr_load_model failed, err: %x !", ret);
            ss_mpi_sys_mmz_free(vi_aibnr_cfg[i].aibnr_model.model.mem_info.phys_addr,
                                vi_aibnr_cfg[i].aibnr_model.model.mem_info.virt_addr);
            goto EXIT;
        }
        aibnr_loaded++;

        ret = ss_mpi_aibnr_set_cfg(vi_aibnr_cfg[i].vi_pipe, &vi_aibnr_cfg[i].aibnr_cfg);
        if (ret)
        {
            ERROR("ss_mpi_aibnr_set_cfg failed, pipe[%d] err: %x !",
                   vi_aibnr_cfg[i].vi_pipe, ret);
            goto EXIT;
        }
        ret = ss_mpi_aibnr_enable(vi_aibnr_cfg[i].vi_pipe);
        if (ret)
        {
            ERROR("ss_mpi_aibnr_enable failed, pipe[%d] err: %x !",
                   vi_aibnr_cfg[i].vi_pipe, ret);
            goto EXIT;
        }
        aibnr_enabled++;

        ret = ss_mpi_aibnr_set_attr(vi_aibnr_cfg[i].vi_pipe, &vi_aibnr_cfg[i].aibnr_attr);
        if (ret)
        {
            ERROR("ss_mpi_aibnr_set_attr failed, pipe[%d] err: %x !",
                   vi_aibnr_cfg[i].vi_pipe, ret);
            goto EXIT;
        }
    }

    return 0;


EXIT:
    for (i = 0; i < aibnr_enabled; i++)
        ss_mpi_aibnr_disable(vi_aibnr_cfg[i].vi_pipe);
    for (i = 0; i < aibnr_loaded; i++)
    {
        ss_mpi_aibnr_unload_model(vi_aibnr_cfg[i].model_id);
        ss_mpi_sys_mmz_free(vi_aibnr_cfg[i].aibnr_model.model.mem_info.phys_addr,
                            vi_aibnr_cfg[i].aibnr_model.model.mem_info.virt_addr);
    }
    if (aibnr_inited)
        ss_mpi_aibnr_exit();

    return -1;
}
static int deinit_aiisp(void)
{
    int i;

    for (i = 0; i < VI_AIBNR_CFG_COUNT; i++)
    {
        ss_mpi_aibnr_disable(vi_aibnr_cfg[i].vi_pipe);
        ss_mpi_aibnr_unload_model(vi_aibnr_cfg[i].model_id);
        if (vi_aibnr_cfg[i].aibnr_model.model.mem_info.phys_addr)
            ss_mpi_sys_mmz_free(vi_aibnr_cfg[i].aibnr_model.model.mem_info.phys_addr,
                                vi_aibnr_cfg[i].aibnr_model.model.mem_info.virt_addr);
    }
    if (VI_AIBNR_CFG_COUNT)
        ss_mpi_aibnr_exit();

    return 0;
}


static int init_vpss(void)
{
    int grp_created = 0;
    int grp_started = 0;
    int chn_enabled = 0;
    td_s32 ret;
    int i;


    for (i = 0; i < VPSS_GRP_CFG_COUNT; i++)
    {
        ret = ss_mpi_vpss_create_grp(vpss_grp_cfg[i].grp, &vpss_grp_cfg[i].attr);
        if (ret)
        {
            ERROR("ss_mpi_vpss_create_grp failed, grp[%d] err: %x !", vpss_grp_cfg[i].grp, ret);
            goto EXIT;
        }
        grp_created++;

        ret = ss_mpi_vpss_start_grp(vpss_grp_cfg[i].grp);
        if (ret)
        {
            ERROR("ss_mpi_vpss_start_grp failed, grp[%d] err: %x !", vpss_grp_cfg[i].grp, ret);
            goto EXIT;
        }
        grp_started++;
    }

    for (i = 0; i < VPSS_CHN_CFG_COUNT; i++)
    {
        ret = ss_mpi_vpss_set_chn_attr( vpss_chn_cfg[i].grp,
                                        vpss_chn_cfg[i].chn,
                                       &vpss_chn_cfg[i].attr);
        if (ret)
        {
            ERROR("ss_mpi_vpss_set_chn_attr failed, grp[%d] chn[%d] err: %x !",
                   vpss_chn_cfg[i].grp, vpss_chn_cfg[i].chn, ret);
            goto EXIT;
        }

        ret = ss_mpi_vpss_enable_chn(vpss_chn_cfg[i].grp, vpss_chn_cfg[i].chn);
        if (ret)
        {
            ERROR("ss_mpi_vpss_enable_chn failed, grp[%d] chn[%d] err: %x !",
                   vpss_chn_cfg[i].grp, vpss_chn_cfg[i].chn, ret);
            goto EXIT;
        }
        chn_enabled++;
    }

    return 0;


EXIT:
    for (i = 0; i < chn_enabled; i++)
        ss_mpi_vpss_disable_chn(vpss_chn_cfg[i].grp, vpss_chn_cfg[i].chn);
    for (i = 0; i < grp_started; i++)
        ss_mpi_vpss_stop_grp(vpss_grp_cfg[i].grp);
    for (i = 0; i < grp_created; i++)
        ss_mpi_vpss_destroy_grp(vpss_grp_cfg[i].grp);

    return -1;
}
static int deinit_vpss(void)
{
    int i;

    for (i = 0; i < VPSS_CHN_CFG_COUNT; i++)
        ss_mpi_vpss_disable_chn(vpss_chn_cfg[i].grp, vpss_chn_cfg[i].chn);
    for (i = 0; i < VPSS_GRP_CFG_COUNT; i++)
    {
        ss_mpi_vpss_stop_grp(vpss_grp_cfg[i].grp);
        ss_mpi_vpss_destroy_grp(vpss_grp_cfg[i].grp);
    }

    return 0;
}


static int init_venc(void)
{
    int chn_created = 0;
    int chn_started = 0;
    td_s32 ret;
    int i;


    for (i = 0; i < VENC_CFG_COUNT; i++)
    {
        ret = ss_mpi_venc_create_chn(venc_cfg[i].chn, &venc_cfg[i].attr);
        if (ret)
        {
            ERROR("ss_mpi_venc_create_chn failed, chn[%d] err: %x !", venc_cfg[i].chn, ret);
            goto EXIT;
        }
        chn_created++;

        ret = ss_mpi_venc_start_chn(venc_cfg[i].chn, &venc_cfg[i].start_param);
        if (ret)
        {
            ERROR("ss_mpi_venc_start_chn failed, chn[%d] err: %x !", venc_cfg[i].chn, ret);
            goto EXIT;
        }
        chn_started++;
    }

    return 0;


EXIT:
    for (i = 0; i < chn_started; i++)
        ss_mpi_venc_stop_chn(venc_cfg[i].chn);
    for (i = 0; i < chn_created; i++)
        ss_mpi_venc_destroy_chn(venc_cfg[i].chn);

    return -1;
}
static int deinit_venc(void)
{
    int i;

    for (i = 0; i < VENC_CFG_COUNT; i++)
    {
        ss_mpi_venc_stop_chn(venc_cfg[i].chn);
        ss_mpi_venc_destroy_chn(venc_cfg[i].chn);
    }

    return 0;
}


#define ACODEC_FILE "/dev/acodec"
static int init_aio(void)
{
    int ai_dev_enabled = 0;
    int ai_chn_enabled = 0;
    int ao_dev_enabled = 0;
    int ao_chn_enabled = 0;
    int fd;
    td_s32 ret;
    int i;


    if ((0 == AI_DEV_CFG_COUNT) && (0 == AO_DEV_CFG_COUNT))
        return 0;

    ret = ss_mpi_audio_init();
    if (ret)
    {
        ERROR("ss_mpi_audio_init failed, err: %x !", ret);
        return -1;
    }

    /* AI */
    for (i = 0; i < AI_DEV_CFG_COUNT; i++)
    {
        ret = ss_mpi_ai_set_pub_attr(ai_dev_cfg[i].dev, &ai_dev_cfg[i].attr);
        if (ret)
        {
            ERROR("ss_mpi_ai_set_pub_attr failed, dev[%d] err: %x !", ai_dev_cfg[i].dev, ret);
            goto EXIT;
        }
        ret = ss_mpi_ai_enable(ai_dev_cfg[i].dev);
        if (ret)
        {
            ERROR("ss_mpi_ai_enable failed, dev[%d] err: %x !", ai_dev_cfg[i].dev, ret);
            goto EXIT;
        }
        ai_dev_enabled++;
    }

    for (i = 0; i < AI_CHN_CFG_COUNT; i++)
    {
        ret = ss_mpi_ai_enable_chn(ai_chn_cfg[i].dev, ai_chn_cfg[i].chn);
        if (ret)
        {
            ERROR("ss_mpi_ai_enable_chn failed, dev[%d] chn[%d] err: %x !",
                   ai_chn_cfg[i].dev, ai_chn_cfg[i].chn, ret);
            goto EXIT;
        }
        ai_chn_enabled++;
    }

    /* AO */
    for (i = 0; i < AO_DEV_CFG_COUNT; i++)
    {
        ret = ss_mpi_ao_set_pub_attr(ao_dev_cfg[i].dev, &ao_dev_cfg[i].attr);
        if (ret)
        {
            ERROR("ss_mpi_ao_set_pub_attr failed, dev[%d] err: %x !", ao_dev_cfg[i].dev, ret);
            goto EXIT;
        }
        ret = ss_mpi_ao_enable(ao_dev_cfg[i].dev);
        if (ret)
        {
            ERROR("ss_mpi_ao_enable failed, dev[%d] err: %x !", ao_dev_cfg[i].dev, ret);
            goto EXIT;
        }
        ao_dev_enabled++;
    }

    for (i = 0; i < AO_CHN_CFG_COUNT; i++)
    {
        ret = ss_mpi_ao_enable_chn(ao_chn_cfg[i].dev, ao_chn_cfg[i].chn);
        if (ret)
        {
            ERROR("ss_mpi_ao_enable_chn failed, dev[%d] chn[%d] err: %x !",
                   ao_chn_cfg[i].dev, ao_chn_cfg[i].chn, ret);
            goto EXIT;
        }
        ao_chn_enabled++;
    }

    /* ACODEC */
    fd = open(ACODEC_FILE, O_RDWR);
    if (0 > fd)
    {
        ERROR("open %s failed !", ACODEC_FILE);
        goto EXIT;
    }
    if (ioctl(fd, OT_ACODEC_SOFT_RESET_CTRL))
    {
        ERROR("OT_ACODEC_SOFT_RESET_CTRL failed !");
        goto EXIT;
    }
    if (ioctl(fd, OT_ACODEC_SET_I2S1_FS, &acodec_cfg.sample_rate))
    {
        ERROR("OT_ACODEC_SET_I2S1_FS failed, sample_rate_sel: %d !", acodec_cfg.sample_rate);
        goto EXIT;
    }
    if (ioctl(fd, OT_ACODEC_SET_MIXER_MIC, &acodec_cfg.input_mode))
    {
        ERROR("OT_ACODEC_SET_MIXER_MIC failed, input_mode: %d !", acodec_cfg.input_mode);
        goto EXIT;
    }
    if (ioctl(fd, OT_ACODEC_SET_INPUT_VOLUME, &acodec_cfg.input_vol))
    {
        ERROR("OT_ACODEC_SET_INPUT_VOLUME failed, input_vol: %d !", acodec_cfg.input_vol);
        goto EXIT;
    }

    close(fd);
    return 0;


EXIT:
    for (i = 0; i < ao_chn_enabled; i++)
        ss_mpi_ao_disable_chn(ao_chn_cfg[i].dev, ao_chn_cfg[i].chn);
    for (i = 0; i < ao_dev_enabled; i++)
        ss_mpi_ao_disable(ao_dev_cfg[i].dev);
    for (i = 0; i < ai_chn_enabled; i++)
        ss_mpi_ai_disable_chn(ai_chn_cfg[i].dev, ai_chn_cfg[i].chn);
    for (i = 0; i < ai_dev_enabled; i++)
        ss_mpi_ai_disable(ai_dev_cfg[i].dev);
    ss_mpi_audio_exit();
    if (0 < fd)
        close(fd);

    return -1;
}
static int deinit_aio(void)
{
    int i;

    for (i = 0; i < AO_CHN_CFG_COUNT; i++)
        ss_mpi_ao_disable_chn(ao_chn_cfg[i].dev, ao_chn_cfg[i].chn);
    for (i = 0; i < AO_DEV_CFG_COUNT; i++)
        ss_mpi_ao_disable(ao_dev_cfg[i].dev);
    for (i = 0; i < AI_CHN_CFG_COUNT; i++)
        ss_mpi_ai_disable_chn(ai_chn_cfg[i].dev, ai_chn_cfg[i].chn);
    for (i = 0; i < AI_DEV_CFG_COUNT; i++)
        ss_mpi_ai_disable(ai_dev_cfg[i].dev);
    ss_mpi_audio_exit();

    return 0;
}


static int bind(void)
{
    td_s32 ret;
    int i;

    for (i = 0; i < BIND_CFG_COUNT; i++)
    {
        ret = ss_mpi_sys_bind(&bind_cfg[i].src, &bind_cfg[i].dst);
        if (ret)
        {
            ERROR("ss_mpi_sys_bind failed, src(%d,%d,%d) dst(%d,%d,%d) err: %x !",
                   bind_cfg[i].src.mod_id, bind_cfg[i].src.dev_id, bind_cfg[i].src.chn_id,
                   bind_cfg[i].dst.mod_id, bind_cfg[i].dst.dev_id, bind_cfg[i].dst.chn_id, ret);
            break;
        }
    }
    if (i < BIND_CFG_COUNT)
    {
        for (i--; i >= 0; i--)
            ss_mpi_sys_unbind(&bind_cfg[i].src, &bind_cfg[i].dst);
        return -1;
    }

    return 0;
}
static int unbind(void)
{
    int i;

    for (i = 0; i < BIND_CFG_COUNT; i++)
        ss_mpi_sys_unbind(&bind_cfg[i].src, &bind_cfg[i].dst);

    return 0;
}


static int init_region(void)
{
    int rgn_created = 0;
    int rgn_attached = 0;
    td_s32 ret;
    int i;


    for (i = 0; i < REGION_CFG_COUNT; i++)
    {
        ret = ss_mpi_rgn_create(region_cfg[i].handle, &region_cfg[i].attr);
        if (ret)
        {
            ERROR("ss_mpi_rgn_create failed, handle[%u] err: %x !", region_cfg[i].handle, ret);
            goto EXIT;
        }
        rgn_created++;
    }

    for (i = 0; i < REGION_ATTACH_CFG_COUNT; i++)
    {
        if (region_attach_cfg[i].attach_chn)
        {
            ret = ss_mpi_rgn_attach_to_chn( region_attach_cfg[i].handle,
                                           &region_attach_cfg[i].chn,
                                           &region_attach_cfg[i].chn_attr);
            if (ret)
            {
                ERROR("ss_mpi_rgn_attach_to_chn failed, handle[%u] chn(%d,%d,%d) err: %x !",
                       region_attach_cfg[i].handle,
                       region_attach_cfg[i].chn.mod_id,
                       region_attach_cfg[i].chn.dev_id,
                       region_attach_cfg[i].chn.chn_id, ret);
                goto EXIT;
            }
            rgn_attached++;
        }
        else
        {
            ret = ss_mpi_rgn_attach_to_dev( region_attach_cfg[i].handle,
                                           &region_attach_cfg[i].chn,
                                           &region_attach_cfg[i].chn_attr);
            if (ret)
            {
                ERROR("ss_mpi_rgn_attach_to_dev failed, handle[%u] chn(%d,%d,%d) err: %x !",
                       region_attach_cfg[i].handle,
                       region_attach_cfg[i].chn.mod_id,
                       region_attach_cfg[i].chn.dev_id,
                       region_attach_cfg[i].chn.chn_id, ret);
                goto EXIT;
            }
            rgn_attached++;
        }
    }

    return 0;


EXIT:
    for (i = 0; i < rgn_attached; i++)
    {
        if (region_attach_cfg[i].attach_chn)
            ss_mpi_rgn_detach_from_chn(region_attach_cfg[i].handle, &region_attach_cfg[i].chn);
        else
            ss_mpi_rgn_detach_from_dev(region_attach_cfg[i].handle, &region_attach_cfg[i].chn);
    }
    for (i = 0; i < rgn_created; i++)
        ss_mpi_rgn_destroy(region_cfg[i].handle);

    return -1;
}
static int deinit_region(void)
{
    int i;

    for (i = 0; i < REGION_ATTACH_CFG_COUNT; i++)
    {
        if (region_attach_cfg[i].attach_chn)
            ss_mpi_rgn_detach_from_chn(region_attach_cfg[i].handle, &region_attach_cfg[i].chn);
        else
            ss_mpi_rgn_detach_from_dev(region_attach_cfg[i].handle, &region_attach_cfg[i].chn);
    }
    for (i = 0; i < REGION_CFG_COUNT; i++)
        ss_mpi_rgn_destroy(region_cfg[i].handle);

    return 0;
}


#define MAX_PACKS 7
static void* get_video_stream(void *arg)
{
    video_stream_cfg_t *stream_cfg = (video_stream_cfg_t *)arg;
    stream_state_t *stream_state;
    stream_id_t stream_id;
    video_format_t format;
    unsigned int width;
    unsigned int height;
    unsigned int fps;
    int cache_id;
    ot_venc_chn chn;
    int fd;
    fd_set read_fds;
    struct timeval timeout;
    ot_venc_chn_status stat;
    ot_venc_stream stream;
    ot_venc_pack packs[MAX_PACKS];
    frame_t *frame;
    unsigned int total_len;
    int ret;
    int i;


    pthread_detach(pthread_self());
    prctl(PR_SET_NAME, (unsigned long)"get_video_stream", 0, 0, 0);

    stream_id = stream_cfg->stream_info.stream_id;
    format    = stream_cfg->stream_info.format;
    width     = stream_cfg->stream_info.width;
    height    = stream_cfg->stream_info.height;
    fps       = stream_cfg->stream_info.fps;
    cache_id  = stream_cfg->cache_id;
    chn       = stream_cfg->venc_chn;

    for (i = 0; i < STREAM_COUNT; i++)
    {
        if (!memcmp(&stream_id, &stream_state_list[i].stream_id, sizeof(stream_id_t)))
            break;
    }
    stream_state = &stream_state_list[i];

    fd = ss_mpi_venc_get_fd(chn);
    if (0 > fd)
    {
        ERROR("ss_mpi_venc_get_fd failed, chn[%d] fd: %d !", chn, fd);
        stream_state->is_running = 0;
        return NULL;
    }
    stream.pack = packs;

    while (stream_state->should_run)
    {
        FD_ZERO(&read_fds);
        FD_SET(fd, &read_fds);
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        ret = select(fd + 1, &read_fds, NULL, NULL, &timeout);
        if (0 > ret)
        {
            ERROR("select failed !");
            sleep(1);
            continue;
        }
        else if (0 == ret)
        {
            //DEBUG("select time out");
            continue;
        }
        if (!FD_ISSET(fd, &read_fds))
        {
            DEBUG("video stream[%u,%u,%u] select null", stream_id.type, stream_id.major, stream_id.minor);
            continue;
        }

        ret = ss_mpi_venc_query_status(chn, &stat);
        if (ret)
        {
            ERROR("ss_mpi_venc_query_status failed, chn[%d] err: %x !", chn, ret);
            continue;
        }
        if (0 == stat.cur_packs)
        {
            DEBUG("video stream[%u,%u,%u] cur_packs is 0",
                   stream_id.type, stream_id.major, stream_id.minor);
            continue;
        }
        if (MAX_PACKS < stat.cur_packs)
        {
            ERROR("ss_mpi_venc_query_status get %u packs exceeds the max(%u) !",
                   stat.cur_packs, MAX_PACKS);
            continue;
        }

        stream.pack_cnt = stat.cur_packs;
        ret = ss_mpi_venc_get_stream(chn, &stream, 0);
        if (ret)
        {
            ERROR("ss_mpi_venc_get_stream failed, chn[%d] err: %x !", chn, ret);
            continue;
        }

        total_len = 0;
        for (i = 0; i < stream.pack_cnt; i++)
        {
            total_len += stream.pack[i].len - stream.pack[i].offset;
        }
        frame = hism_alloc_frame(total_len);
        if (NULL == frame)
        {
            ERROR("hism_alloc_frame(%u) failed !", total_len);
            ret = ss_mpi_venc_release_stream(chn, &stream);
            if (ret)
                ERROR("ss_mpi_venc_release_stream failed, chn[%d] err: %x !", chn, ret);
            continue;
        }

        frame->stream_id = stream_id;
        frame->cache_id = cache_id;
        frame->pts = stream.pack[0].pts;
        frame->video_info.format = format;
        frame->video_info.fps = fps;
        frame->video_info.width = width;
        frame->video_info.height = height;
        frame->size = total_len;
        total_len = 0;
        for (i = 0; i < stream.pack_cnt; i++)
        {
            memcpy(frame->buf + total_len,
                   stream.pack[i].addr + stream.pack[i].offset,
                   stream.pack[i].len  - stream.pack[i].offset);
            total_len += stream.pack[i].len - stream.pack[i].offset;
        }
        if (hism_put_stream_frame(frame))
            hism_free_frame(frame);

        ret = ss_mpi_venc_release_stream(chn, &stream);
        if (ret)
            ERROR("ss_mpi_venc_release_stream failed, chn[%d] err: %x !", chn, ret);
    }

    stream_state->is_running = 0;
    return NULL;
}
static void* get_audio_stream(void *arg)
{
    audio_stream_cfg_t *stream_cfg = (audio_stream_cfg_t *)arg;
    stream_state_t *stream_state;
    stream_id_t stream_id;
    audio_format_t format;
    unsigned int channels;
    unsigned int sample_rate;
    int cache_id;
    ot_audio_dev dev;
    ot_ai_chn chn;
    ot_audio_frame audio_frame;
    ot_aec_frame aec_frame;
    frame_t *frame;
    int ret;
    int i;


    pthread_detach(pthread_self());
    prctl(PR_SET_NAME, (unsigned long)"get_audio_stream", 0, 0, 0);

    stream_id   = stream_cfg->stream_info.stream_id;
    format      = stream_cfg->stream_info.format;
    channels    = stream_cfg->stream_info.channels;
    sample_rate = stream_cfg->stream_info.sample_rate;
    cache_id    = stream_cfg->cache_id;
    dev         = stream_cfg->ai_dev;
    chn         = stream_cfg->ai_chn;

    for (i = 0; i < STREAM_COUNT; i++)
    {
        if (!memcmp(&stream_id, &stream_state_list[i].stream_id, sizeof(stream_id_t)))
            break;
    }
    stream_state = &stream_state_list[i];

    while (stream_state->should_run)
    {
        ret = ss_mpi_ai_get_frame(dev, chn, &audio_frame, &aec_frame, 1000);
        if (ret)
        {
            ERROR("ss_mpi_ai_get_frame failed, dev[%d] chn[%d] err: %x !", dev, chn, ret);
            continue;
        }
        frame = hism_alloc_frame(audio_frame.len);
        if (NULL == frame)
        {
            ERROR("hism_alloc_frame(%u) failed !", audio_frame.len);
            ss_mpi_ai_release_frame(dev, chn, &audio_frame, &aec_frame);
            continue;
        }

        frame->stream_id = stream_id;
        frame->cache_id = cache_id;
        frame->pts = audio_frame.time_stamp;
        frame->audio_info.format = format;
        frame->audio_info.channels = channels;
        frame->audio_info.sample_rate = sample_rate;
        frame->size = audio_frame.len;
        memcpy(frame->buf, audio_frame.virt_addr[0], frame->size);
        if (hism_put_stream_frame(frame))
            hism_free_frame(frame);

        ret = ss_mpi_ai_release_frame(dev, chn, &audio_frame, &aec_frame);
        if (ret)
            ERROR("ss_mpi_ai_release_frame failed, dev[%d] chn[%d] err: %x !", dev, chn, ret);
    }

    stream_state->is_running = 0;
    return NULL;
}


int hism_init(void)
{
    int i, j;


    pthread_mutex_lock(&media_mutex);

    if (0 != media_inited)
    {
        pthread_mutex_unlock(&media_mutex);
        return -1;
    }

    memset(stream_state_list, 0, sizeof(stream_state_list));
    memset(algo_cb_list, 0, sizeof(algo_cb_list));
    i = 0;
    for (j = 0; j < VIDEO_STREAM_CFG_COUNT; j++)
    {
        stream_state_list[i].stream_id = video_stream_cfg[j].stream_info.stream_id;
        i++;
    }
    for (j = 0; j < AUDIO_STREAM_CFG_COUNT; j++)
    {
        stream_state_list[i].stream_id = audio_stream_cfg[j].stream_info.stream_id;
        i++;
    }

    if (hism_cache_init(STREAM_COUNT))
        goto EXIT;
    for (i = 0; i < VIDEO_STREAM_CFG_COUNT; i++)
    {
        video_stream_cfg[i].cache_id = hism_register_stream_id(video_stream_cfg[i].stream_info.stream_id);
        if (0 > video_stream_cfg[i].cache_id)
            goto EXIT_CACHE;
    }
    for (i = 0; i < AUDIO_STREAM_CFG_COUNT; i++)
    {
        audio_stream_cfg[i].cache_id = hism_register_stream_id(audio_stream_cfg[i].stream_info.stream_id);
        if (0 > audio_stream_cfg[i].cache_id)
            goto EXIT_CACHE;
    }
    if (init_sys())
        goto EXIT_CACHE;
    if (init_vi())
        goto EXIT_SYS;
    if (init_aiisp())
        goto EXIT_VI;
    if (init_vpss())
        goto EXIT_AIISP;
    if (init_venc())
        goto EXIT_VPSS;
    if (init_aio())
        goto EXIT_VENC;
    if (bind())
        goto EXIT_AIO;
    if (init_region())
        goto EXIT_BIND;

    media_inited = 1;
    pthread_mutex_unlock(&media_mutex);

    return 0;


EXIT_BIND:
    unbind();
EXIT_AIO:
    deinit_aio();
EXIT_VENC:
    deinit_venc();
EXIT_VPSS:
    deinit_vpss();
EXIT_AIISP:
    deinit_aiisp();
EXIT_VI:
    deinit_vi();
EXIT_SYS:
    deinit_sys();
EXIT_CACHE:
    hism_cache_exit();
EXIT:
    pthread_mutex_unlock(&media_mutex);
    return -1;
}
int hism_exit(void)
{
    int i, j;
    int ret = 0;


    pthread_mutex_lock(&media_mutex);
    if (0 >= media_inited)
    {
        ret = media_inited;
        pthread_mutex_unlock(&media_mutex);
        return ret;
    }

    for (i = 0; i < STREAM_COUNT; i++)
    {
        stream_state_list[i].should_run = 0;
    }
    for (i = 0; i < 70; i++)
    {
        usleep(40 * 1000);
        for (j = 0; j < STREAM_COUNT; j++)
        {
            if (stream_state_list[j].is_running)
                break;
        }
        if (STREAM_COUNT <= j)
            break;
    }
    if (70 <= i)
    {
        ERROR("stream exit timeout !");
        ret = -1;
    }

    deinit_region();
    unbind();
    deinit_aio();
    deinit_venc();
    deinit_vpss();
    deinit_aiisp();
    deinit_vi();
    deinit_sys();
    hism_cache_exit();

    if (ret)
        media_inited = -1;
    else
        media_inited = 0;

    pthread_mutex_unlock(&media_mutex);

    return ret;
}


static int start_video_stream(stream_id_t id)
{
    video_stream_cfg_t *stream_cfg;
    pthread_t tid;
    int i;


    for (i = 0; i < VIDEO_STREAM_CFG_COUNT; i++)
    {
        if (!memcmp(&id, &video_stream_cfg[i].stream_info.stream_id, sizeof(stream_id_t)))
            break;
    }
    if (VIDEO_STREAM_CFG_COUNT <= i)
    {
        ERROR("video stream[%u,%u,%u] is not configured !", id.type, id.major, id.minor);
        return -1;
    }
    stream_cfg = &video_stream_cfg[i];

    pthread_mutex_lock(&media_mutex);
    if (0 >= media_inited)
    {
        pthread_mutex_unlock(&media_mutex);
        return -1;
    }
    for (i = 0; i < STREAM_COUNT; i++)
    {
        if (!memcmp(&id, &stream_state_list[i].stream_id, sizeof(stream_id_t)))
            break;
    }
    if (stream_state_list[i].is_running)
    {
        if (stream_state_list[i].should_run)
        {
            pthread_mutex_unlock(&media_mutex);
            return 0;
        }
        ERROR("stream[%u,%u,%u] is exiting, try again !", id.type, id.major, id.minor);
        pthread_mutex_unlock(&media_mutex);
        sleep(1);
        return -1;
    }
    stream_state_list[i].should_run = 1;
    stream_state_list[i].is_running = 1;
    pthread_mutex_unlock(&media_mutex);

    pthread_create(&tid, NULL, get_video_stream, (void *)stream_cfg);

    return 0;
}
static int start_audio_stream(stream_id_t id)
{
    audio_stream_cfg_t *stream_cfg;
    pthread_t tid;
    int i;


    for (i = 0; i < AUDIO_STREAM_CFG_COUNT; i++)
    {
        if (!memcmp(&id, &audio_stream_cfg[i].stream_info.stream_id, sizeof(stream_id_t)))
            break;
    }
    if (AUDIO_STREAM_CFG_COUNT <= i)
    {
        ERROR("audio stream[%u,%u,%u] is not configured !", id.type, id.major, id.minor);
        return -1;
    }
    stream_cfg = &audio_stream_cfg[i];

    pthread_mutex_lock(&media_mutex);
    if (0 >= media_inited)
    {
        pthread_mutex_unlock(&media_mutex);
        return -1;
    }
    for (i = 0; i < STREAM_COUNT; i++)
    {
        if (!memcmp(&id, &stream_state_list[i].stream_id, sizeof(stream_id_t)))
            break;
    }
    if (stream_state_list[i].is_running)
    {
        if (stream_state_list[i].should_run)
        {
            pthread_mutex_unlock(&media_mutex);
            return 0;
        }
        ERROR("stream[%u,%u,%u] is exiting, try again !", id.type, id.major, id.minor);
        pthread_mutex_unlock(&media_mutex);
        sleep(1);
        return -1;
    }
    stream_state_list[i].should_run = 1;
    stream_state_list[i].is_running = 1;
    pthread_mutex_unlock(&media_mutex);

    pthread_create(&tid, NULL, get_audio_stream, (void *)stream_cfg);

    return 0;
}
int hism_start_stream(stream_id_t id)
{
    if (0 == id.type)
        return start_video_stream(id);
    else if (1 == id.type)
        return start_audio_stream(id);
    else
        return -1;
}
int hism_stop_stream(stream_id_t id)
{
    int i;

    pthread_mutex_lock(&media_mutex);
    if (0 >= media_inited)
    {
        pthread_mutex_unlock(&media_mutex);
        return -1;
    }

    for (i = 0; i < STREAM_COUNT; i++)
    {
        if (!memcmp(&id, &stream_state_list[i].stream_id, sizeof(stream_id_t)))
            break;
    }
    if (STREAM_COUNT <= i)
    {
        pthread_mutex_unlock(&media_mutex);
        return -1;
    }
    stream_state_list[i].should_run = 0;
    pthread_mutex_unlock(&media_mutex);

    return 0;
}


int hism_get_video_stream_info(stream_id_t id, video_stream_info_t *info)
{
    int i;

    if (NULL == info)
        return -1;

    for (i = 0; i < VIDEO_STREAM_CFG_COUNT; i++)
    {
        if (!memcmp(&id, &video_stream_cfg[i].stream_info.stream_id, sizeof(stream_id_t)))
            break;
    }
    if (VIDEO_STREAM_CFG_COUNT <= i)
        return -1;

    *info = video_stream_cfg[i].stream_info;
    return 0;
}
int hism_get_audio_stream_info(stream_id_t id, audio_stream_info_t *info)
{
    int i;

    if (NULL == info)
        return -1;

    for (i = 0; i < AUDIO_STREAM_CFG_COUNT; i++)
    {
        if (!memcmp(&id, &audio_stream_cfg[i].stream_info.stream_id, sizeof(stream_id_t)))
            break;
    }
    if (AUDIO_STREAM_CFG_COUNT <= i)
        return -1;

    *info = audio_stream_cfg[i].stream_info;
    return 0;
}


int hism_get_osd_info(stream_id_t stream_id, osd_id_t osd_id, osd_info_t *info)
{
    int i;

    if (NULL == info)
        return -1;

    for (i = 0; i < OSD_CFG_COUNT; i++)
    {
        if (   (!memcmp(&stream_id, &osd_cfg[i].osd_info.stream_id, sizeof(stream_id_t)))
            && (osd_id == osd_cfg[i].osd_info.osd_id))
            break;
    }
    if (OSD_CFG_COUNT <= i)
        return -1;

    *info = osd_cfg[i].osd_info;
    return 0;
}


int hism_write_osd(stream_id_t stream_id, osd_id_t osd_id, unsigned char *buf)
{
    ot_bmp bmp;
    int i;


    if ((0 >= media_inited) || (NULL == buf))
        return -1;

    for (i = 0; i < OSD_CFG_COUNT; i++)
    {
        if (   (!memcmp(&stream_id, &osd_cfg[i].osd_info.stream_id, sizeof(stream_id_t)))
            && (osd_id == osd_cfg[i].osd_info.osd_id))
            break;
    }
    if (OSD_CFG_COUNT <= i)
        return -1;

    bmp = osd_cfg[i].bmp;
    bmp.data = (void *)buf;
    if (ss_mpi_rgn_set_bmp(osd_cfg[i].handle, &bmp))
        return -1;

    return 0;
}
