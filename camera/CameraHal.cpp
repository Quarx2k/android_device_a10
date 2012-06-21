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
 * Contains implementation of the camera HAL layer in the system running
 * under the emulator.
 *
 * This file contains only required HAL header, which directs all the API calls
 * to the HALCameraFactory class implementation, wich is responsible for
 * managing V4L2Cameras.
 */

#include "HALCameraFactory.h"

/*
 * Required HAL header.
 */
camera_module_t HAL_MODULE_INFO_SYM = {
    common: {
         tag:           HARDWARE_MODULE_TAG,
         version_major: 1,
         version_minor: 0,
         id:            CAMERA_HARDWARE_MODULE_ID,
         name:          "V4L2Camera Module",
         author:        "The Android Open Source Project",
         methods:       &android::HALCameraFactory::mCameraModuleMethods,
         dso:           NULL,
         reserved:      {0},
    },
    get_number_of_cameras:  android::HALCameraFactory::get_number_of_cameras,
    get_camera_info:        android::HALCameraFactory::get_camera_info,
};
