#include "hismedia.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/select.h>
#include <pthread.h>
#include <sys/prctl.h>

#include "mi_sys.h"
#include "mi_sensor.h"
#include "mi_vif.h"
#include "mi_isp.h"
#include "mi_rgn.h"
#include "mi_scl.h"
#include "mi_venc.h"
#include "mi_md.h"
#include "mi_od.h"
#include "mi_vg.h"
#include "mi_vdf.h"
#include "mi_ai.h"
#include "mi_iqserver.h"
#include "mi_isp_iq.h"
#include "mi_isp_cus3a_api.h"




/*************************************************
    MMA Config
**************************************************/
typedef struct
{
    MI_SYS_GlobalPrivPoolConfig_t cfg;
}mma_cfg_t;


/*************************************************
    PAD Config
**************************************************/
typedef struct
{
    MI_U8 pad_id;
    MI_U8 res_id;
    MI_U8 hdr_mode;
    MI_U8 fps;
}pad_cfg_t;


/*************************************************
    VIF Config
**************************************************/
typedef struct
{
    MI_VIF_GROUP grp_id;
    MI_VIF_GroupAttr_t attr;
}vif_grp_cfg_t;

typedef struct
{
    MI_VIF_DEV dev_id;
    MI_VIF_DevAttr_t attr;
}vif_dev_cfg_t;

typedef struct
{
    MI_VIF_DEV dev_id;
    MI_VIF_PORT port_id;
    MI_VIF_OutputPortAttr_t attr;
}vif_outport_cfg_t;


/*************************************************
    ISP Config
**************************************************/
typedef struct
{
    MI_ISP_DEV dev_id;
    MI_ISP_DevAttr_t attr;
}isp_dev_cfg_t;

typedef struct
{
    MI_ISP_DEV dev_id;
    MI_ISP_CHANNEL chn_id;
    char *bin_file;
    MI_ISP_ChannelAttr_t chn_attr;
    MI_ISP_ChnParam_t chn_param;
}isp_chn_cfg_t;

typedef struct
{
    MI_ISP_DEV dev_id;
    MI_ISP_CHANNEL chn_id;
    MI_ISP_PORT port_id;
    MI_ISP_OutPortParam_t param;
}isp_outport_cfg_t;


/*************************************************
    RGN Config
**************************************************/
typedef struct
{
    MI_RGN_PaletteTable_t table;
}rgn_cfg_t;

typedef struct
{
    MI_RGN_HANDLE handle;
    MI_RGN_Attr_t attr;
}rgn_region_cfg_t;

typedef struct
{
    MI_RGN_HANDLE handle;
    MI_RGN_ChnPort_t port;
    MI_RGN_ChnPortParam_t param;
}rgn_attach_cfg_t;


/*************************************************
    SCL Config
**************************************************/
typedef struct
{
    MI_SCL_DEV dev_id;
    MI_SCL_DevAttr_t attr;
}scl_dev_cfg_t;

typedef struct
{
    MI_SCL_DEV dev_id;
    MI_SCL_CHANNEL chn_id;
    MI_SCL_ChannelAttr_t chn_attr;
    MI_SYS_WindowRect_t inport_rect;
    MI_SCL_ChnParam_t chn_param;
}scl_chn_cfg_t;

typedef struct
{
    MI_SCL_DEV dev_id;
    MI_SCL_CHANNEL chn_id;
    MI_SCL_PORT port_id;
    MI_SCL_OutPortParam_t param;
}scl_outport_cfg_t;


/*************************************************
    VENC Config
**************************************************/
typedef struct
{
    MI_VENC_DEV dev_id;
    MI_VENC_InitParam_t param;
}venc_dev_cfg_t;

typedef struct
{
    MI_VENC_DEV dev_id;
    MI_VENC_CHN chn_id;
    MI_VENC_ChnAttr_t attr;
    MI_VENC_InputSourceConfig_t insrc_cfg;
}venc_chn_cfg_t;


/*************************************************
    VDF Config
**************************************************/
typedef struct
{
    MI_VDF_CHANNEL chn_id;
    unsigned int threshold; /* md模式中是MB数量 */
    MI_VDF_ChnAttr_t attr;
}vdf_cfg_t;


/*************************************************
    AI Config
**************************************************/
typedef struct
{
    MI_AUDIO_DEV dev_id;
    MI_AI_Attr_t attr;
    unsigned int ifs_cnt;
    MI_AI_If_e ifs[MI_AI_MAX_CHN_NUM/2];
    MI_S8 left_if_gain[MI_AI_MAX_CHN_NUM/2];
    MI_S8 right_if_gain[MI_AI_MAX_CHN_NUM/2];
    MI_S8 gain[MI_AI_MAX_CHN_NUM];
}ai_dev_cfg_t;


/*************************************************
    Output Port Depth Config
**************************************************/
typedef struct
{
    MI_ModuleId_e mod_id;
    MI_U32 dev_id;
    MI_U32 chn_id;
    MI_U32 port_id;
    MI_U32 user_depth;  /* 用户能取到并且不释放的buf数量，要<=queue_depth */
    MI_U32 queue_depth;  /* port能同时申请的最多buf数，默认4 */
}outport_depth_cfg_t;


/*************************************************
    Bind Config
**************************************************/
typedef struct
{
    MI_SYS_ChnPort_t src;
    MI_SYS_ChnPort_t dst;
    MI_U32 src_frmrate;
    MI_U32 dst_frmrate;
    MI_SYS_BindType_e type;
    MI_U32 param;
}bind_cfg_t;


/*************************************************
    Video Stream Config
**************************************************/
typedef struct
{
    video_stream_info_t stream_info;
    MI_VENC_DEV venc_devid;
    MI_VENC_CHN venc_chnid;
}video_stream_cfg_t;


/*************************************************
    Audio Stream Config
**************************************************/
typedef struct
{
    audio_stream_info_t stream_info;
    MI_AUDIO_DEV dev_id;
    MI_U8 grp_id;
}audio_stream_cfg_t;


/*************************************************
    Signal Frame Config
**************************************************/
typedef struct
{
    signal_frame_id_t frame_id;
    MI_ModuleId_e mod_id;
    MI_U32 dev_id;
    MI_U32 chn_id;
    MI_U32 port_id;
}signal_frame_cfg_t;


/*************************************************
    Jpeg Encode Config
**************************************************/
typedef struct
{
    unsigned int alignment_x;
    unsigned int alignment_y;
    unsigned int alignment_width;
    unsigned int alignment_height;
}jpeg_encode_cfg_t;


/*************************************************
    OSD Config
**************************************************/
typedef struct
{
    osd_info_t osd_info;
    MI_RGN_HANDLE handle;
}osd_cfg_t;


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


static pthread_mutex_t media_mutex = PTHREAD_MUTEX_INITIALIZER;
static stream_state_t stream_state_list[STREAM_ID_MAX];

#define MAX_MD_CB 4
static pthread_mutex_t vdf_mutex = PTHREAD_MUTEX_INITIALIZER;
static algo_motion_det_cb_t md_cb_list[MAX_MD_CB];
static unsigned char vdf_should_run = 0;
static unsigned char vdf_is_running = 0;

static unsigned char media_inited = 0;
static unsigned char media_debug = 1;


void hism_set_debug(unsigned char level)
{
    media_debug = level;
}


static int init_mma(void)
{
    MI_SYS_GlobalPrivPoolConfig_t cfg;
    MI_S32 ret;
    int i;

    for (i = 0; i < MMA_CFG_COUNT; i++)
    {
        ret = MI_SYS_ConfigPrivateMMAPool(0, &mma_cfg[i].cfg);
        if (ret)
        {
            if (E_MI_SYS_PER_DEV_PRIVATE_RING_POOL == mma_cfg[i].cfg.eConfigType)
            {
                ERROR("MI_SYS_ConfigPrivateMMAPool failed, type(%d) creat(%u)  module(%d) dev[%u] max(%u,%u) line(%u) name(%s) err: %x !",
                       mma_cfg[i].cfg.eConfigType,
                       mma_cfg[i].cfg.bCreate,
                       mma_cfg[i].cfg.uConfig.stpreDevPrivRingPoolConfig.eModule,
                       mma_cfg[i].cfg.uConfig.stpreDevPrivRingPoolConfig.u32Devid,
                       mma_cfg[i].cfg.uConfig.stpreDevPrivRingPoolConfig.u16MaxWidth,
                       mma_cfg[i].cfg.uConfig.stpreDevPrivRingPoolConfig.u16MaxHeight,
                       mma_cfg[i].cfg.uConfig.stpreDevPrivRingPoolConfig.u16RingLine,
                       mma_cfg[i].cfg.uConfig.stpreDevPrivRingPoolConfig.u8MMAHeapName,
                       ret);
            }
            goto EXIT;
        }
    }

    return 0;

EXIT:
    for (; i > 0; i--)
    {
        memcpy(&cfg, &mma_cfg[i-1].cfg, sizeof(cfg));
        cfg.bCreate = 0;
        MI_SYS_ConfigPrivateMMAPool(0, &cfg);
    }

    return -1;
}
static int deinit_mma(void)
{
    MI_SYS_GlobalPrivPoolConfig_t cfg;
    int i;

    for (i = 0; i < MMA_CFG_COUNT; i++)
    {
        memcpy(&cfg, &mma_cfg[i].cfg, sizeof(cfg));
        cfg.bCreate = 0;
        MI_SYS_ConfigPrivateMMAPool(0, &cfg);
    }

    return 0;
}


static int init_sensor(void)
{
    MI_S32 ret;
    MI_U32 res_count = 0;
    MI_SNR_Res_t res;
    MI_SNR_PADInfo_t pad_info;
    MI_SNR_PlaneInfo_t snr_plane_info;
    int i, j;
    int enabled = 0;


    for (i = 0; i < PAD_CFG_COUNT; i++)
    {
        ret = MI_SNR_QueryResCount(pad_cfg[i].pad_id, &res_count);
        if (ret)
        {
            ERROR("MI_SNR_QueryResCount failed, pad[%u] err: %x !", pad_cfg[i].pad_id, ret);
            goto EXIT;
        }
        if (media_debug)
        {
            printf("\n=========== pad[%u]_sensor_res ==========\n", pad_cfg[i].pad_id);
            for (j = 0; j < res_count; j++)
            {
                ret = MI_SNR_GetRes(pad_cfg[i].pad_id, j, &res);
                if (ret)
                {
                    ERROR("MI_SNR_GetRes failed, pad[%u] res[%d] err: %x !",
                           pad_cfg[i].pad_id, j, ret);
                    goto EXIT;
                }
                printf("[%d] Crop:(%u,%u,%u,%u) Output:(%u,%u) maxfps:%u minfps:%u ResDesc:%s\n",
                        j,
                        res.stCropRect.u16X,
                        res.stCropRect.u16Y,
                        res.stCropRect.u16Width,
                        res.stCropRect.u16Height,
                        res.stOutputSize.u16Width,
                        res.stOutputSize.u16Height,
                        res.u32MaxFps,
                        res.u32MinFps,
                        res.strResDesc);
            }
        }

        if (media_debug)
        {
            ret = MI_SNR_GetPadInfo(pad_cfg[i].pad_id, &pad_info);
            if (ret)
            {
                ERROR("MI_SNR_GetPadInfo failed, pad[%u] err: %x !", pad_cfg[i].pad_id, ret);
                goto EXIT;
            }
            printf("\n============== pad[%u]_info =============\n", pad_cfg[i].pad_id);
            printf("PlaneCount:%u IntfMode:%d HDRMode:%d EarlyInit:%u\n",
                    pad_info.u32PlaneCount,
                    pad_info.eIntfMode,
                    pad_info.eHDRMode,
                    pad_info.bEarlyInit);
            if (E_MI_SNR_MODE_MIPI == pad_info.eIntfMode)
                printf("MIPI:\nLaneNum:%u DataFormat:%u HsyncMode:%u Sampling_delay:%u HdrHWmode:%d Hdr_Virchn_num:%u\n",
                        pad_info.unIntfAttr.stMipiAttr.u32LaneNum,
                        pad_info.unIntfAttr.stMipiAttr.u32DataFormat,
                        pad_info.unIntfAttr.stMipiAttr.u32HsyncMode,
                        pad_info.unIntfAttr.stMipiAttr.u32Sampling_delay,
                        pad_info.unIntfAttr.stMipiAttr.eHdrHWmode,
                        pad_info.unIntfAttr.stMipiAttr.u32Hdr_Virchn_num);
        }

        if (media_debug)
        {
            ret = MI_SNR_GetPlaneInfo(pad_cfg[i].pad_id, 0, &snr_plane_info);
            if (ret)
            {
                ERROR("MI_SNR_GetPlaneInfo failed, pad[%u] plane[0] err: %x !",
                       pad_cfg[i].pad_id, ret);
                goto EXIT;
            }
            printf("\n========= pad[%u]_plane[0]_info =========\n", pad_cfg[i].pad_id);
            printf("PlaneID:%u  SensorName:%s\n",
                    snr_plane_info.u32PlaneID,
                    snr_plane_info.s8SensorName);
            printf("CapRect:(%u,%u,%u,%u) BayerId:%d PixPrecision:%d HdrSrc:%d ShutterUs:%u SensorGainX1024:%u CompGain:%u Pixel:%d\n",
                    snr_plane_info.stCapRect.u16X,
                    snr_plane_info.stCapRect.u16Y,
                    snr_plane_info.stCapRect.u16Width,
                    snr_plane_info.stCapRect.u16Height,
                    snr_plane_info.eBayerId,
                    snr_plane_info.ePixPrecision,
                    snr_plane_info.eHdrSrc,
                    snr_plane_info.u32ShutterUs,
                    snr_plane_info.u32SensorGainX1024,
                    snr_plane_info.u32CompGain,
                    snr_plane_info.ePixel);
        }

        ret = MI_SNR_SetPlaneMode(pad_cfg[i].pad_id, pad_cfg[i].hdr_mode);
        if (ret)
        {
            ERROR("MI_SNR_SetPlaneMode failed, pad[%u] hdr(%u) err: %x !",
                   pad_cfg[i].pad_id, pad_cfg[i].hdr_mode, ret);
            goto EXIT;
        }

        if (res_count < pad_cfg[i].res_id)
        {
            ERROR("Pad[%u] resolution count is %u, set res_id(%u) error !",
                   pad_cfg[i].pad_id, res_count, pad_cfg[i].res_id);
            goto EXIT;
        }
        DEBUG("Set resolution, pad[%u] res_id:%u", pad_cfg[i].pad_id, pad_cfg[i].res_id);
        ret = MI_SNR_SetRes(pad_cfg[i].pad_id, pad_cfg[i].res_id);
        if (ret)
        {
            ERROR("MI_SNR_SetRes failed, pad[%u] res_id(%u) err: %x !",
                   pad_cfg[i].pad_id, pad_cfg[i].res_id, ret);
            goto EXIT;
        }

        ret = MI_SNR_Enable(pad_cfg[i].pad_id);
        if (ret)
        {
            ERROR("MI_SNR_Enable failed, pad[%u] err: %x !", pad_cfg[i].pad_id, ret);
            goto EXIT;
        }
        enabled++;

        ret = MI_SNR_GetRes(pad_cfg[i].pad_id, pad_cfg[i].res_id, &res);
        if (ret)
        {
            ERROR("MI_SNR_GetRes failed, pad[%u] res[%u] err: %x !",
                   pad_cfg[i].pad_id, pad_cfg[i].res_id, ret);
            goto EXIT;
        }

        if ((res.u32MinFps > pad_cfg[i].fps) || (res.u32MaxFps < pad_cfg[i].fps))
        {
            ERROR("Pad[%u] res[%u] fps range is [%u,%u], set fps(%u) error !",
                   pad_cfg[i].pad_id,
                   pad_cfg[i].res_id,
                   res.u32MinFps,
                   res.u32MaxFps,
                   pad_cfg[i].fps);
            goto EXIT;
        }
        DEBUG("Set pad[%u] fps:%u", pad_cfg[i].pad_id, pad_cfg[i].fps);
        ret = MI_SNR_SetFps(pad_cfg[i].pad_id, pad_cfg[i].fps);
        if (ret)
        {
            ERROR("MI_SNR_SetFps failed, pad[%u] fps(%u) err: %x !",
                   pad_cfg[i].pad_id, pad_cfg[i].fps, ret);
            goto EXIT;
        }
    }

    return 0;

EXIT:
    for (i = 0; i < enabled; i++)
        MI_SNR_Disable(pad_cfg[i].pad_id);

    return -1;
}
static int deinit_sensor(void)
{
    int i;

    for (i = 0; i < PAD_CFG_COUNT; i++)
        MI_SNR_Disable(pad_cfg[i].pad_id);

    return 0;
}


static int init_vif(void)
{
    MI_S32 ret;
    int group_created = 0;
    int dev_enabled = 0;
    int port_enabled = 0;
    int i;


    for (i = 0; i < VIF_GRP_CFG_COUNT; i++)
    {
        ret = MI_VIF_CreateDevGroup(vif_grp_cfg[i].grp_id, &vif_grp_cfg[i].attr);
        if (ret)
        {
            ERROR("MI_VIF_CreateDevGroup failed, grp[%u] err: %x !",
                   vif_grp_cfg[i].grp_id, ret);
            goto EXIT;
        }
        group_created++;
    }

    for (i = 0; i < VIF_DEV_CFG_COUNT; i++)
    {
        ret = MI_VIF_SetDevAttr(vif_dev_cfg[i].dev_id, &vif_dev_cfg[i].attr);
        if (ret)
        {
            ERROR("MI_VIF_SetDevAttr failed, dev[%u] err: %x !", vif_dev_cfg[i].dev_id, ret);
            goto EXIT;
        }

        ret = MI_VIF_EnableDev(vif_dev_cfg[i].dev_id);
        if (ret)
        {
            ERROR("MI_VIF_EnableDev failed, dev[%u] err: %x !", vif_dev_cfg[i].dev_id, ret);
            goto EXIT;
        }
        dev_enabled++;
    }

    for (i = 0; i < VIF_OUTPORT_CFG_COUNT; i++)
    {
        ret = MI_VIF_SetOutputPortAttr(vif_outport_cfg[i].dev_id,
                                       vif_outport_cfg[i].port_id,
                                       &vif_outport_cfg[i].attr);
        if (ret)
        {
            ERROR("MI_VIF_SetOutputPortAttr failed, dev[%u] port[%u] err: %x !",
                   vif_outport_cfg[i].dev_id, vif_outport_cfg[i].port_id, ret);
            goto EXIT;
        }

        ret = MI_VIF_EnableOutputPort(vif_outport_cfg[i].dev_id, vif_outport_cfg[i].port_id);
        if (ret)
        {
            ERROR("MI_VIF_EnableOutputPort failed, dev[%u] port[%u] err: %x !",
                   vif_outport_cfg[i].dev_id, vif_outport_cfg[i].port_id, ret);
            goto EXIT;
        }
        port_enabled++;
    }

    return 0;


EXIT:
    for (i = 0; i < port_enabled; i++)
        MI_VIF_DisableOutputPort(vif_dev_cfg[i].dev_id, vif_outport_cfg[i].port_id);
    for (i = 0; i < dev_enabled; i++)
        MI_VIF_DisableDev(vif_dev_cfg[i].dev_id);
    for (i = 0; i < group_created; i++)
        MI_VIF_DestroyDevGroup(vif_grp_cfg[i].grp_id);

    return -1;
}
static int deinit_vif(void)
{
    int i;

    for (i = 0; i < VIF_OUTPORT_CFG_COUNT; i++)
        MI_VIF_DisableOutputPort(vif_outport_cfg[i].dev_id, vif_outport_cfg[i].port_id);
    for (i = 0; i < VIF_DEV_CFG_COUNT; i++)
        MI_VIF_DisableDev(vif_dev_cfg[i].dev_id);
    for (i = 0; i < VIF_GRP_CFG_COUNT; i++)
        MI_VIF_DestroyDevGroup(vif_grp_cfg[i].grp_id);

    return 0;
}


static int init_isp(void)
{
    MI_S32 ret;
    int dev_created = 0;
    int chn_created = 0;
    int chn_started = 0;
    int outport_enabled = 0;
    int i;


    for (i = 0; i < ISP_DEV_CFG_COUNT; i++)
    {
        ret = MI_ISP_CreateDevice(isp_dev_cfg[i].dev_id, &isp_dev_cfg[i].attr);
        if (ret)
        {
            ERROR("MI_ISP_CreateDevice failed, dev[%u] mask(%x) err: %x !",
                   isp_dev_cfg[i].dev_id, isp_dev_cfg[i].attr.u32DevStitchMask, ret);
            goto EXIT;
        }
        dev_created++;
    }

    for (i = 0; i < ISP_CHN_CFG_COUNT; i++)
    {
        ret = MI_ISP_CreateChannel(isp_chn_cfg[i].dev_id,
                                   isp_chn_cfg[i].chn_id,
                                   &isp_chn_cfg[i].chn_attr);
        if (ret)
        {
            ERROR("MI_ISP_CreateChannel failed, dev[%u] chn[%u] err: %x !",
                   isp_chn_cfg[i].dev_id, isp_chn_cfg[i].chn_id, ret);
            goto EXIT;
        }
        chn_created++;

        ret = MI_ISP_SetChnParam(isp_chn_cfg[i].dev_id,
                                 isp_chn_cfg[i].chn_id,
                                 &isp_chn_cfg[i].chn_param);
        if (ret)
        {
            ERROR("MI_ISP_SetChnParam failed, dev[%u] chn[%u] err: %x !",
                   isp_chn_cfg[i].dev_id, isp_chn_cfg[i].chn_id, ret);
            goto EXIT;
        }

        ret = MI_ISP_StartChannel(isp_chn_cfg[i].dev_id, isp_chn_cfg[i].chn_id);
        if (ret)
        {
            ERROR("MI_ISP_StartChannel failed, dev[%u] chn[%u] err: %x !",
                   isp_chn_cfg[i].dev_id, isp_chn_cfg[i].chn_id, ret);
            goto EXIT;
        }
        chn_started++;
    }

    for (i = 0; i < ISP_OUTPORT_CFG_COUNT; i++)
    {
        ret = MI_ISP_SetOutputPortParam(isp_outport_cfg[i].dev_id,
                                        isp_outport_cfg[i].chn_id,
                                        isp_outport_cfg[i].port_id,
                                        &isp_outport_cfg[i].param);
        if (ret)
        {
            ERROR("MI_ISP_SetOutputPortParam failed, dev[%u] chn[%u] port[%u] err: %x !",
                   isp_outport_cfg[i].dev_id,
                   isp_outport_cfg[i].chn_id,
                   isp_outport_cfg[i].port_id,
                   ret);
            goto EXIT;
        }

        ret = MI_ISP_EnableOutputPort(isp_outport_cfg[i].dev_id,
                                      isp_outport_cfg[i].chn_id,
                                      isp_outport_cfg[i].port_id);
        if (ret)
        {
            ERROR("MI_ISP_EnableOutputPort failed, dev[%u] chn[%u] port[%u] err: %x !",
                   isp_outport_cfg[i].dev_id,
                   isp_outport_cfg[i].chn_id,
                   isp_outport_cfg[i].port_id,
                   ret);
            goto EXIT;
        }
        outport_enabled++;
    }

    return 0;


EXIT:
    for (i = 0; i < outport_enabled; i++)
        MI_ISP_DisableOutputPort(isp_outport_cfg[i].dev_id,
                                 isp_outport_cfg[i].chn_id,
                                 isp_outport_cfg[i].port_id);
    for (i = 0; i < chn_started; i++)
        MI_ISP_StopChannel(isp_chn_cfg[i].dev_id, isp_chn_cfg[i].chn_id);
    for (i = 0; i < chn_created; i++)
        MI_ISP_DestroyChannel(isp_chn_cfg[i].dev_id, isp_chn_cfg[i].chn_id);
    for (i = 0; i < dev_created; i++)
        MI_ISP_DestoryDevice(isp_dev_cfg[i].dev_id);

    return -1;
}
static int deinit_isp(void)
{
    int i;

    for (i = 0; i < ISP_OUTPORT_CFG_COUNT; i++)
        MI_ISP_DisableOutputPort(isp_outport_cfg[i].dev_id,
                                 isp_outport_cfg[i].chn_id,
                                 isp_outport_cfg[i].port_id);
    for (i = 0; i < ISP_CHN_CFG_COUNT; i++)
    {
        MI_ISP_StopChannel(isp_chn_cfg[i].dev_id, isp_chn_cfg[i].chn_id);
        MI_ISP_DestroyChannel(isp_chn_cfg[i].dev_id, isp_chn_cfg[i].chn_id);
    }
    for (i = 0; i < ISP_DEV_CFG_COUNT; i++)
        MI_ISP_DestoryDevice(isp_dev_cfg[i].dev_id);

    return 0;
}


static int init_rgn(void)
{
    MI_S32 ret;
    int region_created = 0;
    int region_attached = 0;
    int i;


    ret = MI_RGN_Init(0, &rgn_cfg.table);
    if (MI_RGN_OK != ret)
    {
        ERROR("MI_RGN_Init failed, err: %x !", ret);
        return -1;
    }

    for (i = 0; i < RGN_REGION_CFG_COUNT; i++)
    {
        ret = MI_RGN_Create(0, rgn_region_cfg[i].handle, &rgn_region_cfg[i].attr);
        if (MI_RGN_OK != ret)
        {
            ERROR("MI_RGN_Create failed, handle(%u) err: %x !", rgn_region_cfg[i].handle, ret);
            goto EXIT;
        }
        region_created++;
    }

    for (i = 0; i < RGN_ATTACH_CFG_COUNT; i++)
    {
        ret = MI_RGN_AttachToChn(0, rgn_attach_cfg[i].handle,
                                   &rgn_attach_cfg[i].port,
                                   &rgn_attach_cfg[i].param);
        if (MI_RGN_OK != ret)
        {
            ERROR("MI_RGN_AttachToChn failed, handle(%u) port(%d,%d,%d,%d) err: %x !",
                   rgn_attach_cfg[i].handle,
                   rgn_attach_cfg[i].port.eModId,
                   rgn_attach_cfg[i].port.s32DevId,
                   rgn_attach_cfg[i].port.s32ChnId,
                   rgn_attach_cfg[i].port.s32PortId,
                   ret);
            goto EXIT;
        }
        region_attached++;
    }

    return 0;


EXIT:
    for (i = 0; i < region_attached; i++)
        MI_RGN_DetachFromChn(0, rgn_attach_cfg[i].handle, &rgn_attach_cfg[i].port);
    for (i = 0; i < region_created; i++)
        MI_RGN_Destroy(0, rgn_region_cfg[i].handle);
    MI_RGN_DeInit(0);

    return -1;
}
static int deinit_rgn(void)
{
    int i;

    for (i = 0; i < RGN_ATTACH_CFG_COUNT; i++)
        MI_RGN_DetachFromChn(0, rgn_attach_cfg[i].handle, &rgn_attach_cfg[i].port);
    for (i = 0; i < RGN_REGION_CFG_COUNT; i++)
        MI_RGN_Destroy(0, rgn_region_cfg[i].handle);
    MI_RGN_DeInit(0);

    return 0;
}


static int init_scl(void)
{
    MI_S32 ret;
    int dev_created = 0;
    int chn_created = 0;
    int chn_started = 0;
    int outport_enabled = 0;
    int i;


    for (i = 0; i < SCL_DEV_CFG_COUNT; i++)
    {
        ret = MI_SCL_CreateDevice(scl_dev_cfg[i].dev_id, &scl_dev_cfg[i].attr);
        if (ret)
        {
            ERROR("MI_SCL_CreateDevice failed, dev[%u] mask(%x) err: %x !",
                   scl_dev_cfg[i].dev_id,
                   scl_dev_cfg[i].attr.u32NeedUseHWOutPortMask,
                   ret);
            goto EXIT;
        }
        dev_created++;
    }

    for (i = 0; i < SCL_CHN_CFG_COUNT; i++)
    {
        ret = MI_SCL_CreateChannel(scl_chn_cfg[i].dev_id,
                                   scl_chn_cfg[i].chn_id,
                                   &scl_chn_cfg[i].chn_attr);
        if (ret)
        {
            ERROR("MI_SCL_CreateChannel failed, dev[%u] chn[%u] err: %x !",
                   scl_chn_cfg[i].dev_id,
                   scl_chn_cfg[i].chn_id,
                   ret);
            goto EXIT;
        }
        chn_created++;

        if (MI_SCL_DEV_ISP_REALTIME0 != scl_chn_cfg[i].dev_id)
        {
            ret = MI_SCL_SetInputPortCrop(scl_chn_cfg[i].dev_id,
                                          scl_chn_cfg[i].chn_id,
                                          &scl_chn_cfg[i].inport_rect);
            if (ret)
            {
                ERROR("MI_SCL_SetInputPortCrop failed, dev[%u] chn[%u] rect(%u,%u,%u,%u) err: %x !",
                       scl_chn_cfg[i].dev_id,
                       scl_chn_cfg[i].chn_id,
                       scl_chn_cfg[i].inport_rect.u16X,
                       scl_chn_cfg[i].inport_rect.u16Y,
                       scl_chn_cfg[i].inport_rect.u16Width,
                       scl_chn_cfg[i].inport_rect.u16Height,
                       ret);
                goto EXIT;
            }
        }

        ret = MI_SCL_SetChnParam(scl_chn_cfg[i].dev_id,
                                 scl_chn_cfg[i].chn_id,
                                 &scl_chn_cfg[i].chn_param);
        if (ret)
        {
            ERROR("MI_SCL_SetChnParam failed, dev[%u] chn[%u] rot(%d) err: %x !",
                   scl_chn_cfg[i].dev_id,
                   scl_chn_cfg[i].chn_id,
                   scl_chn_cfg[i].chn_param.eRot,
                   ret);
            goto EXIT;
        }

        ret = MI_SCL_StartChannel(scl_chn_cfg[i].dev_id, scl_chn_cfg[i].chn_id);
        if (ret)
        {
            ERROR("MI_SCL_StartChannel failed, dev[%u] chn[%u] err: %x !",
                   scl_chn_cfg[i].dev_id,
                   scl_chn_cfg[i].chn_id,
                   ret);
            goto EXIT;
        }
        chn_started++;
    }

    for (i = 0; i < SCL_OUTPORT_CFG_COUNT; i++)
    {
        ret = MI_SCL_SetOutputPortParam(scl_outport_cfg[i].dev_id,
                                        scl_outport_cfg[i].chn_id,
                                        scl_outport_cfg[i].port_id,
                                        &scl_outport_cfg[i].param);
        if (ret)
        {
            ERROR("MI_SCL_SetOutputPortParam failed, dev[%u] chn[%u] port[%u] err: %x !",
                   scl_outport_cfg[i].dev_id,
                   scl_outport_cfg[i].chn_id,
                   scl_outport_cfg[i].port_id,
                   ret);
            goto EXIT;
        }

        ret = MI_SCL_EnableOutputPort(scl_outport_cfg[i].dev_id,
                                      scl_outport_cfg[i].chn_id,
                                      scl_outport_cfg[i].port_id);
        if (ret)
        {
            ERROR("MI_SCL_EnableOutputPort failed, dev[%u] chn[%u] port[%u] err: %x !",
                   scl_outport_cfg[i].dev_id,
                   scl_outport_cfg[i].chn_id,
                   scl_outport_cfg[i].port_id,
                   ret);
            goto EXIT;
        }
        outport_enabled++;
    }

    return 0;


EXIT:
    for (i = 0; i < outport_enabled; i++)
        MI_SCL_DisableOutputPort(scl_outport_cfg[i].dev_id,
                                 scl_outport_cfg[i].chn_id,
                                 scl_outport_cfg[i].port_id);
    for (i = 0; i < chn_started; i++)
        MI_SCL_StopChannel(scl_chn_cfg[i].dev_id, scl_chn_cfg[i].chn_id);
    for (i = 0; i < chn_created; i++)
        MI_SCL_DestroyChannel(scl_chn_cfg[i].dev_id, scl_chn_cfg[i].chn_id);
    for (i = 0; i < dev_created; i++)
        MI_SCL_DestroyDevice(scl_dev_cfg[i].dev_id);

    return -1;
}
static int deinit_scl(void)
{
    int i;

    for (i = 0; i < SCL_OUTPORT_CFG_COUNT; i++)
        MI_SCL_DisableOutputPort(scl_outport_cfg[i].dev_id,
                                 scl_outport_cfg[i].chn_id,
                                 scl_outport_cfg[i].port_id);
    for (i = 0; i < SCL_CHN_CFG_COUNT; i++)
    {
        MI_SCL_StopChannel(scl_chn_cfg[i].dev_id, scl_chn_cfg[i].chn_id);
        MI_SCL_DestroyChannel(scl_chn_cfg[i].dev_id, scl_chn_cfg[i].chn_id);
    }
    for (i = 0; i < SCL_DEV_CFG_COUNT; i++)
        MI_SCL_DestroyDevice(scl_dev_cfg[i].dev_id);

    return 0;
}


static int init_venc(void)
{
    MI_S32 ret;
    int dev_created = 0;
    int chn_created = 0;
    int i;


    for (i = 0; i < VENC_DEV_CFG_COUNT; i++)
    {
        ret = MI_VENC_CreateDev(venc_dev_cfg[i].dev_id, &venc_dev_cfg[i].param);
        if (ret)
        {
            ERROR("MI_VENC_CreateDev failed, dev[%d] maxsize(%u,%u) err: %x !",
                   venc_dev_cfg[i].dev_id,
                   venc_dev_cfg[i].param.u32MaxWidth,
                   venc_dev_cfg[i].param.u32MaxHeight,
                   ret);
            goto EXIT;
        }
        dev_created++;
    }

    for (i = 0; i < VENC_CHN_CFG_COUNT; i++)
    {
        ret = MI_VENC_CreateChn(venc_chn_cfg[i].dev_id,
                                venc_chn_cfg[i].chn_id,
                                &venc_chn_cfg[i].attr);
        if (ret)
        {
            ERROR("MI_VENC_CreateChn failed, dev[%d] chn[%d] err: %x !",
                   venc_chn_cfg[i].dev_id,
                   venc_chn_cfg[i].chn_id,
                   ret);
            goto EXIT;
        }
        chn_created++;

        ret = MI_VENC_SetInputSourceConfig(venc_chn_cfg[i].dev_id,
                                           venc_chn_cfg[i].chn_id,
                                           &venc_chn_cfg[i].insrc_cfg);
        if (ret)
        {
            ERROR("MI_VENC_SetInputSourceConfig failed, dev[%d] chn[%d] mode(%d) err: %x !",
                   venc_chn_cfg[i].dev_id,
                   venc_chn_cfg[i].chn_id,
                   venc_chn_cfg[i].insrc_cfg.eInputSrcBufferMode,
                   ret);
            goto EXIT;
        }

        ret = MI_VENC_StartRecvPic(venc_chn_cfg[i].dev_id, venc_chn_cfg[i].chn_id);
        if (ret)
        {
            ERROR("MI_VENC_StartRecvPic failed, dev[%d] chn[%d] err: %x !",
                   venc_chn_cfg[i].dev_id,
                   venc_chn_cfg[i].chn_id,
                   ret);
            goto EXIT;
        }
    }

    return 0;


EXIT:
    for (i = 0; i < chn_created; i++)
    {
        MI_VENC_StopRecvPic(venc_chn_cfg[i].dev_id, venc_chn_cfg[i].chn_id);
        MI_VENC_DestroyChn(venc_chn_cfg[i].dev_id, venc_chn_cfg[i].chn_id);
    }
    for (i = 0; i < dev_created; i++)
        MI_VENC_DestroyDev(venc_dev_cfg[i].dev_id);

    return -1;
}
static int deinit_venc(void)
{
    int i;

    for (i = 0; i < VENC_CHN_CFG_COUNT; i++)
    {
        MI_VENC_StopRecvPic(venc_chn_cfg[i].dev_id, venc_chn_cfg[i].chn_id);
        MI_VENC_DestroyChn(venc_chn_cfg[i].dev_id, venc_chn_cfg[i].chn_id);
    }
    for (i = 0; i < VENC_DEV_CFG_COUNT; i++)
        MI_VENC_DestroyDev(venc_dev_cfg[i].dev_id);

    return 0;
}


static struct timespec md_pause_time = {0};
static void do_md_cb(const MI_VDF_Result_t *rst, unsigned int threshold)
{
    algo_motion_det_cb_t cb;
    struct timespec time;
    MI_U32 len;
    unsigned int n;
    int i;


    clock_gettime(CLOCK_MONOTONIC, &time);
    if (   (md_pause_time.tv_sec > time.tv_sec)
        || ((md_pause_time.tv_sec == time.tv_sec) && (md_pause_time.tv_nsec > time.tv_nsec)))
    {
        return;
    }

    n = 0;
    len = rst->stMdResult.stSubResultSize.u32RstStatusLen;
    for (i = 0; i < len; i++)
    {
        if (rst->stMdResult.pstMdResultStatus->paddr[i])
            n++;
        if (threshold <= n)
            break;
    }
    if (threshold <= n)
    {
        for (i = 0; i < MAX_MD_CB; i++)
        {
            cb = md_cb_list[i];
            if (cb)
                cb();
        }
    }

    return;
}
static void* get_vdf_result(void *arg)
{
    MI_VDF_Result_t rst;
    MI_S32 ret;
    int i;

    pthread_detach(pthread_self());
    prctl(PR_SET_NAME, (unsigned long)"get_vdf_result", 0, 0, 0);

    while (vdf_should_run)
    {
        usleep(500 * 1000);

        for (i = 0; i < VDF_CFG_COUNT; i++)
        {
            ret = MI_VDF_GetResult(vdf_cfg[i].chn_id, &rst, 0);
            if (ret)
            {
                ERROR("MI_VDF_GetResult failed, err: %x !", ret);
                continue;
            }
            if (0 == rst.stMdResult.u8Enable)
                continue;

            if (E_MI_VDF_WORK_MODE_MD == rst.enWorkMode)
            {
                do_md_cb(&rst, vdf_cfg[i].threshold);
            }
            else if (E_MI_VDF_WORK_MODE_OD == rst.enWorkMode)
            {
            }
            MI_VDF_PutResult(vdf_cfg[i].chn_id, &rst);
        }
    }

    vdf_is_running = 0;
    return NULL;
}
static int init_vdf(void)
{
    MI_S32 ret;
    pthread_t tid;
    int i;


    if (0 == VDF_CFG_COUNT)
        return 0;

    ret = MI_VDF_Init();
    if (ret)
    {
        ERROR("MI_VDF_Init failed, err: %x !", ret);
        return -1;
    }

    MI_VDF_Run(E_MI_VDF_WORK_MODE_MD);
    MI_VDF_Run(E_MI_VDF_WORK_MODE_OD);
    MI_VDF_Run(E_MI_VDF_WORK_MODE_VG);

    for (i = 0; i < VDF_CFG_COUNT; i++)
    {
        ret = MI_VDF_CreateChn(vdf_cfg[i].chn_id, &vdf_cfg[i].attr);
        if (ret)
        {
            ERROR("MI_VDF_CreateChn failed, chn[%d] err: %x !", vdf_cfg[i].chn_id, ret);
            goto EXIT;
        }
        MI_VDF_EnableSubWindow(vdf_cfg[i].chn_id, 0, 0, 1);
    }

    vdf_should_run = 1;
    vdf_is_running = 1;
    pthread_create(&tid, NULL, get_vdf_result, NULL);

    return 0;


EXIT:
    for (; i > 0; i--)
    {
        MI_VDF_EnableSubWindow(vdf_cfg[i-1].chn_id, 0, 0, 0);
        MI_VDF_DestroyChn(vdf_cfg[i-1].chn_id);
    }
    MI_VDF_Stop(E_MI_VDF_WORK_MODE_MD);
    MI_VDF_Stop(E_MI_VDF_WORK_MODE_OD);
    MI_VDF_Stop(E_MI_VDF_WORK_MODE_VG);
    MI_VDF_Uninit();

    return -1;
}
static int deinit_vdf(void)
{
    int i;


    if (0 == VDF_CFG_COUNT)
        return 0;

    vdf_should_run = 0;
    for (i = 0; i < 7; i++)
    {
        usleep(200 * 1000);
        if (0 == vdf_is_running)
            break;
    }
    if (7 <= i)
    {
        ERROR("wait vdf exit timeout !");
    }

    for (i = 0; i < VDF_CFG_COUNT; i++)
    {
        MI_VDF_EnableSubWindow(vdf_cfg[i].chn_id, 0, 0, 0);
        MI_VDF_DestroyChn(vdf_cfg[i].chn_id);
    }
    MI_VDF_Stop(E_MI_VDF_WORK_MODE_MD);
    MI_VDF_Stop(E_MI_VDF_WORK_MODE_OD);
    MI_VDF_Stop(E_MI_VDF_WORK_MODE_VG);
    MI_VDF_Uninit();

    pthread_mutex_lock(&vdf_mutex);
    memset(md_cb_list, 0, sizeof(md_cb_list));
    pthread_mutex_unlock(&vdf_mutex);

    return 0;
}


static int init_ai(void)
{
    MI_S32 ret;
    unsigned char grp_cnt;
    unsigned char chn_per_grp;
    int dev_opened = 0;
    int i, j;


    for (i = 0; i < AI_DEV_CFG_COUNT; i++)
    {
        if (0 >= ai_dev_cfg[i].ifs_cnt)
            goto EXIT;
        chn_per_grp = ai_dev_cfg[i].attr.enSoundMode;
        grp_cnt = ai_dev_cfg[i].ifs_cnt * 2 / chn_per_grp;

        ret = MI_AI_Open(ai_dev_cfg[i].dev_id, &ai_dev_cfg[i].attr);
        if (ret)
        {
            ERROR("MI_AI_Open failed, dev[%d] err: %x !", ai_dev_cfg[i].dev_id, ret);
            goto EXIT;
        }
        dev_opened++;

        ret = MI_AI_AttachIf(ai_dev_cfg[i].dev_id, ai_dev_cfg[i].ifs, ai_dev_cfg[i].ifs_cnt);
        if (ret)
        {
            ERROR("MI_AI_AttachIf failed, dev[%d] err: %x !", ai_dev_cfg[i].dev_id, ret);
            goto EXIT;
        }

        for (j = 0; j < ai_dev_cfg[i].ifs_cnt; j++)
        {
            ret = MI_AI_SetIfGain(ai_dev_cfg[i].ifs[j],
                                  ai_dev_cfg[i].left_if_gain[j],
                                  ai_dev_cfg[i].right_if_gain[j]);
            if (ret)
            {
                ERROR("MI_AI_SetIfGain failed, ifs[%u] gain(%d,%d) err: %x !",
                       ai_dev_cfg[i].ifs[j],
                       ai_dev_cfg[i].left_if_gain[j],
                       ai_dev_cfg[i].right_if_gain[j],
                       ret);
                goto EXIT;
            }
        }

        for (j = 0; j < grp_cnt; j++)
        {
            ret = MI_AI_SetGain(ai_dev_cfg[i].dev_id, (MI_U8)j,
                                &ai_dev_cfg[i].gain[chn_per_grp * j], chn_per_grp);
            if (ret)
            {
                ERROR("MI_AI_SetGain failed, dev[%d] grp[%d] size(%u) err: %x !",
                       ai_dev_cfg[i].dev_id, j, chn_per_grp, ret);
                goto EXIT;
            }

            ret = MI_AI_EnableChnGroup(ai_dev_cfg[i].dev_id, (MI_U8)j);
            if (ret)
            {
                ERROR("MI_AI_EnableChnGroup failed, dev[%d] grp[%d] err: %x !",
                       ai_dev_cfg[i].dev_id, j, ret);
                goto EXIT;
            }
        }

    }

    return 0;


EXIT:
    for (i = 0; i < dev_opened; i++)
    {
        grp_cnt = ai_dev_cfg[i].ifs_cnt * 2 / ai_dev_cfg[i].attr.enSoundMode;
        for (j = 0; j < grp_cnt; j++)
            MI_AI_DisableChnGroup(ai_dev_cfg[i].dev_id, (MI_U8)j);
        MI_AI_Close(ai_dev_cfg[i].dev_id);
    }
    return -1;
}
static int deinit_ai(void)
{
    int i, j;
    unsigned char grp_cnt;

    for (i = 0; i < AI_DEV_CFG_COUNT; i++)
    {
        grp_cnt = ai_dev_cfg[i].ifs_cnt * 2 / ai_dev_cfg[i].attr.enSoundMode;
        for (j = 0; j < grp_cnt; j++)
            MI_AI_DisableChnGroup(ai_dev_cfg[i].dev_id, (MI_U8)j);
        MI_AI_Close(ai_dev_cfg[i].dev_id);
    }
    return 0;
}


static int init_output_port_depth(void)
{
    MI_SYS_ChnPort_t chn_port;
    MI_S32 ret;
    int i;

    for (i = 0; i < OUTPORT_DEPTH_CFG_COUNT; i++)
    {
        chn_port.eModId    = outport_depth_cfg[i].mod_id;
        chn_port.u32DevId  = outport_depth_cfg[i].dev_id;
        chn_port.u32ChnId  = outport_depth_cfg[i].chn_id;
        chn_port.u32PortId = outport_depth_cfg[i].port_id;
        ret = MI_SYS_SetChnOutputPortDepth(0, &chn_port,
                                           outport_depth_cfg[i].user_depth,
                                           outport_depth_cfg[i].queue_depth);
        if (ret)
        {
            ERROR("MI_SYS_SetChnOutputPortDepth failed, mod[%d] dev[%u] chn[%u] port[%u] user_depth(%u) queue_depth(%u) err: %x !",
                   outport_depth_cfg[i].mod_id,
                   outport_depth_cfg[i].dev_id,
                   outport_depth_cfg[i].chn_id,
                   outport_depth_cfg[i].port_id,
                   outport_depth_cfg[i].user_depth,
                   outport_depth_cfg[i].queue_depth,
                   ret);
            return -1;
        }
    }

    return 0;
}


static int bind(void)
{
    MI_S32 ret;
    int i;

    for (i = 0; i < BIND_CFG_COUNT; i++)
    {
        ret = MI_SYS_BindChnPort2(0,
                                  &bind_cfg[i].src,
                                  &bind_cfg[i].dst,
                                  bind_cfg[i].src_frmrate,
                                  bind_cfg[i].dst_frmrate,
                                  bind_cfg[i].type,
                                  bind_cfg[i].param);
        if (ret)
        {
            ERROR("MI_SYS_BindChnPort2 failed, src[%d,%u,%u,%u] dst[%d,%u,%u,%u] src_frmrate(%u) dst_frmrate(%u) type(%d) param(%u) err: %x !",
                   bind_cfg[i].src.eModId,
                   bind_cfg[i].src.u32DevId,
                   bind_cfg[i].src.u32ChnId,
                   bind_cfg[i].src.u32PortId,
                   bind_cfg[i].dst.eModId,
                   bind_cfg[i].dst.u32DevId,
                   bind_cfg[i].dst.u32ChnId,
                   bind_cfg[i].dst.u32PortId,
                   bind_cfg[i].src_frmrate,
                   bind_cfg[i].dst_frmrate,
                   bind_cfg[i].type,
                   bind_cfg[i].param,
                   ret);
            goto EXIT;
        }
    }

    return 0;


EXIT:
    for (; i > 0; i--)
        MI_SYS_UnBindChnPort(0, &bind_cfg[i-1].src, &bind_cfg[i-1].dst);

    return -1;
}
static int unbind(void)
{
    int i;

    for (i = 0; i < BIND_CFG_COUNT; i++)
        MI_SYS_UnBindChnPort(0, &bind_cfg[i].src, &bind_cfg[i].dst);

    return 0;
}


static int load_isp_file(void)
{
    MI_ISP_IQ_VersionInfoType_t ver;
    MI_ISP_IQ_ParamInitInfoType_t init;
    MI_S32 ret;
    int i, j;

    for (i = 0; i < ISP_CHN_CFG_COUNT; i++)
    {
        for (j = 0; j < 10; j++)
        {
            usleep(100 * 1000); /* 大约80ms初始化完 */
            MI_ISP_IQ_GetParaInitStatus(isp_chn_cfg[i].dev_id, isp_chn_cfg[i].chn_id, &init);
            if (E_SS_IQ_TRUE == init.stParaAPI.bFlag)
                break;
        }
        if (10 <= j)
        {
            ERROR("wait for para inited timeout !");
            return -1;
        }

        MI_ISP_IQ_GetVersionInfo(isp_chn_cfg[i].dev_id, isp_chn_cfg[i].chn_id, &ver);
        DEBUG("isp dev[%u] chn[%u] Vendor:%u Ver:%u.%u",
               isp_chn_cfg[i].dev_id, isp_chn_cfg[i].chn_id,
               ver.stParaAPI.u32Vendor, ver.stParaAPI.u32Major, ver.stParaAPI.u32Minor);

        ret = MI_ISP_ApiCmdLoadBinFile(isp_chn_cfg[i].dev_id, isp_chn_cfg[i].chn_id,
                                       isp_chn_cfg[i].bin_file, 0x4d2);
        if (ret)
        {
            ERROR("MI_ISP_ApiCmdLoadBinFile failed, dev[%u] chn[%u] file:\"%s\", err: %x !",
                   isp_chn_cfg[i].dev_id, isp_chn_cfg[i].chn_id, isp_chn_cfg[i].bin_file, ret);
            return -1;
        }
    }

    return 0;
}


#define MAX_PACKS 4
static void* get_video_stream(void *arg)
{
    stream_id_t stream_id = (stream_id_t)arg;
    video_format_t format;
    unsigned int width;
    unsigned int height;
    unsigned int fps;
    MI_VENC_DEV devid;
    MI_VENC_CHN chnid;
    MI_S32 fd;
    fd_set read_fds;
    struct timeval timeout;
    MI_VENC_ChnStat_t stat;
    MI_VENC_Stream_t stream;
    frame_t *frame;
    unsigned int total_len;
    MI_S32 ret;
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
            devid  = video_stream_cfg[i].venc_devid;
            chnid  = video_stream_cfg[i].venc_chnid;
            break;
        }
    }

    stream.pstPack = malloc(sizeof(MI_VENC_Pack_t) * MAX_PACKS);
    if (NULL == stream.pstPack)
    {
        ERROR("malloc %zu failed !", sizeof(MI_VENC_Pack_t) * MAX_PACKS);
        stream_state_list[stream_id].is_running = 0;
        return 0;
    }
    fd = MI_VENC_GetFd(devid, chnid);
    MI_VENC_RequestIdr(devid, chnid, 1);

    while (stream_state_list[stream_id].should_run)
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
            DEBUG("video stream[%d] select null", stream_id);
            continue;
        }

        ret = MI_VENC_Query(devid, chnid, &stat);
        if (MI_SUCCESS != ret)
        {
            ERROR("MI_VENC_Query faild, dev[%d] chn[%d] err: %x !", devid, chnid, ret);
            continue;
        }
        if (0 == stat.u32CurPacks)
        {
            DEBUG("video stream[%d] u32CurPacks is 0", stream_id);
            continue;
        }
        if (MAX_PACKS < stat.u32CurPacks)
        {
            ERROR("MI_VENC_Query err, get %u packs exceeds the max(%u) !",
                   stat.u32CurPacks, MAX_PACKS);
            continue;
        }

        stream.u32PackCount = stat.u32CurPacks;
        ret = MI_VENC_GetStream(devid, chnid, &stream, 1000);
        if (MI_SUCCESS != ret)
        {
            ERROR("MI_VENC_GetStream failed, dev[%d] chn[%d] err: %x !", devid, chnid, ret);
            continue;
        }

        total_len = 0;
        for (i = 0; i < stream.u32PackCount; i++)
        {
            total_len += stream.pstPack[i].u32Len - stream.pstPack[i].u32Offset;
        }
        frame = hism_alloc_frame(total_len);
        if (NULL == frame)
        {
            ERROR("hism_alloc_frame(%u) failed !", total_len);
            if (MI_VENC_ReleaseStream(devid, chnid, &stream))
            {
                ERROR("MI_VENC_ReleaseStream failed, dev[%d] chn[%d] err: %x !",
                       devid, chnid, ret);
            }
            continue;
        }

        frame->stream_id = stream_id; /* 为了防止意外出错，每次都赋值 */
        frame->pts = stream.pstPack[0].u64PTS;
        frame->video_info.format = format;
        frame->video_info.fps = fps;
        frame->video_info.width = width;
        frame->video_info.height = height;
        frame->size = total_len;
        total_len = 0;
        for (i = 0; i < stream.u32PackCount; i++)
        {
            memcpy(frame->buf + total_len,
                   stream.pstPack[i].pu8Addr + stream.pstPack[i].u32Offset,
                   stream.pstPack[i].u32Len  - stream.pstPack[i].u32Offset);
            total_len += stream.pstPack[i].u32Len - stream.pstPack[i].u32Offset;
        }
        if (hism_put_stream_frame(frame))
            free(frame);

        ret = MI_VENC_ReleaseStream(devid, chnid, &stream);
        if (MI_SUCCESS != ret)
        {
            ERROR("MI_VENC_ReleaseStream failed, dev[%d] chn[%d] err: %x !",
                   devid, chnid, ret);
        }
    }

    MI_VENC_CloseFd(devid, chnid);
    free(stream.pstPack);
    stream_state_list[stream_id].is_running = 0;

    return NULL;
}
static void* get_audio_stream(void *arg)
{
    stream_id_t stream_id = (stream_id_t)arg;
    audio_format_t format;
    unsigned int channels;
    unsigned int sample_rate;
    MI_AUDIO_DEV dev_id;
    MI_U8 grp_id;
    MI_AI_Data_t data;
    frame_t *frame;
    MI_S32 ret;
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
            grp_id      = audio_stream_cfg[i].grp_id;
            break;
        }
    }

    while (stream_state_list[stream_id].should_run)
    {
        ret = MI_AI_Read(dev_id, grp_id, &data, NULL, 1000);
        if (ret)
        {
            ERROR("MI_AI_Read failed, dev[%d] grp[%u] err: %x !", dev_id, grp_id, ret);
            continue;
        }
        frame = hism_alloc_frame(data.u32Byte[0]);
        if (NULL == frame)
        {
            ERROR("hism_alloc_frame(%u) failed !", data.u32Byte[0]);
            MI_AI_ReleaseData(dev_id, grp_id, &data, NULL);
            continue;
        }

        frame->stream_id = stream_id;
        frame->pts = data.u64Pts;
        frame->audio_info.format = format;
        frame->audio_info.channels = channels;
        frame->audio_info.sample_rate = sample_rate;
        frame->size = data.u32Byte[0];
        memcpy(frame->buf, data.apvBuffer[0], frame->size);
        if (hism_put_stream_frame(frame))
            free(frame);

        MI_AI_ReleaseData(dev_id, grp_id, &data, NULL);
    }

    stream_state_list[stream_id].is_running = 0;
    return NULL;
}


int hism_init(void)
{
    MI_SYS_Version_t version;
    MI_S32 ret;


    pthread_mutex_lock(&media_mutex);
    if (media_inited)
    {
        pthread_mutex_unlock(&media_mutex);
        return 0;
    }

    if (hism_cache_init())
        goto EXIT;

    memset(stream_state_list, 0, sizeof(stream_state_list));
    memset(md_cb_list, 0, sizeof(md_cb_list));

    ret = MI_SYS_Init(0);
    if (ret)
    {
        ERROR("MI_SYS_Init err: %x !", ret);
        goto EXIT;
    }
    memset(&version, 0, sizeof(version));
    MI_SYS_GetVersion(0, &version);
    DEBUG("MI_Version: %s", version.u8Version);
    MI_SYS_InitPtsBase(0, 0);
    MI_SYS_SyncPts(0, 0);

    if (init_mma())
        goto EXIT_SYS;
    if (init_sensor())
        goto EXIT_MMA;
    if (init_vif())
        goto EXIT_SENSOR;
    if (init_isp())
        goto EXIT_VIF;
    if (init_rgn())
        goto EXIT_ISP;
    if (init_scl())
        goto EXIT_RGN;
    if (init_venc())
        goto EXIT_SCL;
    if (init_vdf())
        goto EXIT_VENC;
    if (init_ai())
        goto EXIT_VDF;
    if (init_output_port_depth())
        goto EXIT_AI;
    if (bind())
        goto EXIT_AI;

    load_isp_file();

    media_inited = 1;
    pthread_mutex_unlock(&media_mutex);

    return 0;


//EXIT_BIND:
    //unbind();
EXIT_AI:
    deinit_ai();
EXIT_VDF:
    deinit_vdf();
EXIT_VENC:
    deinit_venc();
EXIT_SCL:
    deinit_scl();
EXIT_RGN:
    deinit_rgn();
EXIT_ISP:
    deinit_isp();
EXIT_VIF:
    deinit_vif();
EXIT_SENSOR:
    deinit_sensor();
EXIT_MMA:
    deinit_mma();
EXIT_SYS:
    MI_SYS_Exit(0);
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

    for (i = 0; i < STREAM_ID_MAX; i++)
    {
        stream_state_list[i].should_run = 0;
    }
    for (i = 0; i < 7; i++)
    {
        for (j = 0; j < STREAM_ID_MAX; j++)
        {
            if (stream_state_list[j].is_running)
                break;
        }
        if (STREAM_ID_MAX <= j)
            break;
        usleep(200 * 1000);
    }
    if (7 <= i)
    {
        ERROR("wait media exit timeout !");
    }


    unbind();
    deinit_ai();
    deinit_vdf();
    deinit_venc();
    deinit_scl();
    deinit_rgn();
    deinit_isp();
    deinit_vif();
    deinit_sensor();
    deinit_mma();
    MI_SYS_Exit(0);

    media_inited = 0;
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
    MI_IQSERVER_SetDataPath("/config");
    MI_IQSERVER_Open();
    return 0;
}
int hism_stop_isp_tool(void)
{
    MI_IQSERVER_Close();
    return 0;
}


/* 返回的buf是MIU物理地址对应的虚拟地址 */
int hism_get_signal_frame(signal_frame_id_t id, signal_frame_t *frame)
{
    MI_SYS_ChnPort_t chn_port;
    MI_SYS_BufInfo_t info;
    MI_SYS_BUF_HANDLE handle;
    MI_S32 ret;
    int i;

    if (NULL == frame)
        return -1;

    for (i = 0; i < SIGNAL_FRAME_CFG_COUNT; i++)
    {
        if (id != signal_frame_cfg[i].frame_id)
            continue;

        chn_port.eModId    = signal_frame_cfg[i].mod_id;
        chn_port.u32DevId  = signal_frame_cfg[i].dev_id;
        chn_port.u32ChnId  = signal_frame_cfg[i].chn_id;
        chn_port.u32PortId = signal_frame_cfg[i].port_id;
        break;
    }
    if (SIGNAL_FRAME_CFG_COUNT <= i)
    {
        ERROR("signal frame[%d] is not configured !", id);
        return -1;
    }

    ret = MI_SYS_ChnOutputPortGetBuf(&chn_port, &info, &handle);
    if (ret)
    {
        ERROR("MI_SYS_ChnOutputPortGetBuf failed, mod[%d] dev[%u] chn[%u] port[%u] err: %x !",
               chn_port.eModId, chn_port.u32DevId, chn_port.u32ChnId, chn_port.u32PortId, ret);
        return -1;
    }
    if (E_MI_SYS_BUFDATA_FRAME != info.eBufType)
        return -1;

    frame->pts    = info.u64Pts;
    frame->width  = info.stFrameData.u16Width;
    frame->height = info.stFrameData.u16Height;
    frame->stride = info.stFrameData.u32Stride[0];
    frame->size   = info.stFrameData.u32BufSize;
    frame->buf    = info.stFrameData.pVirAddr[0];
    frame->handle = (void *)handle;

    return 0;
}


int hism_resize_frame(const signal_frame_t *in_frame, resize_t *resize, signal_frame_t *out_frame)
{
    MI_SYS_WindowRect_t rect;
    MI_SCL_DirectBuf_t src;
    MI_SCL_DirectBuf_t dst;
    MI_S32 ret;


    if ((NULL == in_frame) || (NULL == in_frame->buf) || (NULL == resize) || (NULL == out_frame))
        return -1;

    if ((in_frame->width % 32) || (in_frame->height % 16))
    {
        ERROR("input image(%u,%u) should be (32,16) aligned !",
               in_frame->width, in_frame->height);
        //return -1;
    }
    /* 32,16对齐 */
    resize->crop_x   &= ~0x1f;
    resize->crop_y   &= ~0xf;
    resize->crop_w   &= ~0x1f;
    resize->crop_h   &= ~0xf;
    resize->output_w &= ~0x1f;
    resize->output_h &= ~0xf;

    rect.u16X      = resize->crop_x;
    rect.u16Y      = resize->crop_y;
    rect.u16Width  = resize->crop_w;
    rect.u16Height = resize->crop_h;

    src.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
    src.u32Width     = in_frame->width;
    src.u32Height    = in_frame->height;
    src.u32Stride[0] = in_frame->stride;
    src.u32Stride[1] = in_frame->stride;
    src.u32BuffSize  = in_frame->size;
    ret = MI_SYS_Va2Pa(in_frame->buf, &src.phyAddr[0]);
    if (ret || (NULL == src.phyAddr[0]))
    {
        ERROR("MI_SYS_Va2Pa failed, err: %x !", ret);
        return -1;
    }
    src.phyAddr[1] = src.phyAddr[0] + src.u32Width * src.u32Height;

    dst.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
    dst.u32Width     = resize->output_w;
    dst.u32Height    = resize->output_h;;
    dst.phyAddr[0]   = NULL;
    dst.phyAddr[1]   = NULL;
    dst.u32Stride[0] = resize->output_w;
    dst.u32Stride[1] = resize->output_w;
    dst.u32BuffSize  = resize->output_w * resize->output_h * 3 / 2;
    ret = MI_SYS_MMA_Alloc(0, (MI_U8*)"mma_heap_name0", dst.u32BuffSize, &dst.phyAddr[0]);
    if (ret || (NULL == dst.phyAddr[0]))
    {
        ERROR("MI_SYS_MMA_Alloc failed, err: %x !", ret);
        goto EXIT;
    }
    dst.phyAddr[1] = dst.phyAddr[0] + dst.u32Width * dst.u32Height;

    ret = MI_SCL_StretchBuf(&src, &rect, &dst, E_MI_SCL_FILTER_TYPE_AUTO);
    if (ret)
    {
        ERROR("MI_SCL_StretchBuf failed, err: %x !", ret);
        goto EXIT;
    }

    ret = MI_SYS_Mmap(dst.phyAddr[0], dst.u32BuffSize, (void **)&out_frame->buf, 0);
    if (ret || (NULL == out_frame->buf))
    {
        ERROR("MI_SYS_Mmap failed, err: %x !", ret);
        goto EXIT;
    }
    out_frame->pts    = in_frame->pts;
    out_frame->width  = dst.u32Width;
    out_frame->height = dst.u32Height;
    out_frame->stride = dst.u32Stride[0];
    out_frame->size   = dst.u32BuffSize;
    out_frame->handle = NULL;

    return 0;


EXIT:
    if (dst.phyAddr[0])
        MI_SYS_MMA_Free(0, dst.phyAddr[0]);

    return -1;
}


static pthread_mutex_t jpeg_mutex = PTHREAD_MUTEX_INITIALIZER;
/* crop会返回实际对齐后的区域，只有裁剪没有缩放 */
int hism_encode_jpeg(const signal_frame_t *frame, resize_t *crop, unsigned char *buf, unsigned int size)
{
    MI_VENC_CropCfg_t crop_cfg;
    MI_VENC_RecvPicParam_t recv_param;
    MI_SYS_ChnPort_t chn_port;
    MI_SYS_BufConf_t buf_conf;
    MI_SYS_BufInfo_t buf_info;
    MI_SYS_BUF_HANDLE handle;
    MI_PHY phy_addr;
    MI_VENC_ChnStat_t stat;
    MI_VENC_Stream_t stream;
    unsigned int total_len;
    MI_S32 ret;
    int i;


    if ((NULL == frame) ||(NULL == crop) ||(NULL == buf))
        return -1;

    if (   (frame->width  < (crop->crop_x + crop->crop_w))
        || (frame->height < (crop->crop_y + crop->crop_h)))
        return -1;
    crop->crop_x = (crop->crop_x / jpeg_encode_cfg.alignment_x) * jpeg_encode_cfg.alignment_x;
    crop->crop_y = (crop->crop_y / jpeg_encode_cfg.alignment_y) * jpeg_encode_cfg.alignment_y;
    crop->crop_w = ((crop->crop_w + jpeg_encode_cfg.alignment_width - 1) / jpeg_encode_cfg.alignment_width)
                   * jpeg_encode_cfg.alignment_width;
    crop->crop_h = ((crop->crop_h + jpeg_encode_cfg.alignment_height - 1) / jpeg_encode_cfg.alignment_height)
                   * jpeg_encode_cfg.alignment_height;
    if (frame->width < (crop->crop_x + crop->crop_w))
        crop->crop_w -= jpeg_encode_cfg.alignment_width;
    if (frame->height < (crop->crop_y + crop->crop_h))
        crop->crop_h -= jpeg_encode_cfg.alignment_height;

    pthread_mutex_lock(&jpeg_mutex);

    /* put frame */
    crop_cfg.bEnable = 1;
    crop_cfg.stRect.u32Left   = crop->crop_x;
    crop_cfg.stRect.u32Top    = crop->crop_y;
    crop_cfg.stRect.u32Width  = crop->crop_w;
    crop_cfg.stRect.u32Height = crop->crop_h;
    ret = MI_VENC_SetCrop(MI_VENC_DEV_ID_JPEG_0, 0, &crop_cfg);
    if (MI_SUCCESS != ret)
    {
        ERROR("MI_VENC_SetCrop failed, dev[%d] chn[0] crop(%u,%u,%u,%u) err: %x !",
               MI_VENC_DEV_ID_JPEG_0, crop->crop_x, crop->crop_y, crop->crop_w, crop->crop_h, ret);
        goto EXIT;
    }

    recv_param.s32RecvPicNum = 1;
    ret = MI_VENC_StartRecvPicEx(MI_VENC_DEV_ID_JPEG_0, 0, &recv_param);
    if (MI_SUCCESS != ret)
    {
        ERROR("MI_VENC_StartRecvPicEx failed, dev[%d] chn[0] err: %x !",
               MI_VENC_DEV_ID_JPEG_0, ret);
        goto EXIT;
    }

    chn_port.eModId    = E_MI_MODULE_ID_VENC;
    chn_port.u32DevId  = MI_VENC_DEV_ID_JPEG_0;
    chn_port.u32ChnId  = 0;
    chn_port.u32PortId = 0;
    buf_conf.eBufType                  = E_MI_SYS_BUFDATA_FRAME;
    buf_conf.u32Flags                  = 0;
    buf_conf.u64TargetPts              = frame->pts;
    buf_conf.bDirectBuf                = 0;
    buf_conf.bCrcCheck                 = 0;
    buf_conf.stFrameCfg.u16Width       = frame->width;
    buf_conf.stFrameCfg.u16Height      = frame->height;
    buf_conf.stFrameCfg.eFrameScanMode = E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE;
    buf_conf.stFrameCfg.eFormat        = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
    buf_conf.stFrameCfg.eCompressMode  = E_MI_SYS_COMPRESS_MODE_NONE;
    ret = MI_SYS_ChnInputPortGetBuf(&chn_port, &buf_conf, &buf_info, &handle, -1);
    if (MI_SUCCESS != ret)
    {
        ERROR("MI_SYS_ChnInputPortGetBuf failed, dev[%d] chn[0] port[0] err: %x !",
               MI_VENC_DEV_ID_JPEG_0, ret);
        goto EXIT;
    }

    ret = MI_SYS_Va2Pa(frame->buf, &phy_addr);
    if (ret)
    {
        ERROR("MI_SYS_Va2Pa failed, err: %x !", ret);
        goto EXIT;
    }
    MI_SYS_MemcpyPa(0, buf_info.stFrameData.phyAddr[0], phy_addr, frame->size);
    ret = MI_SYS_ChnInputPortPutBuf(handle, &buf_info, 0);
    if (MI_SUCCESS != ret)
    {
        ERROR("MI_SYS_ChnInputPortPutBuf failed, err: %x !", ret);
        goto EXIT;
    }


    /* get jpeg */
    for (i = 0; i < 30; i++)
    {
        usleep(10 * 1000);

        ret = MI_VENC_Query(MI_VENC_DEV_ID_JPEG_0, 0, &stat);
        if (MI_SUCCESS != ret)
        {
            ERROR("MI_VENC_Query faild, dev[%d] chn[0] err: %x !",
                   MI_VENC_DEV_ID_JPEG_0, ret);
            continue;
        }
        if (1 <= stat.u32CurPacks)
            break;
    }
    if (30 <= i)
    {
        ERROR("wait encode jpeg time out !");
        goto EXIT;
    }

    stream.pstPack = NULL;
    stream.pstPack = malloc(sizeof(MI_VENC_Pack_t) * stat.u32CurPacks);
    if (NULL == stream.pstPack)
    {
        ERROR("malloc %zu failed !", sizeof(MI_VENC_Pack_t) * stat.u32CurPacks);
        goto EXIT;
    }
    stream.u32PackCount = stat.u32CurPacks;
    ret = MI_VENC_GetStream(MI_VENC_DEV_ID_JPEG_0, 0, &stream, 400);
    if (MI_SUCCESS != ret)
    {
        ERROR("MI_VENC_GetStream failed, dev[%d] chn[0] err: %x !",
               MI_VENC_DEV_ID_JPEG_0, ret);
        goto EXIT;
    }

    total_len = 0;
    for (i = 0; i < stream.u32PackCount; i++)
    {
        total_len += stream.pstPack[i].u32Len - stream.pstPack[i].u32Offset;
    }
    if (size < total_len)
    {
        ERROR("jpeg size(%d) exceeds buf size(%u) !", total_len, size);
        goto EXIT;
    }
    total_len = 0;
    for (i = 0; i < stream.u32PackCount; i++)
    {
        memcpy(buf + total_len,
               stream.pstPack[i].pu8Addr + stream.pstPack[i].u32Offset,
               stream.pstPack[i].u32Len  - stream.pstPack[i].u32Offset);
        total_len += stream.pstPack[i].u32Len  - stream.pstPack[i].u32Offset;
    }

    ret = MI_VENC_ReleaseStream(MI_VENC_DEV_ID_JPEG_0, 0, &stream);
    if (MI_SUCCESS != ret)
    {
        ERROR("MI_VENC_ReleaseStream failed, dev[%d] chn[0] err: %x !",
               MI_VENC_DEV_ID_JPEG_0, ret);
    }

    pthread_mutex_unlock(&jpeg_mutex);
    free(stream.pstPack);

    return (int)total_len;


EXIT:
    pthread_mutex_unlock(&jpeg_mutex);
    if (stream.pstPack)
        free(stream.pstPack);

    return -1;
}


int hism_release_signal_frame(signal_frame_t *frame)
{
    MI_PHY phy_addr;
    MI_S32 ret;

    if (NULL == frame)
        return -1;

    if (frame->handle)
    {
        if (MI_SYS_ChnOutputPortPutBuf((MI_SYS_BUF_HANDLE)frame->handle))
            return -1;
        frame->handle = NULL;
        return 0;
    }

    ret = MI_SYS_Va2Pa(frame->buf, &phy_addr);
    if (ret)
    {
        ERROR("MI_SYS_Va2Pa failed, err: %x !", ret);
        return -1;
    }
    MI_SYS_Munmap(frame->buf, frame->size);
    MI_SYS_MMA_Free(0, phy_addr);
    frame->buf = NULL;

    return 0;
}


int hism_write_osd(stream_id_t stream_id, osd_id_t osd_id, unsigned char *buf)
{
    MI_RGN_CanvasInfo_t info;
    int i;

    for (i = 0; i < OSD_CFG_COUNT; i++)
    {
        if (osd_id == osd_cfg[i].osd_info.osd_id)
            break;
    }
    if (OSD_CFG_COUNT <= i)
        return -1;

    MI_RGN_GetCanvasInfo(0, osd_cfg[i].handle, &info);
    memcpy((void *)info.virtAddr, buf, osd_cfg[i].osd_info.width * osd_cfg[i].osd_info.height / 4);
    MI_RGN_UpdateCanvas(0, osd_cfg[i].handle);

    return 0;
}


/* 暂时没有取消注册操作 */
int hism_register_algo_motion_det_cb(algo_motion_det_cb_t cb)
{
    int i;

    if (NULL == cb)
        return -1;

    pthread_mutex_lock(&vdf_mutex);
    for (i = 0; i < MAX_MD_CB; i++)
    {
        if (NULL == md_cb_list[i])
        {
            md_cb_list[i] = cb;
            pthread_mutex_unlock(&vdf_mutex);
            return 0;
        }
    }
    pthread_mutex_unlock(&vdf_mutex);

    ERROR("cb count exceeds the max(%u) !", MAX_MD_CB);
    return -1;
}
/* 暂停检测s秒 */
int hism_suspend_algo_motion_det(unsigned int s)
{
    struct timespec time;

    DEBUG("suspend motion detection %us", s);

    clock_gettime(CLOCK_MONOTONIC, &time);
    pthread_mutex_lock(&vdf_mutex);
    if (   (md_pause_time.tv_sec > time.tv_sec + s)
        || ((md_pause_time.tv_sec == time.tv_sec + s) && (md_pause_time.tv_nsec > time.tv_nsec)))
    {
        pthread_mutex_unlock(&vdf_mutex);
        return 0;
    }
    md_pause_time.tv_sec  = time.tv_sec + s;
    md_pause_time.tv_nsec = time.tv_nsec;
    pthread_mutex_unlock(&vdf_mutex);

    return 0;
}
