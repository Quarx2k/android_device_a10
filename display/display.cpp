/*
 * Copyright (C) 2008 The Android Open Source Project
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


#define LOG_TAG "display"

#include <cutils/log.h>

#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <asm/page.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/mman.h>

#include <hardware/display.h>
#include <drv_display_sun4i.h>
#include <g2d_driver.h>
#include <fb.h>

#define MAX_DISPLAY_NUM		2
#define DEBUG_MDP_ERRORS 	1

#define LOG_NDEBUG          0

int                         g_displaymode = 0;
int                         g_masterdisplay = 0;
struct display_output_t     g_display[MAX_DISPLAY_NUM];
pthread_mutex_t             mode_lock;
bool                        mutex_inited = false;
/** State information for each device instance */
struct display_context_t 
{
    struct display_device_t     device;
    int                         mFD_fb[MAX_DISPLAY_NUM];
    int		                    mFD_disp;
    int                         mFD_mp;
};

struct display_fbpara_t
{
	__fb_mode_t 				fb_mode;
    __disp_layer_work_mode_t 	layer_mode;
    int 						width;
    int 						height;
    int                         output_width;
    int                         output_height;
    int							valid_width;
    int							valid_height;
    int                         bufno;
    int							format;
};

/**
 * Common hardware methods
 */

static int open_display(const struct hw_module_t* module, const char* name,
        struct hw_device_t** device);

static struct hw_module_methods_t display_module_methods = 
{
    open:  open_display
};

/*
 * The DISPLAY Module
 */
struct display_module_t HAL_MODULE_INFO_SYM = 
{
    common: 
    {
        tag: HARDWARE_MODULE_TAG,
        version_major: 1,
        version_minor: 0,
        id: DISPLAY_HARDWARE_MODULE_ID,
        name: "Crane DISPLAY Module",
        author: "Google, Inc.",
        methods: &display_module_methods
    }
};

      
/*
**********************************************************************************************************************
*                                               display_gethdmistatus
*
* author:           
*
* date:             2011-7-17:11:22:0
*
* Description:      get hdmi hot plug status 
*
* parameters:       
*
* return:           if success return GUI_RET_OK
*                   if fail return the number of fail
* modify history: 
**********************************************************************************************************************
*/
static int display_gethdmistatus(struct display_device_t *dev)
{
    struct display_context_t* ctx = (struct display_context_t*)dev;
    
    if(ctx)
    {
        if(ctx->mFD_disp)
        {
        	unsigned long args[4];
        	
        	args[0] = 0;
        	
            return ioctl(ctx->mFD_disp,DISP_CMD_HDMI_GET_HPD_STATUS,args);
        }
    }

    return 0;    
}

static int display_gethdmimaxmode(struct display_device_t *dev)
{
    struct display_context_t* ctx = (struct display_context_t*)dev;
    
    if(ctx)
    {
        if(ctx->mFD_disp)
        {
        	unsigned long args[4];
        	
        	args[0] = 0;
        	args[1] = DISP_TV_MOD_1080P_60HZ;

            if(ioctl(ctx->mFD_disp,DISP_CMD_HDMI_SUPPORT_MODE,args))
            {
                return DISPLAY_TVFORMAT_1080P_60HZ;
            }
            else 
            {
            	args[1] = DISP_TV_MOD_1080P_50HZ;

	            if(ioctl(ctx->mFD_disp,DISP_CMD_HDMI_SUPPORT_MODE,args))
	            {
	                return DISPLAY_TVFORMAT_1080P_50HZ;
	            }
	            else 
	            {
	            	args[1] = DISPLAY_TVFORMAT_720P_60HZ;

		            if(ioctl(ctx->mFD_disp,DISP_CMD_HDMI_SUPPORT_MODE,args))
		            {
		                return DISPLAY_TVFORMAT_720P_60HZ;
		            }
		            else
		            {
		            	args[1] = DISP_TV_MOD_720P_50HZ;

			            if(ioctl(ctx->mFD_disp,DISP_CMD_HDMI_SUPPORT_MODE,args))
			            {
			                return DISPLAY_TVFORMAT_720P_50HZ;
			            }
			            else
			            {
			            	args[1] = DISP_TV_MOD_1080I_60HZ;

				            if(ioctl(ctx->mFD_disp,DISP_CMD_HDMI_SUPPORT_MODE,args))
				            {
				                return DISPLAY_TVFORMAT_1080I_60HZ;
				            }
				            else
				            {
				            	args[1] = DISP_TV_MOD_1080I_50HZ;

					            if(ioctl(ctx->mFD_disp,DISP_CMD_HDMI_SUPPORT_MODE,args))
					            {
					                return DISPLAY_TVFORMAT_1080I_50HZ;
					            }
					            else
					            {
					            	return DISPLAY_TVFORMAT_720P_50HZ;
					            }
				            }
			            }
		            }
	            }
            }
        }
    }

    return DISPLAY_TVFORMAT_720P_50HZ;    
}      

/*
**********************************************************************************************************************
*                                               display_getoutputtype
*
* author:           
*
* date:             2011-7-17:11:22:29
*
* Description:      获取屏幕output type
*
* parameters:       
*
* return:           if success return GUI_RET_OK
*                   if fail return the number of fail
* modify history: 
**********************************************************************************************************************
*/

static int display_getoutputtype(struct display_device_t *dev,int displayno)
{
    struct display_context_t* ctx = (struct display_context_t*)dev;
    int                       ret;

    if(ctx)
    {
        if(ctx->mFD_disp)
        {
            unsigned long args[4];

            args[0] = displayno;
            ret = ioctl(ctx->mFD_disp,DISP_CMD_GET_OUTPUT_TYPE,args);
            if(ret == DISP_OUTPUT_TYPE_LCD)
            {
                return  DISPLAY_DEVICE_LCD;
            }
            else if(ret == DISP_OUTPUT_TYPE_HDMI)
            {
                return  DISPLAY_DEVICE_HDMI;
            }
        }
    }

    return  DISPLAY_DEVICE_NONE;
}
    
/*
**********************************************************************************************************************
*                                               display_gethotplug
*
* author:           
*
* date:             2011-7-17:11:22:44
*
* Description:      获取显示屏的热插拔状态 
*
* parameters:       
*
* return:           if success return GUI_RET_OK
*                   if fail return the number of fail
* modify history: 
**********************************************************************************************************************
*/

static int display_gethotplug(struct display_device_t *dev,int displayno)
{
    if(g_display[displayno].type == DISPLAY_DEVICE_HDMI)
    {
        g_display[displayno].hotplug    = display_gethdmistatus(dev);
    }

    return  0;
}

static int get_g2dpixelformat(int red_size,int red_offset,
                              int green_size,int green_offset,
                              int blue_size,int blue_offset,
                              int alpha_size,int alpha_offset)
{
    if((red_size == 8) && (red_offset = 0)
       && (green_size == 8) && (green_offset = 8)
       && (blue_size == 8) && (blue_offset = 16)
       && (alpha_size == 8) && (alpha_offset = 24))
    {
        return G2D_FMT_ABGR_AVUY8888;
    }
    else if((red_size == 8) && (red_offset = 16)
       && (green_size == 8) && (green_offset = 8)
       && (blue_size == 8) && (blue_offset = 0)
       && (alpha_size == 8) && (alpha_offset = 24))
    {
        return G2D_FMT_ARGB_AYUV8888;
    }
    else if((red_size == 8) && (red_offset = 8)
       && (green_size == 8) && (green_offset = 16)
       && (blue_size == 8) && (blue_offset = 24)
       && (alpha_size == 8) && (alpha_offset = 0))
    {
        return G2D_FMT_BGRA_VUYA8888;
    }
    else if((red_size == 8) && (red_offset = 24)
       && (green_size == 8) && (green_offset = 16)
       && (blue_size == 8) && (blue_offset = 8)
       && (alpha_size == 8) && (alpha_offset = 0))
    {
        return G2D_FMT_RGBA_YUVA8888;
    }
    else if((red_size == 8) && (red_offset = 16)
       && (green_size == 8) && (green_offset = 8)
       && (blue_size == 8) && (blue_offset = 0)
       && (alpha_size == 0) && (alpha_offset = 24))
    {
        return G2D_FMT_XRGB8888;
    }
    else if((red_size == 8) && (red_offset = 8)
       && (green_size == 8) && (green_offset = 16)
       && (blue_size == 8) && (blue_offset = 24)
       && (alpha_size == 0) && (alpha_offset = 0))
    {
        return G2D_FMT_BGRX8888;
    }
    else if((red_size == 8) && (red_offset = 0)
       && (green_size == 8) && (green_offset = 8)
       && (blue_size == 8) && (blue_offset = 16)
       && (alpha_size == 0) && (alpha_offset = 24))
    {
        return G2D_FMT_XBGR8888;
    }
    else if((red_size == 8) && (red_offset = 24)
       && (green_size == 8) && (green_offset = 16)
       && (blue_size == 8) && (blue_offset = 8)
       && (alpha_size == 0) && (alpha_offset = 0))
    {
        return G2D_FMT_RGBX8888;
    }
    else if((red_size == 5) && (red_offset = 11)
       && (green_size == 6) && (green_offset = 5)
       && (blue_size == 5) && (blue_offset = 0))
    {
        return G2D_FMT_RGB565;
    }
    else if((red_size == 5) && (red_offset = 0)
       && (green_size == 6) && (green_offset = 5)
       && (blue_size == 5) && (blue_offset = 11))
    {
        return G2D_FMT_BGR565;
    }
    else 
    {
        return G2D_FMT_RGBA_YUVA8888;
    }
}
      
/*
**********************************************************************************************************************
*                                               display_copyfb
*
* author:           
*
* date:             2011-7-17:11:22:54
*
* Description:      copy from src fb to dst fb 
*
* parameters:       
*
* return:           if success return GUI_RET_OK
*                   if fail return the number of fail
* modify history: 
**********************************************************************************************************************
*/

static int display_copyfb(struct display_device_t *dev,int srcfb_id,int srcfb_bufno,
                          int dstfb_id,int dstfb_bufno)
{
    struct 	display_context_t*  ctx = (struct display_context_t*)dev;
    
	struct fb_fix_screeninfo    fix_src;
    struct fb_fix_screeninfo    fix_dst;
    struct fb_var_screeninfo    var_src;
    struct fb_var_screeninfo    var_dst;
    char               			node_src[20];
    char               			node_dst[20];
    unsigned int                src_width;
    unsigned int                src_height;
    unsigned int                dst_width;
    unsigned int                dst_height;
    unsigned int                addr_src;
    unsigned int                addr_dst;
    unsigned int                size;
    g2d_stretchblt              blit_para;
    int                         err;
    
    sprintf(node_src, "/dev/graphics/fb%d", srcfb_id);

    if(ctx->mFD_fb[srcfb_id] == 0)
    {
    	ctx->mFD_fb[srcfb_id]			= open(node_src,O_RDWR,0);
    	if(ctx->mFD_fb[srcfb_id] <= 0)
    	{
    		LOGE("open fb%d fail!\n",srcfb_id);
    		
    		ctx->mFD_fb[srcfb_id]		= 0;
    		
    		return  -1;
    	}
	}

    sprintf(node_dst, "/dev/graphics/fb%d", dstfb_id);

    if(ctx->mFD_fb[dstfb_id] == 0)
    {
    	ctx->mFD_fb[dstfb_id]			= open(node_dst,O_RDWR,0);
    	if(ctx->mFD_fb[dstfb_id] <= 0)
    	{
    		LOGE("open fb%d fail!\n",dstfb_id);
    		
    		ctx->mFD_fb[dstfb_id]		= 0;
    		
    		return  -1;
    	}
	}

    if(ctx->mFD_mp == 0)
    {
        ctx->mFD_mp                     = open("/dev/g2d", O_RDWR, 0);
        if(ctx->mFD_mp < 0)
        {
            LOGE("open g2d driver fail!\n");
    		
    		ctx->mFD_mp		= 0;

            return -1;
        }
    }
    
	ioctl(ctx->mFD_fb[srcfb_id],FBIOGET_FSCREENINFO,&fix_src);
	ioctl(ctx->mFD_fb[srcfb_id],FBIOGET_VSCREENINFO,&var_src);
	ioctl(ctx->mFD_fb[dstfb_id],FBIOGET_FSCREENINFO,&fix_dst);
	ioctl(ctx->mFD_fb[dstfb_id],FBIOGET_VSCREENINFO,&var_dst);
	
	src_width   = var_src.xres;
	src_height  = var_src.yres;
    dst_width   = var_dst.xres;
    dst_height  = var_dst.yres;
    
    //LOGD("src_width = %d\n",src_width);
    //LOGD("src_height = %d\n",src_height);
    //LOGD("dst_width = %d\n",dst_width);
    //LOGD("dst_height = %d\n",dst_height);
    
	addr_src = fix_src.smem_start + ((var_src.xres * (srcfb_bufno * var_src.yres) * var_src.bits_per_pixel) >> 3);
	addr_dst = fix_dst.smem_start + ((var_dst.xres * (dstfb_bufno * var_dst.yres) * var_dst.bits_per_pixel) >> 3);
	size = (var_src.xres * var_src.yres * var_src.bits_per_pixel) >> 3;//in byte unit
	
	//LOGD("addr_src = %x\n",addr_src);
    //LOGD("addr_dst = %x\n",addr_dst);
    //LOGD("size = %d\n",size);
	switch (var_src.bits_per_pixel) 
	{			
    	case 16:
    		blit_para.src_image.format      = G2D_FMT_RGB565;
    		break;
    		
    	case 24:
    		blit_para.src_image.format      = G2D_FMT_RGBA_YUVA8888;
    		break;
    		
    	case 32:
    		blit_para.src_image.format      = G2D_FMT_RGBA_YUVA8888;
    		break;
    		
    	default:
    	    LOGE("invalid bits_per_pixel :%d\n", var_src.bits_per_pixel);
    		return -1;
	}

    blit_para.src_image.addr[0]     = addr_src;
    blit_para.src_image.addr[1]     = 0;
    blit_para.src_image.addr[2]     = 0;
    blit_para.src_image.format      = G2D_FMT_ARGB_AYUV8888;
    blit_para.src_image.h           = src_height;
    blit_para.src_image.w           = src_width;
    blit_para.src_image.pixel_seq   = G2D_SEQ_VYUY;

    blit_para.dst_image.addr[0]     = addr_dst;
    blit_para.dst_image.addr[1]     = 0;
    blit_para.dst_image.addr[2]     = 0;
    blit_para.dst_image.format      = G2D_FMT_ARGB_AYUV8888;
    blit_para.dst_image.h           = dst_height;
    blit_para.dst_image.w           = dst_width;
    blit_para.dst_image.pixel_seq   = G2D_SEQ_VYUY;

    //blit_para.dst_x                 = 0;
    //blit_para.dst_y                 = 0;
    blit_para.dst_rect.x            = 0;
    blit_para.dst_rect.y            = 0;
    blit_para.dst_rect.w            = dst_width;
    blit_para.dst_rect.h            = dst_height;

    blit_para.src_rect.x            = 0;
    blit_para.src_rect.y            = 0;
    blit_para.src_rect.w            = src_width;
    blit_para.src_rect.h            = src_height;

    blit_para.flag                 = G2D_BLT_NONE;
			
    err = ioctl(ctx->mFD_mp , G2D_CMD_STRETCHBLT ,(unsigned long)&blit_para);				
    if(err < 0)		
    {    
        LOGE("copy fb failed!\n");
        
        return  -1;      
    }

    return  0;
}

/*
**********************************************************************************************************************
*                                               display_copyfb
*
* author:           
*
* date:             2011-7-17:11:22:54
*
* Description:      copy from src fb to dst fb 
*
* parameters:       
*
* return:           if success return GUI_RET_OK
*                   if fail return the number of fail
* modify history: 
**********************************************************************************************************************
*/

static int display_copyfbsoft(struct display_device_t *dev,int srcfb_id,int srcfb_bufno,
                          int dstfb_id,int dstfb_bufno)
{
    struct 	display_context_t*  ctx = (struct display_context_t*)dev;
    
	struct fb_fix_screeninfo    fix_src;
    struct fb_fix_screeninfo    fix_dst;
    struct fb_var_screeninfo    var_src;
    struct fb_var_screeninfo    var_dst;
    char               			node_src[20];
    char               			node_dst[20];
    unsigned int                src_width;
    unsigned int                src_height;
    unsigned int                dst_width;
    unsigned int                dst_height;
    unsigned int                addr_src;
    unsigned int                addr_dst;
    unsigned int                size;
    g2d_stretchblt              blit_para;
    int                         err;
    
    sprintf(node_src, "/dev/graphics/fb%d", srcfb_id);

    if(ctx->mFD_fb[srcfb_id] == 0)
    {
    	ctx->mFD_fb[srcfb_id]			= open(node_src,O_RDWR,0);
    	if(ctx->mFD_fb[srcfb_id] <= 0)
    	{
    		LOGE("open fb%d fail!\n",srcfb_id);
    		
    		ctx->mFD_fb[srcfb_id]		= 0;
    		
    		return  -1;
    	}
	}

    sprintf(node_dst, "/dev/graphics/fb%d", dstfb_id);

    if(ctx->mFD_fb[dstfb_id] == 0)
    {
    	ctx->mFD_fb[dstfb_id]			= open(node_dst,O_RDWR,0);
    	if(ctx->mFD_fb[dstfb_id] <= 0)
    	{
    		LOGE("open fb%d fail!\n",dstfb_id);
    		
    		ctx->mFD_fb[dstfb_id]		= 0;
    		
    		return  -1;
    	}
	}
    
	ioctl(ctx->mFD_fb[srcfb_id],FBIOGET_FSCREENINFO,&fix_src);
	ioctl(ctx->mFD_fb[srcfb_id],FBIOGET_VSCREENINFO,&var_src);
	ioctl(ctx->mFD_fb[dstfb_id],FBIOGET_FSCREENINFO,&fix_dst);
	ioctl(ctx->mFD_fb[dstfb_id],FBIOGET_VSCREENINFO,&var_dst);
	
	src_width   = var_src.xres;
	src_height  = var_src.yres;
    dst_width   = var_dst.xres;
    dst_height  = var_dst.yres;
    
    //LOGD("src_width = %d\n",src_width);
    //LOGD("src_height = %d\n",src_height);
    //LOGD("dst_width = %d\n",dst_width);
    //LOGD("dst_height = %d\n",dst_height);
    
	addr_src = fix_src.smem_start + ((var_src.xres * (srcfb_bufno * var_src.yres) * var_src.bits_per_pixel) >> 3);
	addr_dst = fix_dst.smem_start + ((var_dst.xres * (dstfb_bufno * var_dst.yres) * var_dst.bits_per_pixel) >> 3);
	size = (var_src.xres * var_src.yres * var_src.bits_per_pixel) >> 3;//in byte unit
	
	//LOGD("addr_src = %x\n",addr_src);
    //LOGD("addr_dst = %x\n",addr_dst);
    //LOGD("size = %d\n",size);
	switch (var_src.bits_per_pixel) 
	{			
    	case 16:
    		blit_para.src_image.format      = G2D_FMT_RGB565;
    		break;
    		
    	case 24:
    		blit_para.src_image.format      = G2D_FMT_RGBA_YUVA8888;
    		break;
    		
    	case 32:
    		blit_para.src_image.format      = G2D_FMT_RGBA_YUVA8888;
    		break;
    		
    	default:
    	    LOGE("invalid bits_per_pixel :%d\n", var_src.bits_per_pixel);
    		return -1;
	}

    blit_para.src_image.addr[0]     = addr_src;
    blit_para.src_image.addr[1]     = 0;
    blit_para.src_image.addr[2]     = 0;
    blit_para.src_image.format      = G2D_FMT_ARGB_AYUV8888;
    blit_para.src_image.h           = src_height;
    blit_para.src_image.w           = src_width;
    blit_para.src_image.pixel_seq   = G2D_SEQ_VYUY;

    blit_para.dst_image.addr[0]     = addr_dst;
    blit_para.dst_image.addr[1]     = 0;
    blit_para.dst_image.addr[2]     = 0;
    blit_para.dst_image.format      = G2D_FMT_ARGB_AYUV8888;
    blit_para.dst_image.h           = dst_height;
    blit_para.dst_image.w           = dst_width;
    blit_para.dst_image.pixel_seq   = G2D_SEQ_VYUY;

    //blit_para.dst_x                 = 0;
    //blit_para.dst_y                 = 0;
    blit_para.dst_rect.x            = 0;
    blit_para.dst_rect.y            = 0;
    blit_para.dst_rect.w            = dst_width;
    blit_para.dst_rect.h            = dst_height;

    blit_para.src_rect.x            = 0;
    blit_para.src_rect.y            = 0;
    blit_para.src_rect.w            = src_width;
    blit_para.src_rect.h            = src_height;

    blit_para.flag                 = G2D_BLT_NONE;
			
    err = ioctl(ctx->mFD_mp , G2D_CMD_STRETCHBLT ,(unsigned long)&blit_para);			
    if(err < 0)		
    {    
        LOGE("copy fb failed!\n");
        
        return  -1;      
    }

    return  0;
}
      
/*
**********************************************************************************************************************
*                                               display_pandisplay
*
* author:           
*
* date:             2011-7-17:11:22:58
*
* Description:      将FB num为fb_id的bufno显示出来 
*
* parameters:       
*
* return:           if success return GUI_RET_OK
*                   if fail return the number of fail
* modify history: 
**********************************************************************************************************************
*/

static int display_pandisplay(struct display_device_t *dev,int fb_id,int bufno)
{
    struct 	display_context_t*  ctx = (struct display_context_t*)dev;
    struct fb_var_screeninfo    var;
    char               node[20];
    
    sprintf(node, "/dev/graphics/fb%d", fb_id);
    
    if(ctx->mFD_fb[fb_id] == 0)
    {
    	ctx->mFD_fb[fb_id]			= open(node,O_RDWR,0);
    	if(ctx->mFD_fb[fb_id] <= 0)
    	{
    		LOGE("open fb%d fail!\n",fb_id);
    		
    		ctx->mFD_fb[fb_id]		= 0;
    		
    		return  -1;
    	}
	}
		
	ioctl(ctx->mFD_fb[fb_id],FBIOGET_VSCREENINFO,&var);
	var.yoffset = bufno * var.yres;
	//LOGD("fb_id = %d,var.yoffset = %d\n",fb_id,var.yoffset);
	ioctl(ctx->mFD_fb[fb_id],FBIOPAN_DISPLAY,&var);

    return 0;
}
  
/*
**********************************************************************************************************************
*                                               get_tvformat
*
* author:           
*
* date:             2011-7-17:11:23:17
*
* Description:      convert DISPLAY_FORMAT to DRIVER format 
*
* parameters:       
*
* return:           if success return GUI_RET_OK
*                   if fail return the number of fail
* modify history: 
**********************************************************************************************************************
*/

static int get_tvformat(int format) 
{
    switch (format) 
    {
	    case DISPLAY_TVFORMAT_480I:       				return DISP_TV_MOD_480I;           
	    case DISPLAY_TVFORMAT_480P:     				return DISP_TV_MOD_480P;    	    
	    case DISPLAY_TVFORMAT_576I:     				return DISP_TV_MOD_576I;     	    
	    case DISPLAY_TVFORMAT_576P:  					return DISP_TV_MOD_576P;  		    
	    case DISPLAY_TVFORMAT_720P_50HZ:  				return DISP_TV_MOD_720P_50HZ;      
	    case DISPLAY_TVFORMAT_720P_60HZ:       			return DISP_TV_MOD_720P_60HZ;      
	    case DISPLAY_TVFORMAT_1080I_50HZ:     			return DISP_TV_MOD_1080I_50HZ;     
	    case DISPLAY_TVFORMAT_1080I_60HZ:       		return DISP_TV_MOD_1080I_60HZ;     
	    case DISPLAY_TVFORMAT_1080P_50HZ:     			return DISP_TV_MOD_1080P_50HZ;     
	    case DISPLAY_TVFORMAT_1080P_60HZ:     			return DISP_TV_MOD_1080P_60HZ;     
	    case DISPLAY_TVFORMAT_1080P_24HZ:  				return DISP_TV_MOD_1080P_24HZ;   
	    case DISPLAY_TVFORMAT_NTSC:       				return DISP_TV_MOD_NTSC;  
	    case DISPLAY_TVFORMAT_NTSC_SVIDEO:      		return DISP_TV_MOD_NTSC_SVIDEO;   
		case DISPLAY_TVFORMAT_PAL:       				return DISP_TV_MOD_PAL;  
	    case DISPLAY_TVFORMAT_PAL_SVIDEO:      			return DISP_TV_MOD_PAL_SVIDEO;   	 
		case DISPLAY_TVFORMAT_PAL_M:     				return DISP_TV_MOD_PAL_M;    	 
		case DISPLAY_TVFORMAT_PAL_M_SVIDEO:     		return DISP_TV_MOD_PAL_M_SVIDEO;     	
		case DISPLAY_TVFORMAT_PAL_NC:     				return DISP_TV_MOD_PAL_NC;    	 
		case DISPLAY_TVFORMAT_PAL_NC_SVIDEO:     		return DISP_TV_MOD_PAL_NC_SVIDEO; 
		case DISPLAY_VGA_H1680_V1050:					return DISP_VGA_H1680_V1050;   
	    case DISPLAY_VGA_H1440_V900:                    return DISP_VGA_H1440_V900;    
	    case DISPLAY_VGA_H1360_V768:                    return DISP_VGA_H1360_V768;    
	    case DISPLAY_VGA_H1280_V1024:                   return DISP_VGA_H1280_V1024;  
	    case DISPLAY_VGA_H1024_V768:                    return DISP_VGA_H1024_V768;    
	    case DISPLAY_VGA_H800_V600:                     return DISP_VGA_H800_V600;     
	    case DISPLAY_VGA_H640_V480:                     return DISP_VGA_H640_V480;    
	    case DISPLAY_VGA_H1440_V900_RB:                 return DISP_VGA_H1440_V900_RB;
	    case DISPLAY_VGA_H1680_V1050_RB:                return DISP_VGA_H1680_V1050_RB;
	    case DISPLAY_VGA_H1920_V1080_RB:                return DISP_VGA_H1920_V1080_RB;
	    case DISPLAY_VGA_H1920_V1080:                   return DISP_VGA_H1920_V1080;   
	    case DISPLAY_VGA_H1280_V720:                    return DISP_VGA_H1280_V720;    
		default:										break;  
	 
    }       
    return -1;
} 


static int get_pixelformat(int format) 
{
    switch (format) 
    {
	    case HAL_PIXEL_FORMAT_RGBA_8888:       			return DISP_FORMAT_ARGB8888;           
	    case HAL_PIXEL_FORMAT_BGRA_8888:     			return DISP_FORMAT_ARGB8888;    	    
	    case HAL_PIXEL_FORMAT_RGBA_4444:     			return DISP_FORMAT_ARGB8888;     	    
	    case HAL_PIXEL_FORMAT_RGBX_8888:  				return DISP_FORMAT_ARGB8888;  		    
	    case HAL_PIXEL_FORMAT_RGB_565:  				return DISP_FORMAT_ARGB8888;         
		default:										return DISP_FORMAT_ARGB8888;  
	 
    }       
    return -1;
}  
         
/*
**********************************************************************************************************************
*                                               display_getwidth
*
* author:           
*
* date:             2011-7-17:11:23:36
*
* Description:      convert DISPLAY_FORMAT to DRIVER format 
*
* parameters:       
*
* return:           if success return GUI_RET_OK
*                   if fail return the number of fail
* modify history: 
**********************************************************************************************************************
*/

static int  display_getwidth(struct display_context_t* ctx,int displayno,int format)
{
    switch (format) 
    {
	    case DISPLAY_TVFORMAT_480I:       				return 720;           
	    case DISPLAY_TVFORMAT_480P:     				return 720;    	    
	    case DISPLAY_TVFORMAT_576I:     				return 720;     	    
	    case DISPLAY_TVFORMAT_576P:  					return 720;  		    
	    case DISPLAY_TVFORMAT_720P_50HZ:  				return 1280;      
	    case DISPLAY_TVFORMAT_720P_60HZ:       			return 1280;      
	    case DISPLAY_TVFORMAT_1080I_50HZ:     			return 1920;     
	    case DISPLAY_TVFORMAT_1080I_60HZ:       		return 1920;     
	    case DISPLAY_TVFORMAT_1080P_50HZ:     			return 1920;     
	    case DISPLAY_TVFORMAT_1080P_60HZ:     			return 1920;     
	    case DISPLAY_TVFORMAT_1080P_24HZ:  				return 1920;   
	    case DISPLAY_TVFORMAT_NTSC:       				return 720;  
	    case DISPLAY_TVFORMAT_NTSC_SVIDEO:      		return 720;   	       
		case DISPLAY_TVFORMAT_NTSC_CVBS_SVIDEO: 		return 720;     		
		case DISPLAY_TVFORMAT_PAL:       				return 720;  
	    case DISPLAY_TVFORMAT_PAL_SVIDEO:      			return 720;   	       
		case DISPLAY_TVFORMAT_PAL_CVBS_SVIDEO: 			return 720;      
		case DISPLAY_TVFORMAT_PAL_M:     				return 720;    	 
		case DISPLAY_TVFORMAT_PAL_M_SVIDEO:     		return 720;     	 
		case DISPLAY_TVFORMAT_PAL_M_CVBS_SVIDEO:  		return 720;  		 
		case DISPLAY_TVFORMAT_PAL_NC:     				return 720;    	 
		case DISPLAY_TVFORMAT_PAL_NC_SVIDEO:     		return 720;     	 
		case DISPLAY_TVFORMAT_PAL_NC_CVBS_SVIDEO:  		return 720;  
		case DISPLAY_VGA_H1680_V1050:					return 1680;   
	    case DISPLAY_VGA_H1440_V900:                    return 1440;    
	    case DISPLAY_VGA_H1360_V768:                    return 1360;    
	    case DISPLAY_VGA_H1280_V1024:                   return 1280;   
	    case DISPLAY_VGA_H1024_V768:                    return 1024;    
	    case DISPLAY_VGA_H800_V600:                     return 800;     
	    case DISPLAY_VGA_H640_V480:                     return 640;     
	    case DISPLAY_VGA_H1440_V900_RB:                 return 1440; 
	    case DISPLAY_VGA_H1680_V1050_RB:                return 1680;
	    case DISPLAY_VGA_H1920_V1080_RB:                return 1920;
	    case DISPLAY_VGA_H1920_V1080:                   return 1920;   
	    case DISPLAY_VGA_H1280_V720:                    return 1280;    
		default:										break;  
	 
    }

    if(format == DISPLAY_DEFAULT)
    {
        unsigned long args[4];

        args[0] = displayno;
        
        return ioctl(ctx->mFD_disp,DISP_CMD_SCN_GET_WIDTH,args);
    }
    
    return -1;
}

#if 1
static int  display_getvalidwidth(struct display_context_t* ctx,int displayno,int format)
{
    switch (format) 
    {
	    case DISPLAY_TVFORMAT_480I:       				return 660;           
	    case DISPLAY_TVFORMAT_480P:     				return 660;    	    
	    case DISPLAY_TVFORMAT_576I:     				return 660;     	    
	    case DISPLAY_TVFORMAT_576P:  					return 660;  		    
	    case DISPLAY_TVFORMAT_720P_50HZ:  				return 1220;      
	    case DISPLAY_TVFORMAT_720P_60HZ:       			return 1220;      
	    case DISPLAY_TVFORMAT_1080I_50HZ:     			return 1840;     
	    case DISPLAY_TVFORMAT_1080I_60HZ:       		return 1840;     
	    case DISPLAY_TVFORMAT_1080P_50HZ:     			return 1840;     
	    case DISPLAY_TVFORMAT_1080P_60HZ:     			return 1840;     
	    case DISPLAY_TVFORMAT_1080P_24HZ:  				return 1840;   
	    case DISPLAY_TVFORMAT_NTSC:       				return 660;  
	    case DISPLAY_TVFORMAT_NTSC_SVIDEO:      		return 660;   	       
		case DISPLAY_TVFORMAT_NTSC_CVBS_SVIDEO: 		return 660;     		
		case DISPLAY_TVFORMAT_PAL:       				return 660;  
	    case DISPLAY_TVFORMAT_PAL_SVIDEO:      			return 660;   	       
		case DISPLAY_TVFORMAT_PAL_CVBS_SVIDEO: 			return 660;      
		case DISPLAY_TVFORMAT_PAL_M:     				return 660;    	 
		case DISPLAY_TVFORMAT_PAL_M_SVIDEO:     		return 660;     	 
		case DISPLAY_TVFORMAT_PAL_M_CVBS_SVIDEO:  		return 660;  		 
		case DISPLAY_TVFORMAT_PAL_NC:     				return 660;    	 
		case DISPLAY_TVFORMAT_PAL_NC_SVIDEO:     		return 660;     	 
		case DISPLAY_TVFORMAT_PAL_NC_CVBS_SVIDEO:  		return 660;  
		default:										break;  
	 
    }

    if(format == DISPLAY_DEFAULT)
    {
        unsigned long args[4];

        args[0] = displayno;
        
        return ioctl(ctx->mFD_disp,DISP_CMD_SCN_GET_WIDTH,args);
    }
    
    return -1;
} 
#else
static int  display_getvalidwidth(struct display_context_t* ctx,int displayno,int format)
{
    switch (format) 
    {
	    case DISPLAY_TVFORMAT_480I:       				return 720;           
	    case DISPLAY_TVFORMAT_480P:     				return 720;    	    
	    case DISPLAY_TVFORMAT_576I:     				return 720;     	    
	    case DISPLAY_TVFORMAT_576P:  					return 720;  		    
	    case DISPLAY_TVFORMAT_720P_50HZ:  				return 1280;      
	    case DISPLAY_TVFORMAT_720P_60HZ:       			return 1280;      
	    case DISPLAY_TVFORMAT_1080I_50HZ:     			return 1920;     
	    case DISPLAY_TVFORMAT_1080I_60HZ:       		return 1920;     
	    case DISPLAY_TVFORMAT_1080P_50HZ:     			return 1920;     
	    case DISPLAY_TVFORMAT_1080P_60HZ:     			return 1920;     
	    case DISPLAY_TVFORMAT_1080P_24HZ:  				return 1920;   
	    case DISPLAY_TVFORMAT_NTSC:       				return 720;  
	    case DISPLAY_TVFORMAT_NTSC_SVIDEO:      		return 720;   	       
		case DISPLAY_TVFORMAT_NTSC_CVBS_SVIDEO: 		return 720;     		
		case DISPLAY_TVFORMAT_PAL:       				return 720;  
	    case DISPLAY_TVFORMAT_PAL_SVIDEO:      			return 720;   	       
		case DISPLAY_TVFORMAT_PAL_CVBS_SVIDEO: 			return 720;      
		case DISPLAY_TVFORMAT_PAL_M:     				return 720;    	 
		case DISPLAY_TVFORMAT_PAL_M_SVIDEO:     		return 720;     	 
		case DISPLAY_TVFORMAT_PAL_M_CVBS_SVIDEO:  		return 720;  		 
		case DISPLAY_TVFORMAT_PAL_NC:     				return 720;    	 
		case DISPLAY_TVFORMAT_PAL_NC_SVIDEO:     		return 720;     	 
		case DISPLAY_TVFORMAT_PAL_NC_CVBS_SVIDEO:  		return 720;  

		default: break;  
	 
    }

    if(format == DISPLAY_DEFAULT)
    {
        unsigned long args[4];

        args[0] = displayno;
        
        return ioctl(ctx->mFD_disp,DISP_CMD_SCN_GET_WIDTH,args);
    }
    
    return -1;
} 
#endif
/*
**********************************************************************************************************************
*                                               display_getheight
*
* author:           
*
* date:             2011-7-17:11:23:44
*
* Description:      display getheight 
*
* parameters:       
*
* return:           if success return GUI_RET_OK
*                   if fail return the number of fail
* modify history: 
**********************************************************************************************************************
*/

static int  display_getheight(struct display_context_t* ctx,int displayno,int format)
{
    switch (format) 
    {
	    case DISPLAY_TVFORMAT_480I:       				return 480;           
	    case DISPLAY_TVFORMAT_480P:     				return 480;    	    
	    case DISPLAY_TVFORMAT_576I:     				return 576;     	    
	    case DISPLAY_TVFORMAT_576P:  					return 576;  		    
	    case DISPLAY_TVFORMAT_720P_50HZ:  				return 720;      
	    case DISPLAY_TVFORMAT_720P_60HZ:       			return 720;      
	    case DISPLAY_TVFORMAT_1080I_50HZ:     			return 1080;     
	    case DISPLAY_TVFORMAT_1080I_60HZ:       		return 1080;     
	    case DISPLAY_TVFORMAT_1080P_50HZ:     			return 1080;     
	    case DISPLAY_TVFORMAT_1080P_60HZ:     			return 1080;     
	    case DISPLAY_TVFORMAT_1080P_24HZ:  				return 1080;   
	    case DISPLAY_TVFORMAT_NTSC:       				return 480;  
	    case DISPLAY_TVFORMAT_NTSC_SVIDEO:      		return 480;   	       
		case DISPLAY_TVFORMAT_NTSC_CVBS_SVIDEO: 		return 480;     		
		case DISPLAY_TVFORMAT_PAL:       				return 576;  
	    case DISPLAY_TVFORMAT_PAL_SVIDEO:      			return 576;   	       
		case DISPLAY_TVFORMAT_PAL_CVBS_SVIDEO: 			return 576;      
		case DISPLAY_TVFORMAT_PAL_M:     				return 576;    	 
		case DISPLAY_TVFORMAT_PAL_M_SVIDEO:     		return 576;     	 
		case DISPLAY_TVFORMAT_PAL_M_CVBS_SVIDEO:  		return 576;  		 
		case DISPLAY_TVFORMAT_PAL_NC:     				return 576;    	 
		case DISPLAY_TVFORMAT_PAL_NC_SVIDEO:     		return 576;     	 
		case DISPLAY_TVFORMAT_PAL_NC_CVBS_SVIDEO:  		return 576;  
		case DISPLAY_VGA_H1680_V1050:					return 1050;   
		default:										break;  
	 
    }  

    if(format == DISPLAY_DEFAULT)
    {
        unsigned long args[4];

        args[0] = displayno;
        
        return ioctl(ctx->mFD_disp,DISP_CMD_SCN_GET_HEIGHT,args);
    }
    
    return -1;
}

#if 1
static int  display_getvalidheight(struct display_context_t* ctx,int displayno,int format)
{
    switch (format) 
    {
	    case DISPLAY_TVFORMAT_480I:       				return 440;           
	    case DISPLAY_TVFORMAT_480P:     				return 440;    	    
	    case DISPLAY_TVFORMAT_576I:     				return 536;     	    
	    case DISPLAY_TVFORMAT_576P:  					return 536;  		    
	    case DISPLAY_TVFORMAT_720P_50HZ:  				return 680;      
	    case DISPLAY_TVFORMAT_720P_60HZ:       			return 680;      
	    case DISPLAY_TVFORMAT_1080I_50HZ:     			return 1040;     
	    case DISPLAY_TVFORMAT_1080I_60HZ:       		return 1040;     
	    case DISPLAY_TVFORMAT_1080P_50HZ:     			return 1040;     
	    case DISPLAY_TVFORMAT_1080P_60HZ:     			return 1040;     
	    case DISPLAY_TVFORMAT_1080P_24HZ:  				return 1040;   
	    case DISPLAY_TVFORMAT_NTSC:       				return 440;  
	    case DISPLAY_TVFORMAT_NTSC_SVIDEO:      		return 440;   	       
		case DISPLAY_TVFORMAT_NTSC_CVBS_SVIDEO: 		return 440;     		
		case DISPLAY_TVFORMAT_PAL:       				return 536;  
	    case DISPLAY_TVFORMAT_PAL_SVIDEO:      			return 536;   	       
		case DISPLAY_TVFORMAT_PAL_CVBS_SVIDEO: 			return 536;      
		case DISPLAY_TVFORMAT_PAL_M:     				return 536;    	 
		case DISPLAY_TVFORMAT_PAL_M_SVIDEO:     		return 536;     	 
		case DISPLAY_TVFORMAT_PAL_M_CVBS_SVIDEO:  		return 536;  		 
		case DISPLAY_TVFORMAT_PAL_NC:     				return 536;    	 
		case DISPLAY_TVFORMAT_PAL_NC_SVIDEO:     		return 536;     	 
		case DISPLAY_TVFORMAT_PAL_NC_CVBS_SVIDEO:  		return 536;  
 
		default:										break;  
	 
    }  

    if(format == DISPLAY_DEFAULT)
    {
        unsigned long args[4];

        args[0] = displayno;
        
        return ioctl(ctx->mFD_disp,DISP_CMD_SCN_GET_HEIGHT,args);
    }
    
    return -1;
}
#else
static int  display_getvalidheight(struct display_context_t* ctx,int displayno,int format)
{
    switch (format) 
    {
	    case DISPLAY_TVFORMAT_480I:       				return 480;           
	    case DISPLAY_TVFORMAT_480P:     				return 480;    	    
	    case DISPLAY_TVFORMAT_576I:     				return 576;     	    
	    case DISPLAY_TVFORMAT_576P:  					return 576;  		    
	    case DISPLAY_TVFORMAT_720P_50HZ:  				return 720;      
	    case DISPLAY_TVFORMAT_720P_60HZ:       			return 720;      
	    case DISPLAY_TVFORMAT_1080I_50HZ:     			return 1080;     
	    case DISPLAY_TVFORMAT_1080I_60HZ:       		return 1080;     
	    case DISPLAY_TVFORMAT_1080P_50HZ:     			return 1080;     
	    case DISPLAY_TVFORMAT_1080P_60HZ:     			return 1080;     
	    case DISPLAY_TVFORMAT_1080P_24HZ:  				return 1080;   
	    case DISPLAY_TVFORMAT_NTSC:       				return 480;  
	    case DISPLAY_TVFORMAT_NTSC_SVIDEO:      		return 480;   	       
		case DISPLAY_TVFORMAT_NTSC_CVBS_SVIDEO: 		return 480;     		
		case DISPLAY_TVFORMAT_PAL:       				return 576;  
	    case DISPLAY_TVFORMAT_PAL_SVIDEO:      			return 576;   	       
		case DISPLAY_TVFORMAT_PAL_CVBS_SVIDEO: 			return 576;      
		case DISPLAY_TVFORMAT_PAL_M:     				return 576;    	 
		case DISPLAY_TVFORMAT_PAL_M_SVIDEO:     		return 576;     	 
		case DISPLAY_TVFORMAT_PAL_M_CVBS_SVIDEO:  		return 576;  		 
		case DISPLAY_TVFORMAT_PAL_NC:     				return 576;    	 
		case DISPLAY_TVFORMAT_PAL_NC_SVIDEO:     		return 576;     	 
		case DISPLAY_TVFORMAT_PAL_NC_CVBS_SVIDEO:  		return 576;  
		default:										break;  
	 
    }  

    if(format == DISPLAY_DEFAULT)
    {
        unsigned long args[4];

        args[0] = displayno;
        
        return ioctl(ctx->mFD_disp,DISP_CMD_SCN_GET_HEIGHT,args);
    }
    
    return -1;
}
#endif
/*
**********************************************************************************************************************
*                                               display_releasefb
*
* author:           
*
* date:             2011-7-17:11:23:48
*
* Description:      释放fb id的framebuffer的相关资源 
*
* parameters:       
*
* return:           if success return GUI_RET_OK
*                   if fail return the number of fail
* modify history: 
**********************************************************************************************************************
*/

static int  display_releasefb(struct display_context_t* ctx,int fb_id)
{
    unsigned long arg[4];
    char node[20];

    sprintf(node, "/dev/graphics/fb%d", fb_id);

    if(ctx->mFD_fb[fb_id] == 0)
    {
    	ctx->mFD_fb[fb_id]			= open(node,O_RDWR,0);
    	if(ctx->mFD_fb[fb_id] <= 0)
    	{
    		LOGE("open fb%d fail!\n",fb_id);
    		
    		ctx->mFD_fb[fb_id]		= 0;
    		
    		return  -1;
    	}
	}

    arg[0] = fb_id;
    ioctl(ctx->mFD_disp,DISP_CMD_FB_RELEASE,(unsigned long)arg);
    
    return 0;
}    
      
/*
**********************************************************************************************************************
*                                               display_requestfb
*
* author:           
*
* date:             2011-7-17:11:23:50
*
* Description:      display requestfb 
*
* parameters:       
*
* return:           if success return GUI_RET_OK
*                   if fail return the number of fail
* modify history: 
**********************************************************************************************************************
*/

static int  display_requestfb(struct display_context_t* ctx,int fb_id,struct display_fbpara_t *displaypara)
{
    __disp_fb_create_para_t 	fb_para;
    struct 						fb_var_screeninfo var;
    struct fb_fix_screeninfo 	fix;
    unsigned long 				arg[4];
    char 						node[20];
    int							ret = -1;
    int							red_size = 8;
    int							green_size = 8;
    int							blue_size = 8;
    int							alpha_size = 8;
    int							red_offset = 0;
    int							green_offset = 8;
    int							blue_offset = 16;
    int							alpha_offset = 24;
    int							bpp;
    int						    screen;
    unsigned long 				fb_layer_hdl;
    __disp_colorkey_t 			ck;
    __disp_rect_t				scn_rect;
    
    sprintf(node, "/dev/graphics/fb%d", fb_id);

    if(ctx->mFD_fb[fb_id] == 0)
    {
    	ctx->mFD_fb[fb_id]			= open(node,O_RDWR,0);
    	if(ctx->mFD_fb[fb_id] <= 0)
    	{
    		LOGE("open fb%d fail!\n",fb_id);
    		
    		ctx->mFD_fb[fb_id]		= 0;
    		
    		return  -1;
    	}
	}
	
	LOGD("ctx->mFD_fb[fb_id] = %x\n",ctx->mFD_fb[fb_id]);

	if(displaypara->format == HAL_PIXEL_FORMAT_RGBX_8888)
	{
		red_size				= 8;
    	green_size				= 8;
    	blue_size				= 8;
    	red_offset				= 0;
    	green_offset			= 8;
    	blue_offset				= 16;
    	bpp						= 32;			
	}
	else if(displaypara->format == HAL_PIXEL_FORMAT_RGB_565)
	{
	    red_size				= 5;
        green_size				= 6;
        blue_size				= 5;
        red_offset				= 0;
        green_offset			= 5;
        blue_offset				= 11;
        bpp						= 16;
	}
	else if(displaypara->format == HAL_PIXEL_FORMAT_BGRA_8888)
	{
	    red_size				= 8;
        green_size				= 8;
        blue_size				= 8;        
        alpha_size				= 8;
        red_offset				= 16;
        green_offset			= 8;
        blue_offset				= 0;
        alpha_offset			= 24;
        bpp						= 32;
	}
	else if(displaypara->format == HAL_PIXEL_FORMAT_RGBA_4444)
	{
	    red_size				= 4;
        green_size				= 4;
        blue_size				= 4;
        alpha_size				= 4;
        red_offset				= 0;
        green_offset			= 4;
        blue_offset				= 8;
        alpha_offset			= 12;
        bpp						= 16;
	}
	else
	{
	    red_size				= 8;
        green_size				= 8;
        blue_size				= 8;
        alpha_size				= 8;
        red_offset				= 0;
        green_offset			= 8;
        blue_offset				= 16;
        alpha_offset			= 24;
        bpp						= 32;
	}
	
    fb_para.mode 				= displaypara->layer_mode;
    fb_para.fb_mode 			= displaypara->fb_mode;
    fb_para.buffer_num 	        = displaypara->bufno;
    fb_para.width 				= displaypara->width;
    fb_para.height 				= displaypara->height;
    fb_para.output_height       = displaypara->output_height;
    fb_para.output_width        = displaypara->output_width;
    fb_para.primary_screen_id 	= 0;
    arg[0] 						= fb_id;
    arg[1] 						= (unsigned long)&fb_para;
    ret = ioctl(ctx->mFD_disp,DISP_CMD_FB_REQUEST,(unsigned long)arg);
    if(ret != 0)
    {
        LOGD("request fb fail\n");
        
        return -1;
    }
    
    //LOGD("request framebuffer success!\n");
	//LOGD("displaypara->width = %x\n",displaypara->width);
	//LOGD("displaypara->height = %x\n",displaypara->height);
	//LOGD("red_size = %x\n",red_size);
	//LOGD("green_size = %x\n",green_size);
	//LOGD("blue_size = %x\n",blue_size);
	//LOGD("red_offset = %x\n",red_offset);
	//LOGD("green_offset = %x\n",green_offset);
	//LOGD("blue_offset = %x\n",blue_offset);
	//LOGD("rdisplaypara->format = %x\n",displaypara->format);
	//LOGD("green_size = %x\n",green_size);
    ioctl(ctx->mFD_fb[fb_id],FBIOGET_FSCREENINFO,&fix);
    ioctl(ctx->mFD_fb[fb_id],FBIOGET_VSCREENINFO,&var);
    var.xoffset				= 0;
    var.yoffset				= 0;
    var.xres 				= displaypara->width;
    var.yres 				= displaypara->height;
    var.xres_virtual		= displaypara->width;
    var.yres_virtual		= displaypara->height * fb_para.buffer_num;
    var.nonstd 				= 0;
    var.bits_per_pixel 		= bpp;
    var.transp.length 		= alpha_size;
    var.red.length 			= red_size;
    var.green.length 		= green_size;
    var.blue.length 		= blue_size;
    var.transp.offset 		= alpha_offset;
    var.red.offset 			= red_offset;
    var.green.offset 		= green_offset;
    var.blue.offset 		= blue_offset;
    
    ioctl(ctx->mFD_fb[fb_id],FBIOPUT_VSCREENINFO,&var);
    
    if(fb_para.fb_mode == FB_MODE_SCREEN1)
    {
    	screen				= 1;
    	ioctl(ctx->mFD_fb[fb_id],FBIOGET_LAYER_HDL_1,&fb_layer_hdl);
    }
    else
    {
    	screen				= 0;
    	ioctl(ctx->mFD_fb[fb_id],FBIOGET_LAYER_HDL_0,&fb_layer_hdl);
    }
    
    if((displaypara->output_height != displaypara->valid_height) || (displaypara->output_width != displaypara->valid_width))
    {
		scn_rect.x			= (displaypara->output_width - displaypara->valid_width)>>1;
		scn_rect.y			= (displaypara->output_height - displaypara->valid_height)>>1;
		scn_rect.width		= displaypara->valid_width;
		scn_rect.height		= displaypara->valid_height;
		
		//LOGD("scn_rect.width = %d,scn_rect.height = %d,screen = %d,fb_layer_hdl = %d,fb_id = %d\n",scn_rect.width,scn_rect.height,screen,fb_layer_hdl,fb_id);
		
		arg[0] 				= screen;
	    arg[1] 				= fb_layer_hdl;
	    arg[2] 				= (unsigned long)(&scn_rect);
	    
	    ioctl(ctx->mFD_disp,DISP_CMD_LAYER_SET_SCN_WINDOW,(void*)arg);//pipe1, different with video layer's pipe
	}
    
	ck.ck_min.alpha 		= 0xff;
	ck.ck_min.red 			= 0x00;
	ck.ck_min.green 		= 0x00;
	ck.ck_min.blue 			= 0x00;
	ck.ck_max.alpha 		= 0xff;
	ck.ck_max.red 			= 0x00;
	ck.ck_max.green 		= 0x00;
	ck.ck_max.blue 			= 0x00;
	ck.red_match_rule 		= 2;
	ck.green_match_rule 	= 2;
	ck.blue_match_rule 		= 2;
	arg[0] 					= screen;
    arg[1] 					= (unsigned long)&ck;
    ioctl(ctx->mFD_disp,DISP_CMD_SET_COLORKEY,(void*)arg);//pipe1, different with video layer's pipe

	arg[0] 					= screen;
    arg[1] 					= fb_layer_hdl;
    arg[2] 					= 0;
    ioctl(ctx->mFD_disp,DISP_CMD_LAYER_SET_PIPE,(void*)arg);//pipe1, different with video layer's pipe

    arg[0] 					= screen;
    arg[1] 					= fb_layer_hdl;
    ioctl(ctx->mFD_disp,DISP_CMD_LAYER_TOP,(void*)arg);

    arg[0] 					= screen;
	arg[1] 					= fb_layer_hdl;
	arg[2]             		= 0xFF;
	ioctl(ctx->mFD_disp,DISP_CMD_LAYER_SET_ALPHA_VALUE,(void*)arg);//disable the global alpha, use the pixel's alpha

    arg[0] 					= screen;
    arg[1] 					= fb_layer_hdl;
    ioctl(ctx->mFD_disp,DISP_CMD_LAYER_ALPHA_ON,(void*)arg);//disable the global alpha, use the pixel's alpha

	arg[0]					= screen;
    arg[1] 					= fb_layer_hdl;
    ioctl(ctx->mFD_disp,DISP_CMD_LAYER_CK_OFF,(void*)arg);//disable the global alpha, use the pixel's alpha

    return 0;
} 

/*
**********************************************************************************************************************
*                                               display_requestfb
*
* author:           
*
* date:             2011-7-17:11:23:50
*
* Description:      display requestfb 
*
* parameters:       
*
* return:           if success return GUI_RET_OK
*                   if fail return the number of fail
* modify history: 
**********************************************************************************************************************
*/

static int  display_setfbrect(struct display_context_t* ctx,int displayno,int fb_id,int x,int y,int width,int height)
{
    __disp_fb_create_para_t 	fb_para;
    struct 						fb_var_screeninfo var;
    struct fb_fix_screeninfo 	fix;
    unsigned long 				arg[4];
    char 						node[20];
    int							ret = -1;
    unsigned long 				fb_layer_hdl;
    __disp_rect_t				scn_rect;
    
    sprintf(node, "/dev/graphics/fb%d", fb_id);

    if(ctx->mFD_fb[fb_id] == 0)
    {
    	ctx->mFD_fb[fb_id]			= open(node,O_RDWR,0);
    	if(ctx->mFD_fb[fb_id] <= 0)
    	{
    		LOGE("open fb%d fail!\n",fb_id);
    		
    		ctx->mFD_fb[fb_id]		= 0;
    		
    		return  -1;
    	}
	}
	
	LOGD("ctx->mFD_fb[fb_id] = %x\n",ctx->mFD_fb[fb_id]);
    
    if(displayno == 1)
    {
    	ioctl(ctx->mFD_fb[fb_id],FBIOGET_LAYER_HDL_1,&fb_layer_hdl);
	}
	else
	{
		ioctl(ctx->mFD_fb[fb_id],FBIOGET_LAYER_HDL_0,&fb_layer_hdl);
	}

	scn_rect.x				= x;
	scn_rect.y				= y;
	scn_rect.width			= width;
	scn_rect.height			= height;
	
	//LOGD("scn_rect.width = %d,scn_rect.height = %d,screen = %d,fb_layer_hdl = %d,fb_id = %d\n",scn_rect.width,scn_rect.height,displayno,fb_layer_hdl,fb_id);
	
	arg[0] 					= displayno;
    arg[1] 					= fb_layer_hdl;
    arg[2] 					= (unsigned long)(&scn_rect);
    
    ioctl(ctx->mFD_disp,DISP_CMD_LAYER_SET_SCN_WINDOW,(void*)arg);//pipe1, different with video layer's pipe

    return 0;
} 
      
/*
**********************************************************************************************************************
*                                               display_on
*
* author:           
*
* date:             2011-7-17:11:23:56
*
* Description:      display on 
*
* parameters:       
*
* return:           if success return GUI_RET_OK
*                   if fail return the number of fail
* modify history: 
**********************************************************************************************************************
*/

static int  display_on(struct display_context_t* ctx,int displayno,int outputtype)
{
	unsigned long 	args[4];
	int 			ret = -1;
	
	args[0]  	= displayno;
	args[1]		= 0;
	args[2]		= 0;
	args[3]		= 0;
	if(outputtype == DISPLAY_DEVICE_LCD)
	{
		ret = ioctl(ctx->mFD_disp,DISP_CMD_LCD_ON,(unsigned long)args);
	}
	else if(outputtype == DISPLAY_DEVICE_HDMI)
	{
		ret = ioctl(ctx->mFD_disp,DISP_CMD_HDMI_ON,(unsigned long)args);
	}
	
	return   ret;
}
      
/*
**********************************************************************************************************************
*                                               display_off
*
* author:           
*
* date:             2011-7-17:11:23:58
*
* Description:      display off 
*
* parameters:       
*
* return:           if success return GUI_RET_OK
*                   if fail return the number of fail
* modify history: 
**********************************************************************************************************************
*/

static int  display_off(struct display_context_t* ctx,int displayno,int outputtype)
{
	unsigned long 	args[4];
	int 			ret = -1;
	
	args[0]  	= displayno;
	args[1]		= 0;
	args[2]		= 0;
	args[3]		= 0;
	if(outputtype == DISPLAY_DEVICE_LCD)
	{
		ret = ioctl(ctx->mFD_disp,DISP_CMD_LCD_OFF,(unsigned long)args);
	}
	else if(outputtype == DISPLAY_DEVICE_HDMI)
	{
		ret = ioctl(ctx->mFD_disp,DISP_CMD_HDMI_OFF,(unsigned long)args);
	}
	return   ret;
}
      
/*
**********************************************************************************************************************
*                                               display_getminsize
*
* author:           
*
* date:             2011-7-17:11:24:1
*
* Description:      display getminsize 
*
* parameters:       
*
* return:           if success return GUI_RET_OK
*                   if fail return the number of fail
* modify history: 
**********************************************************************************************************************
*/

static int  display_getminsize(struct display_context_t* ctx,int *min_width,int *min_height)
{ 
    int     i;
    int     num;
    
    *min_width  = g_display[0].width;
    num         = 0;
    
    for(i = 0;i < MAX_DISPLAY_NUM;i++)
    {
        if(g_display[i].width < (uint32_t)*min_width)
        {
            *min_width = g_display[i].width;
            num = i;
        }
    }

    *min_height = g_display[num].height;

    return  0;
}
      
/*
**********************************************************************************************************************
*                                               display_getmaxdisplayno
*
* author:           
*
* date:             2011-7-17:11:24:3
*
* Description:      display getmaxdisplayno 
*
* parameters:       
*
* return:           if success return GUI_RET_OK
*                   if fail return the number of fail
* modify history: 
**********************************************************************************************************************
*/

static int  display_getmaxdisplayno(struct display_device_t *dev)
{
    int   		i;
    uint32_t   	max_width;
    int   		num;

    pthread_mutex_lock(&mode_lock);
    max_width   = g_display[0].width;
    num         = 0;

    for(i = 0;i < MAX_DISPLAY_NUM;i++)
    {
        if(g_display[i].width > max_width)
        {
            max_width = g_display[i].width;
            num = i;
        }
    }
    pthread_mutex_unlock(&mode_lock);

    return num;
}

static int display_getdisplaybufid(struct display_device_t *dev, int displayno)
{
    struct display_context_t*   ctx = (struct display_context_t*)dev;
    struct fb_var_screeninfo    var_src;
    char               			node_src[20];
    unsigned int                fbid;

    
    //pthread_mutex_lock(&mode_lock);
    
    fbid = g_display[displayno].fb_id;

    sprintf(node_src, "/dev/graphics/fb%d", fbid);

    if(ctx->mFD_fb[fbid] == 0)
    {
    	ctx->mFD_fb[fbid]			= open(node_src,O_RDWR,0);
    	if(ctx->mFD_fb[fbid] <= 0)
    	{
    		LOGE("open fb%d fail!\n",fbid);
    		
    		ctx->mFD_fb[fbid]		= 0;

            pthread_mutex_unlock(&mode_lock);  
            
    		return  -1;
    	}
	}

    ioctl(ctx->mFD_fb[fbid],FBIOGET_VSCREENINFO,&var_src);

    //pthread_mutex_unlock(&mode_lock);  
    
    return  var_src.yoffset/var_src.yres;
      
}
      
/*
**********************************************************************************************************************
*                                               display_output
*
* author:           
*
* date:             2011-7-17:11:24:5
*
* Description:      display output 
*
* parameters:       
*
* return:           if success return GUI_RET_OK
*                   if fail return the number of fail
* modify history: 
**********************************************************************************************************************
*/

static int  display_output(struct display_context_t* ctx, int displayno, int out_type, int mode)
{
    unsigned long 	arg[4];
    int				ret = 0;

    arg[0] = displayno;

    if(out_type == DISPLAY_DEVICE_LCD)
    {
        ret = ioctl(ctx->mFD_disp,DISP_CMD_LCD_ON,(unsigned long)arg);
    }
    else if(out_type == DISPLAY_DEVICE_HDMI)
    {
        arg[1] = (__disp_tv_mode_t)mode;
        ret = ioctl(ctx->mFD_disp,DISP_CMD_HDMI_SET_MODE,(unsigned long)arg);

        ret = ioctl(ctx->mFD_disp,DISP_CMD_HDMI_ON,(unsigned long)arg);
    }
    
    return   ret;
}
      
/*
**********************************************************************************************************************
*                                               display_singlechangemode
*
* author:           
*
* date:             2011-7-17:11:54:1
*
* Description:      display singlechangemode 
*
* parameters:       
*
* return:           if success return GUI_RET_OK
*                   if fail return the number of fail
* modify history: 
**********************************************************************************************************************
*/

static int display_singlechangemode(struct display_device_t *dev,int displayno,int value0,int value1)
{
    struct 						display_context_t* ctx = (struct display_context_t*)dev;
    int 						status = 0;
    struct display_fbpara_t		para;
    int							tvformat = 0;

    if(value0 != DISPLAY_DEVICE_LCD)
    {
        tvformat = get_tvformat(value1);
    	if(tvformat == -1)
    	{
    		LOGE("Invalid TV Format!\n");
    		
    		return  -1;
    	}
    }

	LOGD("value0 = %d,g_display[displayno].type = %d\n",value0,g_display[displayno].type);
    
    if((value0 != (int)g_display[displayno].type)
      ||((value0 == (int)g_display[displayno].type) && (value1 != (int)g_display[displayno].tvformat)))
    {
        display_off(ctx,displayno,g_display[displayno].type);
            
    	display_releasefb(ctx,g_display[displayno].fb_id);

    	para.fb_mode        = (__fb_mode_t)g_display[displayno].fbmode;
        para.format         = g_display[displayno].format;
        if(value0 != DISPLAY_DEVICE_LCD)
        {
            para.height     		= display_getheight(ctx,displayno,value1);
            para.width      		= display_getwidth(ctx,displayno,value1);
            para.output_height      = display_getheight(ctx,displayno,value1);
			para.output_width       = display_getwidth(ctx,displayno,value1);
			para.valid_height       = display_getvalidheight(ctx,displayno,value1);
			para.valid_width        = display_getvalidwidth(ctx,displayno,value1);
        }
        else
        {
            para.height     		= display_getheight(ctx,displayno,DISPLAY_DEFAULT);
            para.width     	 		= display_getwidth(ctx,displayno,DISPLAY_DEFAULT);
            para.output_height      = para.height;
			para.output_width       = para.width;
			para.valid_height      	= para.height;
			para.valid_width       	= para.width;
        }
        para.bufno          = 2;
        para.layer_mode     = DISP_LAYER_WORK_MODE_NORMAL;
        
        display_requestfb(ctx,g_display[displayno].fb_id,&para);
        
        LOGD("para.width = %d\n",para.width);
        LOGD("para.height = %d\n",para.height);

        g_display[displayno].tvformat       = value1;
        g_display[displayno].width          = para.width;
        g_display[displayno].height         = para.height;
        g_display[displayno].valid_width    = para.valid_width;
        g_display[displayno].valid_height   = para.valid_height;
        g_display[displayno].fb_height      = para.height;
        g_display[displayno].fb_width       = para.width;
        g_display[displayno].layermode      = DISP_LAYER_WORK_MODE_NORMAL;
        g_display[displayno].isopen         = DISPLAY_TRUE;
        g_display[displayno].type           = value0;
        g_display[displayno].hotplug        = display_gethotplug(dev,displayno);

        display_output(ctx,displayno,value0,tvformat);
        
        return  0;
    }


    return  1;
}
      
/*
**********************************************************************************************************************
*                                               display_duallcdchangemode
*
* author:           
*
* date:             2011-7-17:11:54:3
*
* Description:      display duallcdchangemode 
*
* parameters:       
*
* return:           if success return GUI_RET_OK
*                   if fail return the number of fail
* modify history: 
**********************************************************************************************************************
*/

static int display_duallcdchangemode(struct display_device_t *dev,int displayno,int value0,int value1)
{
    return  1;
}
      
/*
**********************************************************************************************************************
*                                               display_dualdiffchangemode
*
* author:           
*
* date:             2011-7-17:11:54:5
*
* Description:      display dualdiffchangemode 
*
* parameters:       
*
* return:           if success return GUI_RET_OK
*                   if fail return the number of fail
* modify history: 
**********************************************************************************************************************
*/

static int display_dualdiffchangemode(struct display_device_t *dev,int displayno,int value0,int value1)
{
    struct 						display_context_t* ctx = (struct display_context_t*)dev;
    int 						status = 0;
    struct display_fbpara_t		para;
    int							tvformat = 0;

    if(value0 != DISPLAY_DEVICE_LCD)
    {
        tvformat = get_tvformat(value1);
    	if(tvformat == -1)
    	{
    		LOGE("Invalid TV Format!\n");
    		
    		return  -1;
    	}
    }

    if((value0 != (int)g_display[displayno].type)
      ||((value0 == (int)g_display[displayno].type) && (value1 != (int)g_display[displayno].tvformat)))
    {
        display_off(ctx,displayno,g_display[displayno].type);
            
    	display_releasefb(ctx,g_display[displayno].fb_id);

    	para.fb_mode        = (__fb_mode_t)g_display[displayno].fbmode;
        para.format         = g_display[displayno].format;
        if(value0 != DISPLAY_DEVICE_LCD)
        {
            para.height     		= display_getheight(ctx,displayno,value1);
            para.width      		= display_getwidth(ctx,displayno,value1);
            para.output_height      = display_getheight(ctx,displayno,value1);
			para.output_width       = display_getwidth(ctx,displayno,value1);
			para.valid_height       = display_getvalidheight(ctx,displayno,value1);
			para.valid_width        = display_getvalidwidth(ctx,displayno,value1);
        }
        else
        {
            para.height     		= display_getheight(ctx,displayno,DISPLAY_DEFAULT);
            para.width     	 		= display_getwidth(ctx,displayno,DISPLAY_DEFAULT);
            para.output_height      = para.height;
			para.output_width       = para.width;
			para.valid_height      	= para.height;
			para.valid_width       	= para.width;
        }
        para.bufno          = 2;
        para.layer_mode = DISP_LAYER_WORK_MODE_NORMAL;
        
        display_requestfb(ctx,g_display[displayno].fb_id,&para);

        g_display[displayno].tvformat       = value1;
        g_display[displayno].width          = para.width;
        g_display[displayno].height         = para.height;
        g_display[displayno].valid_width    = para.valid_width;
        g_display[displayno].valid_height   = para.valid_height;
        g_display[displayno].fb_height      = para.height;
        g_display[displayno].fb_width       = para.width;
        g_display[displayno].layermode      = DISP_LAYER_WORK_MODE_NORMAL;
        g_display[displayno].isopen         = DISPLAY_TRUE;
        g_display[displayno].type           = value0;
        g_display[displayno].hotplug        = display_gethotplug(dev,displayno);

        display_output(ctx,displayno,value0,tvformat);
        
        return  0;
    }

    return  1;    
}
      
/*
**********************************************************************************************************************
*                                               display_dualsamechangemode
*
* author:           
*
* date:             2011-7-17:11:54:7
*
* Description:      display dualsamechangemode 
*
* parameters:       
*
* return:           if success return GUI_RET_OK
*                   if fail return the number of fail
* modify history: 
**********************************************************************************************************************
*/

static int display_dualsamechangemode(struct display_device_t *dev,int displayno,int value0,int value1)
{
    struct 						display_context_t* ctx = (struct display_context_t*)dev;
    int 						status = 0;
    struct display_fbpara_t		para;
    int							tvformat = 0;
    int                         minwidth;
    int                         minheight;

    
    if(displayno == g_masterdisplay)
    {        
        if(value0 != DISPLAY_DEVICE_LCD)
        {
            tvformat = get_tvformat(value1);
        	if(tvformat == -1)
        	{
        		LOGE("Invalid TV Format!\n");

        		return  -1;
        	}
        }

        if((value0 != (int)g_display[displayno].type)
          ||((value0 == (int)g_display[displayno].type) && (value1 != (int)g_display[displayno].tvformat)))
        {
            display_off(ctx,displayno,g_display[displayno].type);
                
        	display_releasefb(ctx,g_display[displayno].fb_id);

        	para.fb_mode        = (__fb_mode_t)g_display[displayno].fbmode;
            para.format         = g_display[displayno].format;
            if(value0 != DISPLAY_DEVICE_LCD)
            {
                para.height     		= display_getheight(ctx,displayno,value1);
                para.width      		= display_getwidth(ctx,displayno,value1);
                para.output_height      = display_getheight(ctx,displayno,value1);
				para.output_width       = display_getwidth(ctx,displayno,value1);
				para.valid_height       = display_getvalidheight(ctx,displayno,value1);
				para.valid_width        = display_getvalidwidth(ctx,displayno,value1);
            }
            else
            {
                para.height     		= display_getheight(ctx,displayno,DISPLAY_DEFAULT);
                para.width     	 		= display_getwidth(ctx,displayno,DISPLAY_DEFAULT);
                para.output_height      = para.height;
				para.output_width       = para.width;
				para.valid_height      	= para.height;
				para.valid_width       	= para.width;
            }
            para.bufno          = 2;
            para.layer_mode = DISP_LAYER_WORK_MODE_NORMAL;
            
            display_requestfb(ctx,g_display[displayno].fb_id,&para);

            g_display[displayno].tvformat       = value1;
            g_display[displayno].width          = para.width;
            g_display[displayno].height         = para.height;
            g_display[displayno].valid_width    = para.valid_width;
            g_display[displayno].valid_height   = para.valid_height;
            g_display[displayno].fb_height      = para.height;
            g_display[displayno].fb_width       = para.width;
            g_display[displayno].layermode      = DISP_LAYER_WORK_MODE_NORMAL;
            g_display[displayno].isopen         = DISPLAY_TRUE;
            g_display[displayno].type           = value0;
            g_display[displayno].hotplug        = display_gethotplug(dev,displayno);

            display_output(ctx,displayno,value0,tvformat);
            
            return  0;
        }
    }
    else
    {
        if(value0 != DISPLAY_DEVICE_LCD)
        {
            tvformat = get_tvformat(value1);
        	if(tvformat == -1)
        	{
        		LOGE("Invalid TV Format!\n");
                
        		return  -1;
        	}
        }

        if((value0 != (int)g_display[displayno].type)
          ||((value0 == (int)g_display[displayno].type) && (value1 != (int)g_display[displayno].tvformat)))
        {
            display_off(ctx,displayno,g_display[displayno].type);

			if(value0 != DISPLAY_DEVICE_LCD)
            {
                para.output_height      = display_getheight(ctx,displayno,value1);
                para.output_width       = display_getwidth(ctx,displayno,value1);
                para.valid_width		= display_getvalidwidth(ctx,displayno,value1);
                para.valid_height       = display_getvalidheight(ctx,displayno,value1);
            }
            else
            {
                para.output_height      = display_getheight(ctx,displayno,DISPLAY_DEFAULT);
                para.output_width       = display_getwidth(ctx,displayno,DISPLAY_DEFAULT);
                para.valid_width		= para.output_width;
                para.valid_height		= para.output_height;
            }
            
            display_setfbrect(ctx,g_display[displayno].fb_id,displayno,(para.output_width - para.valid_width)>>1,(para.output_height - para.valid_height)>>1,para.valid_width,para.valid_height);

            g_display[displayno].tvformat       = value1;
            g_display[displayno].width          = para.output_width;
            g_display[displayno].height         = para.output_height;
            g_display[displayno].valid_width    = para.valid_width;
            g_display[displayno].valid_height   = para.valid_height;
            g_display[displayno].layermode      = DISP_LAYER_WORK_MODE_SCALER;
            g_display[displayno].isopen         = DISPLAY_TRUE;
            g_display[displayno].type           = value0;
            g_display[displayno].hotplug        = display_gethotplug(dev,displayno);

            display_output(ctx,displayno,value0,tvformat);
            
            return  0;
        }
    }

    return 1;
}
             
/*
**********************************************************************************************************************
*                                               display_changemode
*
* author:           
*
* date:             2011-7-17:11:24:18
*
* Description:      Set a parameter to value 
*
* parameters:       
*
* return:           if success return GUI_RET_OK
*                   if fail return the number of fail
* modify history: 
**********************************************************************************************************************
*/

static int display_changemode(struct display_device_t *dev,int displayno,int value0,int value1) 
{           
    struct 						display_context_t* ctx = (struct display_context_t*)dev;
    int 						status = 0;
    struct display_fbpara_t		para;
    int							tvformat = 0;
    
    if (ctx) 
    {
    	if(ctx->mFD_disp == 0)
    	{
    		LOGE("Couldn't find display fb!\n");
    		
    		return  -1;
    	}
    	
        switch(displayno) 
        {
        	case 0:
	            displayno = 0;
	            break;
	            
        	case 1:
				displayno = 1;
            	break;
            	
           	default:
           		displayno = 0;
           		break;
         }

         pthread_mutex_lock(&mode_lock);
         
         if(g_display[displayno].type == DISPLAY_DEVICE_NONE || value0 == DISPLAY_DEVICE_NONE)
         {
            LOGE("change output mode from DISPLAY_DEVICE_NONE or to DISPLAY_DEVICE_NONE not support!\n");

            pthread_mutex_unlock(&mode_lock);
            return  -1;
         }

         if(g_displaymode == DISPLAY_MODE_SINGLE)
         {
            status = display_singlechangemode(dev,displayno,value0,value1);
         }
         else if(g_displaymode == DISPLAY_MODE_DUALLCD)
         {
            status = display_duallcdchangemode(dev,displayno,value0,value1);
         }
         else if(g_displaymode == DISPLAY_MODE_DUALDIFF)
         {
            status = display_dualdiffchangemode(dev,displayno,value0,value1);
         }
         else if(g_displaymode == DISPLAY_MODE_DUALSAME)
         {
            status = display_dualsamechangemode(dev,displayno,value0,value1);
         }
         else
         {
            status = -1;
         }

         pthread_mutex_unlock(&mode_lock);
    } 
    else 
    {
        status = -EINVAL;
    }
    
    return status;
}
      
/*
**********************************************************************************************************************
*                                               display_setparameter
*
* author:           
*
* date:             2011-7-17:11:24:36
*
* Description:      display setparameter 
*
* parameters:       
*
* return:           if success return GUI_RET_OK
*                   if fail return the number of fail
* modify history: 
**********************************************************************************************************************
*/

static int display_setparameter(struct display_device_t *dev, int displayno, int value0,int value1)
{
    struct 	display_context_t* ctx = (struct display_context_t*)dev;
    int							tvformat = 0;
    
    if(value0 <= DISPLAY_DEVICE_VGA)
    {
        if(value0 == DISPLAY_DEVICE_NONE)
        {
            LOGE("input type error!\n");
            
            return -1;
        }

        if(value0 != DISPLAY_DEVICE_LCD)
        {
            tvformat = get_tvformat(value1);
    		if(tvformat == -1)
    		{
    			LOGE("Invalid TV Format!\n");

    			return  -1;
    		}

            g_display[displayno].height  		= display_getheight(ctx,displayno,tvformat);
            g_display[displayno].width   		= display_getwidth(ctx,displayno,tvformat);
            g_display[displayno].valid_height  	= display_getvalidheight(ctx,displayno,tvformat);
            g_display[displayno].valid_width   	= display_getvalidwidth(ctx,displayno,tvformat);
        }
        else
        {
            g_display[displayno].height  		= display_getheight(ctx,displayno,DISPLAY_DEFAULT);
            g_display[displayno].width   		= display_getwidth(ctx,displayno,DISPLAY_DEFAULT);
            g_display[displayno].valid_height  	= g_display[displayno].height;
            g_display[displayno].valid_width   	= g_display[displayno].width;
        }

        if(displayno == 0)
        {
            g_display[displayno].fbmode = FB_MODE_SCREEN0;
        }
        else
        {
            g_display[displayno].fbmode = FB_MODE_SCREEN1;
        }

        g_display[displayno].type    = value0;
        g_display[displayno].tvformat= tvformat;
    }
    else if(value0 == DISPLAY_PIXELMODE)
    {
        g_display[displayno].format  = value1;
    }

    return  0;
}
      
/*
**********************************************************************************************************************
*                                               display_getparameter
*
* author:           
*
* date:             2011-7-17:11:24:39
*
* Description:      display getparameter 
*
* parameters:       
*
* return:           if success return GUI_RET_OK
*                   if fail return the number of fail
* modify history: 
**********************************************************************************************************************
*/

static int display_getparameter(struct display_device_t *dev, int displayno, int param)
{
    struct 	display_context_t* ctx = (struct display_context_t*)dev;
    
	if(displayno < 0 || displayno > MAX_DISPLAY_NUM)
	{
        LOGE("Invalid Display No!\n");

        return  -1;
    }

    switch(param)
    {
        case   DISPLAY_OUTPUT_WIDTH:            return  g_display[displayno].width;
        case   DISPLAY_OUTPUT_HEIGHT:           return  g_display[displayno].height;
        case   DISPLAY_FBWIDTH:                 return  g_display[displayno].fb_width;
        case   DISPLAY_FBHEIGHT:                return  g_display[displayno].fb_height;
        case   DISPLAY_OUTPUT_PIXELFORMAT:      return  g_display[displayno].format;
        case   DISPLAY_OUTPUT_FORMAT:           return  g_display[displayno].tvformat;
        case   DISPLAY_OUTPUT_TYPE:             return  g_display[displayno].type;
        case   DISPLAY_OUTPUT_ISOPEN:           return  g_display[displayno].isopen;
        case   DISPLAY_OUTPUT_HOTPLUG:          return  g_display[displayno].hotplug;
        default:
            LOGE("Invalid Display Parameter!\n");

            return  -1;
    }
}
      
/*
**********************************************************************************************************************
*                                               display_releasemode
*
* author:           
*
* date:             2011-7-17:11:24:50
*
* Description:      display releasemode 
*
* parameters:       
*
* return:           if success return GUI_RET_OK
*                   if fail return the number of fail
* modify history: 
**********************************************************************************************************************
*/

static int display_releasemode(struct display_device_t *dev,int mode)
{
    struct 	display_context_t*  ctx = (struct display_context_t*)dev;
    int                         outputtype0;
    int                         outputtype1;

	//LOGD("g_displaymode = %d\n",g_displaymode);
    /*先释放该模式拥有的资源*/
    if(g_displaymode == DISPLAY_MODE_SINGLE)
    {
        outputtype0 = display_getoutputtype(dev,g_masterdisplay);
        
        //LOGD("outputtype0 = %d,g_masterdisplay = %d,g_display[g_masterdisplay].fb_id = %d\n",outputtype0,g_masterdisplay,g_display[g_masterdisplay].fb_id);
            
        display_off(ctx,g_masterdisplay,outputtype0);
            
		display_releasefb(ctx,g_display[g_masterdisplay].fb_id);
    }
    else if(g_displaymode == DISPLAY_MODE_DUALLCD)
    {
        outputtype0 = display_getoutputtype(dev,0);
        outputtype1 = display_getoutputtype(dev,1);
            
        display_off(ctx,0,outputtype0);
            
        display_off(ctx,1,outputtype1);
            
		display_releasefb(ctx,0);
    }
    else if(g_displaymode == DISPLAY_MODE_DUALDIFF)
    {
        outputtype0 = display_getoutputtype(dev,0);
        outputtype1 = display_getoutputtype(dev,1);
            
        display_off(ctx,0,outputtype0);

        display_off(ctx,1,outputtype1);
            
		display_releasefb(ctx,0);

        display_releasefb(ctx,1);
    }
    else
    {
        outputtype0 = display_getoutputtype(dev,0);
        outputtype1 = display_getoutputtype(dev,1);
            
        display_off(ctx,0,outputtype0);

        display_off(ctx,1,outputtype1);
            
		display_releasefb(ctx,0);

        display_releasefb(ctx,1);
    }

    return    0;
}
      
/*
**********************************************************************************************************************
*                                               display_requestsingle
*
* author:           
*
* date:             2011-7-17:11:25:0
*
* Description:      display requestsingle 
*
* parameters:       
*
* return:           if success return GUI_RET_OK
*                   if fail return the number of fail
* modify history: 
**********************************************************************************************************************
*/

static int display_requestsingle(struct display_device_t *dev)
{
    struct 	display_context_t*  ctx = (struct display_context_t*)dev;
    struct  display_fbpara_t	para;
    int							tvformat = 0;

    if(g_display[g_masterdisplay].type != DISPLAY_DEVICE_LCD)
    {
        tvformat = get_tvformat(g_display[g_masterdisplay].tvformat);
        if(tvformat == -1)
        {
            LOGE("Invalid TV Format!\n");

            return  -1;
        } 
    }

    if(g_masterdisplay == 0)
    {
        g_display[g_masterdisplay].fbmode    = FB_MODE_SCREEN0;
    }
    else
    {
        g_display[g_masterdisplay].fbmode    = FB_MODE_SCREEN1;
    }

    para.fb_mode        = (__fb_mode_t)g_display[g_masterdisplay].fbmode;
    para.format         = g_display[g_masterdisplay].format;
    para.height         = g_display[g_masterdisplay].height;
    para.width          = g_display[g_masterdisplay].width;
    para.valid_height   = g_display[g_masterdisplay].valid_height;
    para.valid_width    = g_display[g_masterdisplay].valid_width;
    para.layer_mode     = DISP_LAYER_WORK_MODE_NORMAL;
    display_requestfb(ctx,g_display[g_masterdisplay].fb_id,&para);

    g_display[g_masterdisplay].layermode    = DISP_LAYER_WORK_MODE_NORMAL;
    g_display[g_masterdisplay].isopen       = DISPLAY_TRUE;
    g_display[g_masterdisplay].fb_height    = para.height;
    g_display[g_masterdisplay].fb_width     = para.width;

    display_output(ctx,g_masterdisplay,g_display[g_masterdisplay].type,tvformat);
    g_display[g_masterdisplay].hotplug      = display_gethotplug(dev,g_masterdisplay);

    return     0;
}

      
/*
**********************************************************************************************************************
*                                               display_requestduallcd
*
* author:           
*
* date:             2011-7-17:11:25:10
*
* Description:      display requestduallcd 
*
* parameters:       
*
* return:           if success return GUI_RET_OK
*                   if fail return the number of fail
* modify history: 
**********************************************************************************************************************
*/

static int display_requestduallcd(struct display_device_t *dev)
{
    struct 	display_context_t*  ctx = (struct display_context_t*)dev;
    struct  display_fbpara_t	para;

    g_display[g_masterdisplay].fbmode    = FB_MODE_DUAL_SAME_SCREEN_TB;

    para.fb_mode    = FB_MODE_DUAL_SAME_SCREEN_TB;
    para.format     = g_display[0].format;
    para.layer_mode = DISP_LAYER_WORK_MODE_NORMAL;
	para.height     = 2 * display_getheight(ctx,0,DISPLAY_DEFAULT);
    para.width      = display_getwidth(ctx,0,DISPLAY_DEFAULT);
    
    display_requestfb(ctx,0,&para);
    
    g_display[0].isopen         = DISPLAY_TRUE;
    g_display[1].isopen         = DISPLAY_TRUE;
    
    display_output(ctx,0,DISPLAY_DEVICE_LCD,0);
    display_output(ctx,1,DISPLAY_DEVICE_LCD,0);

    return     0;
}
    
/*
**********************************************************************************************************************
*                                               display_requestdualdiff
*
* author:           
*
* date:             2011-7-17:11:25:18
*
* Description:      display requestdualdiff 
*
* parameters:       
*
* return:           if success return GUI_RET_OK
*                   if fail return the number of fail
* modify history: 
**********************************************************************************************************************
*/

static int display_requestdualdiff(struct display_device_t *dev)
{
    struct 	display_context_t*  ctx = (struct display_context_t*)dev;
    struct  display_fbpara_t	para;
    int							tvformat = 0;
    int                         i;

    for(i = 0;i < MAX_DISPLAY_NUM;i++)
    {
        if(g_display[i].type == DISPLAY_DEVICE_LCD)
        {
            if(i == 0)
            {
                g_display[i].fbmode    = FB_MODE_SCREEN0;
            }
            else
            {
                g_display[i].fbmode    = FB_MODE_SCREEN1;
            }
            para.format     		= g_display[i].format;
            para.layer_mode 		= DISP_LAYER_WORK_MODE_NORMAL;
            
    		para.height     		= display_getheight(ctx,i,DISPLAY_DEFAULT);
            para.width      		= display_getwidth(ctx,i,DISPLAY_DEFAULT);
            para.output_height      = para.height;
			para.output_width       = para.width;
			para.valid_height      	= para.height;
			para.valid_width       	= para.width;

            if(i == g_masterdisplay)
            {
                display_requestfb(ctx,0,&para);
            }
            else
            {
                display_requestfb(ctx,1,&para);
            }
            
            g_display[i].isopen         = DISPLAY_TRUE;
            g_display[i].fb_height      = para.height;
            g_display[i].fb_width       = para.width;
            display_output(ctx,i,DISPLAY_DEVICE_LCD,tvformat);
        }
        else
        {
            tvformat = get_tvformat(g_display[i].tvformat);
            if(tvformat == -1)
            {
                LOGE("Invalid TV Format!\n");

                return  -1;
            }

            g_display[i].fbmode     = FB_MODE_SCREEN0;
            para.fb_mode            = (__fb_mode_t)g_display[i].fbmode;
            para.format             = g_display[i].format;
            para.height             = display_getheight(ctx,i,g_display[i].tvformat);
            para.width              = display_getwidth(ctx,i,g_display[i].tvformat);
            para.output_height      = para.height;
            para.output_width       = para.width;
            para.valid_height       = display_getvalidheight(ctx,i,g_display[i].tvformat);
            para.valid_width        = display_getvalidwidth(ctx,i,g_display[i].tvformat);
            para.layer_mode         = DISP_LAYER_WORK_MODE_NORMAL;
            if(i == g_masterdisplay)
            {
                display_requestfb(ctx,0,&para);
            }
            else
            {
                display_requestfb(ctx,1,&para);
            }
            
            g_display[i].isopen         = DISPLAY_TRUE;
            display_output(ctx,i,g_display[i].type,tvformat);
            g_display[i].hotplug        = display_gethotplug(dev,i);
        }
    }
    
    return     0;
}
      
/*
**********************************************************************************************************************
*                                               display_requestdualsame
*
* author:           
*
* date:             2011-7-17:11:25:28
*
* Description:      display requestdualsame 
*
* parameters:       
*
* return:           if success return GUI_RET_OK
*                   if fail return the number of fail
* modify history: 
**********************************************************************************************************************
*/

static int display_requestdualsame(struct display_device_t *dev)
{
    struct 	display_context_t*  ctx = (struct display_context_t*)dev;
    struct  display_fbpara_t	para;
    int							tvformat = 0;
    int                         i;
    int                         min_width;
    int                         min_height;

    for(i = 0;i < MAX_DISPLAY_NUM;i++)
    {
        if(g_display[i].type == DISPLAY_DEVICE_LCD)
        {
            if(i == 0)
            {
                g_display[i].fbmode    = FB_MODE_SCREEN0;
            }
            else
            {
                g_display[i].fbmode    = FB_MODE_SCREEN1;
            }
            para.format     			= g_display[i].format;
            para.layer_mode 			= DISP_LAYER_WORK_MODE_NORMAL;

    		para.height     			= display_getheight(ctx,i,DISPLAY_DEFAULT);
            para.width      			= display_getwidth(ctx,i,DISPLAY_DEFAULT);
            para.valid_height      		= para.height;
	        para.valid_width       		= para.valid_width;
	        para.output_height      	= para.height;
	        para.output_width       	= para.valid_width;
            if(i == g_masterdisplay)
            {
                display_requestfb(ctx,0,&para);
            }
            else
            {
                display_requestfb(ctx,1,&para);
            }
            g_display[i].isopen         = DISPLAY_TRUE;
            g_display[i].fb_height      = para.height;
            g_display[i].fb_width       = para.width;
            display_output(ctx,i,DISPLAY_DEVICE_LCD,tvformat);
        }
        else
        {
            tvformat = get_tvformat(g_display[i].tvformat);
            if(tvformat == -1)
            {
                LOGE("Invalid TV Format!\n");

                return  -1;
            }

            g_display[i].fbmode     = FB_MODE_SCREEN0;
            para.fb_mode            = (__fb_mode_t)g_display[i].fbmode;
            para.format             = g_display[i].format;
            para.output_height      = g_display[i].height;
            para.output_width       = g_display[i].width;
            para.valid_height      	= g_display[i].valid_height;
            para.valid_width       	= g_display[i].valid_width;
            if(i == g_masterdisplay)
            {
                para.height         = para.output_height;
                para.width          = para.output_width;
                para.layer_mode     = DISP_LAYER_WORK_MODE_NORMAL;

                display_requestfb(ctx,0,&para);
            }
            else
            {
                display_getminsize(ctx,&min_width,&min_height);
                
                para.width          = min_width;
                para.height         = min_height;
                para.layer_mode     = DISP_LAYER_WORK_MODE_SCALER;

                display_requestfb(ctx,1,&para);
            }
            
            g_display[i].isopen         = DISPLAY_TRUE;
            g_display[i].fb_height      = para.height;
            g_display[i].fb_width       = para.width;
            g_display[i].hotplug        = display_gethotplug(dev,i);
            display_output(ctx,i,g_display[i].type,tvformat);
        }
    }
    
    return     0;
}
      
/*
**********************************************************************************************************************
*                                               display_requestmode
*
* author:           
*
* date:             2011-7-17:11:25:32
*
* Description:      display requestmode 
*
* parameters:       
*
* return:           if success return GUI_RET_OK
*                   if fail return the number of fail
* modify history: 
**********************************************************************************************************************
*/

static int display_requestmode(struct display_device_t *dev,int mode)
{
    if(mode == DISPLAY_MODE_SINGLE)
    {
        return  display_requestsingle(dev);
    }
    else if(mode == DISPLAY_MODE_DUALLCD)
    {
        return  display_requestduallcd(dev);
    }
    else if(mode == DISPLAY_MODE_DUALDIFF)
    {
        return  display_requestdualdiff(dev);
    }
    else
    {
        return  display_requestdualsame(dev);
    }
}
      
/*
**********************************************************************************************************************
*                                               display_requestmodelock
*
* author:           
*
* date:             2011-7-17:11:25:34
*
* Description:      display requestmodelock 
*
* parameters:       
*
* return:           if success return GUI_RET_OK
*                   if fail return the number of fail
* modify history: 
**********************************************************************************************************************
*/

static int display_requestmodelock(struct display_device_t *dev)
{
    pthread_mutex_lock(&mode_lock);
    
    return 0;
}
      
/*
**********************************************************************************************************************
*                                               display_releasemodelock
*
* author:           
*
* date:             2011-7-17:11:25:38
*
* Description:      display releasemodelock 
*
* parameters:       
*
* return:           if success return GUI_RET_OK
*                   if fail return the number of fail
* modify history: 
**********************************************************************************************************************
*/

static int  display_releasemodelock(struct display_device_t *dev)
{
    pthread_mutex_unlock(&mode_lock);
    
    return 0;
}

static int display_singleswitchtosame(struct display_device_t *dev,int mode,bool masterchange)
{
	struct 	display_context_t*  ctx = (struct display_context_t*)dev;
    int							tvformat = 0;
    struct  display_fbpara_t	para;
    int                         min_width;
    int                         min_height;
    int                         bufid;
    
    if(masterchange == false)
    {
    	LOGD("g_display[1 - g_masterdisplay].type = %d\n",g_display[1 - g_masterdisplay].type);
    	if(g_display[1 - g_masterdisplay].type == DISPLAY_DEVICE_LCD)
	    {
	        if(g_masterdisplay == 0)
	        {
	            g_display[1 - g_masterdisplay].fbmode    = FB_MODE_SCREEN1;
	        }
	        else
	        {
	            g_display[1 - g_masterdisplay].fbmode    = FB_MODE_SCREEN0;
	        }
	        
	        para.fb_mode            = (__fb_mode_t)g_display[1 - g_masterdisplay].fbmode;
	        para.format             = HAL_PIXEL_FORMAT_BGRA_8888;
	        para.output_height      = g_display[1 - g_masterdisplay].height;
	        para.output_width       = g_display[1 - g_masterdisplay].width;
	        para.valid_height      	= g_display[1 - g_masterdisplay].valid_height;
	        para.valid_width       	= g_display[1 - g_masterdisplay].valid_width;
	        display_getminsize(ctx,&min_width,&min_height);
	            
            para.width          	= min_width;
            para.height         	= min_height;
            para.layer_mode     	= DISP_LAYER_WORK_MODE_SCALER;

            display_requestfb(ctx,1,&para);
	        
	        g_display[1 - g_masterdisplay].isopen         = DISPLAY_TRUE;
	        g_display[1 - g_masterdisplay].fb_height      = para.height;
	        g_display[1 - g_masterdisplay].fb_width       = para.width;
	        g_display[1 - g_masterdisplay].hotplug        = display_gethotplug(dev,1 - g_masterdisplay);
	        display_output(ctx,1 - g_masterdisplay,g_display[1 - g_masterdisplay].type,tvformat);
	    }
	    else
	    {
	        tvformat = get_tvformat(g_display[1 - g_masterdisplay].tvformat);
	        if(tvformat == -1)
	        {
	            LOGE("Invalid TV Format!\n");
	
	            return  -1;
	        }
	        
	        LOGD("tvformat = %d\n",tvformat);
	
	        if(g_masterdisplay == 0)
	        {
	            g_display[1].fbmode    = FB_MODE_SCREEN1;
	        }
	        else
	        {
	            g_display[1].fbmode    = FB_MODE_SCREEN0;
	        }
	        
	        para.fb_mode            = (__fb_mode_t)g_display[1 - g_masterdisplay].fbmode;
	        para.format             = HAL_PIXEL_FORMAT_BGRA_8888;
	        para.output_height      = g_display[1 - g_masterdisplay].height;
	        para.output_width       = g_display[1 - g_masterdisplay].width;
	        para.valid_height      	= g_display[1 - g_masterdisplay].valid_height;
	        para.valid_width       	= g_display[1 - g_masterdisplay].valid_width;
	        display_getminsize(ctx,&min_width,&min_height);
	        para.bufno				= 3;
            para.width          	= min_width;
            para.height         	= min_height;
            //para.layer_mode     	= DISP_LAYER_WORK_MODE_SCALER;
            //para.height      		= 720;
	        //para.width       		= 1280;
            para.layer_mode     	= DISP_LAYER_WORK_MODE_SCALER;

			LOGD("para.fb_mode = %d\n",para.fb_mode);
			LOGD("para.format = %d\n",para.format);
			LOGD("para.output_height = %d\n",para.output_height);
			LOGD("para.output_width = %d\n",para.output_width);
			LOGD("para.width = %d\n",para.width);
			LOGD("para.height = %d\n",para.height);
			LOGD("tvformat = %d\n",tvformat);
            display_requestfb(ctx,1,&para);
	        g_display[1 - g_masterdisplay].fb_id	      = 1;
	        g_display[1 - g_masterdisplay].isopen         = DISPLAY_TRUE;
	        g_display[1 - g_masterdisplay].fb_height      = para.height;
	        g_display[1 - g_masterdisplay].fb_width       = para.width;
	        g_display[1 - g_masterdisplay].hotplug        = display_gethotplug(dev,1 - g_masterdisplay);
	        bufid = display_getdisplaybufid(dev,g_masterdisplay);
	    	LOGD("bufid = %d\n",bufid);
			LOGD("g_display[g_masterdisplay].fb_id = %d\n",g_display[g_masterdisplay].fb_id);
			LOGD("g_display[1 - g_masterdisplay].fb_id = %d\n",g_display[1 - g_masterdisplay].fb_id);
	        display_output(ctx,1 - g_masterdisplay,g_display[1 - g_masterdisplay].type,tvformat);
	        display_copyfb(dev,g_display[g_masterdisplay].fb_id,bufid,g_display[1 - g_masterdisplay].fb_id,0);
	    	display_pandisplay(dev,g_display[1 - g_masterdisplay].fb_id,0);
	    }
	    
	    return  1;
    }
    else
    {
    	LOGD("display_releasemode1!\n");
        /*先为释放原有模式的资源*/
        display_releasemode(dev,mode);
        
        LOGD("display_releasemode2!\n");

        /*再为新设置的模式申请资源*/
        display_requestmode(dev,mode);
        
        LOGD("display_requestmode!\n");
    }
    
    return  0;
}

static int display_sameswitchtosingle(struct display_device_t *dev,int mode,bool masterchange)
{
	struct 	display_context_t*  ctx = (struct display_context_t*)dev;
    int							tvformat = 0;
    int 						outputtype;
    
    if(masterchange == false)
    {
    	outputtype = display_getoutputtype(dev,1 - g_masterdisplay);
            
        display_off(ctx,1 - g_masterdisplay,outputtype);

        display_releasefb(ctx,1);
	    
	    return  1;
    }
    else
    {
    	LOGD("display_releasemode1!\n");
        /*先为释放原有模式的资源*/
        display_releasemode(dev,mode);
        
        LOGD("display_releasemode2!\n");

        /*再为新设置的模式申请资源*/
        display_requestmode(dev,mode);
        
        LOGD("display_requestmode!\n");
    }
    
    return  0;	
}
      
/*
**********************************************************************************************************************
*                                               display_setmode
*
* author:           
*
* date:             2011-7-17:11:25:40
*
* Description:      display setmode 
*
* parameters:       
*
* return:           if success return GUI_RET_OK
*                   if fail return the number of fail
* modify history: 
**********************************************************************************************************************
*/

static int display_setmode(struct display_device_t *dev,int mode,struct display_modepara_t *para)
{
    struct 	display_context_t*  ctx = (struct display_context_t*)dev;
    int							tvformat = 0;
    bool						masterchange;
    int							ret = 0;

    pthread_mutex_lock(&mode_lock);
    
    if(g_displaymode != mode)
    {
    	if(para->d0type == DISPLAY_DEVICE_NONE || para->d1type == DISPLAY_DEVICE_NONE)
        {
            LOGE("input type error!\n");
            
            pthread_mutex_unlock(&mode_lock);
             
            return -1;
        }

        LOGD("para->d0format = %d,para->d0type = %d\n",para->d0format,para->d0type);

        if(para->d0type != DISPLAY_DEVICE_LCD)
        {
            tvformat = get_tvformat(para->d0format);
    		if(tvformat == -1)
    		{
    			LOGE("Invalid TV Format!\n");

				pthread_mutex_unlock(&mode_lock);
				
    			return  -1;
    		}

            g_display[0].height  		= display_getheight(ctx,0,tvformat);
            g_display[0].width   		= display_getwidth(ctx,0,tvformat);
            g_display[0].valid_height  	= display_getvalidheight(ctx,0,tvformat);
            g_display[0].valid_width   	= display_getvalidwidth(ctx,0,tvformat);
        }
        else
        {
            g_display[0].height  		= display_getheight(ctx,0,DISPLAY_DEFAULT);
            g_display[0].width   		= display_getwidth(ctx,0,DISPLAY_DEFAULT);
            g_display[0].valid_height 	= g_display[0].height;
            g_display[0].valid_width 	= g_display[0].width;
        }

        g_display[0].fbmode = FB_MODE_SCREEN0;
        g_display[0].type    = para->d0type;
        g_display[0].tvformat= para->d0format;
   		g_display[0].format  = para->d0pixelformat;
   		
   		if(para->d1type != DISPLAY_DEVICE_LCD)
        {
            tvformat = get_tvformat(para->d1format);
    		if(tvformat == -1)
    		{
    			LOGE("Invalid TV Format!\n");

				pthread_mutex_unlock(&mode_lock);
				
    			return  -1;
    		}

            g_display[1].height  		= display_getheight(ctx,1,tvformat);
            g_display[1].width   		= display_getwidth(ctx,1,tvformat);
            g_display[1].valid_height  	= display_getvalidheight(ctx,1,tvformat);
            g_display[1].valid_width   	= display_getvalidwidth(ctx,1,tvformat);
        }
        else
        {
            g_display[1].height  		= display_getheight(ctx,1,DISPLAY_DEFAULT);
            g_display[1].width   		= display_getwidth(ctx,1,DISPLAY_DEFAULT);
            g_display[1].valid_height 	= g_display[1].height;
            g_display[1].valid_width 	= g_display[1].width;
        }

        g_display[1].fbmode 	= FB_MODE_SCREEN0;
        g_display[1].type    	= para->d1type;
        g_display[1].tvformat	= para->d1format;
   		g_display[1].format  	= para->d1pixelformat;
    	if(para->masterdisplay == g_masterdisplay)
    	{
    		masterchange = false;
    	}
    	else
    	{
    		masterchange = true;
    	}
    	
    	//LOGD("g_displaymode1 = %d,mode = %d\n",g_displaymode,mode);
    	
    	if((g_displaymode == DISPLAY_MODE_SINGLE) && (mode == DISPLAY_MODE_DUALSAME))
    	{
            g_displaymode = mode;
    		ret = display_singleswitchtosame(dev,mode,false);
    	}
    	else if((mode == DISPLAY_MODE_SINGLE) && (g_displaymode == DISPLAY_MODE_DUALSAME))
    	{	
            g_displaymode = mode;
    		ret = display_sameswitchtosingle(dev,mode,false);
    	}
    	else
    	{
            g_displaymode = mode;
	    	//LOGD("display_releasemode1!\n");
	        /*先为释放原有模式的资源*/
	        display_releasemode(dev,mode);
	        
	        //LOGD("display_releasemode2!\n");
	
	        /*再为新设置的模式申请资源*/
	        display_requestmode(dev,mode);
	        
	        //LOGD("display_requestmode!\n");
    	}
        
        pthread_mutex_unlock(&mode_lock);

        return  ret;
    }
    pthread_mutex_unlock(&mode_lock);

    return  1;
}
      
/*
**********************************************************************************************************************
*                                               display_singlesetmaster
*
* author:           
*
* date:             2011-7-17:11:25:42
*
* Description:      display singlesetmaster 
*
* parameters:       
*
* return:           if success return GUI_RET_OK
*                   if fail return the number of fail
* modify history: 
**********************************************************************************************************************
*/

static int display_singlesetmaster(struct display_device_t *dev,int master)
{
    struct 						display_context_t* ctx = (struct display_context_t*)dev;
    struct display_fbpara_t		para;
    int							tvformat = 0;
    
    if(g_masterdisplay != master)
    {
        int        value0;
        int        value1;

        value0  = g_display[master].type;
        value1  = g_display[master].tvformat;

        if(g_display[g_masterdisplay].type == DISPLAY_DEVICE_NONE || value0 == DISPLAY_DEVICE_NONE)
        {
            LOGE("change output mode from DISPLAY_DEVICE_NONE or to DISPLAY_DEVICE_NONE not support!\n");

            return  -1;
        }
        
        display_off(ctx,g_masterdisplay,g_display[g_masterdisplay].type);

        display_releasefb(ctx,0);

        if(master == 0)
        {
            g_display[g_masterdisplay].fbmode    = FB_MODE_SCREEN0;
        }
        else
        {
            g_display[g_masterdisplay].fbmode    = FB_MODE_SCREEN1;
        }

        para.format                         = g_display[g_masterdisplay].format;
        para.layer_mode                     = DISP_LAYER_WORK_MODE_NORMAL;
        g_display[master].fb_id             = 0;
        g_display[master].layermode         = DISP_LAYER_WORK_MODE_NORMAL;
        g_display[master].format            = g_display[g_masterdisplay].format;
        g_display[master].isopen            = DISPLAY_TRUE;
        if(value0 == DISPLAY_DEVICE_LCD)
        {
 			para.height     		= display_getheight(ctx,master,DISPLAY_DEFAULT);
            para.width      		= display_getwidth(ctx,master,DISPLAY_DEFAULT);
            para.output_height      = para.height;
            para.output_width       = para.width;
            para.valid_height      	= para.height;
            para.valid_width       	= para.width;
            
            display_requestfb(ctx,g_display[master].fb_id,&para);
            display_output(ctx,master,DISPLAY_DEVICE_LCD,tvformat);
        }
        else
        {
            tvformat = get_tvformat(value1);
			if(tvformat == -1)
			{
				LOGE("Invalid TV Format!\n");
				
				return  -1;
			}
        
            para.height     		= display_getheight(ctx,master,value1);
            para.width      		= display_getwidth(ctx,master,value1);
            para.output_height      = para.height;
			para.output_width       = para.width;
			para.valid_height      	= para.height;
			para.valid_width       	= para.width;
            para.layer_mode = DISP_LAYER_WORK_MODE_NORMAL;
            display_requestfb(ctx,g_display[master].fb_id,&para);

            g_display[master].hotplug        = display_gethotplug(dev,master);
            display_output(ctx,master,value0,tvformat);
        }

        g_masterdisplay = master;
        
        return  0;
    }

    return 1;
}
      
/*
**********************************************************************************************************************
*                                               display_duallcdsetmaster
*
* author:           
*
* date:             2011-7-17:11:25:46
*
* Description:      display duallcdsetmaster 
*
* parameters:       
*
* return:           if success return GUI_RET_OK
*                   if fail return the number of fail
* modify history: 
**********************************************************************************************************************
*/

static int display_duallcdsetmaster(struct display_device_t *dev,int master)
{
    g_masterdisplay = master;
    return 0;
}
      
/*
**********************************************************************************************************************
*                                               display_dualdiffsetmaster
*
* author:           
*
* date:             2011-7-17:11:25:48
*
* Description:      display dualdiffsetmaster 
*
* parameters:       
*
* return:           if success return GUI_RET_OK
*                   if fail return the number of fail
* modify history: 
**********************************************************************************************************************
*/

static int display_dualdiffsetmaster(struct display_device_t *dev,int master)
{
    g_masterdisplay = master;
    return 0;
}
      
/*
**********************************************************************************************************************
*                                               display_dualsamesetmaster
*
* author:           
*
* date:             2011-7-17:11:25:50
*
* Description:      display dualsamesetmaster 
*
* parameters:       
*
* return:           if success return GUI_RET_OK
*                   if fail return the number of fail
* modify history: 
**********************************************************************************************************************
*/

static int display_dualsamesetmaster(struct display_device_t *dev,int master)
{
    struct 						display_context_t* ctx = (struct display_context_t*)dev;
    struct display_fbpara_t		para;
    int                         minwidth;
    int                         minheight;
    int							tvformat = 0;
    
    if(g_masterdisplay != master)
    {
        int        value0;
        int        value1;

        value0  = g_display[master].type;
        value1  = g_display[master].tvformat;

        if(g_display[g_masterdisplay].type == DISPLAY_DEVICE_NONE || value0 == DISPLAY_DEVICE_NONE)
        {
            LOGE("change output mode from DISPLAY_DEVICE_NONE or to DISPLAY_DEVICE_NONE not support!\n");

            return  -1;
        }
        
        display_off(ctx,g_masterdisplay,g_display[g_masterdisplay].type);
        display_off(ctx,master,g_display[master].type);
        display_releasefb(ctx,0);
        display_releasefb(ctx,1);

        if(master == 0)
        {
            g_display[master].fbmode    = FB_MODE_SCREEN0;
        }
        else
        {
            g_display[master].fbmode    = FB_MODE_SCREEN1;
        }

        para.format                         = g_display[g_masterdisplay].format;
        para.layer_mode                     = DISP_LAYER_WORK_MODE_NORMAL;
        g_display[master].fb_id             = 0;
        g_display[master].layermode         = DISP_LAYER_WORK_MODE_NORMAL;
        g_display[master].format            = g_display[g_masterdisplay].format;
        g_display[master].isopen            = DISPLAY_TRUE;
        if(value0 == DISPLAY_DEVICE_LCD)
        {
 			para.height     		= display_getheight(ctx,master,DISPLAY_DEFAULT);
            para.width      		= display_getwidth(ctx,master,DISPLAY_DEFAULT);
            para.output_height      = para.height;
            para.output_width       = para.width;
            para.valid_height      	= para.height;
            para.valid_width       	= para.width;
            
            display_requestfb(ctx,g_display[master].fb_id,&para);
        }
        else
        {
            tvformat = get_tvformat(value1);
			if(tvformat == -1)
			{
				LOGE("Invalid TV Format!\n");
				
				return  -1;
			}
        
            para.height     		= display_getvalidheight(ctx,master,value1);
            para.width      		= display_getvalidwidth(ctx,master,value1);
            para.output_height      = display_getheight(ctx,master,value1);
            para.output_width       = display_getwidth(ctx,master,value1);
            para.valid_height       = para.height;
            para.valid_width        = para.width;
            
            para.layer_mode 		= DISP_LAYER_WORK_MODE_NORMAL;
            display_requestfb(ctx,g_display[master].fb_id,&para);
        }

        if(g_masterdisplay == 0)
        {
            g_display[g_masterdisplay].fbmode       = FB_MODE_SCREEN0;
        }
        else
        {
            g_display[g_masterdisplay].fbmode       = FB_MODE_SCREEN1;
        }

        para.format                                 = g_display[g_masterdisplay].format;
        para.layer_mode                             = DISP_LAYER_WORK_MODE_SCALER;
        g_display[g_masterdisplay].fb_id            = 1;
        g_display[g_masterdisplay].layermode        = DISP_LAYER_WORK_MODE_SCALER;
        g_display[g_masterdisplay].format           = g_display[g_masterdisplay].format;
        g_display[g_masterdisplay].isopen           = DISPLAY_TRUE;
        display_getminsize(ctx,&minwidth,&minheight);
        para.width                                  = minwidth;
        para.height                                 = minheight;
        if(value0 == DISPLAY_DEVICE_LCD)
        {
 			para.output_height      = display_getheight(ctx,g_masterdisplay,DISPLAY_DEFAULT);
            para.output_width       = display_getwidth(ctx,g_masterdisplay,DISPLAY_DEFAULT);
            para.valid_height		= para.output_height;
            para.valid_width		= para.output_width;
            
            display_requestfb(ctx,g_display[g_masterdisplay].fb_id,&para);
        }
        else
        {
            tvformat = get_tvformat(value1);
			if(tvformat == -1)
			{
				LOGE("Invalid TV Format!\n");
				
				return  -1;
			}
        
            para.output_height      = display_getheight(ctx,g_masterdisplay,value1);
            para.output_width       = display_getwidth(ctx,g_masterdisplay,value1);
            para.valid_height       = display_getvalidheight(ctx,g_masterdisplay,value1);
            para.valid_width        = display_getvalidwidth(ctx,g_masterdisplay,value1);
            
            display_requestfb(ctx,g_display[g_masterdisplay].fb_id,&para);
        }

        display_gethotplug(dev,g_masterdisplay);
        display_output(ctx,g_masterdisplay,g_display[g_masterdisplay].type,tvformat);
        display_output(ctx,master,g_display[master].type,tvformat);
        g_masterdisplay = master;
        
        return 0;
    }

    return  1;
}
      
/*
**********************************************************************************************************************
*                                               display_setmasterdisplay
*
* author:           
*
* date:             2011-7-17:11:25:55
*
* Description:      display setmasterdisplay 
*
* parameters:       
*
* return:           if success return GUI_RET_OK
*                   if fail return the number of fail
* modify history: 
**********************************************************************************************************************
*/

static int display_setmasterdisplay(struct display_device_t *dev,int master)
{
    int     ret;

    pthread_mutex_lock(&mode_lock);
    if(g_displaymode == DISPLAY_MODE_SINGLE)
    {  
        ret = display_singlesetmaster(dev,master);
    }
    else if(g_displaymode == DISPLAY_MODE_DUALSAME)
    {
        ret = display_dualsamesetmaster(dev,master);
    }
    else if(g_displaymode == DISPLAY_MODE_DUALDIFF)
    {
        ret = display_dualdiffsetmaster(dev,master);;
    }
    else
    {
        ret = display_duallcdsetmaster(dev,master);
    }

    pthread_mutex_unlock(&mode_lock);

    return  ret;
}
      
/*
**********************************************************************************************************************
*                                               display_getmasterdisplay
*
* author:           
*
* date:             2011-7-17:11:21:51
*
* Description:      display getmasterdisplay 
*
* parameters:       
*
* return:           if success return GUI_RET_OK
*                   if fail return the number of fail
* modify history: 
**********************************************************************************************************************
*/

static int display_getmasterdisplay(struct display_device_t *dev)
{
    return  g_masterdisplay;
}
      
/*
**********************************************************************************************************************
*                                               display_getdisplaymode
*
* author:           
*
* date:             2011-7-17:11:25:59
*
* Description:      display getdisplaymode 
*
* parameters:       
*
* return:           if success return GUI_RET_OK
*                   if fail return the number of fail
* modify history: 
**********************************************************************************************************************
*/

static int display_getdisplaymode(struct display_device_t *dev)
{   
    return  g_displaymode;
}
      
/*
**********************************************************************************************************************
*                                               display_opendev
*
* author:           
*
* date:             2011-7-17:11:26:1
*
* Description:      display opendev 
*
* parameters:       
*
* return:           if success return GUI_RET_OK
*                   if fail return the number of fail
* modify history: 
**********************************************************************************************************************
*/

static int display_opendev(struct display_device_t *dev,int displayno)
{
    struct display_context_t* ctx = (struct display_context_t*)dev;
    int                       ret;

    pthread_mutex_lock(&mode_lock);
    ret = display_on(ctx,displayno,g_display[displayno].type);
    if(ret == 0)
    {
        g_display[displayno].isopen = DISPLAY_TRUE;
    }

    pthread_mutex_unlock(&mode_lock);

    return ret;
}

static int display_globalinit(struct display_device_t *dev)
{
	//g_display[0].type  	= display_getoutputtype(dev,0);
	//g_display[1].type  	= display_getoutputtype(dev,1);
	//g_display[0].format = display_gettvformat(dev,0);
	//g_display[1].format = display_gettvformat(dev,1);
	//g_display[1].fb_id  =
	g_display[0].type  			= DISPLAY_DEVICE_LCD;
	g_display[0].format 		= HAL_PIXEL_FORMAT_BGRA_8888;
	g_display[0].width 			= 800;
	g_display[0].height 		= 480;  
	g_display[0].fbmode			= FB_MODE_SCREEN0;
	g_display[0].layermode		= DISP_LAYER_WORK_MODE_NORMAL;
	g_display[0].fb_width 		= 800; 
	g_display[0].fb_height 		= 480; 
	
	return  0;
}
      
/*
**********************************************************************************************************************
*                                               display_closedev
*
* author:           
*
* date:             2011-7-17:11:26:3
*
* Description:      display closedev 
*
* parameters:       
*
* return:           if success return GUI_RET_OK
*                   if fail return the number of fail
* modify history: 
**********************************************************************************************************************
*/

static int display_closedev(struct display_device_t *dev,int displayno)
{
    struct display_context_t* ctx = (struct display_context_t*)dev;
    int                       ret;

    pthread_mutex_lock(&mode_lock);
    
    ret = display_off(ctx,displayno,g_display[displayno].type);
    if(ret == 0)
    {
        g_display[displayno].isopen = DISPLAY_FALSE;
    }

    pthread_mutex_unlock(&mode_lock);

    return ret;    
}

static int display_getdisplaycount(struct display_device_t *dev)
{
	return MAX_DISPLAY_NUM;
}
      
/*
**********************************************************************************************************************
*                                               close_display
*
* author:           
*
* date:             2011-7-17:11:26:14
*
* Description:      close display 
*
* parameters:       
*
* return:           if success return GUI_RET_OK
*                   if fail return the number of fail
* modify history: 
**********************************************************************************************************************
*/

static int close_display(struct hw_device_t *dev) 
{
    int    i;
    
    struct display_context_t* ctx = (struct display_context_t*)dev;
    if (ctx) 
    {
        if(ctx->mFD_disp)
        {
            close(ctx->mFD_disp);
        }

        if(ctx->mFD_mp)
        {
            close(ctx->mFD_mp);
        }

        for(i = 0;i < MAX_DISPLAY_NUM;i++)
        {
            if(ctx->mFD_fb[i])
            {
                close(ctx->mFD_fb[i]);
            }
        }
        
        free(ctx);
    }
    return 0;
}
      
/*
**********************************************************************************************************************
*                                               open_display
*
* author:           
*
* date:             2011-7-17:11:26:27
*
* Description:      open display 
*
* parameters:       
*
* return:           if success return GUI_RET_OK
*                   if fail return the number of fail
* modify history: 
**********************************************************************************************************************
*/

static int open_display(const struct hw_module_t* module, const char* name,
        struct hw_device_t** device)
{
    int status = 0;
    display_context_t *ctx;
    ctx = (display_context_t *)malloc(sizeof(display_context_t));
    memset(ctx, 0, sizeof(*ctx));

    ctx->device.common.tag          = HARDWARE_DEVICE_TAG;
    ctx->device.common.version      = 1;
    ctx->device.common.module       = const_cast<hw_module_t*>(module);
    ctx->device.common.close        = close_display;
    ctx->device.changemode          = display_changemode;
    ctx->device.setdisplaymode      = display_setmode;
    ctx->device.setdisplayparameter = display_setparameter;
    ctx->device.gethdmistatus       = display_gethdmistatus;
    ctx->device.opendisplay         = display_opendev;
    ctx->device.closedisplay        = display_closedev;
    ctx->device.getdisplayparameter = display_getparameter;
    ctx->device.copysrcfbtodstfb    = display_copyfb;
    ctx->device.pandisplay          = display_pandisplay;
    ctx->device.request_modelock    = display_requestmodelock;
    ctx->device.release_modelock    = display_releasemodelock;
    ctx->device.setmasterdisplay    = display_setmasterdisplay;
    ctx->device.getmasterdisplay    = display_getmasterdisplay;
    ctx->device.getdisplaybufid     = display_getdisplaybufid;
    ctx->device.getmaxwidthdisplay  = display_getmaxdisplayno;
    ctx->device.getdisplaycount  	= display_getdisplaycount;
    ctx->device.getdisplaymode		= display_getdisplaymode;
    ctx->device.gethdmimaxmode		= display_gethdmimaxmode;

    //LOGD("start open_display!\n");
    ctx->mFD_disp = open("/dev/disp", O_RDWR, 0);
    LOGD("start open_display!ctx->mFD_disp = %x\n",ctx->mFD_disp);
    if (ctx->mFD_disp < 0) 
    {
        status = errno;
        LOGE("Error opening frame buffer errno=%d (%s)",
             status, strerror(status));
        status = -status;
    } 

    if (status == 0) 
    {
        *device = &ctx->device.common;
    } 
    else 
    {
        close_display(&ctx->device.common);
    }

    if(mutex_inited == false)
    {
        pthread_mutex_init(&mode_lock, NULL);

		//display_globalinit(NULL);
		
        mutex_inited = true;
    }
    
    return status;
}

