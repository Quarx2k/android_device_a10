/*
 * Copyright (C) 2011 Freescale Semiconductor Inc.
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

#ifndef ANDROID_ACCEL_SENSOR_H
#define ANDROID_ACCEL_SENSOR_H

#include <stdint.h>
#include <errno.h>
#include <sys/cdefs.h>
#include <sys/types.h>

#include "sensors.h"
#include "SensorBase.h"
#include "InputEventReader.h"

/*****************************************************************************/

struct input_event;

class AccelSensor : public SensorBase {
public:
            AccelSensor();
    virtual ~AccelSensor();

    virtual int setDelay(int32_t handle, int64_t ns);
    virtual int enable(int32_t handle, int enabled);
    virtual int readEvents(sensors_event_t* data, int count);
    void processEvent(int code, int value);

private:
    int isEnabled();
    uint32_t mEnabled;
    InputEventCircularReader mInputReader;
    sensors_event_t mPendingEvents;
    static inline int accel_is_sensor_enabled(uint32_t sensor_type)
    {
        return 1;
    }
    static inline int accel_enable_sensor(uint32_t sensor_type)
    {
        return 0;
    }
    static inline int accel_disable_sensor(uint32_t sensor_type)
    {
       return 0;
    }
};

/*****************************************************************************/

#endif  // ANDROID_ACCEL_SENSOR_H

