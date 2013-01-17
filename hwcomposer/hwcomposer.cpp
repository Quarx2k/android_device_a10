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

//#define LOG_NDEBUG 0

#include <hardware/hardware.h>

#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/ioctl.h>

#include <cutils/log.h>
#include <cutils/atomic.h>

#include <hardware/hwcomposer.h>
#include <sunxi_disp_ioctl.h>
#include <fb.h>
#include <EGL/egl.h>

#include "hwccomposer_priv.h"

/*****************************************************************************/
unsigned int                    g_lcd_width        = 480;
unsigned int                    g_lcd_height       = 800;
unsigned int                    g_lcd_bpp          = 32;

unsigned long                   args[4];
unsigned long                     fb_layer_hdl;


static int hwc_device_open(const struct hw_module_t* module, const char* name,
        struct hw_device_t** device);

static struct hw_module_methods_t hwc_module_methods =
{
    open: hwc_device_open
};

hwc_module_t HAL_MODULE_INFO_SYM =
{
    common:
    {
        tag: HARDWARE_MODULE_TAG,
        version_major: 1,
        version_minor: 0,
        id: HWC_HARDWARE_MODULE_ID,
        name: "Sample hwcomposer module",
        author: "The Android Open Source Project",
        methods: &hwc_module_methods,
    }
};

static int hwc_open_disp(void)
{
	int fd, tmp, ret;

	fd = open("/dev/disp", O_RDWR);
	if (fd < 0) {
		ALOGE("Failed to open overlay device : %s\n", strerror(errno));
		return -1;
	}

	/* check version */
	tmp = SUNXI_DISP_VERSION;
	ret = ioctl(fd, DISP_CMD_VERSION, &tmp);
	if (ret == -1) {
		printf("Warning: kernel sunxi disp driver does not support "
		       "versioning.\n");
	} else if (ret < 0) {
		fprintf(stderr, "Error: ioctl(VERSION) failed: %s\n",
			strerror(-ret));
		return ret;
	} else {
		printf("sunxi disp kernel module version is %d.%d\n",
		       ret >> 16, ret & 0xFFFF);
	}

	return fd;
}

static int hwc_setcolorkey(sun4i_hwc_context_t  *ctx)
{
    int                          fbfh0;
    __disp_colorkey_t             ck;
    int                         ret;
    int                         fd;

    if(ctx->hwc_layer.currenthandle)
    {
        args[0]                            = ctx->hwc_screen;
        args[1]                         = ctx->hwc_layer.currenthandle;
        args[2]                         = 0;
        args[3]                         = 0;
        ret                             = ioctl(ctx->dispfd, DISP_CMD_LAYER_TOP,args);
        if(ret != 0)
        {
            //open display layer failed, need send play end command, and exit
            ALOGE("Set video display Top failed!\n");

            return NULL;
        }

        args[0]                         = ctx->hwc_screen;
        args[1]                         = ctx->hwc_layer.currenthandle;
        if(ctx->hwc_screen == 0)  //screen0 use pixel alpha
        {
            ioctl(ctx->dispfd, DISP_CMD_LAYER_CK_OFF,args);
        }
        else  //screen1 use colorkey
        {
            ioctl(ctx->dispfd, DISP_CMD_LAYER_CK_ON,args);
        }
    }


    fbfh0 = open("/dev/graphics/fb0",O_RDWR);
    if(fbfh0 < 0)
    {
        ALOGE("open fb0 fail \n ");

        return -1;
    }

    ioctl(fbfh0,FBIOGET_LAYER_HDL_0,&fb_layer_hdl);
    close(fbfh0);

    if(ctx->hwc_screen == 0)  //screen0 use pixel alpha
    {
        ck.ck_min.alpha                 = 0xff;
        ck.ck_min.red                     = 0x05; //0x01;
        ck.ck_min.green                 = 0x01; //0x03;
        ck.ck_min.blue                     = 0x07; //0x05;
        ck.ck_max.alpha                 = 0xff;
        ck.ck_max.red                     = 0x05; //0x01;
        ck.ck_max.green                 = 0x01; //0x03;
        ck.ck_max.blue                     = 0x07; //0x05;
    }
    else  //screen1 use colorkey
    {
        ck.ck_min.alpha                 = 0xff;
        ck.ck_min.red                     = 0x00; //0x01;
        ck.ck_min.green                 = 0x00; //0x03;
        ck.ck_min.blue                     = 0x00; //0x05;
        ck.ck_max.alpha                 = 0xff;
        ck.ck_max.red                     = 0x00; //0x01;
        ck.ck_max.green                 = 0x00; //0x03;
        ck.ck_max.blue                     = 0x00; //0x05;
    }

    ck.red_match_rule                 = 2;
    ck.green_match_rule             = 2;
    ck.blue_match_rule                 = 2;
    args[0]                         = 0;
    args[1]                         = (unsigned long)&ck;
    ioctl(ctx->dispfd,DISP_CMD_SET_COLORKEY,(void*)args);//pipe1, different with video layer's pipe

    args[0]                         = ctx->hwc_screen;
    args[1]                         = fb_layer_hdl;
    args[2]                         = 0;
    ioctl(ctx->dispfd,DISP_CMD_LAYER_SET_PIPE,(void*)args);//pipe1, different with video layer's pipe

    args[0]                         = ctx->hwc_screen;
    args[1]                         = fb_layer_hdl;
    ioctl(ctx->dispfd,DISP_CMD_LAYER_TOP,(void*)args);

    args[0]                         = ctx->hwc_screen;
    args[1]                         = fb_layer_hdl;
    args[2]                         = 0xFF;
    ioctl(ctx->dispfd,DISP_CMD_LAYER_SET_ALPHA_VALUE,(void*)args);//disable the global alpha, use the pixel's alpha

    args[0]                         = ctx->hwc_screen;
    args[1]                         = fb_layer_hdl;
    ioctl(ctx->dispfd,DISP_CMD_LAYER_ALPHA_OFF,(void*)args);//disable the global alpha, use the pixel's alpha

    args[0]                            = ctx->hwc_screen;
    args[1]                         = fb_layer_hdl;
    ioctl(ctx->dispfd,DISP_CMD_LAYER_CK_OFF,(void*)args);//disable the global alpha, use the pixel's alpha

    ALOGI("layer open hdl:%d,ret :%d\n",(unsigned long)ctx->hwc_layer.currenthandle,ret);

    return 0;
}

static int hwc_requestlayer(sun4i_hwc_context_t *ctx,uint32_t screenid)
{
    uint32_t            layerhandle;

    if(ctx->dispfd == 0)
    {
        ctx->dispfd = hwc_open_disp();
        if (ctx->dispfd < 0)
            return  -1;
    }

    ALOGV("screenid = %d\n",screenid);
    if(screenid > 1)
    {
        screenid = 1;
    }

    if(ctx->hwc_layer.currenthandle == 0)
    {
        args[0]                         = screenid;
        args[1]                         = 0;
        args[2]                         = 0;
        layerhandle                     = (uint32_t)ioctl(ctx->dispfd, DISP_CMD_LAYER_REQUEST,args);
        if(layerhandle == 0)
        {
            ALOGE("request layer failed!\n");

            return -1;
        }

        ALOGV("hwc_requestlayer layerhandle = %d\n",layerhandle);

        ctx->hwc_layer.currenthandle = layerhandle;
        ctx->hwc_screen                 = screenid;
        ctx->hwc_layeropen             = false;
    }
    else if(screenid != ctx->hwc_screen)
    {
        args[0]                         = ctx->hwc_screen;
        args[1]                         = ctx->hwc_layer.currenthandle;
        args[2]                         = 0;
        ioctl(ctx->dispfd, DISP_CMD_LAYER_RELEASE,args);

        ctx->hwc_layer.currenthandle     = 0;

        args[0]                         = screenid;
        args[1]                         = 0;
        args[2]                         = 0;
        layerhandle                     = (uint32_t)ioctl(ctx->dispfd, DISP_CMD_LAYER_REQUEST,args);
        if(layerhandle == 0)
        {
            ALOGE("request layer failed!\n");

            return -1;
        }

        ctx->hwc_layer.currenthandle     = layerhandle;
        ctx->hwc_screen                     = screenid;
        ctx->hwc_layeropen                 = false;
    }

    ALOGV("ctx->hwc_layer.currenthandle = %d\n",ctx->hwc_layer.currenthandle);
    ALOGV("ctx->hwc_screen = %d\n",ctx->hwc_screen);

    return  0;
}

static void hwc_computerlayerdisplayframe(hwc_composer_device_1_t *dev)
{
    sun4i_hwc_context_t           *ctx = (sun4i_hwc_context_t *)dev;
    sun4i_hwc_layer_1_t            *curlayer = (sun4i_hwc_layer_1_t *)&ctx->hwc_layer;
    int                         temp_x = curlayer->posX_org;
    int                         temp_y = curlayer->posY_org;
    int                         temp_w = curlayer->posW_org;
    int                         temp_h = curlayer->posH_org;
    int                         scn_w;
    int                         scn_h;
    int                            ret;

    if(!ctx)
    {
        ALOGE("parameter error!\n");

        return ;
    }

    if(ctx->dispfd == 0)
    {
        ctx->dispfd = hwc_open_disp();
        if (ctx->dispfd < 0)
            return;
    }

    curlayer->posX_last = curlayer->posX;
    curlayer->posY_last = curlayer->posY;
    curlayer->posW_last = curlayer->posW;
    curlayer->posH_last = curlayer->posH;

    curlayer->posX    = curlayer->posX_org;
    curlayer->posY    = curlayer->posY_org;
    curlayer->posW    = curlayer->posW_org;
    curlayer->posH    = curlayer->posH_org;
    /*
     * This ALOGIc here is to return an error if the rectangle is not fully
     * within the display, unless we have not received a valid position yet,
     * in which case we will do our best to adjust the rectangle to be within
     * the display.
     */
    args[0]                         = ctx->hwc_screen;
    ret = ioctl(ctx->dispfd,DISP_CMD_GET_OUTPUT_TYPE,args);

    args[0]                         = ctx->hwc_screen;
    args[1]                         = 0;
    args[2]                         = 0;
    g_lcd_width                     = ioctl(ctx->dispfd, DISP_CMD_SCN_GET_WIDTH,args);
    g_lcd_height                    = ioctl(ctx->dispfd, DISP_CMD_SCN_GET_HEIGHT,args);

    ALOGV("hdmi mode = %d\n",ret);
    ALOGV("ctx->cur_3denable = %d\n",ctx->cur_3dmode);

    if(ret == DISP_OUTPUT_TYPE_HDMI && (ctx->cur_3denable == true || ctx->cur_half_enable == true))
    {
        curlayer->posX              = 0;
        curlayer->posY                = 0;
        curlayer->posW              = g_lcd_width;
        curlayer->posH                = g_lcd_height;

        return ;
    }

    /* Require a minimum size */
    if (temp_w < 16)
    {
        temp_w = 16;
    }
    if (temp_h < 8)
    {
        temp_h = 8;
    }

    if(temp_x < 0)
    {
        temp_x = 0;
    }

    if(temp_y < 0)
    {
        temp_y = 0;
    }

    if(temp_x + temp_w > (int)curlayer->org_dispW)
    {
        temp_w = curlayer->org_dispW - temp_x;
    }

    if(temp_y + temp_h > (int)curlayer->org_dispH)
    {
        temp_h = curlayer->org_dispH - temp_y;
    }

    if(curlayer->dispW != curlayer->org_dispW || curlayer->dispH != curlayer->org_dispH)
    {
        scn_w  = temp_w * curlayer->dispW/curlayer->org_dispW;
        scn_h  = scn_w * temp_h/temp_w;
        temp_w = scn_w;
        temp_h = scn_h;
        if(curlayer->posX_org == ((curlayer->org_dispW - curlayer->posW_org)>>1))
        {
            if(curlayer->dispW > (uint32_t)scn_w)
            {
                temp_x = ((int)curlayer->dispW - scn_w)>>1;
            }
            else
            {
                temp_x = 0;
            }
        }
        else
        {
            temp_x = temp_x * curlayer->dispW/curlayer->org_dispW;
        }

        if(curlayer->posY_org == ((curlayer->org_dispH - curlayer->posH_org)>>1))
        {
            if(curlayer->dispW > (uint32_t)scn_h)
            {
                temp_y = ((int)curlayer->dispH - scn_h)>>1;
            }
            else
            {
                temp_y = 0;
            }
        }
        else
        {
            temp_y = temp_y * curlayer->dispH/curlayer->org_dispH;
        }

        if (temp_w < 16)
        {
            temp_w = 16;
        }
        if (temp_h < 8)
        {
            temp_h = 8;
        }
    }

    ALOGV("curlayer->dispH = %d\n",curlayer->dispH);
    ALOGV("curlayer->dispW = %d\n",curlayer->dispW);
    ALOGV("curlayer->dispH = %d\n",curlayer->org_dispH);
    ALOGV("curlayer->dispW = %d\n",curlayer->org_dispW);
    ALOGV("scn_w = %d\n",scn_w);
    ALOGV("scn_h = %d\n",scn_h);
    ALOGV("curlayer->posX_org = %d\n",curlayer->posX_org);
    ALOGV("curlayer->posY_org = %d\n",curlayer->posY_org);
    ALOGV("curlayer->posW_org = %d\n",curlayer->posW_org);
    ALOGV("curlayer->posH_org = %d\n",curlayer->posH_org);
    if ( temp_x < 0 )
    {
        temp_x = 0;
    }
    if ( temp_y < 0 )
    {
        temp_y = 0;
    }
    if ( temp_w > (int)curlayer->dispW )
    {
        temp_w = curlayer->dispW;
    }
    if ( temp_h > (int)curlayer->dispH )
    {
        temp_h = curlayer->dispH;
    }
    if ( (temp_x + temp_w) > (int)curlayer->dispW )
    {
        temp_w = curlayer->dispW - temp_x;
    }
    if ( (temp_y + temp_h) > (int)curlayer->dispH )
    {
        temp_h = curlayer->dispH - temp_y;
    }

    ALOGV("temp_x = %d\n",temp_x);
    ALOGV("temp_y = %d\n",temp_y);
    ALOGV("temp_w = %d\n",temp_w);
    ALOGV("temp_h = %d\n",temp_h);

    if(((int)curlayer->posX != temp_x) || ((int)curlayer->posY != temp_y)
          ||((int)curlayer->posW != temp_w) || ((int)curlayer->posH != temp_h) )
    {
        curlayer->posX         = temp_x;
        curlayer->posY         = temp_y;
        curlayer->posW         = temp_w;
        curlayer->posH         = temp_h;
     }
}

static bool hwc_can_render_layer(hwc_layer_1_t *layer)
{
    if((layer->format == HWC_FORMAT_MBYUV420)
        ||(layer->format == HWC_FORMAT_MBYUV422)
        ||(layer->format == HWC_FORMAT_YUV420PLANAR)
        ||(layer->format == HWC_FORMAT_DEFAULT))
    {
        ALOGV("format support overlay");

        return  true;
    }

    return  false;
}

static int hwc_setrect(sun4i_hwc_context_t *ctx,hwc_rect_t *croprect,hwc_rect_t *displayframe)
{
    uint32_t                    overlay;
    int                         fd;
    int                         ret = 0;
    int                         screen;
    bool                        needset = false;

    ALOGV("hwc_setcrop");

    overlay                         = ctx->hwc_layer.currenthandle;
    fd                              = ctx->dispfd;
    screen                          = ctx->hwc_screen;

    if(ctx->hwc_layer.currenthandle)
       {
        unsigned long            tmp_args[4];
        __disp_layer_info_t        tmpLayerAttr;

        tmp_args[0]             = screen;
        tmp_args[1]             = (unsigned long)overlay;
        tmp_args[2]             = (unsigned long) (&tmpLayerAttr);
        tmp_args[3]             = 0;

        ret = ioctl(fd, DISP_CMD_LAYER_GET_PARA, &tmp_args);

        if((tmpLayerAttr.fb.size.width != croprect->right - croprect->left)
           ||(tmpLayerAttr.fb.size.height != croprect->bottom - croprect->top)
           ||(tmpLayerAttr.src_win.x != croprect->left)
           ||(tmpLayerAttr.src_win.y != croprect->top))
        {
            //tmpLayerAttr.fb.size.width         = croprect->right - croprect->left;
            //tmpLayerAttr.fb.size.height     = croprect->bottom - croprect->top;

            //tmpLayerAttr.src_win.x            = croprect->left;
            //tmpLayerAttr.src_win.y            = croprect->top;
            //tmpLayerAttr.src_win.width        = croprect->right - croprect->left;
            //tmpLayerAttr.src_win.height        = croprect->bottom - croprect->top;

            //needset = true;
        }
        if((ctx->hwc_layer.posX_org != displayframe->left)
           ||(ctx->hwc_layer.posY_org != displayframe->top)
           ||(ctx->hwc_layer.posW_org != displayframe->right - displayframe->left)
           ||(ctx->hwc_layer.posH_org != displayframe->bottom - displayframe->top))
        {
            ctx->hwc_layer.posX_org = displayframe->left;
            ctx->hwc_layer.posY_org = displayframe->top;
            ctx->hwc_layer.posW_org = displayframe->right - displayframe->left;
            ctx->hwc_layer.posH_org = displayframe->bottom - displayframe->top;

            hwc_computerlayerdisplayframe((hwc_composer_device_1_t *)ctx);

            tmpLayerAttr.scn_win.x            = ctx->hwc_layer.posX;
            tmpLayerAttr.scn_win.y            = ctx->hwc_layer.posY;
            tmpLayerAttr.scn_win.width        = ctx->hwc_layer.posW;
            tmpLayerAttr.scn_win.height        = ctx->hwc_layer.posH;

            needset = true;
        }

        if(needset)
        {
            ret = ioctl(fd, DISP_CMD_LAYER_SET_PARA, &tmp_args);
        }

        if((ctx->hwc_layeropen == 0) && (ctx->hwc_reqclose == 0) && (ctx->hwc_frameset != 0))
        {
            hwc_setcolorkey(ctx);

            args[0]                 = screen;
            args[1]                 = (unsigned long)overlay;
            args[2]                 = 0;
            args[3]                 = 0;
            ioctl(ctx->dispfd, DISP_CMD_LAYER_OPEN, args);

            // ALOGV("------------------------------SET_PARA--0 addr0:%x addr1:%x ret:%d",layer_info.fb.addr[0],layer_info.fb.addr[1],ret);
            args[0]                 = screen;
            args[1]                 = (unsigned long)overlay;
            args[2]                 = 0;
            args[3]                 = 0;
            ioctl(ctx->dispfd, DISP_CMD_VIDEO_START, args);

            ctx->hwc_layeropen = true;
        }
    }

    return ret;
}

/*****************************************************************************/
static int hwc_prepare(hwc_composer_device_1_t *dev, size_t numDisplays, hwc_display_contents_1_t** lists)
{
    hwc_display_contents_1_t* list = lists[0];
    //ALOGV("hwc_prepare list->numHwLayers = %d\n",list->numHwLayers);
    //list is null on HWComposer->disable() on surfaceflinger
    if (list && (list->flags & HWC_GEOMETRY_CHANGED))
    {
        //ALOGV("hwc_prepare HWC_GEOMETRY_CHANGED list->numHwLayers = %d\n",list->numHwLayers);

        for (size_t i=0 ; i<list->numHwLayers ; i++)
        {
            if(hwc_can_render_layer(&list->hwLayers[i]))
            {
                //dump_layer(&list->hwLayers[i]);
                list->hwLayers[i].compositionType = HWC_OVERLAY;

                //ALOGV("hwc_prepare HWC_OVERLAY i = %d\n",i);
            }
            else
            {
                //dump_layer(&list->hwLayers[i]);
                list->hwLayers[i].compositionType = HWC_FRAMEBUFFER;

                //ALOGV("hwc_prepare HWC_FRAMEBUFFER i = %d\n",i);
            }
        }
    }
    return 0;
}

static int hwc_startset(hwc_composer_device_1_t *dev)
{
    sun4i_hwc_context_t           *ctx = (sun4i_hwc_context_t *)dev;

    if(ctx->dispfd == 0)
    {
        ctx->dispfd = hwc_open_disp();
        if (ctx->dispfd < 0)
            return  -1;
    }

    args[0]                = ctx->hwc_screen;
    args[1]             = 0;
    return ioctl(ctx->dispfd,DISP_CMD_START_CMD_CACHE,(void*)args);//disable the global alpha, use the pixel's alpha
}

static int hwc_endset(hwc_composer_device_1_t *dev)
{
    sun4i_hwc_context_t           *ctx = (sun4i_hwc_context_t *)dev;

    if(ctx->dispfd == 0)
    {
        ctx->dispfd = hwc_open_disp();
        if (ctx->dispfd < 0)
            return  -1;
    }

    args[0]                = ctx->hwc_screen;
    args[1]             = 0;
    return ioctl(ctx->dispfd,DISP_CMD_EXECUTE_CMD_AND_STOP_CACHE,(void*)args);//disable the global alpha, use the pixel's alpha
}


static int hwc_setlayerframepara(sun4i_hwc_context_t *ctx,uint32_t value)
{
    __disp_video_fb_t              tmpFrmBufAddr;
    libhwclayerpara_t            *overlaypara;
    int                            handle;
    __disp_layer_info_t         layer_info;
    int                         ret;
    int                         screen;

    if(!ctx)
    {
        ALOGE("parameter error!\n");

        return -1;
    }

    if(ctx->dispfd == 0)
    {
        ctx->dispfd = hwc_open_disp();
        if (ctx->dispfd < 0)
            return  -1;
    }

    screen                          = ctx->hwc_screen;
    overlaypara                     = (libhwclayerpara_t *)value;
    //set framebuffer parameter to display driver
    tmpFrmBufAddr.interlace         = (overlaypara->bProgressiveSrc?0:1);
    ALOGV("tmpFrmBufAddr.interlace:%d",tmpFrmBufAddr.interlace);
    tmpFrmBufAddr.top_field_first   = overlaypara->bTopFieldFirst;
    tmpFrmBufAddr.frame_rate        = overlaypara->pVideoInfo.frame_rate;
    tmpFrmBufAddr.addr[0]           = overlaypara->top_y;
    tmpFrmBufAddr.addr[1]           = overlaypara->top_c;
    tmpFrmBufAddr.addr[2]            = 0;

    if(overlaypara->bottom_c)
    {
        tmpFrmBufAddr.addr[2]        = 0;
        tmpFrmBufAddr.addr_right[0]    = overlaypara->bottom_y;
        tmpFrmBufAddr.addr_right[1]    = overlaypara->bottom_c;
        tmpFrmBufAddr.addr_right[2]    = 0;
    }
    else
    {
        if(overlaypara->bottom_y)
        {
            tmpFrmBufAddr.addr[2]    = overlaypara->bottom_y;
        }
        else
        {
            tmpFrmBufAddr.addr[2]    = 0;
        }

        tmpFrmBufAddr.addr_right[0]    = 0;
        tmpFrmBufAddr.addr_right[1]    = 0;
        tmpFrmBufAddr.addr_right[2]    = 0;
    }

    tmpFrmBufAddr.id                = overlaypara->number;
    ctx->hwc_layer.cur_frameid        = tmpFrmBufAddr.id;
    handle                            = (unsigned long)ctx->hwc_layer.currenthandle;
    ctx->hwc_frameset               = true;
    //ALOGV("overlaypara->bProgressiveSrc = %x",overlaypara->bProgressiveSrc);
    //ALOGV("overlaypara->bTopFieldFirst = %x",overlaypara->bTopFieldFirst);
    //ALOGV("overlaypara->pVideoInfo.frame_rate = %x",overlaypara->pVideoInfo.frame_rate);
    //ALOGV("overlaypara->first_frame_flg = %x",overlaypara->first_frame_flg);
    //ALOGV("overlay = %x",(unsigned long)handle);
    if(overlaypara->first_frame_flg || ctx->wait_layer_open)
    {
        ALOGV("overlay first ............");
        args[0]                 = screen;
        args[1]                 = handle;
        args[2]                 = (unsigned long) (&layer_info);
        args[3]                 = 0;
        ioctl(ctx->dispfd, DISP_CMD_LAYER_GET_PARA, args);

        layer_info.fb.addr[0]     = tmpFrmBufAddr.addr[0];
        layer_info.fb.addr[1]     = tmpFrmBufAddr.addr[1];
        layer_info.fb.addr[2]     = tmpFrmBufAddr.addr[2];
        layer_info.fb.trd_right_addr[0] = tmpFrmBufAddr.addr_right[0];
        layer_info.fb.trd_right_addr[1] = tmpFrmBufAddr.addr_right[1];
        layer_info.fb.trd_right_addr[2] = tmpFrmBufAddr.addr_right[2];

        args[0]                 = screen;
        args[1]                 = handle;
        args[2]                 = (unsigned long) (&layer_info);
        args[3]                 = 0;
        ret = ioctl(ctx->dispfd, DISP_CMD_LAYER_SET_PARA, args);

        if(ctx->wait_layer_open)
        {
            args[0]                     = screen;
            args[1]                     = handle;
            args[2]                     = 0;
            args[3]                     = 0;
            ioctl(ctx->dispfd, DISP_CMD_LAYER_OPEN,args);
        }
        ctx->wait_layer_open = 0;
    }
    else
    {
        //ALOGV("set FB");
        args[0]                    = screen;
        args[1]                 = handle;
        args[2]                 = (unsigned long)(&tmpFrmBufAddr);
        args[3]                 = 0;
        ioctl(ctx->dispfd, DISP_CMD_VIDEO_SET_FB,args);

    }

    return 0;
}

static int hwc_setlayerpara(sun4i_hwc_context_t *ctx,uint32_t value)
{
    void                        *overlayhandle = 0;
    __disp_layer_info_t         tmpLayerAttr;
    int                          fbfh0;
    __disp_colorkey_t             ck;
    int                         ret;
    layerinitpara_t                *layer_info = (layerinitpara_t *)value;
    uint32_t width                = layer_info->w;
    uint32_t height                = layer_info->h;
    uint32_t format                = layer_info->format;
    uint32_t screenid            = layer_info->screenid;
    uint32_t                    layerhandle;
    __disp_pixel_fmt_t          disp_format;
    __disp_pixel_mod_t            fb_mode = DISP_MOD_MB_UV_COMBINED;
    __disp_pixel_seq_t            disp_seq;
    __disp_cs_mode_t            disp_cs_mode;
    disp_seq = DISP_SEQ_UVUV;
    if (height < 720)
    {
        tmpLayerAttr.fb.cs_mode     = DISP_BT601;
    }
    else
    {
        tmpLayerAttr.fb.cs_mode     = DISP_BT709;
    }
    switch(format)
    {
        case HWC_FORMAT_DEFAULT:
            disp_format = DISP_FORMAT_YUV420;
            fb_mode = DISP_MOD_NON_MB_UV_COMBINED;
            break;
        case HWC_FORMAT_MBYUV420:
            disp_format = DISP_FORMAT_YUV420;
            fb_mode = DISP_MOD_MB_UV_COMBINED;
            break;
        case HWC_FORMAT_MBYUV422:
            disp_format = DISP_FORMAT_YUV422;
            fb_mode = DISP_MOD_MB_UV_COMBINED;
            break;
        case HWC_FORMAT_YUV420PLANAR:
            disp_format = DISP_FORMAT_YUV420;
            fb_mode = DISP_MOD_NON_MB_PLANAR;
            disp_seq = DISP_SEQ_P3210;
            disp_cs_mode    = DISP_YCC;
            break;
        case HWC_FORMAT_RGBA_8888:
            disp_format = DISP_FORMAT_ARGB8888;
            fb_mode = DISP_MOD_NON_MB_PLANAR;
            disp_seq = DISP_SEQ_P3210;
            disp_cs_mode    = DISP_YCC;
            break;
        default:
            disp_format = DISP_FORMAT_YUV420;
            fb_mode = DISP_MOD_NON_MB_PLANAR;
            break;
    }

    if(ctx->dispfd == 0)
    {
        ctx->dispfd = hwc_open_disp();
        if (ctx->dispfd < 0)
            return  -1;
    }

    if(screenid > 1)
    {
        screenid                     = 1;
    }

    ret                             = hwc_requestlayer(ctx,screenid);
    if(ret != 0)
    {
        ALOGE("request layer failed!\n");

        return -1;
    }

    args[0]                         = screenid;
    args[1]                         = 0;
    args[2]                         = 0;
    g_lcd_width                     = ioctl(ctx->dispfd, DISP_CMD_SCN_GET_WIDTH,args);
    g_lcd_height                    = ioctl(ctx->dispfd, DISP_CMD_SCN_GET_HEIGHT,args);

    ALOGV("overlay.cpp:fb_mode:%d,disp_format:%d  %d:%d, %d",fb_mode,disp_format,g_lcd_width,g_lcd_height, __LINE__);
    args[0]                         = screenid;
    args[1]                         = ctx->hwc_layer.currenthandle;
    args[2]                         = (unsigned long) (&tmpLayerAttr);
    args[3]                         = 0;
    ret = ioctl(ctx->dispfd, DISP_CMD_LAYER_GET_PARA, args);

    tmpLayerAttr.fb.mode             = fb_mode;    // DISP_MOD_NON_MB_UV_COMBINED;    // DISP_MOD_MB_UV_COMBINED;
    tmpLayerAttr.fb.format             = disp_format; //DISP_FORMAT_YUV420;//__disp_pixel_fmt_t(format);
    tmpLayerAttr.fb.br_swap         = 0;
    tmpLayerAttr.fb.seq             = disp_seq;
    tmpLayerAttr.fb.cs_mode         = disp_cs_mode;
    tmpLayerAttr.fb.addr[0]         = 0;
    tmpLayerAttr.fb.addr[1]         = 0;

    tmpLayerAttr.fb.size.width         = width;
    tmpLayerAttr.fb.size.height     = height;

    //set color space
    if (height < 720)
    {
        tmpLayerAttr.fb.cs_mode     = DISP_BT601;
    }
    else
    {
        tmpLayerAttr.fb.cs_mode     = DISP_BT709;
    }

    //set video layer attribute
    tmpLayerAttr.mode                 = DISP_LAYER_WORK_MODE_SCALER;
    tmpLayerAttr.alpha_en             = 1;
    tmpLayerAttr.alpha_val             = 0xff;
    tmpLayerAttr.pipe                 = 1;
    tmpLayerAttr.scn_win.x             = ctx->hwc_layer.posX;
    tmpLayerAttr.scn_win.y             = ctx->hwc_layer.posY;
    tmpLayerAttr.scn_win.width         = ctx->hwc_layer.posW;
    tmpLayerAttr.scn_win.height     = ctx->hwc_layer.posH;
    tmpLayerAttr.prio               = 0xff;
    //screen window information
    //frame buffer pst and size information
    tmpLayerAttr.src_win.x          = 0;//tmpVFrmInf->dst_rect.uStartX;
    tmpLayerAttr.src_win.y          = 0;//tmpVFrmInf->dst_rect.uStartY;
    tmpLayerAttr.src_win.width      = width;//tmpVFrmInf->dst_rect.uWidth;
    tmpLayerAttr.src_win.height     = height;//tmpVFrmInf->dst_rect.uHeight;
    tmpLayerAttr.fb.b_trd_src        = ctx->cur_half_enable;
    tmpLayerAttr.b_trd_out            = ctx->cur_3denable;
    tmpLayerAttr.fb.trd_mode         =  (__disp_3d_src_mode_t)ctx->cur_3dmode;
    tmpLayerAttr.out_trd_mode        = DISP_3D_OUT_MODE_FP;
    tmpLayerAttr.b_from_screen         = 0;
    //set channel
    args[0]                         = screenid;
    args[1]                         = ctx->hwc_layer.currenthandle;
    args[2]                         = (unsigned long) (&tmpLayerAttr);
    args[3]                         = 0;
    ret = ioctl(ctx->dispfd, DISP_CMD_LAYER_SET_PARA, args);
    ALOGV("SET_PARA ret:%d",ret);

    args[0]                         = 0;
    args[1]                         = 0;
    args[2]                         = 0;
    ctx->hwc_layer.org_dispW        = ioctl(ctx->dispfd, DISP_CMD_SCN_GET_WIDTH,args);
    ctx->hwc_layer.org_dispH        = ioctl(ctx->dispfd, DISP_CMD_SCN_GET_HEIGHT,args);

    args[0]                         = screenid;
    args[1]                         = 0;
    args[2]                         = 0;
    ctx->hwc_layer.dispW            = ioctl(ctx->dispfd, DISP_CMD_SCN_GET_WIDTH,args);
    ctx->hwc_layer.dispH            = ioctl(ctx->dispfd, DISP_CMD_SCN_GET_HEIGHT,args);
    ctx->hwc_layeropen                 = false;
    ctx->hwc_reqclose                 = false;
    ctx->hwc_layer.posX_org            = 0;
    ctx->hwc_layer.posY_org            = 0;
    ctx->hwc_layer.posW_org            = 0;
    ctx->hwc_layer.posH_org            = 0;
    ctx->hwc_frameset               = false;
    return ret;
}

// for taking photo to avoid preview wrong
static int hwc_show(sun4i_hwc_context_t *ctx,int value)
{
    uint32_t                    overlay;
    int                         fd;
    int                         ret = 0;
    int                         screen;

    ALOGV("hwc_show, value: %d", value);

    overlay                         = ctx->hwc_layer.currenthandle;
    fd                              = ctx->dispfd;
    screen                          = ctx->hwc_screen;
//    ALOGV("handle = %x,tmpFrmBufAddr.addr[0] = %x,tmpFrmBufAddr.addr[1] = %x,screen = %d\n",handle,tmpFrmBufAddr.addr[0],tmpFrmBufAddr.addr[1],screen);

    if(ctx->hwc_layer.currenthandle)
       {
        args[0]                            = screen;
        args[1]                         = ctx->hwc_layer.currenthandle;
        args[2]                         = 0;
        args[3]                         = 0;


        ALOGV("----------screen: %d, handle: %d", screen, ctx->hwc_layer.currenthandle);
        if(value == 1)
        {
            if(ctx->hwc_layeropen == false)
            {
                ALOGV("----------hwc_layeropen false");
                ret = ioctl(fd, DISP_CMD_LAYER_OPEN,args);

                ioctl(ctx->dispfd, DISP_CMD_VIDEO_START, args);

                args[0]                         = ctx->hwc_screen;
                args[1]                         = fb_layer_hdl;
                ioctl(ctx->dispfd,DISP_CMD_LAYER_ALPHA_OFF,(void*)args);//disable the global alpha, use the pixel's alpha
                    ctx->hwc_layeropen = true;
            }
        }
        else
        {
            if(ctx->hwc_layeropen == true)
            {
                ALOGV("----------hwc_layeropen true");
                ret = ioctl(fd, DISP_CMD_LAYER_CLOSE,args);

                ioctl(fd, DISP_CMD_VIDEO_STOP, args);

                args[0]                         = ctx->hwc_screen;
                args[1]                         = fb_layer_hdl;
                ioctl(ctx->dispfd,DISP_CMD_LAYER_ALPHA_ON,(void*)args);//disable the global alpha, use the pixel's alpha

                ctx->hwc_layeropen = false;
            }
        }
    }

    return ret;
}

// for taking photo to avoid preview wrong
static int hwc_reqshow(sun4i_hwc_context_t *ctx,int value)
{
    uint32_t                    overlay;
    int                         fd;
    int                         ret = 0;
    int                         screen;

    ALOGV("hwc_show, value: %d", value);

    overlay                         = ctx->hwc_layer.currenthandle;
    fd                              = ctx->dispfd;
    screen                          = ctx->hwc_screen;
//    ALOGV("handle = %x,tmpFrmBufAddr.addr[0] = %x,tmpFrmBufAddr.addr[1] = %x,screen = %d\n",handle,tmpFrmBufAddr.addr[0],tmpFrmBufAddr.addr[1],screen);

    if(ctx->hwc_layer.currenthandle)
       {
        args[0]                            = screen;
        args[1]                         = ctx->hwc_layer.currenthandle;
        args[2]                         = 0;
        args[3]                         = 0;


        ALOGV("----------screen: %d, handle: %d", screen, ctx->hwc_layer.currenthandle);
        if(value == 1)
        {
            if(ctx->hwc_layeropen == false)
            {
                if((ctx->hwc_layer.posW_org != 0)
                   ||(ctx->hwc_layer.posH_org != 0))
                {
                    ALOGV("----------hwc_layeropen false");
                    ret = ioctl(fd, DISP_CMD_LAYER_OPEN,args);

                    ioctl(ctx->dispfd, DISP_CMD_VIDEO_START, args);

                    args[0]                         = ctx->hwc_screen;
                    args[1]                         = fb_layer_hdl;
                    ioctl(ctx->dispfd,DISP_CMD_LAYER_ALPHA_OFF,(void*)args);//disable the global alpha, use the pixel's alpha
                    ctx->hwc_layeropen = true;
                }
                ctx->hwc_reqclose = false;
            }
        }
        else
        {
            if(ctx->hwc_layeropen == true)
            {
                ALOGV("----------hwc_layeropen true");
                ret = ioctl(fd, DISP_CMD_LAYER_CLOSE,args);

                ioctl(fd, DISP_CMD_VIDEO_STOP, args);

                args[0]                         = ctx->hwc_screen;
                args[1]                         = fb_layer_hdl;
                ioctl(ctx->dispfd,DISP_CMD_LAYER_ALPHA_ON,(void*)args);//disable the global alpha, use the pixel's alpha

                args[0]                         = ctx->hwc_screen;
                   ret = ioctl(ctx->dispfd,DISP_CMD_GET_OUTPUT_TYPE,args);
                   if(ret == DISP_OUTPUT_TYPE_HDMI && (ctx->cur_3denable == true))
                   {
                       args[0]                     = ctx->hwc_screen;
                       args[1]                     = 0;
                       args[2]                     = 0;
                       args[3]                     = 0;
                       ioctl(ctx->dispfd,DISP_CMD_HDMI_OFF,(unsigned long)args);

                       ALOGV("ctx->cur_hdmimode %d",ctx->cur_hdmimode);
                       args[0]                     = ctx->hwc_screen;
                       args[1]                     = ctx->cur_hdmimode;
                       args[2]                     = 0;
                       args[3]                     = 0;
                       ioctl(ctx->dispfd, DISP_CMD_HDMI_SET_MODE, args);

                       args[0]                     = ctx->hwc_screen;
                       args[1]                     = 0;
                       args[2]                     = 0;
                       args[3]                     = 0;
                       ioctl(ctx->dispfd,DISP_CMD_HDMI_ON,(unsigned long)args);
                   }

                ctx->hwc_layeropen = false;
                ctx->hwc_reqclose = true;
            }
        }
    }

    return ret;
}

// for taking photo to avoid preview wrong
static int hwc_release(sun4i_hwc_context_t *ctx)
{
    uint32_t                    overlay;
    int                         fd;
    int                         ret = 0;
    int                         screen;

    ALOGV("hwc_release!ctx->hwc_layer.currenthandle = %d\n",ctx->hwc_layer.currenthandle);

    overlay                         = ctx->hwc_layer.currenthandle;
    fd                              = ctx->dispfd;
    screen                          = ctx->hwc_screen;
//    ALOGV("handle = %x,tmpFrmBufAddr.addr[0] = %x,tmpFrmBufAddr.addr[1] = %x,screen = %d\n",handle,tmpFrmBufAddr.addr[0],tmpFrmBufAddr.addr[1],screen);

    if(ctx->hwc_layer.currenthandle)
       {
        args[0]                            = screen;
        args[1]                         = ctx->hwc_layer.currenthandle;
        args[2]                         = 0;
        args[3]                         = 0;
        ret = ioctl(fd, DISP_CMD_LAYER_CLOSE,args);

        ioctl(fd, DISP_CMD_VIDEO_STOP, args);

        ioctl(ctx->dispfd, DISP_CMD_LAYER_RELEASE,args);

        ctx->hwc_layer.currenthandle    = 0;

        args[0]                         = ctx->hwc_screen;
        args[1]                         = fb_layer_hdl;
        ioctl(ctx->dispfd,DISP_CMD_LAYER_ALPHA_ON,(void*)args);//disable the global alpha, use the pixel's alpha

        args[0]                         = ctx->hwc_screen;
        ret = ioctl(ctx->dispfd,DISP_CMD_GET_OUTPUT_TYPE,args);
        if(ret == DISP_OUTPUT_TYPE_HDMI && (ctx->cur_3denable == true))
        {
            args[0]                     = ctx->hwc_screen;
            args[1]                     = 0;
            args[2]                     = 0;
            args[3]                     = 0;
            ioctl(ctx->dispfd,DISP_CMD_HDMI_OFF,(unsigned long)args);

            ALOGV("ctx->cur_hdmimode %d",ctx->cur_hdmimode);
            args[0]                     = ctx->hwc_screen;
            args[1]                     = ctx->cur_hdmimode;
            args[2]                     = 0;
            args[3]                     = 0;
            ioctl(ctx->dispfd, DISP_CMD_HDMI_SET_MODE, args);

            args[0]                     = ctx->hwc_screen;
            args[1]                     = 0;
            args[2]                     = 0;
            args[3]                     = 0;
            ioctl(ctx->dispfd,DISP_CMD_HDMI_ON,(unsigned long)args);
        }
        ctx->hwc_layeropen = false;
    }

    return ret;
}


// for camera app, such as QQ , it use pixel sequence of DISP_SEQ_VUVU
static int hwc_setformat(sun4i_hwc_context_t *ctx,uint32_t value)
{
    uint32_t                    overlay;
    int                         fd;
    int                         ret = 0;
    int                         screen;

    ALOGV("hwc_setformat");

    overlay                         = ctx->hwc_layer.currenthandle;
    fd                              = ctx->dispfd;
    screen                          = ctx->hwc_screen;

    if(ctx->hwc_layer.currenthandle)
       {
        unsigned long            tmp_args[4];
        __disp_layer_info_t        tmpLayerAttr;

        tmp_args[0]             = screen;
        tmp_args[1]             = (unsigned long)overlay;
        tmp_args[2]             = (unsigned long) (&tmpLayerAttr);
        tmp_args[3]             = 0;

        ret = ioctl(fd, DISP_CMD_LAYER_GET_PARA, &tmp_args);
        ALOGV("DISP_CMD_LAYER_GET_PARA, ret %d", ret);

        tmpLayerAttr.fb.seq = (__disp_pixel_seq_t)value;

        ret = ioctl(fd, DISP_CMD_LAYER_SET_PARA, &tmp_args);
        ALOGV("DISP_CMD_LAYER_GET_PARA, ret %d", ret);
    }

    return ret;
}

static int hwc_setscreen(sun4i_hwc_context_t *ctx,uint32_t value)
{
    int                         fd;
    __disp_layer_info_t         layer_info;
    int                         ret;
    int                         output_mode;
    int                            old_screen;
    uint32_t                    overlay_handle;
    void*                        overlayhandle;
    int                            ctl_fd;

    overlay_handle                = ctx->hwc_layer.currenthandle;
    ALOGV("overlay_handle = %d\n",(unsigned long)overlay_handle);
    old_screen                   = ctx->hwc_screen;
    ALOGV("old_screen = %d\n",(unsigned long)old_screen);
    ctl_fd                       = ctx->dispfd;

    if(old_screen  == value)
    {
        ALOGV("nothing to do!");

        return  0;
    }

    if(value != 0)
    {
        value = 1;
    }

    ALOGV("overlay release first");
    args[0]                     = old_screen;
    args[1]                     = (unsigned long) overlay_handle;
    args[2]                         = (unsigned long) (&layer_info);
    args[3]                     = 0;
    ioctl(ctl_fd, DISP_CMD_LAYER_GET_PARA, args);

    args[0]                     = old_screen;
    args[1]                     = (unsigned long) overlay_handle;
    args[2]                     = 0;
    args[3]                     = 0;
    ioctl(ctl_fd, DISP_CMD_VIDEO_STOP, args);
    ioctl(ctl_fd, DISP_CMD_LAYER_RELEASE,args);

    ALOGV("release overlay = %d,value = %d\n",(unsigned long) overlay_handle,value);

    sleep(2);

    args[0]                     = value;
    args[1]                     = 0;
    args[2]                     = 0;
    overlayhandle                 = (void *)ioctl(ctl_fd, DISP_CMD_LAYER_REQUEST,args);
    if(overlayhandle == 0)
    {
        ALOGE("request layer failed!\n");

        goto error;
    }

    args[0]                         = value;
    ret                             = ioctl(ctl_fd,DISP_CMD_GET_OUTPUT_TYPE,args);
    output_mode = ret;
    if(ret == DISP_OUTPUT_TYPE_HDMI && (ctx->cur_3denable == true))
    {
        args[0]                     = value;
        args[1]                     = 0;
        args[2]                     = 0;
        args[3]                     = 0;
        ctx->cur_hdmimode            = ioctl(ctl_fd, DISP_CMD_HDMI_GET_MODE, args);
        ALOGV("overlay_setScreenid ctx->cur_hdmimode = %d\n",ctx->cur_hdmimode);
        args[0]                     = value;
        args[1]                     = 0;
        args[2]                     = 0;
        args[3]                     = 0;
        ioctl(ctl_fd,DISP_CMD_HDMI_OFF,(unsigned long)args);

        args[0]                     = value;
        args[1]                     = DISP_TV_MOD_1080P_24HZ_3D_FP;
        args[2]                     = 0;
        args[3]                     = 0;
        ioctl(ctl_fd, DISP_CMD_HDMI_SET_MODE, args);

        args[0]                     = value;
        args[1]                     = 0;
        args[2]                     = 0;
        args[3]                     = 0;
        ioctl(ctl_fd,DISP_CMD_HDMI_ON,(unsigned long)args);
    }

    args[0]                         = value;
    args[1]                         = 0;
    args[2]                         = 0;
    g_lcd_width                     = ioctl(ctl_fd, DISP_CMD_SCN_GET_WIDTH,args);
    g_lcd_height                    = ioctl(ctl_fd, DISP_CMD_SCN_GET_HEIGHT,args);

    ctx->hwc_layer.currenthandle    = (unsigned long)overlayhandle;
    ctx->hwc_screen                    = value;
    ALOGV("g_screen = %d",ctx->hwc_screen);
    ALOGV("g_currenthandle = %d",ctx->hwc_layer.currenthandle);
    ctx->hwc_layer.dispW            = g_lcd_width;
    ctx->hwc_layer.dispH            = g_lcd_height;
    ALOGV("ctx->hwc_layer.dispW = %d,ctx->hwc_layer.dispH = %d\n",ctx->hwc_layer.dispW,ctx->hwc_layer.dispH);

    hwc_computerlayerdisplayframe((hwc_composer_device_1_t *)ctx);
    layer_info.scn_win.x             = ctx->hwc_layer.posX;
    layer_info.scn_win.y             = ctx->hwc_layer.posY;
    layer_info.scn_win.width         = ctx->hwc_layer.posW;
    layer_info.scn_win.height         = ctx->hwc_layer.posH;
    //frame buffer pst and size information
    layer_info.b_from_screen        = false;

    layer_info.fb.b_trd_src         = ctx->cur_half_enable;
    layer_info.fb.trd_mode             = (__disp_3d_src_mode_t)ctx->cur_3dmode;
    layer_info.b_trd_out            = ctx->cur_3denable;
    layer_info.out_trd_mode            = DISP_3D_OUT_MODE_FP;
    if(output_mode != DISP_OUTPUT_TYPE_HDMI && (ctx->cur_3denable == true))
    {
        layer_info.fb.b_trd_src         = false;
        layer_info.b_trd_out            = false;
    }

    ALOGV("request overlay.cpp:w:%d,h:%d  %d:%d",layer_info.fb.size.width,layer_info.fb.size.height,g_lcd_width,g_lcd_height);
    ALOGV("request overlay.cpp:fb_mode:%d,disp_format:%d  %d:%d",layer_info.fb.mode,layer_info.fb.format,g_lcd_width,g_lcd_height);

    //set channel
    args[0]                         = value;
    args[1]                         = (unsigned long)overlayhandle;
    args[2]                         = (unsigned long) (&layer_info);
    args[3]                         = 0;
    ret = ioctl(ctl_fd, DISP_CMD_LAYER_SET_PARA, args);
    ALOGV("SET_PARA ret:%d",ret);

    args[0]                            = value;
    args[1]                         = (unsigned long)overlayhandle;
    args[2]                         = 0;
    args[3]                         = 0;
    ioctl(ctl_fd, DISP_CMD_LAYER_BOTTOM,args);

    hwc_setcolorkey(ctx);

    args[0]                         = value;
    args[1]                         = (unsigned long) overlayhandle;
    args[2]                         = 0;
    args[3]                         = 0;
    ioctl(ctl_fd, DISP_CMD_LAYER_OPEN, args);

    args[0]                         = value;
    args[1]                         = (unsigned long) overlayhandle;
    args[2]                         = 0;
    args[3]                         = 0;
    ioctl(ctl_fd, DISP_CMD_VIDEO_START, args);

    return 0;

error:
    if(overlayhandle)
    {
        args[0] = value;
        args[1] = (unsigned long)overlayhandle;
        args[2] = 0;
        args[3] = 0;
        ioctl(ctl_fd, DISP_CMD_LAYER_RELEASE,args);
    }

    return -1;
}

// for taking photo to avoid preview wrong
static int hwc_vppon(sun4i_hwc_context_t *ctx,int value)
{
    uint32_t                    overlay;
    int                         fd;
    int                         ret = 0;
    int                         screen;

    ALOGV("hwc_vppon");

    overlay                         = ctx->hwc_layer.currenthandle;
    fd                              = ctx->dispfd;
    screen                          = ctx->hwc_screen;
//    ALOGV("handle = %x,tmpFrmBufAddr.addr[0] = %x,tmpFrmBufAddr.addr[1] = %x,screen = %d\n",handle,tmpFrmBufAddr.addr[0],tmpFrmBufAddr.addr[1],screen);

    if(ctx->hwc_layer.currenthandle)
       {
        args[0]                            = screen;
        args[1]                         = (unsigned long)overlay;
        args[2]                         = 0;
        args[3]                         = 0;
        if(value != 0)
        {
            ret = ioctl(fd, DISP_CMD_LAYER_VPP_ON,args);
        }
        else
        {
            ret = ioctl(fd, DISP_CMD_LAYER_VPP_OFF,args);
        }

    }

    return ret;
}

// for taking photo to avoid preview wrong
static int hwc_getvppon(sun4i_hwc_context_t *ctx)
{
    uint32_t                    overlay;
    int                         fd;
    int                         ret = 0;
    int                         screen;

    ALOGV("hwc_vppon");

    overlay                         = ctx->hwc_layer.currenthandle;
    fd                              = ctx->dispfd;
    screen                          = ctx->hwc_screen;

    if(ctx->hwc_layer.currenthandle)
       {
        args[0]                            = screen;
        args[1]                         = (unsigned long)overlay;
        args[2]                         = 0;
        args[3]                         = 0;

        ret = ioctl(fd, DISP_CMD_LAYER_GET_VPP_EN,args);
    }

    return ret;
}

// for taking photo to avoid preview wrong
static int hwc_setlumasharp(sun4i_hwc_context_t *ctx,int value)
{
    uint32_t                    overlay;
    int                         fd;
    int                         ret = 0;
    int                         screen;

    ALOGV("hwc_vppon");

    overlay                         = ctx->hwc_layer.currenthandle;
    fd                              = ctx->dispfd;
    screen                          = ctx->hwc_screen;

    if(ctx->hwc_layer.currenthandle)
       {
        args[0]                            = screen;
        args[1]                         = (unsigned long)overlay;
        args[2]                         = value;
        args[3]                         = 0;

        ret = ioctl(fd, DISP_CMD_LAYER_SET_LUMA_SHARP_LEVEL,args);
    }

    return ret;
}

// for taking photo to avoid preview wrong
static int hwc_getlumasharp(sun4i_hwc_context_t *ctx)
{
    uint32_t                    overlay;
    int                         fd;
    int                         ret = 0;
    int                         screen;

    ALOGV("hwc_vppon");

    overlay                         = ctx->hwc_layer.currenthandle;
    fd                              = ctx->dispfd;
    screen                          = ctx->hwc_screen;

    if(ctx->hwc_layer.currenthandle)
       {
        args[0]                            = screen;
        args[1]                         = (unsigned long)overlay;
        args[2]                         = 0;
        args[3]                         = 0;

        ret = ioctl(fd, DISP_CMD_LAYER_GET_LUMA_SHARP_LEVEL,args);
    }

    return ret;
}

// for taking photo to avoid preview wrong
static int hwc_setchromasharp(sun4i_hwc_context_t *ctx,int value)
{
    uint32_t                    overlay;
    int                         fd;
    int                         ret = 0;
    int                         screen;

    ALOGV("hwc_vppon");

    overlay                         = ctx->hwc_layer.currenthandle;
    fd                              = ctx->dispfd;
    screen                          = ctx->hwc_screen;

    if(ctx->hwc_layer.currenthandle)
       {
        args[0]                            = screen;
        args[1]                         = (unsigned long)overlay;
        args[2]                         = value;
        args[3]                         = 0;

        ret = ioctl(fd, DISP_CMD_LAYER_SET_CHROMA_SHARP_LEVEL,args);
    }

    return ret;
}

// for taking photo to avoid preview wrong
static int hwc_getchromasharp(sun4i_hwc_context_t *ctx)
{
    uint32_t                    overlay;
    int                         fd;
    int                         ret = 0;
    int                         screen;

    ALOGV("hwc_vppon");

    overlay                         = ctx->hwc_layer.currenthandle;
    fd                              = ctx->dispfd;
    screen                          = ctx->hwc_screen;

    if(ctx->hwc_layer.currenthandle)
       {
        args[0]                            = screen;
        args[1]                         = (unsigned long)overlay;
        args[2]                         = 0;
        args[3]                         = 0;

        ret = ioctl(fd, DISP_CMD_LAYER_GET_CHROMA_SHARP_LEVEL,args);
    }

    return ret;
}

// for taking photo to avoid preview wrong
static int hwc_setwhiteexten(sun4i_hwc_context_t *ctx,int value)
{
    uint32_t                    overlay;
    int                         fd;
    int                         ret = 0;
    int                         screen;

    ALOGV("hwc_vppon");

    overlay                         = ctx->hwc_layer.currenthandle;
    fd                              = ctx->dispfd;
    screen                          = ctx->hwc_screen;

    if(ctx->hwc_layer.currenthandle)
       {
        args[0]                            = screen;
        args[1]                         = (unsigned long)overlay;
        args[2]                         = value;
        args[3]                         = 0;

        ret = ioctl(fd, DISP_CMD_LAYER_SET_WHITE_EXTEN_LEVEL,args);
    }

    return ret;
}

// for taking photo to avoid preview wrong
static int hwc_getwhiteexten(sun4i_hwc_context_t *ctx)
{
    uint32_t                    overlay;
    int                         fd;
    int                         ret = 0;
    int                         screen;

    ALOGV("hwc_vppon");

    overlay                         = ctx->hwc_layer.currenthandle;
    fd                              = ctx->dispfd;
    screen                          = ctx->hwc_screen;

    if(ctx->hwc_layer.currenthandle)
       {
        args[0]                            = screen;
        args[1]                         = (unsigned long)overlay;
        args[2]                         = 0;
        args[3]                         = 0;

        ret = ioctl(fd, DISP_CMD_LAYER_GET_WHITE_EXTEN_LEVEL,args);
    }

    return ret;
}

// for taking photo to avoid preview wrong
static int hwc_setblackexten(sun4i_hwc_context_t *ctx,int value)
{
    uint32_t                    overlay;
    int                         fd;
    int                         ret = 0;
    int                         screen;

    ALOGV("hwc_vppon");

    overlay                         = ctx->hwc_layer.currenthandle;
    fd                              = ctx->dispfd;
    screen                          = ctx->hwc_screen;

    if(ctx->hwc_layer.currenthandle)
       {
        args[0]                            = screen;
        args[1]                         = (unsigned long)overlay;
        args[2]                         = value;
        args[3]                         = 0;

        ret = ioctl(fd, DISP_CMD_LAYER_SET_BLACK_EXTEN_LEVEL,args);
    }

    return ret;
}

// for taking photo to avoid preview wrong
static int hwc_getblackexten(sun4i_hwc_context_t *ctx)
{
    uint32_t                    overlay;
    int                         fd;
    int                         ret = 0;
    int                         screen;

    ALOGV("hwc_vppon");

    overlay                         = ctx->hwc_layer.currenthandle;
    fd                              = ctx->dispfd;
    screen                          = ctx->hwc_screen;

    if(ctx->hwc_layer.currenthandle)
       {
        args[0]                            = screen;
        args[1]                         = (unsigned long)overlay;
        args[2]                         = 0;
        args[3]                         = 0;

        ret = ioctl(fd, DISP_CMD_LAYER_GET_BLACK_EXTEN_LEVEL,args);
    }

    return ret;
}

static int hwc_set3dmode(sun4i_hwc_context_t *ctx,int para)
{
    int                         ctl_fd;
    void                        *overlay;
    int                         fd;
    int                            handle;
    int                         ret;
    int                         screen;
    int                         value;
    int                         mode;
    int                            is_mode_changed;
    int i;
    __disp_layer_info_t         layer_info;
    layerinitpara_t                layer_para;
    video3Dinfo_t                 *_3d_info;

    ALOGV("overlay_show");

    memset(&layer_info, 0, sizeof(__disp_layer_info_t));

    overlay                         = (void *)ctx->hwc_layer.currenthandle;
    fd                              = ctx->dispfd;
    screen                          = ctx->hwc_screen;
    _3d_info                         = (video3Dinfo_t *)para;
    value                             = _3d_info->_3d_mode;
    mode                              = _3d_info->display_mode;
    is_mode_changed                 = _3d_info->is_mode_changed;

    if(is_mode_changed)
    {
        args[0]                     = screen;
        args[1]                     = (unsigned long) overlay;
        args[2]                     = 0;
        args[3]                     = 0;
        ioctl(fd, DISP_CMD_LAYER_CLOSE,args);
        ioctl(fd, DISP_CMD_VIDEO_STOP, args);
    }
    ALOGV("width %d, height %d, format %x, value %d, mode %d, is mode changed %d", _3d_info->width, _3d_info->height, _3d_info->format, _3d_info->_3d_mode, _3d_info->display_mode, _3d_info->is_mode_changed);

    layer_para.w              = _3d_info->width;
    layer_para.h              = _3d_info->height;
    layer_para.format          = _3d_info->format;
    layer_para.screenid      = screen;
    hwc_setlayerpara(ctx, (uint32_t)&layer_para);

    if(ctx->hwc_layer.currenthandle)
    {
        args[0]                     = screen;
        args[1]                     = (unsigned long) overlay;
        args[2]                         = (unsigned long) (&layer_info);
        args[3]                     = 0;
        ioctl(fd, DISP_CMD_LAYER_GET_PARA, args);
        args[0] = screen;
        ret = ioctl(fd,DISP_CMD_GET_OUTPUT_TYPE,args);
        if(ret == DISP_OUTPUT_TYPE_HDMI)
        {
            ALOGV("value = %d, f_trd_srd = %d, trd_mode = %d, b_trd_out %d", value, layer_info.fb.b_trd_src, layer_info.fb.trd_mode, layer_info.b_trd_out);

            if(mode == HWC_DISP_MODE_3D && value != HWC_3D_OUT_MODE_NORMAL)
            {
                if(layer_info.b_trd_out == false || value != (int)layer_info.fb.trd_mode )
                {
                    if(layer_info.b_trd_out == false){
                        args[0]                     = screen;
                        args[1]                     = 0;
                        args[2]                     = 0;
                        args[3]                     = 0;
                        ctx->cur_hdmimode    = ioctl(fd, DISP_CMD_HDMI_GET_MODE, args);
                    }

                    args[0]                     = screen;
                    args[1]                     = 0;
                    args[2]                     = 0;
                    args[3]                     = 0;
                    ioctl(fd,DISP_CMD_HDMI_OFF,(unsigned long)args);

                    args[0]                     = screen;
                    args[1]                     = DISP_TV_MOD_1080P_24HZ_3D_FP;
                    args[2]                     = 0;
                    args[3]                     = 0;
                    ioctl(fd, DISP_CMD_HDMI_SET_MODE, args);

                    args[0]                     = screen;
                    args[1]                     = 0;
                    args[2]                     = 0;
                    args[3]                     = 0;
                    ioctl(fd,DISP_CMD_HDMI_ON,(unsigned long)args);
                    args[0]                     = screen;
                    args[1]                     = 0;
                    args[2]                     = 0;
                    g_lcd_width                 = ioctl(fd, DISP_CMD_SCN_GET_WIDTH,args);
                    g_lcd_height                = ioctl(fd, DISP_CMD_SCN_GET_HEIGHT,args);

                    ctx->hwc_layer.dispW        = g_lcd_width;
                    ctx->hwc_layer.dispH        = g_lcd_height;
                    layer_info.scn_win.x         = 0;
                    layer_info.scn_win.y         = 0;
                    layer_info.scn_win.width     = g_lcd_width;
                    layer_info.scn_win.height     = g_lcd_height;
                    ALOGV("%d, 3d mode %d, value is %d ", __LINE__, mode, value);

                    layer_info.fb.trd_mode         =  (__disp_3d_src_mode_t)value;
                    layer_info.out_trd_mode        = DISP_3D_OUT_MODE_FP;
                    layer_info.fb.b_trd_src    = true;
                    layer_info.b_trd_out    = true;

                    ctx->cur_3dmode                = value;
                    ctx->cur_3denable            = true;
                    ctx->cur_half_enable        = true;
                    ALOGV("line %d, screen width %d, screen height %d", __LINE__, g_lcd_width, g_lcd_height);

                    args[0]                     = screen;
                    args[1]                     = (unsigned long) overlay;
                    args[2]                     = (unsigned long) (&layer_info);
                    args[3]                     = 0;
                    ioctl(fd, DISP_CMD_LAYER_SET_PARA, args);
                }
            }
            else
            {
                if(value != HWC_3D_OUT_MODE_NORMAL)
                {
                    if(layer_info.b_trd_out == true )
                    {
                        args[0]                     = screen;
                        args[1]                     = 0;
                        args[2]                     = 0;
                        args[3]                     = 0;
                        ioctl(fd,DISP_CMD_HDMI_OFF,(unsigned long)args);

                        args[0]                     = screen;
                        args[1]                     = ctx->cur_hdmimode;
                        args[2]                     = 0;
                        args[3]                     = 0;
                        ioctl(fd, DISP_CMD_HDMI_SET_MODE, args);

                        args[0]                     = screen;
                        args[1]                     = 0;
                        args[2]                     = 0;
                        args[3]                     = 0;
                        ioctl(fd,DISP_CMD_HDMI_ON,(unsigned long)args);

                        args[0]                     = screen;
                        args[1]                     = 0;
                        args[2]                     = 0;
                        g_lcd_width                 = ioctl(fd, DISP_CMD_SCN_GET_WIDTH,args);
                        g_lcd_height                = ioctl(fd, DISP_CMD_SCN_GET_HEIGHT,args);
                        ctx->hwc_layer.dispW          = g_lcd_width;
                        ctx->hwc_layer.dispH          = g_lcd_height;

                        layer_info.scn_win.x         = 0;
                        layer_info.scn_win.y         = 0;
                        layer_info.scn_win.width     = g_lcd_width;
                        layer_info.scn_win.height     = g_lcd_height;
                    }
                    if(mode == HWC_DISP_MODE_2D)
                    {
                        ALOGV("%d, 3d mode with one picture", __LINE__);
                        layer_info.fb.b_trd_src     = true;
                        layer_info.b_trd_out        = false;
                    }
                    else
                    {
                        ALOGV("%d, original mode", __LINE__);
                        layer_info.fb.b_trd_src     = false;
                        layer_info.b_trd_out        = false;
                    }
                    layer_info.fb.trd_mode         = (__disp_3d_src_mode_t)value;
                    layer_info.out_trd_mode        = DISP_3D_OUT_MODE_FP;

                    ctx->cur_3denable            = layer_info.b_trd_out;
                    ctx->cur_half_enable        = layer_info.fb.b_trd_src;
                    ctx->cur_3dmode                = value;

                    ALOGV("line %d, screen width %d, screen height %d", __LINE__, g_lcd_width, g_lcd_height);
                    args[0]                     = screen;
                    args[1]                     = (unsigned long) overlay;
                    args[2]                     = (unsigned long) (&layer_info);
                    args[3]                     = 0;
                    ioctl(fd, DISP_CMD_LAYER_SET_PARA, args);
                }
            }
        }
        else
        {
            layer_info.fb.b_trd_src     = (mode == HWC_DISP_MODE_2D)?true:false;
            layer_info.b_trd_out        = false;
            layer_info.fb.trd_mode         =  (__disp_3d_src_mode_t)value;

            ctx->cur_3dmode                = value;
            ctx->cur_3denable            = false;
            ctx->cur_half_enable        = layer_info.fb.b_trd_src;

            if(mode == HWC_DISP_MODE_3D)
            {
                ctx->cur_3denable       = true;
                ctx->cur_half_enable    = true;
            }

            args[0]                     = 0;
            args[1]                     = (unsigned long) overlay;
            args[2]                     = (unsigned long) (&layer_info);
            args[3]                     = 0;
            ioctl(fd, DISP_CMD_LAYER_SET_PARA, args);
        }
    }

    if(is_mode_changed)
    {
        ctx->wait_layer_open = 1;

        args[0]                     = screen;
        args[1]                     = (unsigned long) overlay;
        args[2]                     = 0;
        args[3]                     = 0;
        ioctl(fd, DISP_CMD_VIDEO_START, args);
    }

    return 0;
}

static int hwc_setparameter(hwc_composer_device_1_t *dev,uint32_t param,uint32_t value)
{
    int                         ret = 0;
    sun4i_hwc_context_t           *ctx = (sun4i_hwc_context_t *)dev;
    int                            ctl_fd;

    ctl_fd   = ctx->dispfd;
    if(param == HWC_LAYER_SETINITPARA)
    {
        ctx->cur_3denable            = false;
        ctx->cur_half_enable        = false;
        ctx->cur_3dmode                = value;
        ret = hwc_setlayerpara(ctx,value);
    }
    else if(param == HWC_LAYER_SETFRAMEPARA)
    {
        //ALOGV("set parameter overlay = %x",(unsigned long)overlay);
        ret = hwc_setlayerframepara(ctx,value);
    }
    else if(param == HWC_LAYER_GETCURFRAMEPARA)
    {
        ret = ioctl(ctl_fd, DISP_CMD_VIDEO_GET_FRAME_ID, args);
        if(ret == -1)
        {
            ret = ctx->hwc_layer.cur_frameid;
            ALOGV("ret = -1 HWC_LAYER_GETCURFRAMEPARA =%d",ret);
        }
        ALOGV("HWC_LAYER_GETCURFRAMEPARA =%d",ret);
    }
    else if(param == HWC_LAYER_SETSCREEN)
    {
        ALOGV("param == HWC_LAYER_SETSCREEN,value = %d\n",value);
        ret = hwc_setscreen(ctx,value);
    }
    else if(param == HWC_LAYER_SHOW)
    {
        ALOGV("param == HWC_LAYER_SHOW,value = %d\n",value);
        ret = hwc_reqshow(ctx,value);
    }
    else if(param == HWC_LAYER_RELEASE)
    {
        ALOGV("param == HWC_LAYER_RELEASE,value = %d\n",value);
        ret = hwc_release(ctx);
    }
    else if(param == HWC_LAYER_SET3DMODE)
    {
        ALOGV("param == HWC_LAYER_SET3DMODE,value = %d\n",value);
        ret = hwc_set3dmode(ctx,value);
    }
    else if(param == HWC_LAYER_SETFORMAT)
    {
        ALOGV("param == HWC_LAYER_SETFORMAT,value = %d\n",value);
        ret = hwc_setformat(ctx,value);
    }
    else if(param == HWC_LAYER_VPPON)
    {
        ALOGV("param == HWC_LAYER_SETFORMAT,value = %d\n",value);
        ret = hwc_vppon(ctx,value);
    }
    else if(param == HWC_LAYER_VPPGETON)
    {
        ALOGV("param == HWC_LAYER_VPPGETON,value = %d\n",value);
        ret = hwc_getvppon(ctx);
    }
    else if(param == HWC_LAYER_SETLUMASHARP)
    {
        ALOGV("param == HWC_LAYER_SETLUMASHARP,value = %d\n",value);
        ret = hwc_setlumasharp(ctx,value);
    }
    else if(param == HWC_LAYER_GETLUMASHARP)
    {
        ALOGV("param == HWC_LAYER_GETLUMASHARP,value = %d\n",value);
        ret = hwc_getlumasharp(ctx);
    }
    else if(param == HWC_LAYER_SETCHROMASHARP)
    {
        ALOGV("param == HWC_LAYER_SETCHROMASHARP,value = %d\n",value);
        ret = hwc_setchromasharp(ctx,value);
    }
    else if(param == HWC_LAYER_GETCHROMASHARP)
    {
        ALOGV("param == HWC_LAYER_GETCHROMASHARP,value = %d\n",value);
        ret = hwc_getchromasharp(ctx);
    }
    else if(param == HWC_LAYER_SETWHITEEXTEN)
    {
        ALOGV("param == HWC_LAYER_SETWHITEEXTEN,value = %d\n",value);
        ret = hwc_setwhiteexten(ctx,value);
    }
    else if(param == HWC_LAYER_GETWHITEEXTEN)
    {
        ALOGV("param == HWC_LAYER_GETWHITEEXTEN,value = %d\n",value);
        ret = hwc_getwhiteexten(ctx);
    }
    else if(param == HWC_LAYER_SETBLACKEXTEN)
    {
        ALOGV("param == HWC_LAYER_SETBLACKEXTEN,value = %d\n",value);
        ret = hwc_setblackexten(ctx,value);
    }
    else if(param == HWC_LAYER_GETBLACKEXTEN)
    {
        ALOGV("param == HWC_LAYER_GETBLACKEXTEN,value = %d\n",value);
        ret = hwc_getblackexten(ctx);
    }

    return ( ret );
}

static uint32_t hwc_getparameter(hwc_composer_device_1_t *dev,uint32_t cmd)
{
    return  0;
}

static int hwc_set_layer(hwc_composer_device_1_t *dev, hwc_display_contents_1_t* list)
{
    int                         ret = 0;
    sun4i_hwc_context_t           *ctx = (sun4i_hwc_context_t *)dev;
    bool                        findoverlay = false;

    //ALOGV("hwc_set_layer list->numHwLayers = %d\n",list->numHwLayers);

    for (size_t i=0 ; i<list->numHwLayers ; i++)
    {
        if(list->hwLayers[i].compositionType == HWC_OVERLAY)
        {
            ret = hwc_setrect(ctx,&list->hwLayers[i].sourceCrop,&list->hwLayers[i].displayFrame);

            findoverlay = true;
        }
    }

    if(!findoverlay)
    {
        hwc_show(ctx,0);
    }

    return ret;
}

static int hwc_set(hwc_composer_device_1_t *dev,
        size_t numDisplays,
        hwc_display_contents_1_t** lists)
{
    hwc_display_contents_1_t* list = lists[0];
    //for (size_t i=0 ; i<list->numHwLayers ; i++) {
    //    dump_layer(&list->hwLayers[i]);
    //}
    EGLBoolean sucess = eglSwapBuffers((EGLDisplay)list->dpy, (EGLSurface)list->sur);
    if (unlikely(!sucess))
        return HWC_EGL_ERROR;

    //don't continue if layer list is NULL
    if (unlikely(list == NULL))
        return 0;

    return hwc_set_layer(dev,list);
}

static int hwc_device_close(struct hw_device_t *dev)
{
    sun4i_hwc_context_t* ctx = (sun4i_hwc_context_t*)dev;
    int ret;
    if (ctx)
    {
        if(ctx->hwc_layer.currenthandle)
        {
            args[0] = ctx->hwc_screen;
            args[1] = (unsigned long)ctx->hwc_layer.currenthandle;
            args[2] = 0;
            args[3] = 0;

            args[0]                         = ctx->hwc_screen;
            args[1]                         = fb_layer_hdl;
            ioctl(ctx->dispfd,DISP_CMD_LAYER_ALPHA_ON,(void*)args);//disable the global alpha, use the pixel's alpha

            ioctl(ctx->dispfd, DISP_CMD_VIDEO_STOP, args);
            ioctl(ctx->dispfd, DISP_CMD_LAYER_RELEASE,args);
        }
        args[0]                         = ctx->hwc_screen;
        ret = ioctl(ctx->dispfd,DISP_CMD_GET_OUTPUT_TYPE,args);
        if(ret == DISP_OUTPUT_TYPE_HDMI && (ctx->cur_3denable == true))
        {
            args[0]                     = ctx->hwc_screen;
            args[1]                     = 0;
            args[2]                     = 0;
            args[3]                     = 0;
            ioctl(ctx->dispfd,DISP_CMD_HDMI_OFF,(unsigned long)args);

            ALOGV("ctx->cur_hdmimode %d",ctx->cur_hdmimode);
            args[0]                     = ctx->hwc_screen;
            args[1]                     = ctx->cur_hdmimode;
            args[2]                     = 0;
            args[3]                     = 0;
            ioctl(ctx->dispfd, DISP_CMD_HDMI_SET_MODE, args);

            args[0]                     = ctx->hwc_screen;
            args[1]                     = 0;
            args[2]                     = 0;
            args[3]                     = 0;
            ioctl(ctx->dispfd,DISP_CMD_HDMI_ON,(unsigned long)args);
        }

        if(ctx->dispfd)
        {
            close(ctx->dispfd);
        }
        free(ctx);
    }
    return 0;
}

static void *hwc_vsync_thread(void *data)
{
    int arg;
    #define HZ 90
    struct timespec ts = {0, 1000000000/HZ}, tt = {0, 0};
    hwc_context_t *ctx = (hwc_context_t *)data;
    int fb = open("/dev/graphics/fb0", O_RDWR);
    if(fb < 0) {
        ALOGE("failed to open fb0\n");
        return NULL;
    }

    setpriority(PRIO_PROCESS, 0, HAL_PRIORITY_URGENT_DISPLAY);

    while (true) {
        arg = 0;
        //ioctl(fb, FBIO_WAITFORVSYNC, &arg);
        if (nanosleep(&ts, &tt) == -1 && errno == EINTR)
            nanosleep(&tt, NULL);

        if (ctx->vsync_enabled)
            ctx->procs->vsync(ctx->procs, 0, time(NULL));
    }

    close(fb);

    return NULL;
}

static int hwc_blank(hwc_composer_device_1* a, int b, int c)
{
    /* STUB */
    return 0;
}

static int hwc_eventControl(hwc_composer_device_1* dev, int dpy, int event,
                            int enabled)
{
    struct hwc_context_t* ctx = (sun4i_hwc_context_t*) dev;

    switch (event) {
    case HWC_EVENT_VSYNC:
        ctx->vsync_enabled = !!enabled;
        return 0;
    }
    return -EINVAL;
}

static void hwc_registerProcs(struct hwc_composer_device_1* dev,
        hwc_procs_t const* procs)
{
    struct hwc_context_t* ctx = (struct hwc_context_t*)dev;
    ctx->procs = const_cast<hwc_procs_t *>(procs);
}

/*****************************************************************************/

static int hwc_device_open(const struct hw_module_t* module, const char* name,
        struct hw_device_t** device)
{
    int status = -EINVAL;
    if (!strcmp(name, HWC_HARDWARE_COMPOSER))
    {
        sun4i_hwc_context_t *dev;
        dev = (sun4i_hwc_context_t*)malloc(sizeof(*dev));

        /* initialize our state here */
        memset(dev, 0, sizeof(*dev));

        /* initialize the procs */
        dev->device.common.tag      = HARDWARE_DEVICE_TAG;
        dev->device.common.version  = HWC_DEVICE_API_VERSION_1_0;
        dev->device.common.module   = const_cast<hw_module_t*>(module);
        dev->device.common.close    = hwc_device_close;

        dev->device.prepare         = hwc_prepare;
        dev->device.set             = hwc_set;
        dev->device.blank           = hwc_blank;
        dev->device.eventControl    = hwc_eventControl;
        dev->device.registerProcs   = hwc_registerProcs;
        dev->device.setparameter    = hwc_setparameter;
        dev->device.getparameter    = hwc_getparameter;

        *device = &dev->device.common;

        dev->vsync_enabled = false;
        pthread_create(&dev->vsync_thread, NULL, hwc_vsync_thread, dev);

        status = 0;
    }
    return status;
}
