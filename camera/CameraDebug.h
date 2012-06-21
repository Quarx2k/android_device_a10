#ifndef CAMERA_DEBUG_H
#define CAMERA_DEBUG_H

// #define LOG_NDEBUG 0
#include <cutils/log.h>

#define F_LOG LOGV("%s, line: %d", __FUNCTION__, __LINE__);

#endif // CAMERA_DEBUG_H

