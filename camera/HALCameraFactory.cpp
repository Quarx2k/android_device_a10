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
 * Contains implementation of a class HALCameraFactory that manages cameras
 * available for emulation.
 */

#define LOG_TAG "HALCameraFactory"
#include "CameraDebug.h"

#include <cutils/properties.h>

#include "CameraHardwareDevice.h"
#include "HALCameraFactory.h"

extern camera_module_t HAL_MODULE_INFO_SYM;

/* A global instance of HALCameraFactory is statically instantiated and
 * initialized when camera emulation HAL is loaded.
 */
android::HALCameraFactory  gEmulatedCameraFactory;

namespace android {

HALCameraFactory::HALCameraFactory()
        : mHardwareCameras(NULL),
          mCameraHardwareNum(0),
          mConstructedOK(false)
{
	F_LOG;

	// camera config information
	mCameraConfig = new CCameraConfig(0);
	if(mCameraConfig == 0)
	{
		LOGE("create CCameraConfig failed");
		return ;
	}

	mCameraConfig->initParameters();
	mCameraConfig->dumpParameters();

	mCameraHardwareNum = mCameraConfig->numberOfCamera();
	
    /* Make sure that array is allocated (in case there were no 'qemu'
     * cameras created. */
    if (mHardwareCameras == NULL) {
        mHardwareCameras = new CameraHardware*[mCameraHardwareNum];
        if (mHardwareCameras == NULL) {
            LOGE("%s: Unable to allocate V4L2Camera array for %d entries",
                 __FUNCTION__, mCameraHardwareNum);
            return;
        }
        memset(mHardwareCameras, 0, mCameraHardwareNum * sizeof(CameraHardware*));
    }

    /* Create, and initialize the fake camera */
	for (int id = 0; id < mCameraHardwareNum; id++)
	{
		mHardwareCameras[id] = new CameraHardwareDevice(id, &HAL_MODULE_INFO_SYM.common);
		if (mHardwareCameras[id] != NULL) 
		{
	        if (mHardwareCameras[id]->Initialize() != NO_ERROR) 
			{
	            delete mHardwareCameras[id];
	            mHardwareCameras--;
				return;
		    } 
		}
		else 
		{
	        mHardwareCameras--;
	        LOGE("%s: Unable to instantiate fake camera class", __FUNCTION__);
			return;
	    }
	}

	LOGV("%d cameras are being created.", mCameraHardwareNum);

    mConstructedOK = true;
}

HALCameraFactory::~HALCameraFactory()
{
	F_LOG;
    if (mHardwareCameras != NULL) {
        for (int n = 0; n < mCameraHardwareNum; n++) {
            if (mHardwareCameras[n] != NULL) {
                delete mHardwareCameras[n];
            }
        }
        delete[] mHardwareCameras;
    }
}

/****************************************************************************
 * Camera HAL API handlers.
 *
 * Each handler simply verifies existence of an appropriate CameraHardware
 * instance, and dispatches the call to that instance.
 *
 ***************************************************************************/

int HALCameraFactory::cameraDeviceOpen(int camera_id, hw_device_t** device)
{
    LOGV("%s: id = %d", __FUNCTION__, camera_id);

    *device = NULL;

    if (!isConstructedOK()) {
        LOGE("%s: HALCameraFactory has failed to initialize", __FUNCTION__);
        return -EINVAL;
    }

    if (camera_id < 0 || camera_id >= getCameraHardwareNum()) {
        LOGE("%s: Camera id %d is out of bounds (%d)",
             __FUNCTION__, camera_id, getCameraHardwareNum());
        return -EINVAL;
    }

    return mHardwareCameras[camera_id]->connectCamera(device);
}

int HALCameraFactory::getCameraInfo(int camera_id, struct camera_info* info)
{
    LOGV("%s: id = %d", __FUNCTION__, camera_id);

    if (!isConstructedOK()) {
        LOGE("%s: HALCameraFactory has failed to initialize", __FUNCTION__);
        return -EINVAL;
    }

    if (camera_id < 0 || camera_id >= getCameraHardwareNum()) {
        LOGE("%s: Camera id %d is out of bounds (%d)",
             __FUNCTION__, camera_id, getCameraHardwareNum());
        return -EINVAL;
    }

    return mHardwareCameras[camera_id]->getCameraInfo(info);
}

/****************************************************************************
 * Camera HAL API callbacks.
 ***************************************************************************/

int HALCameraFactory::device_open(const hw_module_t* module,
                                       const char* name,
                                       hw_device_t** device)
{
	F_LOG;
    /*
     * Simply verify the parameters, and dispatch the call inside the
     * HALCameraFactory instance.
     */

    if (module != &HAL_MODULE_INFO_SYM.common) {
        LOGE("%s: Invalid module %p expected %p",
             __FUNCTION__, module, &HAL_MODULE_INFO_SYM.common);
        return -EINVAL;
    }
    if (name == NULL) {
        LOGE("%s: NULL name is not expected here", __FUNCTION__);
        return -EINVAL;
    }

    return gEmulatedCameraFactory.cameraDeviceOpen(atoi(name), device);
}

int HALCameraFactory::get_number_of_cameras(void)
{
	F_LOG;
    return gEmulatedCameraFactory.getCameraHardwareNum();
}

int HALCameraFactory::get_camera_info(int camera_id,
                                           struct camera_info* info)
{
	F_LOG;
    return gEmulatedCameraFactory.getCameraInfo(camera_id, info);
}

/********************************************************************************
 * Initializer for the static member structure.
 *******************************************************************************/

/* Entry point for camera HAL API. */
struct hw_module_methods_t HALCameraFactory::mCameraModuleMethods = {
    open: HALCameraFactory::device_open
};

}; /* namespace android */
