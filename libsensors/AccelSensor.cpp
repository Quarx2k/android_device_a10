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
#include <dlfcn.h>
#include <cutils/log.h>

#include "AccelSensor.h"

/*****************************************************************************/
AccelSensor::AccelSensor()
: SensorBase(NULL, NULL),
      mEnabled(0),
      mPendingMask(0),
      mInputReader(32),
      mMinPollDelay(0),
      mMaxPollDelay(0)
{
    data_name = "mma7660";

    if (data_name) {
        data_fd = openInput(data_name);
    }
    memset(mPendingEvents, 0, sizeof(mPendingEvents));

    mPendingEvents[Accelerometer].version = sizeof(sensors_event_t);
    mPendingEvents[Accelerometer].sensor = ID_A;
    mPendingEvents[Accelerometer].type = SENSOR_TYPE_ACCELEROMETER;
    mPendingEvents[Accelerometer].acceleration.status = SENSOR_STATUS_ACCURACY_HIGH;

    // read the actual value of all sensors if they're enabled already
    struct input_absinfo absinfo;
    short flags = 0;
}

AccelSensor::~AccelSensor()
{
}

int AccelSensor::enable(int32_t handle, int en)
{
    int what = -1;

    switch (handle) {
        case ID_A: what = Accelerometer; break;
    }

    if (uint32_t(what) >= numSensors)
        return -EINVAL;

    int newState  = en ? 1 : 0;
    int err = 0;

    if ((uint32_t(newState)<<what) != (mEnabled & (1<<what))) {
        uint32_t sensor_type;
       switch (what) {
            case Accelerometer: sensor_type = SENSOR_TYPE_ACCELEROMETER;  break;
        }
        short flags = newState;
        if (en)
            err = accel_enable_sensor(sensor_type);
        else
            err = accel_disable_sensor(sensor_type);

        LOGE_IF(err, "Could not change sensor state (%s)", strerror(-err));
        if (!err) {
            mEnabled &= ~(1<<what);
            mEnabled |= (uint32_t(flags)<<what);
        }
    }
    return err;
}


int AccelSensor::setDelay(int32_t handle, int64_t ns)
{
    FILE *fd = NULL;
    int n, len, ms, ret = -1;
    char buf[6];

    ms = ns / 1000 / 1000;
    LOGD("AccelSensor....setDelay, ms=%d\n", ms);

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
            mInputReader.next();
        } else if (type == EV_SYN) {
            int64_t time = timevalToNano(event->time);
            for (int j=0 ; count && mPendingMask && j<numSensors ; j++) {
                if (mPendingMask & (1<<j)) {
                    mPendingMask &= ~(1<<j);
                    mPendingEvents[j].timestamp = time;
                    if (mEnabled & (1<<j)) {
                        *data++ = mPendingEvents[j];
                        count--;
                        numEventReceived++;
                    }
                }
            }
            if (!mPendingMask) {
                mInputReader.next();
            }
        } else {
            LOGE("AccelSensor: unknown event (type=%d, code=%d)",
                    type, event->code);
            mInputReader.next();
        }
    }

    return numEventReceived;
}

void AccelSensor::processEvent(int code, int value)
{
    switch (code) {
        case EVENT_TYPE_ACCEL_Y:
            mPendingMask |= 1<<Accelerometer;
            mPendingEvents[Accelerometer].acceleration.x = value * CONVERT_A_Y;
            break;
        case EVENT_TYPE_ACCEL_X:
            mPendingMask |= 1<<Accelerometer;
            mPendingEvents[Accelerometer].acceleration.y = value * CONVERT_A_X;
            break;    	
        case EVENT_TYPE_ACCEL_Z:
            mPendingMask |= 1<<Accelerometer;
            mPendingEvents[Accelerometer].acceleration.z = value * CONVERT_A_Z;
            break;
    }
}

