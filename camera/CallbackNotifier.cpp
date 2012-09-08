/*
 * Copyright (C) 2011 The Android Open Source Project
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

/*
 * Contains implementation of a class CallbackNotifier that manages callbacks set
 * via set_callbacks, enable_msg_type, and disable_msg_type camera HAL API.
 */

#define LOG_TAG "CallbackNotifier"
#include "CameraDebug.h"

#include <media/hardware/MetadataBufferType.h>
#include <type_camera.h>
#include "V4L2Camera.h"
#include "CallbackNotifier.h"
#include "JpegCompressor.h"

extern "C" int JpegEnc(void * pBufOut, int * bufSize, JPEG_ENC_t *jpeg_enc);

namespace android {

/* String representation of camera messages. */
static const char* lCameraMessages[] =
{
    "CAMERA_MSG_ERROR",
    "CAMERA_MSG_SHUTTER",
    "CAMERA_MSG_FOCUS",
    "CAMERA_MSG_ZOOM",
    "CAMERA_MSG_PREVIEW_FRAME",
    "CAMERA_MSG_VIDEO_FRAME",
    "CAMERA_MSG_POSTVIEW_FRAME",
    "CAMERA_MSG_RAW_IMAGE",
    "CAMERA_MSG_COMPRESSED_IMAGE",
    "CAMERA_MSG_RAW_IMAGE_NOTIFY",
    "CAMERA_MSG_PREVIEW_METADATA"
};
static const int lCameraMessagesNum = sizeof(lCameraMessages) / sizeof(char*);

/* Builds an array of strings for the given set of messages.
 * Param:
 *  msg - Messages to get strings for,
 *  strings - Array where to save strings
 *  max - Maximum number of entries in the array.
 * Return:
 *  Number of strings saved into the 'strings' array.
 */
static int GetMessageStrings(uint32_t msg, const char** strings, int max)
{
    int index = 0;
    int out = 0;
    while (msg != 0 && out < max && index < lCameraMessagesNum) {
        while ((msg & 0x1) == 0 && index < lCameraMessagesNum) {
            msg >>= 1;
            index++;
        }
        if ((msg & 0x1) != 0 && index < lCameraMessagesNum) {
            strings[out] = lCameraMessages[index];
            out++;
            msg >>= 1;
            index++;
        }
    }

    return out;
}

/* Logs messages, enabled by the mask. */
static void PrintMessages(uint32_t msg)
{
    const char* strs[lCameraMessagesNum];
    const int translated = GetMessageStrings(msg, strs, lCameraMessagesNum);
    for (int n = 0; n < translated; n++) {
        ALOGV("    %s", strs[n]);
    }
}

CallbackNotifier::CallbackNotifier()
    : mNotifyCB(NULL),
      mDataCB(NULL),
      mDataCBTimestamp(NULL),
      mGetMemoryCB(NULL),
      mCallbackCookie(NULL),
      mLastFrameTimestamp(0),
      mFrameRefreshFreq(0),
      mMessageEnabler(0),
      mJpegQuality(90),
      mVideoRecEnabled(false),
      mTakingPicture(false),
      mUseMetaDataBufferMode(false),
      mGpsLatitude(0.0),
	  mGpsLongitude(0.0),
	  mGpsAltitude(0),
	  mGpsTimestamp(0),
	  mThumbWidth(0),
	  mThumbHeight(0),
	  mFocalLength(0.0)
{
	memset(mGpsMethod, 0, 100);
}

CallbackNotifier::~CallbackNotifier()
{
}

/****************************************************************************
 * Camera API
 ***************************************************************************/

void CallbackNotifier::setCallbacks(camera_notify_callback notify_cb,
                                    camera_data_callback data_cb,
                                    camera_data_timestamp_callback data_cb_timestamp,
                                    camera_request_memory get_memory,
                                    void* user)
{
    ALOGV("%s: %p, %p, %p, %p (%p)",
         __FUNCTION__, notify_cb, data_cb, data_cb_timestamp, get_memory, user);

    Mutex::Autolock locker(&mObjectLock);
    mNotifyCB = notify_cb;
    mDataCB = data_cb;
    mDataCBTimestamp = data_cb_timestamp;
    mGetMemoryCB = get_memory;
    mCallbackCookie = user;
}

void CallbackNotifier::enableMessage(uint msg_type)
{
    ALOGV("%s: msg_type = 0x%x", __FUNCTION__, msg_type);
    PrintMessages(msg_type);

    Mutex::Autolock locker(&mObjectLock);
    mMessageEnabler |= msg_type;
    ALOGV("**** Currently enabled messages:");
    PrintMessages(mMessageEnabler);
}

void CallbackNotifier::disableMessage(uint msg_type)
{
    ALOGV("%s: msg_type = 0x%x", __FUNCTION__, msg_type);
    PrintMessages(msg_type);

    Mutex::Autolock locker(&mObjectLock);
    mMessageEnabler &= ~msg_type;
    ALOGV("**** Currently enabled messages:");
    PrintMessages(mMessageEnabler);
}

status_t CallbackNotifier::enableVideoRecording(int fps)
{
    ALOGV("%s: FPS = %d", __FUNCTION__, fps);

    Mutex::Autolock locker(&mObjectLock);
    mVideoRecEnabled = true;
    mLastFrameTimestamp = 0;
    mFrameRefreshFreq = 1000000000LL / fps;

    return NO_ERROR;
}

void CallbackNotifier::disableVideoRecording()
{
    ALOGV("%s:", __FUNCTION__);

    Mutex::Autolock locker(&mObjectLock);
    mVideoRecEnabled = false;
    mLastFrameTimestamp = 0;
    mFrameRefreshFreq = 0;
}

void CallbackNotifier::releaseRecordingFrame(const void* opaque)
{
    /* We don't really have anything to release here, since we report video
     * frames by copying them directly to the camera memory. */
}

status_t CallbackNotifier::storeMetaDataInBuffers(bool enable)
{
    /* Return INVALID_OPERATION means HAL does not support metadata. So HAL will
     * return actual frame data with CAMERA_MSG_VIDEO_FRAME. Return
     * INVALID_OPERATION to mean metadata is not supported. */
     
	ALOGD("storeMetaDataInBuffers, %s", enable ? "true" : "false");
    mUseMetaDataBufferMode = enable;

    return NO_ERROR;
}

/****************************************************************************
 * Public API
 ***************************************************************************/

void CallbackNotifier::cleanupCBNotifier()
{
    Mutex::Autolock locker(&mObjectLock);
    mMessageEnabler = 0;
    mNotifyCB = NULL;
    mDataCB = NULL;
    mDataCBTimestamp = NULL;
    mGetMemoryCB = NULL;
    mCallbackCookie = NULL;
    mLastFrameTimestamp = 0;
    mFrameRefreshFreq = 0;
    mJpegQuality = 90;
    mVideoRecEnabled = false;
    mTakingPicture = false;
}

void CallbackNotifier::onNextFrameAvailable(const void* frame,
                                            nsecs_t timestamp,
                                            V4L2Camera* camera_dev,
                                         	bool bUseMataData)
{
    if (bUseMataData)
    {
    	onNextFrameHW(frame, timestamp, camera_dev);
    }
	else
	{
    	onNextFrameSW(frame, timestamp, camera_dev);
	}
}

void CallbackNotifier::onNextFrameHW(const void* frame,
			                            nsecs_t timestamp,
			                            V4L2Camera* camera_dev)
{
	if (isMessageEnabled(CAMERA_MSG_VIDEO_FRAME) && isVideoRecordingEnabled() &&
            isNewVideoFrameTime(timestamp)) 
	{
        camera_memory_t* cam_buff = mGetMemoryCB(-1, sizeof(V4L2BUF_t), 1, NULL);
        if (NULL != cam_buff && NULL != cam_buff->data) 
		{
            memcpy(cam_buff->data, frame, sizeof(V4L2BUF_t));
            mDataCBTimestamp(timestamp, CAMERA_MSG_VIDEO_FRAME,
                               cam_buff, 0, mCallbackCookie);
			cam_buff->release(cam_buff);
        } 
		else 
		{
            ALOGE("%s: Memory failure in CAMERA_MSG_VIDEO_FRAME", __FUNCTION__);
        }
    }

    if (isMessageEnabled(CAMERA_MSG_PREVIEW_FRAME)) 
	{
        camera_memory_t* cam_buff = mGetMemoryCB(-1, sizeof(V4L2BUF_t), 1, NULL);
        if (NULL != cam_buff && NULL != cam_buff->data) 
		{
            memcpy(cam_buff->data, frame, sizeof(V4L2BUF_t));
			mDataCB(CAMERA_MSG_PREVIEW_FRAME, cam_buff, 0, NULL, mCallbackCookie);
			cam_buff->release(cam_buff);
        } 
		else 
		{
            ALOGE("%s: Memory failure in CAMERA_MSG_PREVIEW_FRAME", __FUNCTION__);
        }
    }
}

void CallbackNotifier::onNextFrameSW(const void* frame,
		                               nsecs_t timestamp,
		                               V4L2Camera* camera_dev)
{
	if (isMessageEnabled(CAMERA_MSG_VIDEO_FRAME) && isVideoRecordingEnabled() &&
            isNewVideoFrameTime(timestamp)) {
        camera_memory_t* cam_buff =
            mGetMemoryCB(-1, camera_dev->getFrameBufferSize(), 1, NULL);
        if (NULL != cam_buff && NULL != cam_buff->data) {
            memcpy(cam_buff->data, frame, camera_dev->getFrameBufferSize());
            mDataCBTimestamp(timestamp, CAMERA_MSG_VIDEO_FRAME,
                               cam_buff, 0, mCallbackCookie);
			cam_buff->release(cam_buff);		// star add
        } else {
            ALOGE("%s: Memory failure in CAMERA_MSG_VIDEO_FRAME", __FUNCTION__);
        }
    }

    if (isMessageEnabled(CAMERA_MSG_PREVIEW_FRAME)) {
        camera_memory_t* cam_buff =
            mGetMemoryCB(-1, camera_dev->getFrameBufferSize(), 1, NULL);
        if (NULL != cam_buff && NULL != cam_buff->data) {
            memcpy(cam_buff->data, frame, camera_dev->getFrameBufferSize());
            mDataCB(CAMERA_MSG_PREVIEW_FRAME, cam_buff, 0, NULL, mCallbackCookie);
            cam_buff->release(cam_buff);
        } else {
            ALOGE("%s: Memory failure in CAMERA_MSG_PREVIEW_FRAME", __FUNCTION__);
        }
    }
}

status_t CallbackNotifier::autoFocus()
{
	if (isMessageEnabled(CAMERA_MSG_FOCUS))
        mNotifyCB(CAMERA_MSG_FOCUS, true, 0, mCallbackCookie);
    return NO_ERROR;
}

void CallbackNotifier::takePicture(const void* frame, V4L2Camera* camera_dev, bool bUseMataData)
{
	if (bUseMataData)
	{
		takePictureHW(frame, camera_dev);
	}
	else
	{
		takePictureSW(frame, camera_dev);
	}
}

void CallbackNotifier::takePictureHW(const void* frame, V4L2Camera* camera_dev)
{
	if (!mTakingPicture) 
	{
		return ;
	}
	
	ALOGD("%s, taking photo begin", __FUNCTION__);
    /* This happens just once. */
    mTakingPicture = false;
    /* The sequence of callbacks during picture taking is:
     *  - CAMERA_MSG_SHUTTER
     *  - CAMERA_MSG_RAW_IMAGE_NOTIFY
     *  - CAMERA_MSG_COMPRESSED_IMAGE
     */
    if (isMessageEnabled(CAMERA_MSG_SHUTTER)) 
	{
		F_LOG;
        mNotifyCB(CAMERA_MSG_SHUTTER, 0, 0, mCallbackCookie);
    }
	
    if (isMessageEnabled(CAMERA_MSG_RAW_IMAGE_NOTIFY)) 
	{
		F_LOG;
		camera_memory_t* cam_buff =
        	mGetMemoryCB(-1, camera_dev->getFrameBufferSize(), 1, NULL);
        if (NULL != cam_buff && NULL != cam_buff->data) 
		{
            memset(cam_buff->data, 0xff, camera_dev->getFrameBufferSize());
			mDataCB(CAMERA_MSG_RAW_IMAGE_NOTIFY, cam_buff, 0, NULL, mCallbackCookie);
			// mNotifyCB(CAMERA_MSG_RAW_IMAGE_NOTIFY, 0, 0, mCallbackCookie);
			// mNotifyCB(CAMERA_MSG_RAW_IMAGE_NOTIFY, cam_buff, 0, NULL, mCallbackCookie);
            cam_buff->release(cam_buff);
        } 
		else 
		{
            ALOGE("%s: Memory failure in CAMERA_MSG_PREVIEW_FRAME", __FUNCTION__);
        }
    }
	
    if (isMessageEnabled(CAMERA_MSG_COMPRESSED_IMAGE)) 
	{
		V4L2BUF_t * pbuf = (V4L2BUF_t *)frame;
		void * pOutBuf = NULL;
		int bufSize = 0;
		int pic_w, pic_h;

		camera_dev->getPictureSize(&pic_w, &pic_h);
			
        JPEG_ENC_t jpeg_enc;
		memset(&jpeg_enc, 0, sizeof(jpeg_enc));
		jpeg_enc.addrY			= pbuf->addrPhyY;
		jpeg_enc.addrC			= pbuf->addrPhyY + camera_dev->getFrameWidth() * camera_dev->getFrameHeight();
		jpeg_enc.src_w			= camera_dev->getFrameWidth();
		jpeg_enc.src_h			= camera_dev->getFrameHeight();
		jpeg_enc.pic_w			= pic_w;
		jpeg_enc.pic_h			= pic_h;
		jpeg_enc.colorFormat	= JPEG_COLOR_YUV420;
		jpeg_enc.quality		= mJpegQuality;
		jpeg_enc.rotate			= mJpegRotate;
		
		// do not use thumb now
		jpeg_enc.thumbWidth		= 0; // mThumbWidth;
		jpeg_enc.thumbHeight	= 0; // mThumbHeight;

		jpeg_enc.focal_length	= mFocalLength;

		if (0 != strlen(mGpsMethod))
		{
			jpeg_enc.enable_gps			= 1;
			jpeg_enc.gps_latitude		= mGpsLatitude;
			jpeg_enc.gps_longitude		= mGpsLongitude;
			jpeg_enc.gps_altitude		= mGpsAltitude;
			jpeg_enc.gps_timestamp		= mGpsTimestamp;
			strcpy(jpeg_enc.gps_processing_method, mGpsMethod);
			memset(mGpsMethod, 0, sizeof(mGpsMethod));
		}
		else
		{
			jpeg_enc.enable_gps			= 0;
		}
		
		ALOGD("addrY: %x, src: %dx%d, pic: %dx%d, quality: %d, rotate: %d,Gps method: %s, thumbW: %d, thumbH: %d", 
			jpeg_enc.addrY, 
			jpeg_enc.src_w, jpeg_enc.src_h,
			jpeg_enc.pic_w, jpeg_enc.pic_h,
			jpeg_enc.quality, jpeg_enc.rotate,
			jpeg_enc.gps_processing_method,
			jpeg_enc.thumbWidth,
			jpeg_enc.thumbHeight);
		
		pOutBuf = (void *)malloc(jpeg_enc.pic_w * jpeg_enc.pic_h << 2);
		if (pOutBuf == NULL)
		{
			ALOGE("malloc picture memory failed");
			return ;
		}
		
		int ret = JpegEnc(pOutBuf, &bufSize, &jpeg_enc);
		if (ret < 0)
		{
			ALOGE("JpegEnc failed");
			return ;			
		}

		camera_memory_t* jpeg_buff = mGetMemoryCB(-1, bufSize, 1, NULL);
		if (NULL != jpeg_buff && NULL != jpeg_buff->data) 
		{
			memcpy(jpeg_buff->data, (uint8_t *)pOutBuf, bufSize); 
			mDataCB(CAMERA_MSG_COMPRESSED_IMAGE, jpeg_buff, 0, NULL, mCallbackCookie);
			jpeg_buff->release(jpeg_buff);
		} 
		else 
		{
			ALOGE("%s: Memory failure in CAMERA_MSG_COMPRESSED_IMAGE", __FUNCTION__);
		}

		if(pOutBuf != NULL)
		{
			free(pOutBuf);
			pOutBuf = NULL;
		}
    }

	ALOGD("taking photo to CAMERA_MSG_POSTVIEW_FRAME");

	if (isMessageEnabled(CAMERA_MSG_POSTVIEW_FRAME) )
	{
		F_LOG;
		camera_memory_t* cam_buff =
        	mGetMemoryCB(-1, camera_dev->getFrameBufferSize(), 1, NULL);
        if (NULL != cam_buff && NULL != cam_buff->data) 
		{
            memset(cam_buff->data, 0xff, camera_dev->getFrameBufferSize());
			mDataCB(CAMERA_MSG_POSTVIEW_FRAME, cam_buff, 0, NULL, mCallbackCookie);
            cam_buff->release(cam_buff);
        } 
		else 
		{
            ALOGE("%s: Memory failure in CAMERA_MSG_PREVIEW_FRAME", __FUNCTION__);
        }
	}
	
	ALOGD("taking photo end");
}

void CallbackNotifier::takePictureSW(const void* frame, V4L2Camera* camera_dev)
{
	if (!mTakingPicture) 
	{
		return ;
	}
	
	ALOGD("%s, taking photo begin", __FUNCTION__);
    /* This happens just once. */
    mTakingPicture = false;
    /* The sequence of callbacks during picture taking is:
     *  - CAMERA_MSG_SHUTTER
     *  - CAMERA_MSG_RAW_IMAGE_NOTIFY
     *  - CAMERA_MSG_COMPRESSED_IMAGE
     */
    if (isMessageEnabled(CAMERA_MSG_SHUTTER)) {
        mNotifyCB(CAMERA_MSG_SHUTTER, 0, 0, mCallbackCookie);
    }
    if (isMessageEnabled(CAMERA_MSG_RAW_IMAGE_NOTIFY)) {
        mNotifyCB(CAMERA_MSG_RAW_IMAGE_NOTIFY, 0, 0, mCallbackCookie);
    }
    if (isMessageEnabled(CAMERA_MSG_COMPRESSED_IMAGE)) {
        /* Compress the frame to JPEG. Note that when taking pictures, we
         * have requested camera device to provide us with NV21 frames. */
        NV21JpegCompressor compressor;
        status_t res =
            compressor.compressRawImage(frame, camera_dev->getFrameWidth(),
                                        camera_dev->getFrameHeight(),
                                        mJpegQuality);
        if (res == NO_ERROR) {
            camera_memory_t* jpeg_buff =
                mGetMemoryCB(-1, compressor.getCompressedSize(), 1, NULL);
            if (NULL != jpeg_buff && NULL != jpeg_buff->data) {
                compressor.getCompressedImage(jpeg_buff->data);
                mDataCB(CAMERA_MSG_COMPRESSED_IMAGE, jpeg_buff, 0, NULL, mCallbackCookie);
                jpeg_buff->release(jpeg_buff);
            } else {
                ALOGE("%s: Memory failure in CAMERA_MSG_COMPRESSED_IMAGE", __FUNCTION__);
            }
        } else {
            ALOGE("%s: Compression failure in CAMERA_MSG_COMPRESSED_IMAGE", __FUNCTION__);
        }
    }
}

void CallbackNotifier::onCameraDeviceError(int err)
{
    if (isMessageEnabled(CAMERA_MSG_ERROR) && mNotifyCB != NULL) {
        mNotifyCB(CAMERA_MSG_ERROR, err, 0, mCallbackCookie);
    }
}

/****************************************************************************
 * Private API
 ***************************************************************************/

bool CallbackNotifier::isNewVideoFrameTime(nsecs_t timestamp)
{
    return true;		// to do here
	
    Mutex::Autolock locker(&mObjectLock);
    if ((timestamp - mLastFrameTimestamp) >= mFrameRefreshFreq) {
        mLastFrameTimestamp = timestamp;
        return true;
    }
    return false;
}

}; /* namespace android */
