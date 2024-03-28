#ifndef _HISMEDIA_CONFIG_H_
#define _HISMEDIA_CONFIG_H_


/*************************************************
    SYS Config
**************************************************/
static const sys_cfg_t sys_cfg =
{
    /* isp_osd_pool_size  ipu_osd_pool_size */
                       0,        128 * 1024
};


/*************************************************
    ISP Config
**************************************************/
static isp_cfg_t isp_cfg[] =
{
    {
        IMPVI_MAIN,                           /* vi_num          */
        {                                     /* sensor_info     */
            "sc4336",                         /* name            */
            TX_SENSOR_CONTROL_INTERFACE_I2C,  /* cbus_type       */
            .i2c =
            {
                "sc4336",                     /* type            */
                0x30,                         /* addr            */
                0                             /* i2c_adapter_id  */
            },
            GPIO_PA(18),                      /* rst_gpio        */
            GPIO_PA(19),                      /* pwdn_gpio       */
            -1,                               /* power_gpio      */
            0,                                /* sensor_id       */
            IMPISP_SENSOR_VI_MIPI_CSI0,       /* video_interface */
            IMPISP_SENSOR_MCLK0,              /* mclk            */
            0                                 /* default_boot    */
        },
        {25, 1},                              /* fps             */
        {                                     /* flip_attr       */
            IMPISP_FLIP_HV_MODE,              /* sensor_mode     */
            {                                 /* isp_mode        */
                IMPISP_FLIP_NORMAL_MODE,
                IMPISP_FLIP_NORMAL_MODE,
                IMPISP_FLIP_NORMAL_MODE
            }
        }
    }
};
#define ISP_CFG_COUNT (sizeof(isp_cfg) / sizeof(isp_cfg[0]))




/*************************************************
    FrameSource Config
**************************************************/
static fs_cfg_t fs_cfg[] =
{
    {
        0,                   /* chn_id        */
        {                    /* attr          */
            {                /* i2dattr       */
                0,           /* i2d_enable    */
                0,           /* flip_enable   */
                0,           /* mirr_enable   */
                0,           /* rotate_enable */
                0            /* rotate_angle  */
            },
            2560,            /* picWidth      */
            1440,            /* picHeight     */
            PIX_FMT_NV12,    /* pixFmt        */
            {                /* crop          */
                0,           /* enable        */
                0,           /* left          */
                0,           /* top           */
                0,           /* width         */
                0            /* height        */
            },
            {                /* scaler        */
                0,           /* enable        */
                0,           /* outwidth      */
                0            /* outheight     */
            },
            25,              /* outFrmRateNum */
            1,               /* outFrmRateDen */
            2,               /* nrVBs         */
            FS_PHY_CHANNEL,  /* type          */
            {                /* fcrop         */
                0,           /* enable        */
                0,           /* left          */
                0,           /* top           */
                0,           /* width         */
                0            /* height        */
            },
            0                /* mirr_enable   */
        }
    },
    {
        1,                   /* chn_id        */
        {                    /* attr          */
            {                /* i2dattr       */
                0,           /* i2d_enable    */
                0,           /* flip_enable   */
                0,           /* mirr_enable   */
                0,           /* rotate_enable */
                0            /* rotate_angle  */
            },
            1280,            /* picWidth      */
            720,             /* picHeight     */
            PIX_FMT_NV12,    /* pixFmt        */
            {                /* crop          */
                0,           /* enable        */
                0,           /* left          */
                0,           /* top           */
                0,           /* width         */
                0            /* height        */
            },
            {                /* scaler        */
                1,           /* enable        */
                1280,        /* outwidth      */
                720          /* outheight     */
            },
            25,              /* outFrmRateNum */
            1,               /* outFrmRateDen */
            2,               /* nrVBs         */
            FS_PHY_CHANNEL,  /* type          */
            {                /* fcrop         */
                0,           /* enable        */
                0,           /* left          */
                0,           /* top           */
                0,           /* width         */
                0            /* height        */
            },
            0                /* mirr_enable   */
        }
    },
    {
        2,                   /* chn_id        */
        {                    /* attr          */
            {                /* i2dattr       */
                0,           /* i2d_enable    */
                0,           /* flip_enable   */
                0,           /* mirr_enable   */
                0,           /* rotate_enable */
                0            /* rotate_angle  */
            },
            640,             /* picWidth      */
            360,             /* picHeight     */
            PIX_FMT_NV12,    /* pixFmt        */
            {                /* crop          */
                0,           /* enable        */
                0,           /* left          */
                0,           /* top           */
                0,           /* width         */
                0            /* height        */
            },
            {                /* scaler        */
                1,           /* enable        */
                640,         /* outwidth      */
                360          /* outheight     */
            },
            25,              /* outFrmRateNum */
            1,               /* outFrmRateDen */
            3,               /* nrVBs         */
            FS_PHY_CHANNEL,  /* type          */
            {                /* fcrop         */
                0,           /* enable        */
                0,           /* left          */
                0,           /* top           */
                0,           /* width         */
                0            /* height        */
            },
            0                /* mirr_enable   */
        }
    }
};
#define FS_CFG_COUNT (sizeof(fs_cfg) / sizeof(fs_cfg[0]))




/*************************************************
    IVS Config
**************************************************/
static const ivs_grp_cfg_t ivs_grp_cfg[] =
{
    {0}
};
#define IVS_GRP_CFG_COUNT (sizeof(ivs_grp_cfg) / sizeof(ivs_grp_cfg[0]))

static ivs_chn_cfg_t ivs_chn_cfg[] =
{
    {
        0,                 /* grp_id     */
        0,                 /* chn_id     */
        0,                 /* is_running */
        NULL,              /* handler    */
        IVS_MOVE_DETECT,   /* func       */
        .move_param =
        {                  /* move_param         */
            {              /* perms              */
                {          /* perms[0]           */
                    NULL,  /* p                  */
                    4,     /* pcnt               */
                    0,     /* fun                */
                    0      /* alarm_last_time    */
                }
            },
            1,             /* permcnt            */
            3,             /* sense              */
            0,             /* min_h              */
            0,             /* min_w              */
            NULL,          /* rois               */
            0,             /* cntRoi             */
            0,             /* isSkipFrame        */
            0,             /* light              */
            0.7,           /* level              */
            110,           /* timeon             */
            0,             /* timeoff            */
            0,             /* isLightRemove      */
            10,            /* det_w              */
            10,            /* det_h              */
            {              /* frameInfo          */
                NULL,      /* data               */
                640,       /* width              */
                360,       /* height             */
                0,         /* pixfmt             */
                0          /* timeStamp          */
            }
        }
    },
    {
        0,                 /* grp_id     */
        1,                 /* chn_id     */
        0,                 /* is_running */
        NULL,              /* handler    */
        IVS_PERSONVEHICLEPET_DETECT, /* func     */
        .personvehiclepet_param =
        {                  /* personvehiclepet_param */
            0,             /* ptime              */
            13,            /* max_personvehiclepet_box */
            "/usr/model/personvehiclepet_det.bin", /* model_path */
            5,             /* sense              */
            0,             /* detdist            */
            0,             /* enable_move        */
            0,             /* open_move_filter   */
            3,             /* move_sense         */
            10,            /* move_min_h         */
            10,            /* move_min_w         */
            0,             /* enable_perm        */
            {              /* perms              */
                {0},
                {0},
                {0},
                {0}
            },
            0,             /* permcnt            */
            0,             /* mod                */
            2,             /* skip_num           */
            0,             /* delay              */
            {              /* frameInfo          */
                NULL,      /* data               */
                640,       /* width              */
                360,       /* height             */
                0,         /* pixfmt             */
                0          /* timeStamp          */
            },
            0,             /* rot90              */
            1,             /* switch_track       */
            0,             /* switch_stop_det    */
            0              /* fast_update_params */
        }
    }
};
#define IVS_CHN_CFG_COUNT (sizeof(ivs_chn_cfg) / sizeof(ivs_chn_cfg[0]))




/*************************************************
    OSD Config
**************************************************/
static isp_osd_region_cfg_t isp_osd_region_cfg[0] =
{
    /*     osd_id  width  height  chn  handle */
    //{ OSD_ID_TIME,   320,     32,   0,     -1  }
};
#define ISP_OSD_REGION_CFG_COUNT (sizeof(isp_osd_region_cfg) / sizeof(isp_osd_region_cfg[0]))

static osd_grp_cfg_t osd_grp_cfg[] =
{
    {
        STREAM_ID_V_RGB_MAIN,  /* stream_id         */
        0,                     /* grp               */
        10,                    /* rect_rgn_cnt      */
        30,                    /* slash_rgn_cnt     */
        1,                     /* rect_linewidth    */
        2,                     /* slash_linewidth   */
        {0},                   /* rect_rgn_handles  */
        {0},                   /* slash_rgn_handles */
        {                      /* def_grp_attr      */
            0,                 /* show              */
            {0,0},             /* offPos            */
            1,                 /* scalex            */
            1,                 /* scaley            */
            0,                 /* gAlphaEn          */
            0xff,              /* fgAlhpa           */
            0x00,              /* bgAlhpa           */
            1                  /* layer             */
        }
    },
    {
        STREAM_ID_V_RGB_SUB1,  /* stream_id         */
        1,                     /* grp               */
        10,                    /* rect_rgn_cnt      */
        30,                    /* slash_rgn_cnt     */
        0,                     /* rect_linewidth    */
        1,                     /* slash_linewidth   */
        {0},                   /* rect_rgn_handles  */
        {0},                   /* slash_rgn_handles */
        {                      /* def_grp_attr      */
            0,                 /* show              */
            {0,0},             /* offPos            */
            1,                 /* scalex            */
            1,                 /* scaley            */
            0,                 /* gAlphaEn          */
            0xff,              /* fgAlhpa           */
            0x00,              /* bgAlhpa           */
            1                  /* layer             */
        }
    },
    {
        STREAM_ID_V_RGB_SUB2,  /* stream_id         */
        2,                     /* grp               */
        10,                    /* rect_rgn_cnt      */
        30,                    /* slash_rgn_cnt     */
        0,                     /* rect_linewidth    */
        1,                     /* slash_linewidth   */
        {0},                   /* rect_rgn_handles  */
        {0},                   /* slash_rgn_handles */
        {                      /* def_grp_attr      */
            0,                 /* show              */
            {0,0},             /* offPos            */
            1,                 /* scalex            */
            1,                 /* scaley            */
            0,                 /* gAlphaEn          */
            0xff,              /* fgAlhpa           */
            0x00,              /* bgAlhpa           */
            1                  /* layer             */
        }
    }
};
#define OSD_GRP_CFG_COUNT (sizeof(osd_grp_cfg) / sizeof(osd_grp_cfg[0]))

static osd_region_cfg_t osd_region_cfg[] =
{
    {
        STREAM_ID_V_RGB_MAIN,       /* stream_id         */
        0,                          /* grp               */
        OSD_ID_TIME,                /* osd_id            */
        320,                        /* width             */
        32,                         /* height            */
        -1,                         /* handle            */
        {                           /* rgn_attr          */
            OSD_REG_BITMAP,         /* type              */
            {                       /* rect              */
                {0,0},              /* p0                */
                {320-1,32-1}        /* p1                */
            },
            {{0}},                  /* line              */
            PIX_FMT_MONOWHITE,      /* fmt               */
            {                       /* data              */
                .bitmapData = NULL  /* pData             */
            },
            {                       /* osdispdraw        */
                {0},
                {0},
                {0, 0, 0, 0, 0, 0, 0, 0, {{0},{0}}}
            },
            {                       /* fontData          */
                0,                  /* invertColorSwitch */
                0,                  /* luminance         */
                0,                  /* length            */
                {0,0},              /* data              */
                0,                  /* istimestamp       */
                {0}                 /* colType           */
            },
            {0}                     /* mosaicAttr        */
        },
        {                           /* grp_attr          */
            0,                      /* show              */
            {0,0},                  /* offPos            */
            1,                      /* scalex            */
            1,                      /* scaley            */
            0,                      /* gAlphaEn          */
            0xff,                   /* fgAlhpa           */
            0x00,                   /* bgAlhpa           */
            1                       /* layer             */
        }
    },
    {
        STREAM_ID_V_RGB_SUB1,       /* stream_id         */
        1,                          /* grp               */
        OSD_ID_TIME,                /* osd_id            */
        320,                        /* width             */
        32,                         /* height            */
        -1,                         /* handle            */
        {                           /* rgn_attr          */
            OSD_REG_BITMAP,         /* type              */
            {                       /* rect              */
                {0,0},              /* p0                */
                {320-1,32-1}        /* p1                */
            },
            {{0}},                  /* line              */
            PIX_FMT_MONOWHITE,      /* fmt               */
            {                       /* data              */
                .bitmapData = NULL  /* pData             */
            },
            {                       /* osdispdraw        */
                {0},
                {0},
                {0, 0, 0, 0, 0, 0, 0, 0, {{0},{0}}}
            },
            {                       /* fontData          */
                0,                  /* invertColorSwitch */
                0,                  /* luminance         */
                0,                  /* length            */
                {0,0},              /* data              */
                0,                  /* istimestamp       */
                {0}                 /* colType           */
            },
            {0}                     /* mosaicAttr        */
        },
        {                           /* grp_attr          */
            0,                      /* show              */
            {0,0},                  /* offPos            */
            1,                      /* scalex            */
            1,                      /* scaley            */
            0,                      /* gAlphaEn          */
            0xff,                   /* fgAlhpa           */
            0x00,                   /* bgAlhpa           */
            1                       /* layer             */
        }
    },
    {
        STREAM_ID_V_RGB_SUB2,       /* stream_id         */
        2,                          /* grp               */
        OSD_ID_TIME,                /* osd_id            */
        320,                        /* width             */
        32,                         /* height            */
        -1,                         /* handle            */
        {                           /* rgn_attr          */
            OSD_REG_BITMAP,         /* type              */
            {                       /* rect              */
                {0,0},              /* p0                */
                {320-1,32-1}        /* p1                */
            },
            {{0}},                  /* line              */
            PIX_FMT_MONOWHITE,      /* fmt               */
            {                       /* data              */
                .bitmapData = NULL  /* pData             */
            },
            {                       /* osdispdraw        */
                {0},
                {0},
                {0, 0, 0, 0, 0, 0, 0, 0, {{0},{0}}}
            },
            {                       /* fontData          */
                0,                  /* invertColorSwitch */
                0,                  /* luminance         */
                0,                  /* length            */
                {0,0},              /* data              */
                0,                  /* istimestamp       */
                {0}                 /* colType           */
            },
            {0}                     /* mosaicAttr        */
        },
        {                           /* grp_attr          */
            0,                      /* show              */
            {0,0},                  /* offPos            */
            1,                      /* scalex            */
            1,                      /* scaley            */
            0,                      /* gAlphaEn          */
            0xff,                   /* fgAlhpa           */
            0x00,                   /* bgAlhpa           */
            1                       /* layer             */
        }
    }
};
#define OSD_REGION_CFG_COUNT (sizeof(osd_region_cfg) / sizeof(osd_region_cfg[0]))




/*************************************************
    Encoder Config
**************************************************/
static const enc_grp_cfg_t enc_grp_cfg[] =
{
    {0}, {1}, {2}
};
#define ENC_GRP_CFG_COUNT (sizeof(enc_grp_cfg) / sizeof(enc_grp_cfg[0]))

static const enc_chn_cfg_t enc_chn_cfg[] =
{
    /* grp chn                    profile              rc_mode width height fps gop bitrate */
    {    0,  0, IMP_ENC_PROFILE_HEVC_MAIN, IMP_ENC_RC_MODE_VBR, 2560,  1440, 25, 50,   2000  },
    {    1,  1, IMP_ENC_PROFILE_HEVC_MAIN, IMP_ENC_RC_MODE_VBR, 1280,   720, 25, 50,   1200  },
    {    2,  2, IMP_ENC_PROFILE_HEVC_MAIN, IMP_ENC_RC_MODE_VBR,  640,   360, 25, 50,   1000  }
};
#define ENC_CHN_CFG_COUNT (sizeof(enc_chn_cfg) / sizeof(enc_chn_cfg[0]))




/*************************************************
    AI Config
**************************************************/
static ai_dev_cfg_t ai_dev_cfg[] =
{
    /* numPerFrm会影响音视频同步，这个值修改完要验证 !!! */
    /* dev               samplerate            bitwidth              soundmode frmNum PerFrm chnCnt */
    {    1, {AUDIO_SAMPLE_RATE_8000, AUDIO_BIT_WIDTH_16, AUDIO_SOUND_MODE_MONO,    25,   320,     1} }
};
#define AI_DEV_CFG_COUNT (sizeof(ai_dev_cfg) / sizeof(ai_dev_cfg[0]))

static ai_chn_cfg_t ai_chn_cfg[] =
{
    /* dev_id  chn_id  vol  gain   usrFrmDepth  aecChn  rev */
    {       1,      0,  60,   23, {         25,      0,   0} }
};
#define AI_CHN_CFG_COUNT (sizeof(ai_chn_cfg) / sizeof(ai_chn_cfg[0]))




/*************************************************
    AO Config
**************************************************/
static ao_dev_cfg_t ao_dev_cfg[] =
{
    /* dev               samplerate            bitwidth              soundmode frmNum PerFrm chnCnt */
    {    0, {AUDIO_SAMPLE_RATE_8000, AUDIO_BIT_WIDTH_16, AUDIO_SOUND_MODE_MONO,    25,   640,     1} }
};
#define AO_DEV_CFG_COUNT (sizeof(ao_dev_cfg) / sizeof(ao_dev_cfg[0]))

static ao_chn_cfg_t ao_chn_cfg[] =
{
    /* dev_id  chn_id  vol  gain */
    {       0,      0,  60,   23  }
};
#define AO_CHN_CFG_COUNT (sizeof(ao_chn_cfg) / sizeof(ao_chn_cfg[0]))




/*************************************************
    Bind Config
**************************************************/
static bind_cfg_t bind_cfg[] =
{
    /* deviceID  groupID  outputID      deviceID  groupID  outputID */
    {{ DEV_ID_FS,      0,        0}, {DEV_ID_OSD,       0,        0} },
    {{DEV_ID_OSD,      0,        0}, {DEV_ID_ENC,       0,        0} },
    {{ DEV_ID_FS,      1,        0}, {DEV_ID_OSD,       1,        0} },
    {{DEV_ID_OSD,      1,        0}, {DEV_ID_ENC,       1,        0} },
    {{ DEV_ID_FS,      2,        0}, {DEV_ID_IVS,       0,        0} },
    {{DEV_ID_IVS,      0,        0}, {DEV_ID_OSD,       2,        0} },
    {{DEV_ID_OSD,      2,        0}, {DEV_ID_ENC,       2,        0} }
};
#define BIND_CFG_COUNT (sizeof(bind_cfg) / sizeof(bind_cfg[0]))




/*************************************************
    Video Stream Config
**************************************************/
static const video_stream_cfg_t video_stream_cfg[] =
{
    /*           stream_id             format  width  height  fps   enc_chn */
    {{STREAM_ID_V_RGB_MAIN, VIDEO_FORMAT_H265,  2560,   1440,  25},       0  },
    {{STREAM_ID_V_RGB_SUB1, VIDEO_FORMAT_H265,  1280,    720,  25},       1  },
    {{STREAM_ID_V_RGB_SUB2, VIDEO_FORMAT_H265,   640,    360,  25},       2  }
};
#define VIDEO_STREAM_CFG_COUNT (sizeof(video_stream_cfg) / sizeof(video_stream_cfg[0]))




/*************************************************
    Audio Stream Config
**************************************************/
static const audio_stream_cfg_t audio_stream_cfg[] =
{
    /*       stream_id            format  chns  rate   dev_id  chn_id */
    {{STREAM_ID_A_MAIN, AUDIO_FORMAT_PCM,    1, 8000},      1,      0  }
};
#define AUDIO_STREAM_CFG_COUNT (sizeof(audio_stream_cfg) / sizeof(audio_stream_cfg[0]))




/*************************************************
    Audio Play Config
**************************************************/
static audio_play_cfg_t audio_play_cfg =
{
    /*        format  chns  rate  dev_id  chn_id  max_size */
    AUDIO_FORMAT_PCM,    1, 8000,      0,      0,     1280
};




/*************************************************
    Single Frame Config
**************************************************/
static const single_frame_cfg_t single_frame_cfg[] =
{
    /*        single_frame_id  width  height  chn */
    {SINGLE_FRAME_ID_RGB_MAIN,  2560,   1440,   0  }
};
#define SINGLE_FRAME_CFG_COUNT (sizeof(single_frame_cfg) / sizeof(single_frame_cfg[0]))




#endif
