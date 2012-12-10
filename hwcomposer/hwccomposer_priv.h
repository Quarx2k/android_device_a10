#ifndef __HWCOMPOSER_PRIV_H__
#define __HWCOMPOSER_PRIV_H__

#include <hardware/hardware.h>

#include <fcntl.h>
#include <errno.h>

#include <cutils/log.h>
#include <cutils/atomic.h>

#include <hardware/hwcomposer.h>

#include <EGL/egl.h>

#define  MAX_FBNUM        8
#define  MAX_LAYERNUM    8

typedef struct sun4i_hwc_layer
{
    hwc_layer_1_t            base;

    uint32_t            dispW;
    uint32_t            dispH;
    uint32_t            org_dispW;
    uint32_t            org_dispH;

    uint32_t            cropX;
    uint32_t            cropY;
    uint32_t            cropW;
    uint32_t            cropH;

    uint32_t            posX;
    uint32_t                posY;
    uint32_t            posW;
    uint32_t            posH;

    uint32_t            posX_last;
    uint32_t            posY_last;
    uint32_t            posW_last;
    uint32_t            posH_last;

    uint32_t            posX_org;
    uint32_t            posY_org;
    uint32_t            posW_org;
    uint32_t            posH_org;

    uint32_t            cur_framehandle;
    uint32_t            screen;
    uint32_t            currenthandle;
    uint32_t            cur_frameid;
} sun4i_hwc_layer_1_t;

typedef struct hwc_context_t
{
    hwc_composer_device_1_t     device;
    hwc_procs_t             *procs;
    int                        dispfd;
    sun4i_hwc_layer_1_t        hwc_layer;
    uint32_t                hwc_screen;
    bool                    hwc_layeropen;
    bool                    hwc_frameset;  /*is frame set*/
    bool                    hwc_reqclose;  /*is request close with parameter cmd*/
    uint32_t                cur_hdmimode;
    uint32_t                cur_3dmode;
    bool                    cur_half_enable;
    bool                    cur_3denable;
    /* our private state goes below here */
    bool                    wait_layer_open;
    bool                    vsync_enabled;
    pthread_t               vsync_thread;
}sun4i_hwc_context_t;

#ifdef __GNUC__
#define likely(x)       __builtin_expect(!!(x),1)
#define unlikely(x)     __builtin_expect(!!(x),0)
#else
#define likely(x)       (x)
#define unlikely(x)     (x)
#endif

#endif
