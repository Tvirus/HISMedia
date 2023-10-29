#ifndef _HISMEDIA_CONFIG_H_
#define _HISMEDIA_CONFIG_H_


/*************************************************
    MMA Config
**************************************************/
static mma_cfg_t mma_cfg[] =
{
    {
        {
            E_MI_SYS_PER_DEV_PRIVATE_RING_POOL,  /* eConfigType   */
            1,                                   /* bCreate       */
            {
                .stpreDevPrivRingPoolConfig =
                {
                    E_MI_MODULE_ID_SCL,          /* eModule       */
                    MI_SCL_DEV_ISP_REALTIME0,    /* u32Devid      */
                    2560,                        /* u16MaxWidth   */
                    1440,                        /* u16MaxHeight  */
                    1440/1,                      /* u16RingLine   */
                    ""                           /* u8MMAHeapName */
                }
            }
        }
    },
    {
        {
            E_MI_SYS_PER_DEV_PRIVATE_RING_POOL,  /* eConfigType   */
            1,                                   /* bCreate       */
            {
                .stpreDevPrivRingPoolConfig =
                {
                    E_MI_MODULE_ID_VENC,         /* eModule       */
                    MI_VENC_DEV_ID_H264_H265_0,  /* u32Devid      */
                    2560,                        /* u16MaxWidth   */
                    1440,                        /* u16MaxHeight  */
                    1440,                        /* u16RingLine   */
                    ""                           /* u8MMAHeapName */
                }
            }
        }
    }
};
#define MMA_CFG_COUNT (sizeof(mma_cfg) / sizeof(mma_cfg[0]))




/*************************************************
    PAD Config
**************************************************/
static const pad_cfg_t pad_cfg[] =
{
    /* pad_id  res_id  hdr_mode  fps */
    {       0,      0,        0,  25  }
};
#define PAD_CFG_COUNT (sizeof(pad_cfg) / sizeof(pad_cfg[0]))




/*************************************************
    VIF Config
**************************************************/
static vif_grp_cfg_t vif_grp_cfg[] =
{
    {
        0,  /* GroupId */
        {
            E_MI_VIF_MODE_MIPI,                    /* eIntfMode          */
            E_MI_VIF_WORK_MODE_1MULTIPLEX,         /* eWorkMode          */
            E_MI_VIF_HDR_TYPE_OFF,                 /* eHDRType           */
            E_MI_VIF_CLK_EDGE_DOUBLE,              /* eClkEdge           */
            E_MI_VIF_MCLK_27MHZ,                   /* eMclk              */
            E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE,  /* eScanMode          */
            E_MI_VIF_GROUPMASK_ID0                 /* u32GroupStitchMask */
        }
    }
};
#define VIF_GRP_CFG_COUNT (sizeof(vif_grp_cfg) / sizeof(vif_grp_cfg[0]))

/* eInputPixel的定义见RGB_BAYER_PIXEL */
static vif_dev_cfg_t vif_dev_cfg[] =
{
    /* DevId    InputPixel     stInputRect                    eField  EnH2T1PMode */
    {      0,  {        34, {0,0,2560,1440}, E_MI_SYS_FIELDTYPE_NONE,           0} }
};
#define VIF_DEV_CFG_COUNT (sizeof(vif_dev_cfg) / sizeof(vif_dev_cfg[0]))

static vif_outport_cfg_t vif_outport_cfg[] =
{
    /* Dev Port   CapRect(x,y,w,h)    DestSize  PixFmt               eFrameRate                eCompressMode */
    {    0,   0, { {0,0,2560,1440}, {2560,1440},    34, E_MI_VIF_FRAMERATE_FULL, E_MI_SYS_COMPRESS_MODE_NONE} }
};
#define VIF_OUTPORT_CFG_COUNT (sizeof(vif_outport_cfg) / sizeof(vif_outport_cfg[0]))




/*************************************************
    ISP Config
**************************************************/
static isp_dev_cfg_t isp_dev_cfg[] =
{
    /*     DevId          u32DevStitchMask */
    {MI_ISP_DEV0, {E_MI_ISP_DEVICEMASK_ID0} }
};
#define ISP_DEV_CFG_COUNT (sizeof(isp_dev_cfg) / sizeof(isp_dev_cfg[0]))

static isp_chn_cfg_t isp_chn_cfg[] =
{
    {
        MI_ISP_DEV0,  /* DevId */
        0,            /* ChnId */
        "/config/rgb.bin",
        /* MI_ISP_ChannelAttr_t */
        {
            E_MI_ISP_SENSOR0,                         /* u32SensorBindId  */
            {{0,0,{0}}},                              /* stIspCustIqParam */
            E_MI_ISP_SYNC3A_AE | E_MI_ISP_SYNC3A_AWB | E_MI_ISP_SYNC3A_IQ  /* u32Sync3AType    */
        },
        /* MI_ISP_ChnParam_t */
        {
            E_MI_ISP_HDR_TYPE_OFF,                    /* eHDRType         */
            E_MI_ISP_3DNR_LEVEL1,                     /* e3DNRLevel       */
            0,                                        /* bMirror          */
            0,                                        /* bFlip            */
            E_MI_SYS_ROTATE_NONE,                     /* eRot             */
            0                                         /* bY2bEnable       */
        }
    }
};
#define ISP_CHN_CFG_COUNT (sizeof(isp_chn_cfg) / sizeof(isp_chn_cfg[0]))

static isp_outport_cfg_t isp_outport_cfg[] =
{
    /*     DevId Chn Port   CropRect                              ePixelFormat                eCompressMode                        eBufLayout */
    {MI_ISP_DEV0,  0,   0, {{0,0,0,0}, E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420, E_MI_SYS_COMPRESS_MODE_NONE, E_MI_ISP_BUFFER_LAYOUT_ONE_FRAME} }
};
#define ISP_OUTPORT_CFG_COUNT (sizeof(isp_outport_cfg) / sizeof(isp_outport_cfg[0]))




/*************************************************
    RGN Config
**************************************************/
static rgn_cfg_t rgn_cfg =
{
    {                                /* MI_RGN_PaletteTable_t   */
        {                            /* MI_RGN_PaletteElement_t */
            {  0,   0,   0,   0},    /* astElement[0] */
            {255, 255,   0,   0},    /* astElement[1] */
            {255,   0, 255,   0},    /* astElement[2] */
            {255,   0,   0, 255}     /* astElement[3] */
        }
    }
};

static rgn_region_cfg_t rgn_region_cfg[] =
{
    /* handle               eType                  ePixelFmt   stSize  */
    {       0, {E_MI_RGN_TYPE_OSD, {E_MI_RGN_PIXEL_FORMAT_I2, {640,64}}}}
};
#define RGN_REGION_CFG_COUNT (sizeof(rgn_region_cfg) / sizeof(rgn_region_cfg[0]))

static rgn_attach_cfg_t rgn_attach_cfg[] =
{
    {
        0,                                     /* handle                     */
        {                                      /* MI_RGN_ChnPort_t           */
            E_MI_MODULE_ID_VENC,               /* eModId                     */
            MI_VENC_DEV_ID_H264_H265_0,        /* s32DevId                   */
            0,                                 /* s32ChnId                   */
            0,                                 /* s32PortId                  */
        },
        {                                      /* MI_RGN_ChnPortParam_t      */
            1,                                 /* bShow                      */
            {0, 0},                            /* stPoint                    */
            {                                  /* MI_RGN_ChnPortParamUnion_u */
                .stOsdChnPort =
                {                              /* MI_RGN_OsdChnPortParam_t   */
                    1,                         /* u32Layer                   */
                    {                          /* MI_RGN_OsdAlphaAttr_t      */
                        E_MI_RGN_PIXEL_ALPHA,  /* eAlphaMode            */
                        {                      /* MI_RGN_AlphaModePara_u     */
                            .stArgb1555Alpha =
                            {                  /* MI_RGN_OsdArgb1555Alpha_t  */
                                0,             /* u8BgAlpha                  */
                                255            /* u8FgAlpha                  */
                            }
                        }
                    }
                }
            }
        }
    }
};
#define RGN_ATTACH_CFG_COUNT (sizeof(rgn_attach_cfg) / sizeof(rgn_attach_cfg[0]))



/*************************************************
    SCL Config
**************************************************/
static scl_dev_cfg_t scl_dev_cfg[] =
{
    /*                  DevId                               u32NeedUseHWOutPortMask */
    {MI_SCL_DEV_ISP_REALTIME0, {E_MI_SCL_HWSCL0 | E_MI_SCL_HWSCL1 | E_MI_SCL_HWSCL2} },
    {        MI_SCL_DEV_RDMA0, {                                    E_MI_SCL_HWSCL3} }
};
#define SCL_DEV_CFG_COUNT (sizeof(scl_dev_cfg) / sizeof(scl_dev_cfg[0]))

static scl_chn_cfg_t scl_chn_cfg[] =
{
    /*                  DevId  Chn  rsv  CropInfo                    eRot */
    {MI_SCL_DEV_ISP_REALTIME0,   0, {0}, {0,0,0,0}, {E_MI_SYS_ROTATE_NONE} }
};
#define SCL_CHN_CFG_COUNT (sizeof(scl_chn_cfg) / sizeof(scl_chn_cfg[0]))

static scl_outport_cfg_t scl_outport_cfg[] =
{
    /*                  DevId Chn Port  OutCropRect   OutputSize  Mirror Flip                             ePixelFormat */
    {MI_SCL_DEV_ISP_REALTIME0,  0,   0,   {{0,0,0,0}, {2560,1440},     0,   0, E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420} },
    {MI_SCL_DEV_ISP_REALTIME0,  0,   2,   {{0,0,0,0}, {1920,1080},     0,   0, E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420} }
};
#define SCL_OUTPORT_CFG_COUNT (sizeof(scl_outport_cfg) / sizeof(scl_outport_cfg[0]))




/*************************************************
    VENC Config
**************************************************/
static venc_dev_cfg_t venc_dev_cfg[] =
{
    /*                    DevId     MaxSize */
    {MI_VENC_DEV_ID_H264_H265_0, {2560,1440} },
    {     MI_VENC_DEV_ID_JPEG_0, {2560,1440} }
};
#define VENC_DEV_CFG_COUNT (sizeof(venc_dev_cfg) / sizeof(venc_dev_cfg[0]))

static venc_chn_cfg_t venc_chn_cfg[] =
{
    {
        MI_VENC_DEV_ID_H264_H265_0,  /* DevId */
        0,                           /* ChnId */
        {                                    /* MI_VENC_ChnAttr_t   */
            {                                /* MI_VENC_Attr_t      */
                E_MI_VENC_MODTYPE_H264E,     /* eType               */
                .stAttrH264e =
                {
                    2560,                    /* u32MaxPicWidth      */
                    1440,                    /* u32MaxPicHeight     */
                    0,                       /* u32BufSize 0:默认值 */
                    0,                       /* u32Profile 0:Baseline 1:MainProfile 2:HighProfile */
                    1,                       /* bByFrame 按帧取流   */
                    2560,                    /* u32PicWidth         */
                    1440,                    /* u32PicHeight        */
                    0,                       /* u32BFrameNum        */
                    0                        /* u32RefNum           */
                }
            },
            {                                /* MI_VENC_RcAttr_t    */
                E_MI_VENC_RC_MODE_H264AVBR,  /* eRcMode             */
                .stAttrH264Avbr =
                {
                    50,                      /* u32Gop              */
                    3,                       /* u32StatTime         */
                    25,                      /* u32SrcFrmRateNum    */
                    1,                       /* u32SrcFrmRateDen    */
                    4*1024*1024,             /* u32MaxBitRate       */
                    48,                      /* u32MaxQp            */
                    20                       /* u32MinQp            */
                }
            }
        },
        {                                    /* MI_VENC_InputSourceConfig_t */
            E_MI_VENC_INPUT_MODE_RING_UNIFIED_DMA  /* eInputSrcBufferMode */
        }
    },
    {
        MI_VENC_DEV_ID_JPEG_0,  /* DevId */
        0,                      /* ChnId */
        {                                    /* MI_VENC_ChnAttr_t   */
            {                                /* MI_VENC_Attr_t      */
                E_MI_VENC_MODTYPE_JPEGE,     /* eType               */
                .stAttrJpeg =
                {
                    2560,                    /* u32MaxPicWidth      */
                    1440,                    /* u32MaxPicHeight     */
                    0,                       /* u32BufSize 0:默认值 */
                    1,                       /* bByFrame 按帧取流   */
                    2560,                    /* u32PicWidth         */
                    1440,                    /* u32PicHeight        */
                    0,                       /* bSupportDCF         */
                    0                        /* u32RestartMakerPerRowCnt */
                }
            },
            {                                /* MI_VENC_RcAttr_t    */
                E_MI_VENC_RC_MODE_MJPEGFIXQP,/* eRcMode             */
                .stAttrMjpegFixQp =
                {
                    1,                       /* u32SrcFrmRateNum    */
                    1,                       /* u32SrcFrmRateDen    */
                    70                       /* u32Qfactor          */
                }
            }
        },
        {                                    /* MI_VENC_InputSourceConfig_t */
            E_MI_VENC_INPUT_MODE_NORMAL_FRMBASE  /* eInputSrcBufferMode */
        }
    }
};
#define VENC_CHN_CFG_COUNT (sizeof(venc_chn_cfg) / sizeof(venc_chn_cfg[0]))

#if 0
            {
                E_MI_VENC_RC_MODE_H264CBR,   /* eRcMode             */
                .stAttrH264Cbr =
                {
                    50,                      /* u32Gop I帧间隔      */
                    3,                       /* u32StatTime 码率统计时间 */
                    25,                      /* u32SrcFrmRateNum    */
                    1,                       /* u32SrcFrmRateDen    */
                    8*1024*1024,             /* u32BitRate 码率     */
                    0                        /* u32FluctuateLevel   */
                }
            }

            {
                E_MI_VENC_RC_MODE_H264AVBR,  /* eRcMode             */
                .stAttrH264Avbr =
                {
                    50,                      /* u32Gop              */
                    3,                       /* u32StatTime         */
                    25,                      /* u32SrcFrmRateNum    */
                    1,                       /* u32SrcFrmRateDen    */
                    4*1024*1024,             /* u32MaxBitRate       */
                    48,                      /* u32MaxQp            */
                    20                       /* u32MinQp            */
                }
            }
#endif




/*************************************************
    VDF Config
**************************************************/
static const vdf_cfg_t vdf_cfg[] =
{
    {
        0,   /* chn_id    */
        20,  /* threshold */
        {
            E_MI_VDF_WORK_MODE_MD,            /* enWorkMode     */
            .stMdAttr =
            {
                1,                            /* u8Enable        */
                4,                            /* u8MdBufCnt      */
                0,                            /* u8VDFIntvl      */
                0,                            /* u32RstBufSize   */
                {                             /* stSubResultSize */
                    0,                        /* u32RstStatusLen */
                    0,                        /* u32RstSadLen    */
                    0                         /* u32RstObjLen    */
                },
                {                             /* ccl_ctrl        */
                    8,                        /* u16InitAreaThr  */
                    2                         /* u16Step         */
                },
                {                             /* stMdStaticParamsIn */
                    1920,                     /* width           */
                    1080,                     /* height          */
                    1,                        /* color           */
                    1920,                     /* stride          */
                    MDMB_MODE_MB_8x8,         /* mb_size         */
                    MDSAD_OUT_CTRL_8BIT_SAD,  /* sad_out_ctrl    */
                    {                         /* roi_md          */
                        4,                    /* num             */
                        {
                            {0,0},{1920-1,0},{1920-1,1080-1},{0,1080-1}
                        }
                    },
                    MDALG_MODE_SAD            /* md_alg_mode     */
                },
                {                             /* stMdDynamicParamsIn */
                    90,                       /* sensitivity     */
                    128,                      /* learn_rate      */
                    40,                       /* md_thr          */
                    0,                        /* obj_num_max     */
                    0                         /* LSD_open        */
                }
            }
        }
    }
};
#define VDF_CFG_COUNT (sizeof(vdf_cfg) / sizeof(vdf_cfg[0]))




/*************************************************
    AI Config
**************************************************/
static const ai_dev_cfg_t ai_dev_cfg[] =
{
    {
        MI_AI_DEV_1,  /* dev_id */
        {                                  /* MI_AI_Attr_t   */
            E_MI_AUDIO_FORMAT_PCM_S16_LE,  /* enFormat       */
            E_MI_AUDIO_SOUND_MODE_MONO,    /* enSoundMode    */
            E_MI_AUDIO_SAMPLE_RATE_8000,   /* enSampleRate   */
            /* PeriodSize会影响音视频同步，这个值修改完要验证 !!! */
            512,                           /* u32PeriodSize  */
            1                              /* bInterleaved   */
        },
        1,                                 /* ifs_cnt 注意!  */
        {                                  /* MI_AI_If_e     */
            E_MI_AI_IF_ADC_AB
        },
        {18},                              /* ps8LeftIfGain  */
        { 0},                              /* ps8RightIfGain */
        {-10, -10}                         /* gain           */
    }
};
#define AI_DEV_CFG_COUNT (sizeof(ai_dev_cfg) / sizeof(ai_dev_cfg[0]))




/*************************************************
    Output Port Depth Config
**************************************************/
static const outport_depth_cfg_t outport_depth_cfg[] =
{
    /*           mod_id,                   dev_id chn port user_depth queue_depth */
    {E_MI_MODULE_ID_SCL, MI_SCL_DEV_ISP_REALTIME0,  0,   2,         1,          4  },
    { E_MI_MODULE_ID_AI,              MI_AI_DEV_1,  0,   0,         1,          4  }
};
#define OUTPORT_DEPTH_CFG_COUNT (sizeof(outport_depth_cfg) / sizeof(outport_depth_cfg[0]))




/*************************************************
    Bind Config
**************************************************/
static bind_cfg_t bind_cfg[] =
{
    {
        {
            E_MI_MODULE_ID_VIF,          /* Src ModuleId */
            0,                           /* DevId        */
            0,                           /* ChnId        */
            0                            /* PortId       */
        },
        {
            E_MI_MODULE_ID_ISP,          /* Dst ModuleId */
            MI_ISP_DEV0,                 /* DevId        */
            0,                           /* ChnId        */
            0                            /* PortId       */
        },
        25,                              /* SrcFrmrate   */
        25,                              /* DstFrmrate   */
        E_MI_SYS_BIND_TYPE_REALTIME,     /* eBindType    */
        0                                /* BindParam    */
    },
    {
        {
            E_MI_MODULE_ID_ISP,          /* Src ModuleId */
            MI_ISP_DEV0,                 /* DevId        */
            0,                           /* ChnId        */
            0                            /* PortId       */
        },
        {
            E_MI_MODULE_ID_SCL,          /* Dst ModuleId */
            MI_SCL_DEV_ISP_REALTIME0,    /* DevId        */
            0,                           /* ChnId        */
            0                            /* PortId       */
        },
        25,                              /* SrcFrmrate   */
        25,                              /* DstFrmrate   */
        E_MI_SYS_BIND_TYPE_REALTIME,     /* eBindType    */
        0                                /* BindParam    */
    },
    {
        {
            E_MI_MODULE_ID_SCL,          /* Src ModuleId */
            MI_SCL_DEV_ISP_REALTIME0,    /* DevId        */
            0,                           /* ChnId        */
            0                            /* PortId       */
        },
        {
            E_MI_MODULE_ID_VENC,         /* Dst ModuleId */
            MI_VENC_DEV_ID_H264_H265_0,  /* DevId        */
            0,                           /* ChnId        */
            0                            /* PortId       */
        },
        25,                              /* SrcFrmrate   */
        25,                              /* DstFrmrate   */
        E_MI_SYS_BIND_TYPE_HW_RING,      /* eBindType    */
        0                                /* BindParam    */
    },
    {
        {
            E_MI_MODULE_ID_SCL,          /* Src ModuleId */
            MI_SCL_DEV_ISP_REALTIME0,    /* DevId        */
            0,                           /* ChnId        */
            2                            /* PortId       */
        },
        {
            E_MI_MODULE_ID_VDF,          /* Dst ModuleId */
            0,                           /* DevId        */
            0,                           /* ChnId        */
            0                            /* PortId       */
        },
        25,                              /* SrcFrmrate   */
        5,                               /* DstFrmrate   */
        E_MI_SYS_BIND_TYPE_FRAME_BASE,   /* eBindType    */
        0                                /* BindParam    */
    }
};
#define BIND_CFG_COUNT (sizeof(bind_cfg) / sizeof(bind_cfg[0]))




/*************************************************
    Video Stream Config
**************************************************/
static const video_stream_cfg_t video_stream_cfg[] =
{
    /*           stream_id             format width height fps                   venc_devid chn */
    {{STREAM_ID_V_RGB_MAIN, VIDEO_FORMAT_H264, 2560,  1440, 25}, MI_VENC_DEV_ID_H264_H265_0,  0  }
};
#define VIDEO_STREAM_CFG_COUNT (sizeof(video_stream_cfg) / sizeof(video_stream_cfg[0]))




/*************************************************
    Audio Stream Config
**************************************************/
static const audio_stream_cfg_t audio_stream_cfg[] =
{
    /*       stream_id             format  chns  rate        dev_id  grp_id */
    {{STREAM_ID_A_MAIN, AUDIO_FORMAT_PCMA,    1, 8000}, MI_AI_DEV_1,      0  }
};
#define AUDIO_STREAM_CFG_COUNT (sizeof(audio_stream_cfg) / sizeof(audio_stream_cfg[0]))




/*************************************************
    Signal Frame Config
**************************************************/
static const signal_frame_cfg_t signal_frame_cfg[] =
{
    /*        signal_frame_id              mod_id                    dev_id  chn  port */
    {SIGNAL_FRAME_ID_RGB_MAIN, E_MI_MODULE_ID_SCL, MI_SCL_DEV_ISP_REALTIME0,   0,    2  }
};
#define SIGNAL_FRAME_CFG_COUNT (sizeof(signal_frame_cfg) / sizeof(signal_frame_cfg[0]))




/*************************************************
    Jpeg Encode Config
**************************************************/
static const jpeg_encode_cfg_t jpeg_encode_cfg =
{
    16, 2, 8 ,2
};




/*************************************************
    OSD Config
**************************************************/
static const osd_cfg_t osd_cfg[] =
{
    /*           stream_id       osd_id           format  width  height  handle */
    {{STREAM_ID_V_RGB_MAIN, OSD_ID_TIME, OSD_FORMAT_BIT2,   640,     64},     0  }
};
#define OSD_CFG_COUNT (sizeof(osd_cfg) / sizeof(osd_cfg[0]))




#endif
