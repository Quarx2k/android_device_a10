#ifndef __CAMERA_CONFIG_H__
#define __CAMERA_CONFIG_H__

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define CAMERA_KEY_CONFIG_PATH	"/system/etc/camera.cfg"

#define KEY_LENGTH	256

#define kNUMBER_OF_CAMERA					"number_of_camera"
#define kCAMERA_FACING						"camera_facing"
#define kCAMERA_DEVICE						"camera_device"
#define kDEVICE_ID							"device_id"

#define kUSED_PREVIEW_SIZE					"used_preview_size"
#define kSUPPORT_PREVIEW_SIZE				"key_support_preview_size"
#define kDEFAULT_PREVIEW_SIZE				"key_default_preview_size"

#define kUSED_PICTURE_SIZE					"used_picture_size"
#define kSUPPORT_PICTURE_SIZE				"key_support_picture_size"
#define kDEFAULT_PICTURE_SIZE				"key_default_picture_size"

#define kUSED_FLASH_MODE					"used_flash_mode"
#define kSUPPORT_FLASH_MODE					"key_support_flash_mode"
#define kDEFAULT_FLASH_MODE					"key_default_flash_mode"

#define kUSED_COLOR_EFFECT					"used_color_effect"
#define kSUPPORT_COLOR_EFFECT				"key_support_color_effect"
#define kDEFAULT_COLOR_EFFECT				"key_default_color_effect"

#define kUSED_FRAME_RATE					"used_frame_rate"
#define kSUPPORT_FRAME_RATE					"key_support_frame_rate"
#define kDEFAULT_FRAME_RATE					"key_default_frame_rate"

#define kUSED_FOCUS_MODE					"used_focus_mode"
#define kSUPPORT_FOCUS_MODE					"key_support_focus_mode"
#define kDEFAULT_FOCUS_MODE					"key_default_focus_mode"

#define kUSED_SCENE_MODE					"used_scene_mode"
#define kSUPPORT_SCENE_MODE					"key_support_scene_mode"
#define kDEFAULT_SCENE_MODE					"key_default_scene_mode"

#define kUSED_WHITE_BALANCE					"used_white_balance"
#define kSUPPORT_WHITE_BALANCE				"key_support_white_balance"
#define kDEFAULT_WHITE_BALANCE				"key_default_white_balance"

#define kUSED_EXPOSURE_COMPENSATION			"used_exposure_compensation"
#define kMIN_EXPOSURE_COMPENSATION			"key_min_exposure_compensation"
#define kMAX_EXPOSURE_COMPENSATION			"key_max_exposure_compensation"
#define kSTEP_EXPOSURE_COMPENSATION			"key_step_exposure_compensation"
#define kDEFAULT_EXPOSURE_COMPENSATION		"key_default_exposure_compensation"

#define kUSED_ZOOM            				"used_zoom"
#define kZOOM_SUPPORTED       				"key_zoom_supported"
#define kSMOOTH_ZOOM_SUPPORTED				"key_smooth_zoom_supported"
#define kZOOM_RATIOS          				"key_zoom_ratios"
#define kMAX_ZOOM             				"key_max_zoom"
#define kDEFAULT_ZOOM         				"key_default_zoom"	

#define MEMBER_DEF(mem)				\
	char mUsed##mem[2];				\
	char * mSupport##mem##Value;	\
	char * mDefault##mem##Value;

class CCameraConfig
{
public:
	CCameraConfig(int id);
	~CCameraConfig();

	void initParameters();
	void dumpParameters();

	int numberOfCamera()
	{
		return mNumberOfCamera;
	}

	int cameraFacing()
	{
		return mCameraFacing;
	}

	char * cameraDevice()
	{
		return mCameraDevice;
	}

	int getDeviceID()
	{
		return mDeviceID;
	}

	bool supportPreviewSize();
	char * supportPreviewSizeValue();
	char * defaultPreviewSizeValue();
	
	bool supportPictureSize();
	char * supportPictureSizeValue();
	char * defaultPictureSizeValue();

	bool supportFlashMode();
	char * supportFlashModeValue();
	char * defaultFlashModeValue();
	
	bool supportColorEffect();
	char * supportColorEffectValue();
	char * defaultColorEffectValue();

	bool supportFrameRate();
	char * supportFrameRateValue();
	char * defaultFrameRateValue();

	bool supportFocusMode();
	char * supportFocusModeValue();
	char * defaultFocusModeValue();

	bool supportSceneMode();
	char * supportSceneModeValue();
	char * defaultSceneModeValue();

	bool supportWhiteBalance();
	char * supportWhiteBalanceValue();
	char * defaultWhiteBalanceValue();

	// exposure compensation
	bool supportExposureCompensation()
	{
		return usedKey(mUsedExposureCompensation);
	}

	char * minExposureCompensationValue()
	{
		return mMinExposureCompensation;
	}

	char * maxExposureCompensationValue()
	{
		return mMaxExposureCompensation;
	}

	char * stepExposureCompensationValue()
	{
		return mStepExposureCompensation;
	}

	char * defaultExposureCompensationValue()
	{
		return mDefaultExposureCompensation;
	}

	// zoom
	bool supportZoom()
	{
		return usedKey(mUsedZoom);
	}

	char * zoomSupportedValue()
	{
		return mZoomSupported;
	}

	char * smoothZoomSupportedValue()
	{
		return mSmoothZoomSupported;
	}

	char * zoomRatiosValue()
	{
		return mZoomRatios;
	}

	char * maxZoomValue()
	{
		return mMaxZoom;
	}

	char * defaultZoomValue()
	{
		return mDefaultZoom;
	}

private:
	bool readKey(char *key, char *value);
	void getValue(char *line, char *value);
	bool usedKey(char *value);

	FILE * mhKeyFile;

	int mCurCameraId;
	int mNumberOfCamera;
	int mCameraFacing;
	char mCameraDevice[64];
	int mDeviceID;

	MEMBER_DEF(PreviewSize)
	MEMBER_DEF(PictureSize)
	MEMBER_DEF(FlashMode)
	MEMBER_DEF(ColorEffect)
	MEMBER_DEF(FrameRate)
	MEMBER_DEF(FocusMode)
	MEMBER_DEF(SceneMode)
	MEMBER_DEF(WhiteBalance)
	// MEMBER_DEF(ExposureCompensation)

	char mUsedExposureCompensation[2];
	char mMinExposureCompensation[4];
	char mMaxExposureCompensation[4];
	char mStepExposureCompensation[4];
	char mDefaultExposureCompensation[4];

	char mUsedZoom[2];
	char mZoomSupported[8];
	char mSmoothZoomSupported[8];
	char mZoomRatios[KEY_LENGTH];
	char mMaxZoom[4];
	char mDefaultZoom[4];
};

#endif // __CAMERA_CONFIG_H__
