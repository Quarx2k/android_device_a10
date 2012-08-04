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

#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <poll.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/select.h>
#include <cutils/log.h>

#include "AccelSensor.h"


/*****************************************************************************/

AccelSensor::AccelSensor()
: SensorBase(NULL, NULL),
      mEnabled(0),
      mInputReader(32)
{
    mPendingEvents.version = sizeof(sensors_event_t);
    mPendingEvents.sensor = ID_A;
    mPendingEvents.type = SENSOR_TYPE_ACCELEROMETER;
    mPendingEvents.acceleration.status = SENSOR_STATUS_ACCURACY_HIGH;

    data_name = "mma7660";

    if (data_name) {
        data_fd = openInput(data_name);
    }
}

AccelSensor::~AccelSensor()
{
}

int AccelSensor::enable(int32_t handle, int en)
{
    int newState  = en ? 1 : 0;
    int err = 0;

    if ((uint32_t(newState)) != (mEnabled)) {
        uint32_t sensor_type;

        if (en)
            err = accel_enable_sensor(sensor_type);
        else
            err = accel_disable_sensor(sensor_type);

        ALOGE_IF(err, "Could not change sensor state (%s)", strerror(-err));
    }

    if (!err) {
        mEnabled = newState;
    }

    return err;
}


int AccelSensor::setDelay(int32_t handle, int64_t ns)
{
    int ret = 0;

    int ms = ns / 1000000;
    ALOGD("AccelSensor....setDelay, ms=%d\n", ms);

    return ret;
}

int AccelSensor::readEvents(sensors_event_t* data, int count)
{

    if (count < 1)
        return -EINVAL;

    ssize_t n = mInputReader.fill(data_fd);
    if (n < 0)
        return n;

    int numEventReceived = 0;
    input_event const* event;

    while (count && mInputReader.readEvent(&event)) {
        int type = event->type;
        if (type == EV_ABS) {
            processEvent(event->code, event->value);
        } else if (type == EV_SYN) {
            mPendingEvents.timestamp = timevalToNano(event->time);
            *data++ = mPendingEvents;
            count--;
            numEventReceived++;
        } else {
            ALOGE("AccelSensor: unknown event (type=%d, code=%d)",type, event->code);
        }
            mInputReader.next();
    }
    return numEventReceived;
}

void AccelSensor::processEvent(int code, int value)
{
    switch (code) {
        case EVENT_TYPE_ACCEL_Y:
            mPendingEvents.acceleration.x = value * CONVERT_A_Y;
            break;
        case EVENT_TYPE_ACCEL_X:
            mPendingEvents.acceleration.y = value * CONVERT_A_X;
            break;    	
        case EVENT_TYPE_ACCEL_Z:
            mPendingEvents.acceleration.z = value * CONVERT_A_Z;
            break;
    }
}

