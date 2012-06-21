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

#ifndef HW_EMULATOR_CAMERA_EMULATED_FAKE_CAMERA_H
#define HW_EMULATOR_CAMERA_EMULATED_FAKE_CAMERA_H

/*
 * Contains declaration of a class CameraHardwareDevice that encapsulates
 * functionality of a fake camera. This class is nothing more than a placeholder
 * for V4L2CameraDevice instance.
 */

#include "CameraHardware.h"
#include "V4L2CameraDevice.h"

namespace android {

/* Encapsulates functionality of a fake camera.
 * This class is nothing more than a placeholder for V4L2CameraDevice
 * instance that emulates a fake camera device.
 */
class CameraHardwareDevice : public CameraHardware {
public:
    /* Constructs CameraHardwareDevice instance. */
    CameraHardwareDevice(int cameraId, struct hw_module_t* module);

    /* Destructs CameraHardwareDevice instance. */
    ~CameraHardwareDevice();

    /****************************************************************************
     * CameraHardware virtual overrides.
     ***************************************************************************/

public:
    /* Initializes CameraHardwareDevice instance. */
     status_t Initialize();

    /****************************************************************************
     * CameraHardware abstract API implementation.
     ***************************************************************************/

protected:
    /* Gets V4L2Camera device ised by this instance of the V4L2Camera.
     */
    V4L2CameraDevice* getCameraDevice();

	virtual void releaseRecordingFrame(const void* opaque);

    /****************************************************************************
     * Data memebers.
     ***************************************************************************/

protected:
    /* Contained v4l2 camera device object. */
    V4L2CameraDevice *   mV4L2CameraDevice;
};

}; /* namespace android */

#endif  /* HW_EMULATOR_CAMERA_EMULATED_FAKE_CAMERA_H */
