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
 * Contains implementation of a class CameraHardwareDevice that encapsulates
 * functionality of a fake camera.
 */

#define LOG_TAG "CameraHardwareDevice"
#include "CameraDebug.h"

#include <cutils/properties.h>
#include "CameraHardwareDevice.h"
#include "HALCameraFactory.h"

namespace android {

CameraHardwareDevice::CameraHardwareDevice(int cameraId, struct hw_module_t* module)
        : CameraHardware(cameraId, module),
          mV4L2CameraDevice(NULL)
{
	F_LOG;
}

CameraHardwareDevice::~CameraHardwareDevice()
{
	F_LOG;
	if (mV4L2CameraDevice != NULL)
	{
		delete mV4L2CameraDevice;
		mV4L2CameraDevice = NULL;
	}
}

/****************************************************************************
 * Public API overrides
 ***************************************************************************/

status_t CameraHardwareDevice::Initialize()
{
	F_LOG;

	// instance V4L2CameraDevice object
	mV4L2CameraDevice = new V4L2CameraDevice(this, mCameraID);
	if (mV4L2CameraDevice == NULL)
	{
		ALOGE("Failed to create V4L2Camera instance");
		return NO_MEMORY;
	}
	
    status_t res = mV4L2CameraDevice->Initialize();
    if (res != NO_ERROR) {
        return res;
    }

	// set v4l2 device name and device id
	// note: device id is not the same as camera id
	char * pDevice = mCameraConfig->cameraDevice();
	int deviceId = mCameraConfig->getDeviceID();
	int cameraFacing = mCameraConfig->cameraFacing();
	mV4L2CameraDevice->setV4L2DeviceName(pDevice);
	mV4L2CameraDevice->setV4L2DeviceID(deviceId);
	mV4L2CameraDevice->setCameraFacing(cameraFacing);

    res = CameraHardware::Initialize();
    if (res != NO_ERROR) {
        return res;
    }

    /*
     * Parameters provided by the camera device.
     */
     
    return NO_ERROR;
}

V4L2CameraDevice* CameraHardwareDevice::getCameraDevice()
{
	F_LOG;
    return mV4L2CameraDevice;
}

void CameraHardwareDevice::releaseRecordingFrame(const void* opaque)
{
	mV4L2CameraDevice->releasePreviewFrame(*(int*)opaque);
}

};  /* namespace android */
