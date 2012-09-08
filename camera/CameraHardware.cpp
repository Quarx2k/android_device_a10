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
 * Contains implementation of a class CameraHardware that encapsulates
 * functionality common to all V4L2Cameras ("fake", "webcam", "video file",
 * etc.). Instances of this class (for each V4L2Camera) are created during
 * the construction of the HALCameraFactory instance. This class serves as
 * an entry point for all camera API calls that defined by camera_device_ops_t
 * API.
 */

#define LOG_TAG "CameraHardware"
#include "CameraDebug.h"

#include <ui/Rect.h>
#include <drv_display_sun4i.h>
#include <videodev2.h>
#include "CameraHardware.h"
#include "V4L2CameraDevice.h"
#include "Converters.h"

/* Defines whether we should trace parameter changes. */
#define DEBUG_PARAM 1

namespace android {

#if DEBUG_PARAM
/* Calculates and logs parameter changes.
 * Param:
 *  current - Current set of camera parameters.
 *  new_par - String representation of new parameters.
 */
static void PrintParamDiff(const CameraParameters& current, const char* new_par);
#else
#define PrintParamDiff(current, new_par)   (void(0))
#endif  /* DEBUG_PARAM */

/* A helper routine that adds a value to the camera parameter.
 * Param:
 *  param - Camera parameter to add a value to.
 *  val - Value to add.
 * Return:
 *  A new string containing parameter with the added value on success, or NULL on
 *  a failure. If non-NULL string is returned, the caller is responsible for
 *  freeing it with 'free'.
 */
static char* AddValue(const char* param, const char* val);

CameraHardware::CameraHardware(int cameraId, struct hw_module_t* module)
        : mPreviewWindow(),
          mCallbackNotifier(),
          mCameraID(cameraId),
          mCameraConfig(NULL),
          bPixFmtNV12(false)
{
    /*
     * Initialize camera_device descriptor for this object.
     */
	F_LOG;

    /* Common header */
    common.tag = HARDWARE_DEVICE_TAG;
    common.version = 0;
    common.module = module;
    common.close = CameraHardware::close;

    /* camera_device fields. */
    ops = &mDeviceOps;
    priv = this;

	mCameraConfig = new CCameraConfig(cameraId);
	if (mCameraConfig == NULL)
	{
		ALOGE("create CCameraConfig failed");
	}
	
	mCameraConfig->initParameters();
	mCameraConfig->dumpParameters();
}

CameraHardware::~CameraHardware()
{
}

/****************************************************************************
 * Public API
 ***************************************************************************/

status_t CameraHardware::Initialize()
{
	F_LOG;

	if (mCameraConfig == NULL)
	{
		return UNKNOWN_ERROR;
	}

	initDefaultParameters();

    return NO_ERROR;
}

void CameraHardware::initDefaultParameters()
{
    CameraParameters p = mParameters;
	String8 parameterString;
	char * value;

	ALOGV("CameraHardware::initDefaultParameters");
	
	if (mCameraConfig->cameraFacing() == CAMERA_FACING_BACK)
	{
	    p.set(CameraHardware::FACING_KEY, CameraHardware::FACING_BACK);
	    ALOGV("%s: camera is facing %s", __FUNCTION__, CameraHardware::FACING_BACK);
	}
	else
	{
	    p.set(CameraHardware::FACING_KEY, CameraHardware::FACING_FRONT);
	    ALOGV("%s: camera is facing %s", __FUNCTION__, CameraHardware::FACING_FRONT);
	}
	
    p.set(CameraHardware::ORIENTATION_KEY, 0);
	
	ALOGV("to init preview size");
	// preview size
	if (mCameraConfig->supportPreviewSize())
	{
		value = mCameraConfig->supportPreviewSizeValue();
		p.set(CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES, value);
		ALOGV("supportPreviewSizeValue: [%s] %s", CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES, value);
		p.set(CameraParameters::KEY_SUPPORTED_VIDEO_SIZES, value);

		value = mCameraConfig->defaultPreviewSizeValue();
		p.set(CameraParameters::KEY_PREVIEW_SIZE, value);
		p.set(CameraParameters::KEY_VIDEO_SIZE, value);
		p.set(CameraParameters::KEY_PREFERRED_PREVIEW_SIZE_FOR_VIDEO, value);
	}

	ALOGV("to init picture size");
	// picture size
	if (mCameraConfig->supportPictureSize())
	{
		value = mCameraConfig->supportPictureSizeValue();
		p.set(CameraParameters::KEY_SUPPORTED_PICTURE_SIZES, value);
		ALOGV("supportPreviewSizeValue: [%s] %s", CameraParameters::KEY_SUPPORTED_PICTURE_SIZES, value);

		value = mCameraConfig->defaultPictureSizeValue();
		p.set(CameraParameters::KEY_PICTURE_SIZE, value);
		
		p.setPictureFormat(CameraParameters::PIXEL_FORMAT_JPEG);
	}

	ALOGV("to init frame rate");
	// frame rate
	if (mCameraConfig->supportFrameRate())
	{
		value = mCameraConfig->supportFrameRateValue();
		p.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FRAME_RATES, value);
		ALOGV("supportFrameRateValue: [%s] %s", CameraParameters::KEY_SUPPORTED_PREVIEW_FRAME_RATES, value);

		p.set(CameraParameters::KEY_PREVIEW_FPS_RANGE, "5000,30000");				// add temp for CTS
		p.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FPS_RANGE, "(5000,30000)");	// add temp for CTS

		value = mCameraConfig->defaultFrameRateValue();
		p.set(CameraParameters::KEY_PREVIEW_FRAME_RATE, value);
	}

	ALOGV("to init focus");
	// focus
	if (mCameraConfig->supportFocusMode())
	{
		value = mCameraConfig->supportFocusModeValue();
		p.set(CameraParameters::KEY_SUPPORTED_FOCUS_MODES, value);
		value = mCameraConfig->defaultFocusModeValue();
	    p.set(CameraParameters::KEY_FOCUS_MODE, value);
	}
	else
	{
		// add for CTS
		p.set(CameraParameters::KEY_SUPPORTED_FOCUS_MODES, CameraParameters::FOCUS_MODE_AUTO);
		p.set(CameraParameters::KEY_FOCUS_MODE, CameraParameters::FOCUS_MODE_AUTO);
		p.set(CameraParameters::KEY_FOCAL_LENGTH, "3.43");
		mCallbackNotifier.setFocalLenght(3.43);
		p.set(CameraParameters::KEY_FOCUS_DISTANCES, "0.10,1.20,Infinity");
	}

	ALOGV("to init color effect");
	// color effect	
	if (mCameraConfig->supportColorEffect())
	{
		value = mCameraConfig->supportColorEffectValue();
		p.set(CameraParameters::KEY_SUPPORTED_EFFECTS, value);
		value = mCameraConfig->defaultColorEffectValue();
		p.set(CameraParameters::KEY_EFFECT, value);
	}

	ALOGV("to init flash mode");
	// flash mode	
	if (mCameraConfig->supportFlashMode())
	{
		value = mCameraConfig->supportFlashModeValue();
		p.set(CameraParameters::KEY_SUPPORTED_FLASH_MODES, value);
		value = mCameraConfig->defaultFlashModeValue();
		p.set(CameraParameters::KEY_FLASH_MODE, value);
	}

	ALOGV("to init scene mode");
	// scene mode	
	if (mCameraConfig->supportSceneMode())
	{
		value = mCameraConfig->supportSceneModeValue();
		p.set(CameraParameters::KEY_SUPPORTED_SCENE_MODES, value);
		value = mCameraConfig->defaultSceneModeValue();
		p.set(CameraParameters::KEY_SCENE_MODE, value);
	}

	ALOGV("to init white balance");
	// white balance	
	if (mCameraConfig->supportWhiteBalance())
	{
		value = mCameraConfig->supportWhiteBalanceValue();
		p.set(CameraParameters::KEY_SUPPORTED_WHITE_BALANCE, value);
		value = mCameraConfig->defaultWhiteBalanceValue();
		p.set(CameraParameters::KEY_WHITE_BALANCE, value);
	}

	ALOGV("to init exposure compensation");
	// exposure compensation
	if (mCameraConfig->supportExposureCompensation())
	{
		value = mCameraConfig->minExposureCompensationValue();
		p.set(CameraParameters::KEY_MIN_EXPOSURE_COMPENSATION, value);

		value = mCameraConfig->maxExposureCompensationValue();
		p.set(CameraParameters::KEY_MAX_EXPOSURE_COMPENSATION, value);

		value = mCameraConfig->stepExposureCompensationValue();
		p.set(CameraParameters::KEY_EXPOSURE_COMPENSATION_STEP, value);

		value = mCameraConfig->defaultExposureCompensationValue();
		p.set(CameraParameters::KEY_EXPOSURE_COMPENSATION, value);
	}
	else
	{
		p.set(CameraParameters::KEY_MIN_EXPOSURE_COMPENSATION, "0");
		p.set(CameraParameters::KEY_MAX_EXPOSURE_COMPENSATION, "0");
		p.set(CameraParameters::KEY_EXPOSURE_COMPENSATION_STEP, "0");
		p.set(CameraParameters::KEY_EXPOSURE_COMPENSATION, "0");
	}

	ALOGV("to init zoom");
	// zoom
	if (mCameraConfig->supportZoom())
	{
		value = mCameraConfig->zoomSupportedValue();
		p.set(CameraParameters::KEY_ZOOM_SUPPORTED, value);

		value = mCameraConfig->smoothZoomSupportedValue();
		p.set(CameraParameters::KEY_SMOOTH_ZOOM_SUPPORTED, value);

		value = mCameraConfig->zoomRatiosValue();
		p.set(CameraParameters::KEY_ZOOM_RATIOS, value);

		value = mCameraConfig->maxZoomValue();
		p.set(CameraParameters::KEY_MAX_ZOOM, value);

		value = mCameraConfig->defaultZoomValue();
		p.set(CameraParameters::KEY_ZOOM, value);
	}


	// preview formats, CTS must support at least 2 formats
	parameterString = CameraParameters::PIXEL_FORMAT_YUV420SP;
	parameterString.append(",");
	parameterString.append(CameraParameters::PIXEL_FORMAT_YUV420P);
	p.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FORMATS, parameterString.string());
	
	p.set(CameraParameters::KEY_VIDEO_FRAME_FORMAT, CameraParameters::PIXEL_FORMAT_YUV420SP);
	p.set(CameraParameters::KEY_PREVIEW_FORMAT, CameraParameters::PIXEL_FORMAT_YUV420SP);

    p.set(CameraParameters::KEY_SUPPORTED_PICTURE_FORMATS, CameraParameters::PIXEL_FORMAT_JPEG);
    p.set(CameraParameters::KEY_VIDEO_FRAME_FORMAT, CameraParameters::PIXEL_FORMAT_YUV420SP);
	
	p.set(CameraParameters::KEY_JPEG_QUALITY, "90"); // maximum quality
	p.set(CameraParameters::KEY_SUPPORTED_JPEG_THUMBNAIL_SIZES, "320x240,0x0");
	p.set(CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH, "320");
	p.set(CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT, "240");
	p.set(CameraParameters::KEY_JPEG_THUMBNAIL_QUALITY, "90");

	mCallbackNotifier.setJpegThumbnailSize(320, 240);

	// rotation
	p.set(CameraParameters::KEY_ROTATION, 0);
		
	// add for CTS
	p.set(CameraParameters::KEY_HORIZONTAL_VIEW_ANGLE, "51.2");
    p.set(CameraParameters::KEY_VERTICAL_VIEW_ANGLE, "39.4");

	mParameters = p;

	ALOGV("CameraHardware::initDefaultParameters ok");
}

bool CameraHardware::onNextFrameAvailable(const void* frame,
                                          nsecs_t timestamp,
                                          V4L2Camera* camera_dev,
                                          bool bUseMataData)
{
	// F_LOG;
	bool ret = false;
	
    /* Notify the preview window first. */
	ret = mPreviewWindow.onNextFrameAvailable(frame, timestamp, camera_dev, bUseMataData);
    if(!ret)
	{
		return ret;
	}

    /* Notify callback notifier next. */
	mCallbackNotifier.onNextFrameAvailable(frame, timestamp, camera_dev, bUseMataData);

	return true;
}

bool CameraHardware::onNextFramePreview(const void* frame,
									  nsecs_t timestamp,
									  V4L2Camera* camera_dev,
                                      bool bUseMataData)
{
	// F_LOG;
    /* Notify the preview window first. */
    return mPreviewWindow.onNextFrameAvailable(frame, timestamp, camera_dev, bUseMataData);
}

void CameraHardware::onNextFrameCB(const void* frame,
									  nsecs_t timestamp,
									  V4L2Camera* camera_dev,
                                      bool bUseMataData)
{
	/* Notify callback notifier next. */
    mCallbackNotifier.onNextFrameAvailable(frame, timestamp, camera_dev, bUseMataData);
}

void CameraHardware::onTakingPicture(const void* frame, V4L2Camera* camera_dev, bool bUseMataData)
{
	mCallbackNotifier.takePicture(frame, camera_dev, bUseMataData);
}

void CameraHardware::onCameraDeviceError(int err)
{
	F_LOG;
    /* Errors are reported through the callback notifier */
    mCallbackNotifier.onCameraDeviceError(err);
}

/****************************************************************************
 * Camera API implementation.
 ***************************************************************************/

status_t CameraHardware::connectCamera(hw_device_t** device)
{
    ALOGV("%s", __FUNCTION__);

    status_t res = EINVAL;
    V4L2Camera* const camera_dev = getCameraDevice();
    ALOGE_IF(camera_dev == NULL, "%s: No camera device instance.", __FUNCTION__);

    if (camera_dev != NULL) {
        /* Connect to the camera device. */
        res = getCameraDevice()->connectDevice();
        if (res == NO_ERROR) {
            *device = &common;
        }
    }

    return -res;
}

status_t CameraHardware::closeCamera()
{
    ALOGV("%s", __FUNCTION__);

    return cleanupCamera();
}

status_t CameraHardware::getCameraInfo(struct camera_info* info)
{
    ALOGV("%s", __FUNCTION__);

    const char* valstr = NULL;

    valstr = mParameters.get(CameraHardware::FACING_KEY);
    if (valstr != NULL) {
        if (strcmp(valstr, CameraHardware::FACING_FRONT) == 0) {
            info->facing = CAMERA_FACING_FRONT;
        }
        else if (strcmp(valstr, CameraHardware::FACING_BACK) == 0) {
            info->facing = CAMERA_FACING_BACK;
        }
    } else {
        info->facing = CAMERA_FACING_BACK;
    }

    valstr = mParameters.get(CameraHardware::ORIENTATION_KEY);
    if (valstr != NULL) {
        info->orientation = atoi(valstr);
    } else {
        info->orientation = 0;
    }

    return NO_ERROR;
}

status_t CameraHardware::setPreviewWindow(struct preview_stream_ops* window)
{
	F_LOG;
    /* Callback should return a negative errno. */
	return -mPreviewWindow.setPreviewWindow(window,
                                             mParameters.getPreviewFrameRate());
}

void CameraHardware::setCallbacks(camera_notify_callback notify_cb,
                                  camera_data_callback data_cb,
                                  camera_data_timestamp_callback data_cb_timestamp,
                                  camera_request_memory get_memory,
                                  void* user)
{
	F_LOG;
    mCallbackNotifier.setCallbacks(notify_cb, data_cb, data_cb_timestamp,
                                    get_memory, user);
}

void CameraHardware::enableMsgType(int32_t msg_type)
{
	F_LOG;
    mCallbackNotifier.enableMessage(msg_type);
}

void CameraHardware::disableMsgType(int32_t msg_type)
{
	F_LOG;
    mCallbackNotifier.disableMessage(msg_type);
}

int CameraHardware::isMsgTypeEnabled(int32_t msg_type)
{
	F_LOG;
    return mCallbackNotifier.isMessageEnabled(msg_type);
}

status_t CameraHardware::startPreview()
{
	F_LOG;
    /* Callback should return a negative errno. */
    return -doStartPreview();
}

void CameraHardware::stopPreview()
{
	F_LOG;
    doStopPreview();
}

int CameraHardware::isPreviewEnabled()
{
	F_LOG;
    return mPreviewWindow.isPreviewEnabled();
}

status_t CameraHardware::storeMetaDataInBuffers(int enable)
{
	F_LOG;
    /* Callback should return a negative errno. */
    return -mCallbackNotifier.storeMetaDataInBuffers(enable);
}

status_t CameraHardware::startRecording()
{
	F_LOG;

	bPixFmtNV12 = true;
	
	// get video size
	int new_video_width = 0;
	int new_video_height = 0;
	mParameters.getVideoSize(&new_video_width, &new_video_height);
	
	if (mCurPreviewWidth != new_video_width
		|| mCurPreviewHeight != new_video_height
		|| bPixFmtNV12)
	{
		doStopPreview();
		mPreviewWindow.showLayer(false);
		doStartPreview();
	}
	
    /* Callback should return a negative errno. */
    return -mCallbackNotifier.enableVideoRecording(mParameters.getPreviewFrameRate());
}

void CameraHardware::stopRecording()
{
	F_LOG;
    mCallbackNotifier.disableVideoRecording();
	bPixFmtNV12 = false;
//	mCallbackNotifier.storeMetaDataInBuffers(false);

	// get video size
	int new_video_width = 0;
	int new_video_height = 0;
	mParameters.getVideoSize(&new_video_width, &new_video_height);
	
	// do not use high size for preview
	ALOGV("last preview size: %dx%d", mLastPreviewWidth, mLastPreviewHeight);
	mParameters.setPreviewSize(mLastPreviewWidth, mLastPreviewHeight);
	mParameters.setVideoSize(mLastPreviewWidth, mLastPreviewHeight);
	
	if (mLastPreviewWidth != new_video_width
		|| mLastPreviewHeight != new_video_height)
	{
		doStopPreview();
		mPreviewWindow.showLayer(false);
		doStartPreview();
	}
}

int CameraHardware::isRecordingEnabled()
{
	F_LOG;
    return mCallbackNotifier.isVideoRecordingEnabled();
}

void CameraHardware::releaseRecordingFrame(const void* opaque)
{
	F_LOG;
    mCallbackNotifier.releaseRecordingFrame(opaque);
}

status_t CameraHardware::setAutoFocus()
{
    ALOGV("%s", __FUNCTION__);

    return mCallbackNotifier.autoFocus();
}

status_t CameraHardware::cancelAutoFocus()
{
    ALOGV("%s", __FUNCTION__);

    /* TODO: Future enhancements. */
    return NO_ERROR;
}

status_t CameraHardware::takePicture()
{
    ALOGV("%s", __FUNCTION__);

    status_t res;
    int pic_width, pic_height, frame_width, frame_height;
    uint32_t org_fmt;

	// prepare take picture, copy buffer for preview
	getCameraDevice()->prepareTakePhoto(true);
	getCameraDevice()->waitPrepareTakePhoto();

    /* Collect frame info for the picture. */
    mParameters.getPictureSize(&pic_width, &pic_height);
	frame_width = pic_width;
	frame_height = pic_height;
	getCameraDevice()->tryFmtSize(&frame_width, &frame_height);
	// mParameters.setPreviewSize(frame_width, frame_height);
	ALOGD("%s, pic_size: %dx%d, frame_size: %dx%d", 
		__FUNCTION__, pic_width, pic_height, frame_width, frame_height);

	getCameraDevice()->setPictureSize(pic_width, pic_height);
	
    const char* pix_fmt = mParameters.getPictureFormat();
    if (strcmp(pix_fmt, CameraParameters::PIXEL_FORMAT_YUV420P) == 0) {
        org_fmt = V4L2_PIX_FMT_YUV420;
    } else if (strcmp(pix_fmt, CameraParameters::PIXEL_FORMAT_RGBA8888) == 0) {
        org_fmt = V4L2_PIX_FMT_RGB32;
    } else if (strcmp(pix_fmt, CameraParameters::PIXEL_FORMAT_YUV420SP) == 0) {
        org_fmt = V4L2_PIX_FMT_NV12;
    } else if (strcmp(pix_fmt, CameraParameters::PIXEL_FORMAT_JPEG) == 0) {
        /* We only have JPEG converted for NV21 format. */
        org_fmt = V4L2_PIX_FMT_NV12;
    } else {
        ALOGE("%s: Unsupported pixel format %s", __FUNCTION__, pix_fmt);
        return EINVAL;
    }
    /* Get JPEG quality. */
    int jpeg_quality = mParameters.getInt(CameraParameters::KEY_JPEG_QUALITY);
    if (jpeg_quality <= 0) {
        jpeg_quality = 90;  /* Fall back to default. */
    }

	/* Get JPEG rotate. */
    int jpeg_rotate = mParameters.getInt(CameraParameters::KEY_ROTATION);
    if (jpeg_rotate <= 0) {
        jpeg_rotate = 0;  /* Fall back to default. */
    }

    /*
     * Make sure preview is not running, and device is stopped before taking
     * picture.
     */

    const bool preview_on = mPreviewWindow.isPreviewEnabled();
    if (preview_on) {
        doStopPreview();
    }

    /* Camera device should have been stopped when the shutter message has been
     * enabled. */
    V4L2Camera* const camera_dev = getCameraDevice();
    if (camera_dev->isStarted()) {
        ALOGW("%s: Camera device is started", __FUNCTION__);
        camera_dev->stopDeliveringFrames();
        camera_dev->stopDevice();
    }

    /*
     * Take the picture now.
     */
     
	// close layer before taking picture, 
	// mPreviewWindow.showLayer(false);
	
	camera_dev->setTakingPicture(true);
    /* Start camera device for the picture frame. */
    ALOGD("Starting camera for picture: %.4s(%s)[%dx%d]",
         reinterpret_cast<const char*>(&org_fmt), pix_fmt, frame_width, frame_height);
    res = camera_dev->startDevice(frame_width, frame_height, org_fmt);
    if (res != NO_ERROR) {
        if (preview_on) {
            doStartPreview();
        }
        return res;
    }
	
    /* Deliver one frame only. */
    mCallbackNotifier.setJpegQuality(jpeg_quality);
	mCallbackNotifier.setJpegRotate(jpeg_rotate);
    mCallbackNotifier.setTakingPicture(true);
    // res = camera_dev->startDeliveringFrames(true);	
	res = camera_dev->startDeliveringFrames(false);		// star modify
    if (res != NO_ERROR) {
        mCallbackNotifier.setTakingPicture(false);
        if (preview_on) {
            doStartPreview();
        }
    }
	
    return res;
}

status_t CameraHardware::cancelPicture()
{
    ALOGV("%s", __FUNCTION__);

    return NO_ERROR;
}

status_t CameraHardware::setParameters(const char* p)
{
    ALOGV("%s", __FUNCTION__);
	int ret = UNKNOWN_ERROR;
	
    PrintParamDiff(mParameters, p);

    CameraParameters params;
	String8 str8_param(p);
    params.unflatten(str8_param);

	V4L2CameraDevice* pV4L2Device = getCameraDevice();
	if (pV4L2Device == NULL)
	{
		ALOGE("%s, getCameraDevice failed", __FUNCTION__);
		return UNKNOWN_ERROR;
	}

	// preview format
	const char * new_preview_format = params.getPreviewFormat();
	ALOGD("new_preview_format : %s", new_preview_format);
	if (strcmp(new_preview_format, CameraParameters::PIXEL_FORMAT_YUV420SP) == 0
		|| strcmp(new_preview_format, CameraParameters::PIXEL_FORMAT_YUV420P) == 0) 
	{
		mParameters.setPreviewFormat(new_preview_format);
	}
	else
    {
        ALOGE("Only yuv420sp or yuv420p preview is supported");
        return -EINVAL;
    }

	if (strcmp(params.getPictureFormat(), CameraParameters::PIXEL_FORMAT_JPEG) != 0) 
    {
        ALOGE("Only jpeg still pictures are supported");
        return -EINVAL;
    }

	// picture size
	int new_picture_width  = 0;
    int new_picture_height = 0;
    params.getPictureSize(&new_picture_width, &new_picture_height);
    ALOGV("%s : new_picture_width x new_picture_height = %dx%d", __FUNCTION__, new_picture_width, new_picture_height);
    if (0 < new_picture_width && 0 < new_picture_height) 
	{
		mParameters.setPictureSize(new_picture_width, new_picture_height);
    }
	else
	{
		ALOGE("error picture size");
		return -EINVAL;
	}

	// preview size
    int new_preview_width  = 0;
    int new_preview_height = 0;
    params.getPreviewSize(&new_preview_width, &new_preview_height);
    ALOGV("%s : new_preview_width x new_preview_height = %dx%d",
         __FUNCTION__, new_preview_width, new_preview_height);
	if (0 < new_preview_width && 0 < new_preview_height)
	{
		mLastPreviewWidth = mCurPreviewWidth;
		mLastPreviewHeight = mCurPreviewHeight;
		
		// try size
		ret = pV4L2Device->tryFmtSize(&new_preview_width, &new_preview_height);
		if(ret < 0)
		{
			return ret;
		}
		
		mParameters.setPreviewSize(new_preview_width, new_preview_height);
		mParameters.setVideoSize(new_preview_width, new_preview_height);
	}
	else
	{
		ALOGE("error preview size");
		return -EINVAL;
	}

	// frame rate
	int new_min_frame_rate, new_max_frame_rate;
	params.getPreviewFpsRange(&new_min_frame_rate, &new_max_frame_rate);
	int new_preview_frame_rate = params.getPreviewFrameRate();
	if (0 < new_preview_frame_rate && 0 < new_min_frame_rate 
		&& new_min_frame_rate <= new_max_frame_rate)
	{
		mParameters.setPreviewFrameRate(pV4L2Device->getFrameRate());
	}
	else
	{
		ALOGE("error preview frame rate");
		return -EINVAL;
	}

	// JPEG image quality
    int new_jpeg_quality = params.getInt(CameraParameters::KEY_JPEG_QUALITY);
    ALOGV("%s : new_jpeg_quality %d", __FUNCTION__, new_jpeg_quality);
    if (new_jpeg_quality >=1 && new_jpeg_quality <= 100) 
	{
		mParameters.set(CameraParameters::KEY_JPEG_QUALITY, new_jpeg_quality);
    }
	else
	{
		ALOGE("error picture quality");
		return -EINVAL;
	}

	// rotation	
	int new_rotation = params.getInt(CameraParameters::KEY_ROTATION);
    ALOGV("%s : new_rotation %d", __FUNCTION__, new_rotation);
    if (0 <= new_rotation) 
	{
        mParameters.set(CameraParameters::KEY_ROTATION, new_rotation);
    }
	else
	{
		ALOGE("error rotate");
		return -EINVAL;
	}

	// image effect
	if (mCameraConfig->supportColorEffect())
	{
		const char *new_image_effect_str = params.get(CameraParameters::KEY_EFFECT);
	    if (new_image_effect_str != NULL) {

	        int  new_image_effect = -1;

	        if (!strcmp(new_image_effect_str, CameraParameters::EFFECT_NONE))
	            new_image_effect = V4L2_COLORFX_NONE;
	        else if (!strcmp(new_image_effect_str, CameraParameters::EFFECT_MONO))
	            new_image_effect = V4L2_COLORFX_BW;
	        else if (!strcmp(new_image_effect_str, CameraParameters::EFFECT_SEPIA))
	            new_image_effect = V4L2_COLORFX_SEPIA;
	        else if (!strcmp(new_image_effect_str, CameraParameters::EFFECT_AQUA))
	            new_image_effect = V4L2_COLORFX_GRASS_GREEN;
	        else if (!strcmp(new_image_effect_str, CameraParameters::EFFECT_NEGATIVE))
	            new_image_effect = V4L2_COLORFX_NEGATIVE;
	        else {
	            //posterize, whiteboard, blackboard, solarize
	            ALOGE("ERR(%s):Invalid effect(%s)", __FUNCTION__, new_image_effect_str);
	            ret = UNKNOWN_ERROR;
	        }

	        if (new_image_effect >= 0) {
	            if (pV4L2Device->setImageEffect(new_image_effect) < 0) {
	                ALOGE("ERR(%s):Fail on mV4L2Camera->setImageEffect(effect(%d))", __FUNCTION__, new_image_effect);
	                ret = UNKNOWN_ERROR;
	            } else {
	                mParameters.set(CameraParameters::KEY_EFFECT, new_image_effect_str);
	            }
	        }
	    }
	}

	// white balance
	if (mCameraConfig->supportWhiteBalance())
	{
		const char *new_white_str = params.get(CameraParameters::KEY_WHITE_BALANCE);
	    ALOGV("%s : new_white_str %s", __FUNCTION__, new_white_str);
	    if (new_white_str != NULL) {
	        int new_white = -1;

	        if (!strcmp(new_white_str, CameraParameters::WHITE_BALANCE_AUTO))
	            new_white = V4L2_WB_AUTO;
	        else if (!strcmp(new_white_str,
	                         CameraParameters::WHITE_BALANCE_DAYLIGHT))
	            new_white = V4L2_WB_DAYLIGHT;
	        else if (!strcmp(new_white_str,
	                         CameraParameters::WHITE_BALANCE_CLOUDY_DAYLIGHT))
	            new_white = V4L2_WB_CLOUD;
	        else if (!strcmp(new_white_str,
	                         CameraParameters::WHITE_BALANCE_FLUORESCENT))
	            new_white = V4L2_WB_FLUORESCENT;
	        else if (!strcmp(new_white_str,
	                         CameraParameters::WHITE_BALANCE_INCANDESCENT))
	            new_white = V4L2_WB_INCANDESCENCE;
	        else if (!strcmp(new_white_str,
	                         CameraParameters::WHITE_BALANCE_WARM_FLUORESCENT))
	            new_white = V4L2_WB_TUNGSTEN;
	        else{
	            ALOGE("ERR(%s):Invalid white balance(%s)", __FUNCTION__, new_white_str); //twilight, shade
	            ret = UNKNOWN_ERROR;
	        }

	        if (0 <= new_white)
			{
	            if (pV4L2Device->setWhiteBalance(new_white) < 0) 
				{
	                ALOGE("ERR(%s):Fail on mV4L2Camera->setWhiteBalance(white(%d))", __FUNCTION__, new_white);
	                ret = UNKNOWN_ERROR;
	            } 
				else 
				{
	                mParameters.set(CameraParameters::KEY_WHITE_BALANCE, new_white_str);
	            }
	        }
	    }
	}
	
	// exposure compensation
	if (mCameraConfig->supportExposureCompensation())
	{
		int new_exposure_compensation = params.getInt(CameraParameters::KEY_EXPOSURE_COMPENSATION);
		int max_exposure_compensation = params.getInt(CameraParameters::KEY_MAX_EXPOSURE_COMPENSATION);
		int min_exposure_compensation = params.getInt(CameraParameters::KEY_MIN_EXPOSURE_COMPENSATION);
		ALOGV("%s : new_exposure_compensation %d", __FUNCTION__, new_exposure_compensation);
		if ((min_exposure_compensation <= new_exposure_compensation) &&
			(max_exposure_compensation >= new_exposure_compensation)) {
			if (pV4L2Device->setExposure(new_exposure_compensation) < 0) {
				ALOGE("ERR(%s):Fail on mV4L2Camera->setBrightness(brightness(%d))", __FUNCTION__, new_exposure_compensation);
				ret = UNKNOWN_ERROR;
			} else {
				mParameters.set(CameraParameters::KEY_EXPOSURE_COMPENSATION, new_exposure_compensation);
			}
		}
	}
	
	// flash mode	
	if (mCameraConfig->supportFlashMode())
	{
		const char *new_flash_mode_str = params.get(CameraParameters::KEY_FLASH_MODE);
		mParameters.set(CameraParameters::KEY_FLASH_MODE, new_flash_mode_str);
	}

	// zoom
	if (mCameraConfig->supportZoom())
	{
		int new_zoom = params.getInt(CameraParameters::KEY_ZOOM);
		ALOGV("new_zoom: %d", new_zoom);
		mParameters.set(CameraParameters::KEY_ZOOM, new_zoom);
	}

	// focus
	const char *new_focus_mode_str = params.get(CameraParameters::KEY_FOCUS_MODE);
	if (strcmp(new_focus_mode_str, CameraParameters::FOCUS_MODE_AUTO))
	{
		ALOGE("invalid focus mode: %s", new_focus_mode_str);
		return -EINVAL;
	}

	// gps latitude
    const char *new_gps_latitude_str = params.get(CameraParameters::KEY_GPS_LATITUDE);
	if (new_gps_latitude_str) {
		mCallbackNotifier.setGPSLatitude(atof(new_gps_latitude_str));
        mParameters.set(CameraParameters::KEY_GPS_LATITUDE, new_gps_latitude_str);
    } else {
        mParameters.remove(CameraParameters::KEY_GPS_LATITUDE);
    }

    // gps longitude
    const char *new_gps_longitude_str = params.get(CameraParameters::KEY_GPS_LONGITUDE);
    if (new_gps_longitude_str) {
		mCallbackNotifier.setGPSLongitude(atof(new_gps_longitude_str));
        mParameters.set(CameraParameters::KEY_GPS_LONGITUDE, new_gps_longitude_str);
    } else {
        mParameters.remove(CameraParameters::KEY_GPS_LONGITUDE);
    }
  
    // gps altitude
    const char *new_gps_altitude_str = params.get(CameraParameters::KEY_GPS_ALTITUDE);
	if (new_gps_altitude_str) {
		mCallbackNotifier.setGPSAltitude(atol(new_gps_altitude_str));
        mParameters.set(CameraParameters::KEY_GPS_ALTITUDE, new_gps_altitude_str);
    } else {
        mParameters.remove(CameraParameters::KEY_GPS_ALTITUDE);
    }

    // gps timestamp
    const char *new_gps_timestamp_str = params.get(CameraParameters::KEY_GPS_TIMESTAMP);
	if (new_gps_timestamp_str) {
		mCallbackNotifier.setGPSTimestamp(atol(new_gps_timestamp_str));
        mParameters.set(CameraParameters::KEY_GPS_TIMESTAMP, new_gps_timestamp_str);
    } else {
        mParameters.remove(CameraParameters::KEY_GPS_TIMESTAMP);
    }

    // gps processing method
    const char *new_gps_processing_method_str = params.get(CameraParameters::KEY_GPS_PROCESSING_METHOD);
	if (new_gps_processing_method_str) {
		mCallbackNotifier.setGPSMethod(new_gps_processing_method_str);
        mParameters.set(CameraParameters::KEY_GPS_PROCESSING_METHOD, new_gps_processing_method_str);
    } else {
        mParameters.remove(CameraParameters::KEY_GPS_PROCESSING_METHOD);
    }
	
	// JPEG thumbnail size
	int new_jpeg_thumbnail_width = params.getInt(CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH);
	int new_jpeg_thumbnail_height= params.getInt(CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT);
	ALOGV("new_jpeg_thumbnail_width: %d, new_jpeg_thumbnail_height: %d",
		new_jpeg_thumbnail_width, new_jpeg_thumbnail_height);
	if (0 <= new_jpeg_thumbnail_width && 0 <= new_jpeg_thumbnail_height) {
		mCallbackNotifier.setJpegThumbnailSize(new_jpeg_thumbnail_width, new_jpeg_thumbnail_height);
		mParameters.set(CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH, new_jpeg_thumbnail_width);
		mParameters.set(CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT, new_jpeg_thumbnail_height);
	}
	
    return NO_ERROR;
}

/* A dumb variable indicating "no params" / error on the exit from
 * CameraHardware::getParameters(). */
static char lNoParam = '\0';
char* CameraHardware::getParameters()
{
	F_LOG;
    String8 params(mParameters.flatten());
    char* ret_str =
        reinterpret_cast<char*>(malloc(sizeof(char) * (params.length()+1)));
    memset(ret_str, 0, params.length()+1);
    if (ret_str != NULL) {
        strncpy(ret_str, params.string(), params.length()+1);
        return ret_str;
    } else {
        ALOGE("%s: Unable to allocate string for %s", __FUNCTION__, params.string());
        /* Apparently, we can't return NULL fron this routine. */
        return &lNoParam;
    }
}

void CameraHardware::putParameters(char* params)
{
	F_LOG;
    /* This method simply frees parameters allocated in getParameters(). */
    if (params != NULL && params != &lNoParam) {
        free(params);
    }
}

status_t CameraHardware::sendCommand(int32_t cmd, int32_t arg1, int32_t arg2)
{
    ALOGV("%s: cmd = %d, arg1 = %d, arg2 = %d", __FUNCTION__, cmd, arg1, arg2);

    /* TODO: Future enhancements. */

	// CAMERA_CMD_START_FACE_DETECTION
	// CAMERA_CMD_STOP_FACE_DETECTION

	switch (cmd)
	{
	case CAMERA_CMD_SET_SCREEN_ID:
		mPreviewWindow.setScreenID(arg1);
		return OK;
	}

    return -EINVAL;
}

void CameraHardware::releaseCamera()
{
    ALOGV("%s", __FUNCTION__);

    cleanupCamera();
}

status_t CameraHardware::dumpCamera(int fd)
{
    ALOGV("%s", __FUNCTION__);

    /* TODO: Future enhancements. */
    return -EINVAL;
}

/****************************************************************************
 * Preview management.
 ***************************************************************************/

status_t CameraHardware::doStartPreview()
{
    ALOGV("%s", __FUNCTION__);
	
    V4L2Camera* camera_dev = getCameraDevice();
    if (camera_dev->isStarted()) {
        camera_dev->stopDeliveringFrames();
        camera_dev->stopDevice();
    }

    status_t res = mPreviewWindow.startPreview();
    if (res != NO_ERROR) {
        return res;
    }

    /* Make sure camera device is connected. */
    if (!camera_dev->isConnected()) {
        res = camera_dev->connectDevice();
        if (res != NO_ERROR) {
            mPreviewWindow.stopPreview();
            return res;
        }
    }

    int width, height;
    /* Lets see what should we use for frame width, and height. */
    if (mParameters.get(CameraParameters::KEY_VIDEO_SIZE) != NULL) {
        mParameters.getVideoSize(&width, &height);
    } else {
        mParameters.getPreviewSize(&width, &height);
    }
    /* Lets see what should we use for the frame pixel format. Note that there
     * are two parameters that define pixel formats for frames sent to the
     * application via notification callbacks:
     * - KEY_VIDEO_FRAME_FORMAT, that is used when recording video, and
     * - KEY_PREVIEW_FORMAT, that is used for preview frame notification.
     * We choose one or the other, depending on "recording-hint" property set by
     * the framework that indicating intention: video, or preview. */
    const char* pix_fmt = NULL;
    const char* is_video = mParameters.get(CameraHardware::RECORDING_HINT_KEY);
    if (is_video == NULL) {
        is_video = CameraParameters::FALSE;
    }
    if (strcmp(is_video, CameraParameters::TRUE) == 0) {
        /* Video recording is requested. Lets see if video frame format is set. */
        pix_fmt = mParameters.get(CameraParameters::KEY_VIDEO_FRAME_FORMAT);
    }
    /* If this was not video recording, or video frame format is not set, lets
     * use preview pixel format for the main framebuffer. */
    if (pix_fmt == NULL) {
        pix_fmt = mParameters.getPreviewFormat();
    }
    if (pix_fmt == NULL) {
        ALOGE("%s: Unable to obtain video format", __FUNCTION__);
        mPreviewWindow.stopPreview();
        return EINVAL;
    }

    /* Convert framework's pixel format to the FOURCC one. */
    uint32_t org_fmt;
    if (strcmp(pix_fmt, CameraParameters::PIXEL_FORMAT_YUV420P) == 0) {
        org_fmt = V4L2_PIX_FMT_YUV420;
    } else if (strcmp(pix_fmt, CameraParameters::PIXEL_FORMAT_RGBA8888) == 0) {
        org_fmt = V4L2_PIX_FMT_RGB32;
    } else if (strcmp(pix_fmt, CameraParameters::PIXEL_FORMAT_YUV420SP) == 0) {
    	if (bPixFmtNV12) {
			ALOGV("============== DISP_SEQ_UVUV");
			mPreviewWindow.setLayerFormat(DISP_SEQ_UVUV);
        	org_fmt = V4L2_PIX_FMT_NV12;		// for HW encoder
    	} else {
	    	ALOGV("============== DISP_SEQ_VUVU");
			mPreviewWindow.setLayerFormat(DISP_SEQ_VUVU);
        	org_fmt = V4L2_PIX_FMT_NV21;		// for some apps
    	}
    } else {
        ALOGE("%s: Unsupported pixel format %s", __FUNCTION__, pix_fmt);
        mPreviewWindow.stopPreview();
        return EINVAL;
    }
	
    ALOGD("Starting camera: %dx%d -> %.4s(%s)",
         width, height, reinterpret_cast<const char*>(&org_fmt), pix_fmt);
    res = camera_dev->startDevice(width, height, org_fmt);
    if (res != NO_ERROR) {
        mPreviewWindow.stopPreview();
        return res;
    }

	// save current preview size
	mCurPreviewWidth = width;
	mCurPreviewHeight = height;

    res = camera_dev->startDeliveringFrames(false);
    if (res != NO_ERROR) {
        camera_dev->stopDevice();
        mPreviewWindow.stopPreview();
    }

    return res;
}

status_t CameraHardware::doStopPreview()
{
    ALOGV("%s", __FUNCTION__);

	// set layer off can avoid Flicker
	// mPreviewWindow.showLayer(false);
	
    status_t res = NO_ERROR;
    if (mPreviewWindow.isPreviewEnabled()) {
        /* Stop the camera. */
        if (getCameraDevice()->isStarted()) {
            getCameraDevice()->stopDeliveringFrames();
            mPreviewWindow.stopPreview();
            res = getCameraDevice()->stopDevice();
        }

     //   if (res == NO_ERROR) {
            /* Disable preview as well. */
    //        mPreviewWindow.stopPreview();
    //    }
    }

    return NO_ERROR;
}

/****************************************************************************
 * Private API.
 ***************************************************************************/

status_t CameraHardware::cleanupCamera()
{
	F_LOG;

    status_t res = NO_ERROR;

	// reset preview format to yuv420sp
	mParameters.set(CameraParameters::KEY_PREVIEW_FORMAT, CameraParameters::PIXEL_FORMAT_YUV420SP);
	mCallbackNotifier.storeMetaDataInBuffers(false);
	
    /* If preview is running - stop it. */
    res = doStopPreview();
    if (res != NO_ERROR) {
        return -res;
    }

    /* Stop and disconnect the camera device. */
    V4L2Camera* const camera_dev = getCameraDevice();
    if (camera_dev != NULL) {
        if (camera_dev->isStarted()) {
            camera_dev->stopDeliveringFrames();
            res = camera_dev->stopDevice();
            if (res != NO_ERROR) {
                return -res;
            }
        }
        if (camera_dev->isConnected()) {
			mPreviewWindow.showLayer(false);			// only here to close layer
            res = camera_dev->disconnectDevice();
            if (res != NO_ERROR) {
                return -res;
            }
        }
    }

    mCallbackNotifier.cleanupCBNotifier();

    return NO_ERROR;
}

/****************************************************************************
 * Camera API callbacks as defined by camera_device_ops structure.
 *
 * Callbacks here simply dispatch the calls to an appropriate method inside
 * CameraHardware instance, defined by the 'dev' parameter.
 ***************************************************************************/

int CameraHardware::set_preview_window(struct camera_device* dev,
                                       struct preview_stream_ops* window)
{
	F_LOG;

    CameraHardware* ec = reinterpret_cast<CameraHardware*>(dev->priv);
    if (ec == NULL) {
        ALOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return -EINVAL;
    }
    return ec->setPreviewWindow(window);
}

void CameraHardware::set_callbacks(
        struct camera_device* dev,
        camera_notify_callback notify_cb,
        camera_data_callback data_cb,
        camera_data_timestamp_callback data_cb_timestamp,
        camera_request_memory get_memory,
        void* user)
{
	F_LOG;

    CameraHardware* ec = reinterpret_cast<CameraHardware*>(dev->priv);
    if (ec == NULL) {
        ALOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return;
    }
    ec->setCallbacks(notify_cb, data_cb, data_cb_timestamp, get_memory, user);
}

void CameraHardware::enable_msg_type(struct camera_device* dev, int32_t msg_type)
{
	F_LOG;

    CameraHardware* ec = reinterpret_cast<CameraHardware*>(dev->priv);
    if (ec == NULL) {
        ALOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return;
    }
    ec->enableMsgType(msg_type);
}

void CameraHardware::disable_msg_type(struct camera_device* dev, int32_t msg_type)
{
	F_LOG;

    CameraHardware* ec = reinterpret_cast<CameraHardware*>(dev->priv);
    if (ec == NULL) {
        ALOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return;
    }
    ec->disableMsgType(msg_type);
}

int CameraHardware::msg_type_enabled(struct camera_device* dev, int32_t msg_type)
{
	F_LOG;

    CameraHardware* ec = reinterpret_cast<CameraHardware*>(dev->priv);
    if (ec == NULL) {
        ALOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return -EINVAL;
    }
    return ec->isMsgTypeEnabled(msg_type);
}

int CameraHardware::start_preview(struct camera_device* dev)
{
	F_LOG;

    CameraHardware* ec = reinterpret_cast<CameraHardware*>(dev->priv);
    if (ec == NULL) {
        ALOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return -EINVAL;
    }
    return ec->startPreview();
}

void CameraHardware::stop_preview(struct camera_device* dev)
{
	F_LOG;

    CameraHardware* ec = reinterpret_cast<CameraHardware*>(dev->priv);
    if (ec == NULL) {
        ALOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return;
    }
    ec->stopPreview();
}

int CameraHardware::preview_enabled(struct camera_device* dev)
{
	F_LOG;

    CameraHardware* ec = reinterpret_cast<CameraHardware*>(dev->priv);
    if (ec == NULL) {
        ALOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return -EINVAL;
    }
    return ec->isPreviewEnabled();
}

int CameraHardware::store_meta_data_in_buffers(struct camera_device* dev,
                                               int enable)
{
	F_LOG;

    CameraHardware* ec = reinterpret_cast<CameraHardware*>(dev->priv);
    if (ec == NULL) {
        ALOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return -EINVAL;
    }
    return ec->storeMetaDataInBuffers(enable);
}

int CameraHardware::start_recording(struct camera_device* dev)
{
	F_LOG;

    CameraHardware* ec = reinterpret_cast<CameraHardware*>(dev->priv);
    if (ec == NULL) {
        ALOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return -EINVAL;
    }
    return ec->startRecording();
}

void CameraHardware::stop_recording(struct camera_device* dev)
{
	F_LOG;

    CameraHardware* ec = reinterpret_cast<CameraHardware*>(dev->priv);
    if (ec == NULL) {
        ALOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return;
    }
    ec->stopRecording();
}

int CameraHardware::recording_enabled(struct camera_device* dev)
{
	F_LOG;

    CameraHardware* ec = reinterpret_cast<CameraHardware*>(dev->priv);
    if (ec == NULL) {
        ALOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return -EINVAL;
    }
    return ec->isRecordingEnabled();
}

void CameraHardware::release_recording_frame(struct camera_device* dev,
                                             const void* opaque)
{
	// F_LOG;

    CameraHardware* ec = reinterpret_cast<CameraHardware*>(dev->priv);
    if (ec == NULL) {
        ALOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return;
    }
    ec->releaseRecordingFrame(opaque);
}

int CameraHardware::auto_focus(struct camera_device* dev)
{
	F_LOG;

    CameraHardware* ec = reinterpret_cast<CameraHardware*>(dev->priv);
    if (ec == NULL) {
        ALOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return -EINVAL;
    }
    return ec->setAutoFocus();
}

int CameraHardware::cancel_auto_focus(struct camera_device* dev)
{
	F_LOG;

    CameraHardware* ec = reinterpret_cast<CameraHardware*>(dev->priv);
    if (ec == NULL) {
        ALOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return -EINVAL;
    }
    return ec->cancelAutoFocus();
}

int CameraHardware::take_picture(struct camera_device* dev)
{
	F_LOG;

    CameraHardware* ec = reinterpret_cast<CameraHardware*>(dev->priv);
    if (ec == NULL) {
        ALOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return -EINVAL;
    }
    return ec->takePicture();
}

int CameraHardware::cancel_picture(struct camera_device* dev)
{
	F_LOG;

    CameraHardware* ec = reinterpret_cast<CameraHardware*>(dev->priv);
    if (ec == NULL) {
        ALOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return -EINVAL;
    }
    return ec->cancelPicture();
}

int CameraHardware::set_parameters(struct camera_device* dev, const char* parms)
{
	F_LOG;

    CameraHardware* ec = reinterpret_cast<CameraHardware*>(dev->priv);
    if (ec == NULL) {
        ALOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return -EINVAL;
    }
    return ec->setParameters(parms);
}

char* CameraHardware::get_parameters(struct camera_device* dev)
{
	F_LOG;

    CameraHardware* ec = reinterpret_cast<CameraHardware*>(dev->priv);
    if (ec == NULL) {
        ALOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return NULL;
    }
    return ec->getParameters();
}

void CameraHardware::put_parameters(struct camera_device* dev, char* params)
{
	F_LOG;

    CameraHardware* ec = reinterpret_cast<CameraHardware*>(dev->priv);
    if (ec == NULL) {
        ALOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return;
    }
    ec->putParameters(params);
}

int CameraHardware::send_command(struct camera_device* dev,
                                 int32_t cmd,
                                 int32_t arg1,
                                 int32_t arg2)
{
	F_LOG;

    CameraHardware* ec = reinterpret_cast<CameraHardware*>(dev->priv);
    if (ec == NULL) {
        ALOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return -EINVAL;
    }
    return ec->sendCommand(cmd, arg1, arg2);
}

void CameraHardware::release(struct camera_device* dev)
{
	F_LOG;

    CameraHardware* ec = reinterpret_cast<CameraHardware*>(dev->priv);
    if (ec == NULL) {
        ALOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return;
    }
    ec->releaseCamera();
}

int CameraHardware::dump(struct camera_device* dev, int fd)
{
	F_LOG;

    CameraHardware* ec = reinterpret_cast<CameraHardware*>(dev->priv);
    if (ec == NULL) {
        ALOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return -EINVAL;
    }
    return ec->dumpCamera(fd);
}

int CameraHardware::close(struct hw_device_t* device)
{
	F_LOG;

    CameraHardware* ec =
        reinterpret_cast<CameraHardware*>(reinterpret_cast<struct camera_device*>(device)->priv);
    if (ec == NULL) {
        ALOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return -EINVAL;
    }
    return ec->closeCamera();
}

// -------------------------------------------------------------------------
// extended interfaces here <***** star *****>
// -------------------------------------------------------------------------

bool CameraHardware::isUseMetaDataBufferMode()
{
	return mCallbackNotifier.isUseMetaDataBufferMode();
}

/****************************************************************************
 * Static initializer for the camera callback API
 ****************************************************************************/

camera_device_ops_t CameraHardware::mDeviceOps = {
    CameraHardware::set_preview_window,
    CameraHardware::set_callbacks,
    CameraHardware::enable_msg_type,
    CameraHardware::disable_msg_type,
    CameraHardware::msg_type_enabled,
    CameraHardware::start_preview,
    CameraHardware::stop_preview,
    CameraHardware::preview_enabled,
    CameraHardware::store_meta_data_in_buffers,
    CameraHardware::start_recording,
    CameraHardware::stop_recording,
    CameraHardware::recording_enabled,
    CameraHardware::release_recording_frame,
    CameraHardware::auto_focus,
    CameraHardware::cancel_auto_focus,
    CameraHardware::take_picture,
    CameraHardware::cancel_picture,
    CameraHardware::set_parameters,
    CameraHardware::get_parameters,
    CameraHardware::put_parameters,
    CameraHardware::send_command,
    CameraHardware::release,
    CameraHardware::dump
};

/****************************************************************************
 * Common keys
 ***************************************************************************/

const char CameraHardware::FACING_KEY[]         = "prop-facing";
const char CameraHardware::ORIENTATION_KEY[]    = "prop-orientation";
const char CameraHardware::RECORDING_HINT_KEY[] = "recording-hint";

/****************************************************************************
 * Common string values
 ***************************************************************************/

const char CameraHardware::FACING_BACK[]      = "back";
const char CameraHardware::FACING_FRONT[]     = "front";

/****************************************************************************
 * Helper routines
 ***************************************************************************/

static char* AddValue(const char* param, const char* val)
{
    const size_t len1 = strlen(param);
    const size_t len2 = strlen(val);
    char* ret = reinterpret_cast<char*>(malloc(len1 + len2 + 2));
    ALOGE_IF(ret == NULL, "%s: Memory failure", __FUNCTION__);
    if (ret != NULL) {
        memcpy(ret, param, len1);
        ret[len1] = ',';
        memcpy(ret + len1 + 1, val, len2);
        ret[len1 + len2 + 1] = '\0';
    }
    return ret;
}

/****************************************************************************
 * Parameter debugging helpers
 ***************************************************************************/

#if DEBUG_PARAM
static void PrintParamDiff(const CameraParameters& current,
                            const char* new_par)
{
    char tmp[2048];
    const char* wrk = new_par;

    /* Divided with ';' */
    const char* next = strchr(wrk, ';');
    while (next != NULL) {
        snprintf(tmp, sizeof(tmp), "%.*s", next-wrk, wrk);
        /* in the form key=value */
        char* val = strchr(tmp, '=');
        if (val != NULL) {
            *val = '\0'; val++;
            const char* in_current = current.get(tmp);
            if (in_current != NULL) {
                if (strcmp(in_current, val)) {
                    ALOGD("=== Value changed: %s: %s -> %s", tmp, in_current, val);
                }
            } else {
                ALOGD("+++ New parameter: %s=%s", tmp, val);
            }
        } else {
            ALOGW("No value separator in %s", tmp);
        }
        wrk = next + 1;
        next = strchr(wrk, ';');
    }
}
#endif  /* DEBUG_PARAM */

}; /* namespace android */
