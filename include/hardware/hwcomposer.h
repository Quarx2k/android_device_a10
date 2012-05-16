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

#ifndef ANDROID_INCLUDE_HARDWARE_HWCOMPOSER_H
#define ANDROID_INCLUDE_HARDWARE_HWCOMPOSER_H

#include <stdint.h>
#include <sys/cdefs.h>

#include <hardware/gralloc.h>
#include <hardware/hardware.h>
#include <cutils/native_handle.h>

__BEGIN_DECLS

/*****************************************************************************/

#define HWC_API_VERSION 1

/**
 * The id of this module
 */
#define HWC_HARDWARE_MODULE_ID "hwcomposer"

/**
 * Name of the sensors device to open
 */
#define HWC_HARDWARE_COMPOSER   "composer"


enum {
    /* hwc_composer_device_t::set failed in EGL */
    HWC_EGL_ERROR = -1
};

/*
 * hwc_layer_t::hints values
 * Hints are set by the HAL and read by SurfaceFlinger
 */
enum {
    /*
     * HWC can set the HWC_HINT_TRIPLE_BUFFER hint to indicate to SurfaceFlinger
     * that it should triple buffer this layer. Typically HWC does this when
     * the layer will be unavailable for use for an extended period of time,
     * e.g. if the display will be fetching data directly from the layer and
     * the layer can not be modified until after the next set().
     */
    HWC_HINT_TRIPLE_BUFFER  = 0x00000001,

    /*
     * HWC sets HWC_HINT_CLEAR_FB to tell SurfaceFlinger that it should clear the
     * framebuffer with transparent pixels where this layer would be.
     * SurfaceFlinger will only honor this flag when the layer has no blending
     *
     */
    HWC_HINT_CLEAR_FB       = 0x00000002
};

/*
 * hwc_layer_t::flags values
 * Flags are set by SurfaceFlinger and read by the HAL
 */
enum {
    /*
     * HWC_SKIP_LAYER is set by SurfaceFlnger to indicate that the HAL
     * shall not consider this layer for composition as it will be handled
     * by SurfaceFlinger (just as if compositionType was set to HWC_OVERLAY).
     */
    HWC_SKIP_LAYER = 0x00000001,
};

/*
 * hwc_layer_t::compositionType values
 */
enum {
    /* this layer is to be drawn into the framebuffer by SurfaceFlinger */
    HWC_FRAMEBUFFER = 0,

    /* this layer will be handled in the HWC */
    HWC_OVERLAY = 1,
};

/*
 * hwc_layer_t::blending values
 */
enum {
    /* no blending */
    HWC_BLENDING_NONE     = 0x0100,

    /* ONE / ONE_MINUS_SRC_ALPHA */
    HWC_BLENDING_PREMULT  = 0x0105,

    /* SRC_ALPHA / ONE_MINUS_SRC_ALPHA */
    HWC_BLENDING_COVERAGE = 0x0405
};

/*
 * hwc_layer_t::transform values
 */
enum {
    /* flip source image horizontally */
    HWC_TRANSFORM_FLIP_H = HAL_TRANSFORM_FLIP_H,
    /* flip source image vertically */
    HWC_TRANSFORM_FLIP_V = HAL_TRANSFORM_FLIP_V,
    /* rotate source image 90 degrees clock-wise */
    HWC_TRANSFORM_ROT_90 = HAL_TRANSFORM_ROT_90,
    /* rotate source image 180 degrees */
    HWC_TRANSFORM_ROT_180 = HAL_TRANSFORM_ROT_180,
    /* rotate source image 270 degrees clock-wise */
    HWC_TRANSFORM_ROT_270 = HAL_TRANSFORM_ROT_270,
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
    HWC_FORMAT_MBYUV420		= 0x56,
    HWC_FORMAT_MBYUV422		= 0x57,
    HWC_FORMAT_YUV420PLANAR	= 0x58,
    HWC_FORMAT_DEFAULT      = 0x99,    // The actual color format is determined
    HWC_FORMAT_MAXVALUE     = 0x100
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
    /* rotation of the source image in degrees (0 to 359) */
    HWC_LAYER_ROTATION_DEG  	= 1,
    /* enable or disable dithering */
    HWC_LAYER_DITHER        	= 3,
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

/* enable/disable value setParameter() */
enum {
    HWC_LAYER_DISABLE = 0,
    HWC_LAYER_ENABLE  = 1
};

/*****************************************************************************/

typedef enum tag_RepeatField
{
    REPEAT_FIELD_NONE,          //means no field should be repeated

    REPEAT_FIELD_TOP,           //means the top field should be repeated
    REPEAT_FIELD_BOTTOM,        //means the bottom field should be repeated

    REPEAT_FIELD_
}repeatfield_t;

typedef struct tag_HWCLayerInitPara
{
	uint32_t		w;
	uint32_t		h;
	uint32_t		format;
	uint32_t		screenid;
}layerinitpara_t;

typedef struct tag_VideoInfo
{
    unsigned short              width;          //the stored picture width for luma because of mapping
    unsigned short              height;         //the stored picture height for luma because of mapping
    unsigned short              frame_rate;     //the source picture frame rate
    unsigned short              eAspectRatio;   //the source picture aspect ratio
    unsigned short              color_format;   //the source picture color format
}videoinfo_t;

typedef struct tag_Video3DInfo
{
	unsigned int width;
	unsigned int height;
	unsigned int format;
	unsigned int _3d_mode;
	unsigned int display_mode;
	unsigned int is_mode_changed;
}video3Dinfo_t;

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
}libhwclayerpara_t;

/*****************************************************************************/


typedef struct hwc_rect {
    int left;
    int top;
    int right;
    int bottom;
} hwc_rect_t;

typedef struct hwc_region {
    size_t numRects;
    hwc_rect_t const* rects;
} hwc_region_t;

typedef struct hwc_layer {
    /*
     * initially set to HWC_FRAMEBUFFER, indicates the layer will
     * be drawn into the framebuffer using OpenGL ES.
     * The HWC can toggle this value to HWC_OVERLAY, to indicate
     * it will handle the layer.
     */
    int32_t 	compositionType;

    /* see hwc_layer_t::hints above */
    uint32_t 	hints;

    /* see hwc_layer_t::flags above */
    uint32_t 	flags;

	uint32_t	format;
	
    /* handle of buffer to compose. this handle is guaranteed to have been
     * allocated with gralloc */
    buffer_handle_t handle;

    /* transformation to apply to the buffer during composition */
    uint32_t transform;

    /* blending to apply during composition */
    int32_t blending;

    /* area of the source to consider, the origin is the top-left corner of
     * the buffer */
    hwc_rect_t sourceCrop;

    /* where to composite the sourceCrop onto the display. The sourceCrop
     * is scaled using linear filtering to the displayFrame. The origin is the
     * top-left corner of the screen.
     */
    hwc_rect_t displayFrame;

    /* visible region in screen space. The origin is the
     * top-left corner of the screen.
     * The visible region INCLUDES areas overlapped by a translucent layer.
     */
    hwc_region_t visibleRegionScreen;
} hwc_layer_t;


/*
 * hwc_layer_list_t::flags values
 */
enum {
    /*
     * HWC_GEOMETRY_CHANGED is set by SurfaceFlinger to indicate that the list
     * passed to (*prepare)() has changed by more than just the buffer handles.
     */
    HWC_GEOMETRY_CHANGED = 0x00000001,
};

/*
 * List of layers.
 * The handle members of hwLayers elements must be unique.
 */
typedef struct hwc_layer_list {
    uint32_t flags;
    size_t numHwLayers;
    hwc_layer_t hwLayers[0];
} hwc_layer_list_t;

/* This represents a display, typically an EGLDisplay object */
typedef void* hwc_display_t;

/* This represents a surface, typically an EGLSurface object  */
typedef void* hwc_surface_t;


/* see hwc_composer_device::registerProcs()
 * Any of the callbacks can be NULL, in which case the corresponding
 * functionality is not supported.
 */
typedef struct hwc_procs {
    /*
     * (*invalidate)() triggers a screen refresh, in particular prepare and set
     * will be called shortly after this call is made. Note that there is
     * NO GUARANTEE that the screen refresh will happen after invalidate()
     * returns (in particular, it could happen before).
     * invalidate() is GUARANTEED TO NOT CALL BACK into the h/w composer HAL and
     * it is safe to call invalidate() from any of hwc_composer_device
     * hooks, unless noted otherwise.
     */
    void (*invalidate)(struct hwc_procs* procs);
} hwc_procs_t;


/*****************************************************************************/

typedef struct hwc_module {
    struct hw_module_t common;
} hwc_module_t;


typedef struct hwc_composer_device {
    struct hw_device_t common;

    /*
     * (*prepare)() is called for each frame before composition and is used by
     * SurfaceFlinger to determine what composition steps the HWC can handle.
     *
     * (*prepare)() can be called more than once, the last call prevails.
     *
     * The HWC responds by setting the compositionType field to either
     * HWC_FRAMEBUFFER or HWC_OVERLAY. In the former case, the composition for
     * this layer is handled by SurfaceFlinger with OpenGL ES, in the later
     * case, the HWC will have to handle this layer's composition.
     *
     * (*prepare)() is called with HWC_GEOMETRY_CHANGED to indicate that the
     * list's geometry has changed, that is, when more than just the buffer's
     * handles have been updated. Typically this happens (but is not limited to)
     * when a window is added, removed, resized or moved.
     *
     * a NULL list parameter or a numHwLayers of zero indicates that the
     * entire composition will be handled by SurfaceFlinger with OpenGL ES.
     *
     * returns: 0 on success. An negative error code on error. If an error is
     * returned, SurfaceFlinger will assume that none of the layer will be
     * handled by the HWC.
     */
    int (*prepare)(struct hwc_composer_device *dev, hwc_layer_list_t* list);


    /*
     * (*set)() is used in place of eglSwapBuffers(), and assumes the same
     * functionality, except it also commits the work list atomically with
     * the actual eglSwapBuffers().
     *
     * The list parameter is guaranteed to be the same as the one returned
     * from the last call to (*prepare)().
     *
     * When this call returns the caller assumes that:
     *
     * - the display will be updated in the near future with the content
     *   of the work list, without artifacts during the transition from the
     *   previous frame.
     *
     * - all objects are available for immediate access or destruction, in
     *   particular, hwc_region_t::rects data and hwc_layer_t::layer's buffer.
     *   Note that this means that immediately accessing (potentially from a
     *   different process) a buffer used in this call will not result in
     *   screen corruption, the driver must apply proper synchronization or
     *   scheduling (eg: block the caller, such as gralloc_module_t::lock(),
     *   OpenGL ES, Camera, Codecs, etc..., or schedule the caller's work
     *   after the buffer is freed from the actual composition).
     *
     * a NULL list parameter or a numHwLayers of zero indicates that the
     * entire composition has been handled by SurfaceFlinger with OpenGL ES.
     * In this case, (*set)() behaves just like eglSwapBuffers().
     *
     * dpy, sur, and list are set to NULL to indicate that the screen is
     * turning off. This happens WITHOUT prepare() being called first.
     * This is a good time to free h/w resources and/or power
     * the relevant h/w blocks down.
     *
     * IMPORTANT NOTE: there is an implicit layer containing opaque black
     * pixels behind all the layers in the list.
     * It is the responsibility of the hwcomposer module to make
     * sure black pixels are output (or blended from).
     *
     * returns: 0 on success. An negative error code on error:
     *    HWC_EGL_ERROR: eglGetError() will provide the proper error code
     *    Another code for non EGL errors.
     *
     */
    int (*set)(struct hwc_composer_device *dev,
                hwc_display_t dpy,
                hwc_surface_t sur,
                hwc_layer_list_t* list);
    /*
     * This hook is OPTIONAL.
     *
     * If non NULL it will be called by SurfaceFlinger on dumpsys
     */
    void (*dump)(struct hwc_composer_device* dev, char *buff, int buff_len);

    /*
     * This hook is OPTIONAL.
     *
     * (*registerProcs)() registers a set of callbacks the h/w composer HAL
     * can later use. It is FORBIDDEN to call any of the callbacks from
     * within registerProcs(). registerProcs() must save the hwc_procs_t pointer
     * which is needed when calling a registered callback.
     * Each call to registerProcs replaces the previous set of callbacks.
     * registerProcs is called with NULL to unregister all callbacks.
     *
     * Any of the callbacks can be NULL, in which case the corresponding
     * functionality is not supported.
     */
    void (*registerProcs)(struct hwc_composer_device* dev,
            hwc_procs_t const* procs);

    int         (*setparameter)(struct hwc_composer_device* dev,uint32_t cmd,uint32_t value);
    uint32_t    (*getparameter)(struct hwc_composer_device* dev,uint32_t cmd);
    
    void* reserved_proc[6];

} hwc_composer_device_t;


/** convenience API for opening and closing a device */

static inline int hwc_open(const struct hw_module_t* module,
        hwc_composer_device_t** device) {
    return module->methods->open(module,
            HWC_HARDWARE_COMPOSER, (struct hw_device_t**)device);
}

static inline int hwc_close(hwc_composer_device_t* device) {
    return device->common.close(&device->common);
}


/*****************************************************************************/

__END_DECLS

#endif /* ANDROID_INCLUDE_HARDWARE_HWCOMPOSER_H */
