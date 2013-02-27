/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* This header contains deprecated HWCv0 interface declarations. Don't include
 * this header directly; it will be included by <hardware/hwcomposer.h> unless
 * HWC_REMOVE_DEPRECATED_VERSIONS is defined to non-zero.
 */
#ifndef ANDROID_INCLUDE_HARDWARE_HWCOMPOSER_H
#error "This header should only be included by hardware/hwcomposer.h"
#endif

#ifndef ANDROID_INCLUDE_HARDWARE_HWCOMPOSER_AW_H
#define ANDROID_INCLUDE_HARDWARE_HWCOMPOSER_AW_H

/* Allwinner additions */

/*****************************************************************************/

typedef enum tag_RepeatField
{
    REPEAT_FIELD_NONE,          //means no field should be repeated

    REPEAT_FIELD_TOP,           //means the top field should be repeated
    REPEAT_FIELD_BOTTOM,        //means the bottom field should be repeated

    REPEAT_FIELD_
} repeatfield_t;

enum
{
    HWC_3D_SRC_MODE_TB = 0x0,//top bottom
    HWC_3D_SRC_MODE_FP = 0x1,//frame packing
    HWC_3D_SRC_MODE_SSF = 0x2,//side by side full
    HWC_3D_SRC_MODE_SSH = 0x3,//side by side half
    HWC_3D_SRC_MODE_LI = 0x4,//line interleaved

    HWC_3D_SRC_MODE_NORMAL = 0xFF//2d
};

enum
{
    HWC_3D_OUT_MODE_TB = 0x0,//top bottom
    HWC_3D_OUT_MODE_FP = 0x1,//frame packing
    HWC_3D_OUT_MODE_SSF = 0x2,//side by side full
    HWC_3D_OUT_MODE_SSH = 0x3,//side by side half
    HWC_3D_OUT_MODE_LI = 0x4,//line interleaved
    HWC_3D_OUT_MODE_CI_1 = 0x5,//column interlaved 1
    HWC_3D_OUT_MODE_CI_2 = 0x6,//column interlaved 2
    HWC_3D_OUT_MODE_CI_3 = 0x7,//column interlaved 3
    HWC_3D_OUT_MODE_CI_4 = 0x8,//column interlaved 4
    HWC_3D_OUT_MODE_LIRGB = 0x9,//line interleaved rgb
    HWC_3D_OUT_MODE_FA = 0xa,//field alternative
    HWC_3D_OUT_MODE_LA = 0xb,//line alternative

    HWC_3D_OUT_MODE_NORMAL = 0xFF,//line alternative
};

/* names for setParameter() */
enum {
    HWC_DISP_MODE_2D 		= 0x0,
    HWC_DISP_MODE_3D 		= 0x1,
    HWC_DISP_MODE_ANAGLAGH 	= 0x2,
    HWC_DISP_MODE_ORIGINAL 	= 0x3,
};

/* names for setParameter() */
enum {
    HWC_3D_OUT_MODE_2D 		            = 0x0,//left picture
    HWC_3D_OUT_MODE_HDMI_3D_1080P24_FP 	= 0x1,
    HWC_3D_OUT_MODE_ANAGLAGH 	        = 0x2,//·ÖÉ«
    HWC_3D_OUT_MODE_ORIGINAL 	        = 0x3,//original pixture

    HWC_3D_OUT_MODE_HDMI_3D_720P50_FP   = 0x9,
    HWC_3D_OUT_MODE_HDMI_3D_720P60_FP   = 0xa
};

enum
{
    HWC_MODE_SCREEN0                = 0,
    HWC_MODE_SCREEN1                = 1,
    HWC_MODE_SCREEN0_TO_SCREEN1     = 2,
    HWC_MODE_SCREEN0_AND_SCREEN1    = 3,
    HWC_MODE_SCREEN0_BE             = 4,
    HWC_MODE_SCREEN0_GPU            = 5,
};

typedef struct tag_HWCLayerInitPara
{
    uint32_t		w;
    uint32_t		h;
    uint32_t		format;
    uint32_t		screenid;
} layerinitpara_t;

typedef struct tag_VideoInfo
{
    unsigned short              width;          //the stored picture width for luma because of mapping
    unsigned short              height;         //the stored picture height for luma because of mapping
    unsigned short              frame_rate;     //the source picture frame rate
    unsigned short              eAspectRatio;   //the source picture aspect ratio
    unsigned short              color_format;   //the source picture color format
} videoinfo_t;

typedef struct tag_Video3DInfo
{
    unsigned int width;
    unsigned int height;
    unsigned int format;
    unsigned int src_mode;
    unsigned int display_mode;
    unsigned int _3d_mode;
    unsigned int is_mode_changed;
} video3Dinfo_t;

typedef struct tag_PanScanInfo
{
    unsigned long               uNumberOfOffset;
    signed short                HorizontalOffsets[3];
} panscaninfo_t;


typedef struct tag_VdrvRect
{
    signed short                uStartX;    // Horizontal start point.
    signed short                uStartY;    // Vertical start point.
    signed short                uWidth;     // Horizontal size.
    signed short                uHeight;    // Vertical size.
} vdrvrect_t;

typedef struct tag_LIBHWCLAYERPARA
{
    signed char                 bProgressiveSrc;    // Indicating the source is progressive or not
    signed char                 bTopFieldFirst;     // VPO should check this flag when bProgressiveSrc is FALSE
    repeatfield_t               eRepeatField;       // only check it when frame rate is 24FPS and interlace output
    videoinfo_t                 pVideoInfo;         // a pointer to structure stored video information
    panscaninfo_t               pPanScanInfo;
    vdrvrect_t                  src_rect;           // source valid size
    vdrvrect_t                  dst_rect;           // source display size
    unsigned char               top_index;          // frame buffer index containing the top field
    unsigned long               top_y;              // the address of frame buffer, which contains top field luminance
    unsigned long               top_c;              // the address of frame buffer, which contains top field chrominance

    //the following is just for future
    unsigned char               bottom_index;       // frame buffer index containing the bottom field
    unsigned long               bottom_y;           // the address of frame buffer, which contains bottom field luminance
    unsigned long               bottom_c;           // the address of frame buffer, which contains bottom field chrominance

    //time stamp of the frame
    unsigned long               uPts;               // time stamp of the frame (ms?)
    unsigned char				first_frame_flg;
    unsigned long               number;
    unsigned long   flag_addr;//dit maf flag address
    unsigned long   flag_stride;//dit maf flag line stride
    unsigned char  maf_valid;
    unsigned char  pre_frame_valid;
} libhwclayerpara_t;

/*****************************************************************************/
/* End Allwinner additions */

/* Allwinner defs */
/* names for setParameter() */
enum {
    HWC_LAYER_ROTATION_DEG  	= 1,
    /* enable or disable dithering */
    HWC_LAYER_DITHER        	= 3,
    HWC_LAYER_SETMODE = 9,
    /* transformation applied (this is a superset of COPYBIT_ROTATION_DEG) */
    HWC_LAYER_SETINITPARA,
    /* set videoplayer init overlay parameter */
    HWC_LAYER_SETVIDEOPARA,
    /* set videoplayer play frame overlay parameter*/
    HWC_LAYER_SETFRAMEPARA,
    /* get videoplayer play frame overlay parameter*/
    HWC_LAYER_GETCURFRAMEPARA,
    /* query video blank interrupt*/
    HWC_LAYER_QUERYVBI,
    /* set overlay screen id*/
    HWC_LAYER_SETSCREEN,

    HWC_LAYER_SHOW,

    HWC_LAYER_RELEASE,

    HWC_LAYER_SET3DMODE,
    HWC_LAYER_SETFORMAT,

    HWC_LAYER_VPPON,
    HWC_LAYER_VPPGETON,

    HWC_LAYER_SETLUMASHARP,
    HWC_LAYER_GETLUMASHARP,

    HWC_LAYER_SETCHROMASHARP,
    HWC_LAYER_GETCHROMASHARP,

    HWC_LAYER_SETWHITEEXTEN,
    HWC_LAYER_GETWHITEEXTEN,

    HWC_LAYER_SETBLACKEXTEN,
    HWC_LAYER_GETBLACKEXTEN,
};

/* possible overlay formats */
enum
{
    HWC_FORMAT_MINVALUE     = 0x50,
    HWC_FORMAT_RGBA_8888    = 0x51,
    HWC_FORMAT_RGB_565      = 0x52,
    HWC_FORMAT_BGRA_8888    = 0x53,
    HWC_FORMAT_YCbYCr_422_I = 0x54,
    HWC_FORMAT_CbYCrY_422_I = 0x55,
    HWC_FORMAT_MBYUV420	    = 0x56,
    HWC_FORMAT_MBYUV422	    = 0x57,
    HWC_FORMAT_YUV420PLANAR = 0x58,
    HWC_FORMAT_DEFAULT      = 0x99,    // The actual color format is determined
    HWC_FORMAT_MAXVALUE     = 0x100
};
/* End of Allwinner additions */
/*****************************************************************************/

#endif
