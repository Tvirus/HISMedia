#ifndef _HISMEDIA_CONFIG_H_
#define _HISMEDIA_CONFIG_H_


/*************************************************
    VB Config
**************************************************/
static const vb_cfg_t vb_cfg =
{
    {
        1,                              /* max_pool_cnt */
        {                               /* common_pool  */
            {
                2688 * 1520 * 2,        /* blk_size     */
                46,                     /* blk_cnt      */
                OT_VB_REMAP_MODE_NONE,  /* remap_mode   */
                ""                      /* mmz_name     */
            }
        }
    }
};

static user_vb_pool_cfg_t user_vb_pool_cfg[] =
{
    {
        -1,                         /* id         */
        {                           /* cfg        */
            2688 * 1520 * 2,        /* blk_size   */
            7,                      /* blk_cnt    */
            OT_VB_REMAP_MODE_NONE,  /* remap_mode */
            ""                      /* mmz_name   */
        }
    }
};
#define USER_VB_CFG_COUNT (sizeof(user_vb_pool_cfg) / sizeof(user_vb_pool_cfg[0]))




/*************************************************
    VI Config
**************************************************/
static const lane_divide_mode_t lane_divide_mode = LANE_DIVIDE_MODE_0;
static const vi_mipi_cfg_t vi_mipi_cfg[] =
{
    {
        0,  /* dev */
        {                                      /* combo_dev_attr      */
            0,                                 /* devno               */
            INPUT_MODE_MIPI,                   /* input_mode          */
            MIPI_DATA_RATE_X1,                 /* data_rate           */
            {0, 0, 2688, 1520},                /* img_rect            */
            .mipi_attr =
            {
                DATA_TYPE_RAW_12BIT,           /* input_data_type     */
                OT_MIPI_WDR_MODE_NONE,         /* wdr_mode            */
                {0, 1, 2, 3, -1, -1, -1, -1},  /* lane_id             */
                .data_type = {0}
            }
        },
        {                                      /* ext_data_attr       */
            0,                                 /* devno               */
            3,                                 /* num                 */
            {12, 12, 12},                      /* ext_data_bit_width  */
            {0x37, 0x2c, 0x2c}                 /* ext_data_type       */
        }
    }
};
#define VI_MIPI_CFG_COUNT (sizeof(vi_mipi_cfg) / sizeof(vi_mipi_cfg[0]))

static const vi_dev_cfg_t vi_dev_cfg[] =
{
    {
        0,                                   /* dev              */
        {                                    /* attr             */
            OT_VI_INTF_MODE_MIPI,            /* intf_mode        */
            OT_VI_WORK_MODE_MULTIPLEX_1,     /* work_mode        */
            {0xfff00000, 0x00000000},        /* component_mask   */
            OT_VI_SCAN_PROGRESSIVE,          /* scan_mode        */
            {-1, -1, -1, -1},                /* ad_chn_id        */
            OT_VI_DATA_SEQ_YVYU,             /* (input)data_seq  */
            {                                /* sync_cfg         */
                OT_VI_VSYNC_FIELD,           /* vsync            */
                OT_VI_VSYNC_NEG_HIGH,        /* vsync_neg        */
                OT_VI_HSYNC_VALID_SIG,       /* hsync            */
                OT_VI_HSYNC_NEG_HIGH,        /* hsync_neg        */
                OT_VI_VSYNC_VALID_SIG,       /* vsync_valid      */
                OT_VI_VSYNC_VALID_NEG_HIGH,  /* vsync_valid_neg  */
                {                            /* timing_blank     */
                    /* hfb act hbb vfb vact vbb vbfb vbact vbbb  */
                         0,  0,  0,  0,   0,  0,   0,    0,   0
                }
            },
            OT_VI_DATA_TYPE_RAW,             /* (input)data_type */
            TD_FALSE,                        /* data_reverse     */
            {2688, 1520},                    /* in_size          */
            OT_DATA_RATE_X1                  /* data_rate        */
        },
        {                                    /* bind_pipe        */
            1,                               /* pipe_num         */
            {0}                              /* pipe_id          */
        }
    }
};
#define VI_DEV_CFG_COUNT (sizeof(vi_dev_cfg) / sizeof(vi_dev_cfg[0]))

static const vi_grp_cfg_t vi_grp_cfg[] =
{
    {
        0,                     /* fusion_grp      */
        {                      /* fusion_grp_attr */
            OT_WDR_MODE_NONE,  /* wdr_mode        */
            1520,              /* cache_line      */
            {0},               /* pipe_id         */
            0                  /* pipe_reverse    */
        }
    }
};
#define VI_GRP_CFG_COUNT (sizeof(vi_grp_cfg) / sizeof(vi_grp_cfg[0]))


static const ot_vi_vpss_mode vi_vpss_mode_cfg =
{
    {
        OT_VI_OFFLINE_VPSS_OFFLINE,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0
    }
};

static const vi_pipe_cfg_t vi_pipe_cfg[] =
{
    {
        0,                                    /* vi_pipe          */
        40,                                   /* bnr_bnf_num      */
        {                                     /* pipe_attr        */
            OT_VI_PIPE_BYPASS_NONE,           /* pipe_bypass_mode */
            0,                                /* isp_bypass       */
            {2688, 1520},                     /* (input)size      */
            OT_PIXEL_FORMAT_RGB_BAYER_12BPP,  /* pixel_format     */
            OT_COMPRESS_MODE_NONE,            /* compress_mode    */
            {                                 /* frame_rate_ctrl  */
                -1,                           /* src_frame_rate   */
                -1                            /* dst_frame_rate   */
            }
        },
        0,                                    /* vc_number        */
        {                                     /* attr_3dnr        */
            /* 只有主pipe才需要设置enable为1 !!! */
            1,                                /* enable           */
            OT_NR_TYPE_VIDEO_NORM,            /* nr_type          */
            OT_COMPRESS_MODE_FRAME,           /* compress_mode    */
            OT_NR_MOTION_MODE_NORM            /* nr_motion_mode   */
        },
        OT_VB_SRC_USER,                       /* vb_src           */
        0,                                    /* user_vb_pool_idx */
    }
};
#define VI_PIPE_CFG_COUNT (sizeof(vi_pipe_cfg) / sizeof(vi_pipe_cfg[0]))

/* 只有主pipe才需要设置chn !!! */
static const vi_chn_cfg_t vi_chn_cfg[] =
{
    {
        0,                                       /* vi_pipe         */
        0,                                       /* vi_chn          */
        {                                        /* attr            */
            {2688, 1520},                        /* (output)size    */
            OT_PIXEL_FORMAT_YVU_SEMIPLANAR_420,  /* pixel_format    */
            OT_DYNAMIC_RANGE_SDR8,               /* dynamic_range   */
            OT_VIDEO_FORMAT_LINEAR,              /* video_format    */
            OT_COMPRESS_MODE_SEG,                /* compress_mode   */
            0,                                   /* mirror_en       */
            0,                                   /* flip_en         */
            0,                                   /* depth           */
            {                                    /* frame_rate_ctrl */
                -1,                              /* src_frame_rate  */
                -1                               /* dst_frame_rate  */
            }
        },
        OT_FMU_MODE_OFF                          /* fmu_mode        */
    }
};
#define VI_CHN_CFG_COUNT (sizeof(vi_chn_cfg) / sizeof(vi_chn_cfg[0]))

/* 只有主pipe才需要设置isp !!! */
static vi_isp_cfg_t vi_isp_cfg[] =
{
    {
        0,                           /* vi_pipe          */
        &g_sns_os04a10_obj,          /* sns_obj          */
        {                            /* ae_lib           */
            0,                       /* (算法库实例)id   */
            "ot_ae_lib"              /* lib_name         */
        },
        {                            /* awb_lib          */
            0,                       /* (算法库实例)id   */
            "ot_awb_lib"             /* lib_name         */
        },
        {                            /* sns_bus_info     */
            .i2c_dev = 4             /* i2c_dev          */
        },
        {                            /* isp_pub_attr     */
            {0, 0, 2688, 1520},      /* wnd_rect裁剪窗口 */
            {2688, 1520},            /* sns_size         */
            30,                      /* frame_rate       */
            OT_ISP_BAYER_RGGB,       /* bayer_format     */
            OT_WDR_MODE_NONE,        /* wdr_mode         */
            0,                       /* sns_mode         */
            0,                       /* sns_flip_en      */
            0,                       /* sns_mirror_en    */
            {                        /* mipi_crop_attr   */
                0,                   /* mipi_crop_en     */
                {0, 0, 0, 0}         /* mipi_crop_offset */
            }
        }
    }
};
#define VI_ISP_CFG_COUNT (sizeof(vi_isp_cfg) / sizeof(vi_isp_cfg[0]))


static vi_aibnr_cfg_t vi_aibnr_cfg[] =
{
    {
        0,                                /* vi_pipe      */
        "/usr/model/aibnr_model_4M.bin",  /* model_file   */
        -1,                               /* model_id     */
        {                                 /* aibnr_model  */
            {                             /* model        */
                {0},                      /* mem_info     */
                {2688, 1520}              /* image_size   */
            },
            0,                            /* is_wdr_mode  */
            OT_AIBNR_REF_MODE_NORM        /* ref_mode     */
        },
        {                                 /* aibnr_cfg    */
            OT_AIBNR_REF_MODE_NORM        /* ref_mode     */
        },
        {                                 /* aibnr_attr   */
            1,                            /* enable       */
            0,                            /* bnr_bypass   */
            0,                            /* blend        */
            0,                            /* input_depth  */
            0,                            /* output_depth */
            OT_OP_MODE_MANUAL,            /* op_type      */
            {                             /* manual_attr  */
                31,                       /* sfs          */
                31                        /* tfs          */
            },
            {                             /* auto_attr    */
                {0}, {0}
            }
        }
    }
};
#define VI_AIBNR_CFG_COUNT (sizeof(vi_aibnr_cfg) / sizeof(vi_aibnr_cfg[0]))




/*************************************************
    VPSS Config
**************************************************/
static const vpss_grp_cfg_t vpss_grp_cfg[] =
{
    {
        0,                                       /* grp            */
        {                                        /* attr           */
            0,                                   /* ie_en          */
            0,                                   /* dci_en         */
            0,                                   /* buf_share_en   */
            0,                                   /* mcf_en         */
            2688,                                /* max_width      */
            1520,                                /* max_height     */
            0,                                   /* max_dei_width  */
            0,                                   /* max_dei_height */
            OT_DYNAMIC_RANGE_SDR8,               /* dynamic_range  */
            OT_PIXEL_FORMAT_YVU_SEMIPLANAR_420,  /* pixel_format   */
            OT_VPSS_DEI_MODE_OFF,                /* dei_mode       */
            0,                                   /* buf_share_chn  */
            {                                    /* frame_rate     */
                -1,                              /* src_frame_rate */
                -1                               /* dst_frame_rate */
            }
        }
    }
};
#define VPSS_GRP_CFG_COUNT (sizeof(vpss_grp_cfg) / sizeof(vpss_grp_cfg[0]))

static const vpss_chn_cfg_t vpss_chn_cfg[] =
{
    {
        0,                                       /* grp               */
        0,                                       /* chn               */
        {                                        /* attr              */
            0,                                   /* mirror_en         */
            0,                                   /* flip_en           */
            0,                                   /* border_en         */
            2688,                                /* (output)width     */
            1520,                                /* (output)height    */
            0,                                   /* (user)depth       */
            OT_VPSS_CHN_MODE_USER,               /* chn_mode          */
            OT_VIDEO_FORMAT_LINEAR,              /* video_format      */
            OT_DYNAMIC_RANGE_SDR8,               /* dynamic_range     */
            OT_PIXEL_FORMAT_YVU_SEMIPLANAR_420,  /* (out)pixel_format */
            OT_COMPRESS_MODE_SEG_COMPACT,        /* compress_mode     */
            {                                    /* frame_rate        */
                -1,                              /* src_frame_rate    */
                -1                               /* dst_frame_rate    */
            },
            {                                    /* border_attr       */
                0,                               /* top_width         */
                0,                               /* bottom_width      */
                0,                               /* left_width        */
                0,                               /* right_width       */
                0                                /* color             */
            },
            {                                    /* aspect_ratio      */
                OT_ASPECT_RATIO_NONE,            /* mode              */
                0,                               /* bg_color          */
                {0, 0, 0, 0}                     /* video_rect        */
            }
        }
    },
    {
        0,                                       /* grp               */
        1,                                       /* chn               */
        {                                        /* attr              */
            0,                                   /* mirror_en         */
            0,                                   /* flip_en           */
            0,                                   /* border_en         */
            1920,                                /* (output)width     */
            1080,                                /* (output)height    */
            0,                                   /* (user)depth       */
            OT_VPSS_CHN_MODE_USER,               /* chn_mode          */
            OT_VIDEO_FORMAT_LINEAR,              /* video_format      */
            OT_DYNAMIC_RANGE_SDR8,               /* dynamic_range     */
            OT_PIXEL_FORMAT_YVU_SEMIPLANAR_420,  /* (out)pixel_format */
            OT_COMPRESS_MODE_SEG_COMPACT,        /* compress_mode     */
            {                                    /* frame_rate        */
                -1,                              /* src_frame_rate    */
                -1                               /* dst_frame_rate    */
            },
            {                                    /* border_attr       */
                0,                               /* top_width         */
                0,                               /* bottom_width      */
                0,                               /* left_width        */
                0,                               /* right_width       */
                0                                /* color             */
            },
            {                                    /* aspect_ratio      */
                OT_ASPECT_RATIO_NONE,            /* mode              */
                0,                               /* bg_color          */
                {0, 0, 0, 0}                     /* video_rect        */
            }
        }
    },
    {
        0,                                       /* grp               */
        2,                                       /* chn               */
        {                                        /* attr              */
            0,                                   /* mirror_en         */
            0,                                   /* flip_en           */
            0,                                   /* border_en         */
            640,                                 /* (output)width     */
            360,                                 /* (output)height    */
            0,                                   /* (user)depth       */
            OT_VPSS_CHN_MODE_USER,               /* chn_mode          */
            OT_VIDEO_FORMAT_LINEAR,              /* video_format      */
            OT_DYNAMIC_RANGE_SDR8,               /* dynamic_range     */
            OT_PIXEL_FORMAT_YVU_SEMIPLANAR_420,  /* (out)pixel_format */
            OT_COMPRESS_MODE_NONE,               /* compress_mode     */
            {                                    /* frame_rate        */
                -1,                              /* src_frame_rate    */
                -1                               /* dst_frame_rate    */
            },
            {                                    /* border_attr       */
                0,                               /* top_width         */
                0,                               /* bottom_width      */
                0,                               /* left_width        */
                0,                               /* right_width       */
                0                                /* color             */
            },
            {                                    /* aspect_ratio      */
                OT_ASPECT_RATIO_NONE,            /* mode              */
                0,                               /* bg_color          */
                {0, 0, 0, 0}                     /* video_rect        */
            }
        }
    }
};
#define VPSS_CHN_CFG_COUNT (sizeof(vpss_chn_cfg) / sizeof(vpss_chn_cfg[0]))




/*************************************************
    VENC Config
**************************************************/
static const venc_cfg_t venc_cfg[] =
{
    {
        0,                                  /* chn                  */
        {                                   /* attr                 */
            {                               /* venc_attr            */
                OT_PT_H265,                 /* type                 */
                2688,                       /* max_pic_width        */
                1520,                       /* max_pic_height       */
                2688 * 1520,                /* buf_size             */
                0,                          /* profile              */
                1,                          /* is_by_frame          */
                2688,                       /* pic_width            */
                1520,                       /* pic_height           */
                .h265_attr =
                {
                    0,                      /* rcn_ref_share_buf_en */
                    100                     /* frame_buf_ratio      */
                }
            },
            {                               /* rc_attr              */
                OT_VENC_RC_MODE_H265_AVBR,  /* rc_mode              */
                .h265_avbr =
                {
                    30,                     /* gop                  */
                    3,                      /* stats_time           */
                    30,                     /* src_frame_rate       */
                    30,                     /* dst_frame_rate       */
                    6144                    /* max_bit_rate         */
                }
            },
            {                               /* gop_attr             */
                OT_VENC_GOP_MODE_SMART_P,   /* gop_mode             */
                .smart_p =
                {
                    90,                     /* bg_interval          */
                    4,                      /* bg_qp_delta          */
                    2                       /* vi_qp_delta          */
                }
            }
        },
        {                                   /* start_param          */
            -1,                             /* recv_pic_num         */
        }
    },
    {
        1,                                  /* chn                  */
        {                                   /* attr                 */
            {                               /* venc_attr            */
                OT_PT_H265,                 /* type                 */
                1920,                       /* max_pic_width        */
                1080,                       /* max_pic_height       */
                1920 * 1080,                /* buf_size             */
                0,                          /* profile              */
                1,                          /* is_by_frame          */
                1920,                       /* pic_width            */
                1080,                       /* pic_height           */
                .h265_attr =
                {
                    0,                      /* rcn_ref_share_buf_en */
                    100                     /* frame_buf_ratio      */
                }
            },
            {                               /* rc_attr              */
                OT_VENC_RC_MODE_H265_AVBR,  /* rc_mode              */
                .h265_avbr =
                {
                    30,                     /* gop                  */
                    3,                      /* stats_time           */
                    30,                     /* src_frame_rate       */
                    30,                     /* dst_frame_rate       */
                    4096                    /* max_bit_rate         */
                }
            },
            {                               /* gop_attr             */
                OT_VENC_GOP_MODE_SMART_P,   /* gop_mode             */
                .smart_p =
                {
                    90,                     /* bg_interval          */
                    4,                      /* bg_qp_delta          */
                    2                       /* vi_qp_delta          */
                }
            }
        },
        {                                   /* start_param          */
            -1,                             /* recv_pic_num         */
        }
    },
    {
        2,                                  /* chn                  */
        {                                   /* attr                 */
            {                               /* venc_attr            */
                OT_PT_H265,                 /* type                 */
                640,                        /* max_pic_width        */
                360,                        /* max_pic_height       */
                640 * 360,                  /* buf_size             */
                0,                          /* profile              */
                1,                          /* is_by_frame          */
                640,                        /* pic_width            */
                360,                        /* pic_height           */
                .h265_attr =
                {
                    0,                      /* rcn_ref_share_buf_en */
                    100                     /* frame_buf_ratio      */
                }
            },
            {                               /* rc_attr              */
                OT_VENC_RC_MODE_H265_AVBR,  /* rc_mode              */
                .h265_avbr =
                {
                    30,                     /* gop                  */
                    3,                      /* stats_time           */
                    30,                     /* src_frame_rate       */
                    30,                     /* dst_frame_rate       */
                    2048                    /* max_bit_rate         */
                }
            },
            {                               /* gop_attr             */
                OT_VENC_GOP_MODE_SMART_P,   /* gop_mode             */
                .smart_p =
                {
                    90,                     /* bg_interval          */
                    4,                      /* bg_qp_delta          */
                    2                       /* vi_qp_delta          */
                }
            }
        },
        {                                   /* start_param          */
            -1,                             /* recv_pic_num         */
        }
    }
};
#define VENC_CFG_COUNT (sizeof(venc_cfg) / sizeof(venc_cfg[0]))




/*************************************************
    AIO Config
**************************************************/
static const ai_dev_cfg_t ai_dev_cfg[] =
{
    {
        0,                               /* dev                 */
        {                                /* attr                */
            OT_AUDIO_SAMPLE_RATE_8000,   /* sample_rate         */
            OT_AUDIO_BIT_WIDTH_16,       /* bit_width           */
            OT_AIO_MODE_I2S_MASTER,      /* work_mode           */
            OT_AUDIO_SOUND_MODE_MONO,    /* snd_mode            */
            0,                           /* expand_flag         */
            64,                          /* frame_num           */
            320,                         /* point_num_per_frame */
            2,                           /* chn_cnt             */
            1,                           /* clk_share           */
            OT_AIO_I2STYPE_INNERCODEC    /* i2s_type            */
        }
    }
};
#define AI_DEV_CFG_COUNT (sizeof(ai_dev_cfg) / sizeof(ai_dev_cfg[0]))

static const ai_chn_cfg_t ai_chn_cfg[] =
{
    {
        0,      /* dev             */
        0,      /* chn             */
        {       /* param           */
            32  /* usr_frame_depth */
        }
    },
    {
        0,      /* dev             */
        1,      /* chn             */
        {       /* param           */
            32  /* usr_frame_depth */
        }
    }
};
#define AI_CHN_CFG_COUNT (sizeof(ai_chn_cfg) / sizeof(ai_chn_cfg[0]))

static const ao_dev_cfg_t ao_dev_cfg[] =
{
    {
        0,                               /* dev                 */
        {                                /* attr                */
            OT_AUDIO_SAMPLE_RATE_8000,   /* sample_rate         */
            OT_AUDIO_BIT_WIDTH_16,       /* bit_width           */
            OT_AIO_MODE_I2S_MASTER,      /* work_mode           */
            OT_AUDIO_SOUND_MODE_MONO,    /* snd_mode            */
            0,                           /* expand_flag         */
            8,                           /* frame_num           */
            320,                         /* point_num_per_frame */
            2,                           /* chn_cnt             */
            1,                           /* clk_share           */
            OT_AIO_I2STYPE_INNERCODEC    /* i2s_type            */
        }
    }
};
#define AO_DEV_CFG_COUNT (sizeof(ao_dev_cfg) / sizeof(ao_dev_cfg[0]))

static const ao_chn_cfg_t ao_chn_cfg[] =
{
    {
        0,    /* dev */
        0     /* chn */
    },
    {
        0,    /* dev */
        1     /* chn */
    }
};
#define AO_CHN_CFG_COUNT (sizeof(ao_chn_cfg) / sizeof(ao_chn_cfg[0]))

static acodec_cfg_t acodec_cfg =
{
    OT_ACODEC_FS_8000,     /* sample_rate */
    OT_ACODEC_MIXER_IN_D,  /* input_mode  */
    40                     /* input_vol   */
};




/*************************************************
    Bind Config
**************************************************/
static const bind_cfg_t bind_cfg[] =
{
    /*       mod dev chn           mod dev chn */
    {{  OT_ID_VI,  0,  0}, {OT_ID_VPSS,  0,  0} },
    {{OT_ID_VPSS,  0,  0}, {OT_ID_VENC,  0,  0} },
    {{OT_ID_VPSS,  0,  1}, {OT_ID_VENC,  0,  1} },
    {{OT_ID_VPSS,  0,  2}, {OT_ID_VENC,  0,  2} }
};
#define BIND_CFG_COUNT (sizeof(bind_cfg) / sizeof(bind_cfg[0]))




/*************************************************
    Region Config
**************************************************/
static const region_cfg_t region_cfg[] =
{
    {
        0,                                       /* handle       */
        {                                        /* attr         */
            OT_RGN_OVERLAY,                      /* type         */
            {                                    /* attr         */
                .overlay =
                {
                    OT_PIXEL_FORMAT_ARGB_CLUT2,  /* pixel_format */
                    0,                           /* bg_color     */
                    {320, 32},                   /* size         */
                    2,                           /* canvas_num   */
                    {                            /* clut         */
                        0x00000000,
                        0xa1000000,
                        0xa100ff00,
                        0xa1ffffff
                    }
                }
            }
        }
    }
};
#define REGION_CFG_COUNT (sizeof(region_cfg) / sizeof(region_cfg[0]))

static const region_attach_cfg_t region_attach_cfg[] =
{
    {
        0,                           /* handle     */
        1,                           /* attach_chn */
        {OT_ID_VENC, 0, 0},          /* chn        */
        {                            /* chn_attr   */
            1,                       /* is_show    */
            OT_RGN_OVERLAY,          /* type       */
            {                        /* attr       */
                .overlay_chn =
                {
                    {0, 0},          /* point      */
                    255,             /* fg_alpha   */
                    128,               /* bg_alpha   */
                    3,               /* layer      */
                    {                /* qp_info    */
                        0,           /* enable     */
                        1,           /* is_abs_qp  */
                        0            /* qp_val     */
                    }
                }
            }
        }
    }
};
#define REGION_ATTACH_CFG_COUNT (sizeof(region_attach_cfg) / sizeof(region_attach_cfg[0]))




/*************************************************
    Video Stream Config
**************************************************/
static const video_stream_cfg_t video_stream_cfg[] =
{
    /*           stream_id             format  width  height  fps   venc_chn */
    {{STREAM_ID_V_RGB_MAIN, VIDEO_FORMAT_H265,  2688,   1520,  30},        0  },
    {{STREAM_ID_V_RGB_SUB1, VIDEO_FORMAT_H265,  1920,   1080,  30},        1  },
    {{STREAM_ID_V_RGB_SUB2, VIDEO_FORMAT_H265,   640,    360,  30},        2  }
};
#define VIDEO_STREAM_CFG_COUNT (sizeof(video_stream_cfg) / sizeof(video_stream_cfg[0]))




/*************************************************
    Audio Stream Config
**************************************************/
static const audio_stream_cfg_t audio_stream_cfg[] =
{
    /*       stream_id            format  chns  rate   ai_dev  ai_chn */
    {{STREAM_ID_A_MAIN, AUDIO_FORMAT_PCM,    1, 8000},      0,      0  }
};
#define AUDIO_STREAM_CFG_COUNT (sizeof(audio_stream_cfg) / sizeof(audio_stream_cfg[0]))




/*************************************************
    OSD Config
**************************************************/
static const osd_cfg_t osd_cfg[] =
{
    /*           stream_id       osd_id           format    w   h  handle                 pixel_format    w   h  data */
    {{STREAM_ID_V_RGB_MAIN, OSD_ID_TIME, OSD_FORMAT_BIT2, 320, 32},     0, {OT_PIXEL_FORMAT_ARGB_CLUT2, 320, 32, NULL }}
};
#define OSD_CFG_COUNT (sizeof(osd_cfg) / sizeof(osd_cfg[0]))




#endif
