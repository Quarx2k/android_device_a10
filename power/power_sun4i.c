/*
 * Copyright (C) 2012 The Android Open Source Project
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
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define LOG_TAG "SUN4I PowerHAL"
#include <utils/Log.h>

#include <hardware/hardware.h>
#include <hardware/power.h>

#define SCALINGMAXFREQ_PATH "/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq"
#define BOOSTPULSE_PATH "/sys/devices/system/cpu/cpufreq/interactive/boostpulse"

#define MAX_BUF_SZ  10

/* initialize to something safe */
static char screen_off_max_freq[MAX_BUF_SZ] = "700000";
static char scaling_max_freq[MAX_BUF_SZ] = "1008000";

struct sun4i_power_module {
    struct power_module base;
    pthread_mutex_t lock;
    int boostpulse_fd;
    int boostpulse_warned;
};

int sysfs_read(const char *path, char *buf, size_t size)
{
    int fd, len;

    fd = open(path, O_RDONLY);
    if (fd < 0)
        return -1;

    do {
        len = read(fd, buf, size);
    } while (len < 0 && errno == EINTR);

    close(fd);

    return len;
}

static void sysfs_write(char *path, char *s)
{
    char buf[80];
    int len;
    int fd = open(path, O_WRONLY);

    if (fd < 0) {
        strerror_r(errno, buf, sizeof(buf));
        ALOGE("Error opening %s: %s\n", path, buf);
        return;
    }

    len = write(fd, s, strlen(s));
    if (len < 0) {
        strerror_r(errno, buf, sizeof(buf));
        ALOGE("Error writing to %s: %s\n", path, buf);
    }

    close(fd);
}

static void sun4i_power_init(struct power_module *module)
{
    /*
     * cpufreq interactive governor: timer 20ms, min sample 80ms,
     * hispeed 700MHz at load 85%, enable input boost.
     */

    sysfs_write("/sys/devices/system/cpu/cpufreq/interactive/timer_rate",
                "20000");
    sysfs_write("/sys/devices/system/cpu/cpufreq/interactive/min_sample_time",
                "80000");
    sysfs_write("/sys/devices/system/cpu/cpufreq/interactive/hispeed_freq",
                "700000");
    sysfs_write("/sys/devices/system/cpu/cpufreq/interactive/go_hispeed_load",
                "85");
    sysfs_write("/sys/devices/system/cpu/cpufreq/interactive/above_hispeed_delay",
                "20000");
}

static int boostpulse_open(struct sun4i_power_module *sun4i)
{
    char buf[80];

    pthread_mutex_lock(&sun4i->lock);

    if (sun4i->boostpulse_fd < 0) {
        sun4i->boostpulse_fd = open(BOOSTPULSE_PATH, O_WRONLY);

        if (sun4i->boostpulse_fd < 0) {
            if (!sun4i->boostpulse_warned) {
                strerror_r(errno, buf, sizeof(buf));
                ALOGE("Error opening %s: %s\n", BOOSTPULSE_PATH, buf);
                sun4i->boostpulse_warned = 1;
            }
        }
    }

    pthread_mutex_unlock(&sun4i->lock);
    return sun4i->boostpulse_fd;
}

static void sun4i_power_set_interactive(struct power_module *module, int on)
{
    int len;

    char buf[MAX_BUF_SZ];

    /*
     * Lower maximum frequency when screen is off.
     */

    if (!on) {
        /* read the current scaling max freq and save it before updating */
        len = sysfs_read(SCALINGMAXFREQ_PATH, buf, sizeof(buf));

        /* make sure it's not the screen off freq, if the "on"
         * call is skipped (can happen if you press the power
         * button repeatedly) we might have read it. We should
         * skip it if that's the case
         */
        if (len != -1 && strncmp(buf, screen_off_max_freq,
                strlen(screen_off_max_freq)) != 0)
            memcpy(scaling_max_freq, buf, sizeof(buf));

        sysfs_write(SCALINGMAXFREQ_PATH, screen_off_max_freq);
    } else
        sysfs_write(SCALINGMAXFREQ_PATH, scaling_max_freq);

    sysfs_write("/sys/devices/system/cpu/cpufreq/interactive/input_boost",
                on ? "1" : "0");
}

static void sun4i_power_hint(struct power_module *module, power_hint_t hint,
                            void *data)
{
    struct sun4i_power_module *sun4i = (struct sun4i_power_module *) module;
    char buf[80];
    int len;

    switch (hint) {
    case POWER_HINT_INTERACTION:
    case POWER_HINT_CPU_BOOST:
        if (boostpulse_open(sun4i) >= 0) {
	    len = write(sun4i->boostpulse_fd, "1", 1);

	    if (len < 0) {
	        strerror_r(errno, buf, sizeof(buf));
		ALOGE("Error writing to %s: %s\n", BOOSTPULSE_PATH, buf);
	    }
	}
        break;

    case POWER_HINT_VSYNC:
        break;

    default:
        break;
    }
}

static struct hw_module_methods_t power_module_methods = {
    .open = NULL,
};

struct sun4i_power_module HAL_MODULE_INFO_SYM = {
    base: {
        common: {
            tag: HARDWARE_MODULE_TAG,
            module_api_version: POWER_MODULE_API_VERSION_0_2,
            hal_api_version: HARDWARE_HAL_API_VERSION,
            id: POWER_HARDWARE_MODULE_ID,
            name: "SUN4I Power HAL",
            author: "The Android Open Source Project",
            methods: &power_module_methods,
        },

       init: sun4i_power_init,
       setInteractive: sun4i_power_set_interactive,
       powerHint: sun4i_power_hint,
    },

    lock: PTHREAD_MUTEX_INITIALIZER,
    boostpulse_fd: -1,
    boostpulse_warned: 0,
};
