#ifndef CLHELPERS_CL_H
#define CLHELPERS_CL_H

#define CL_USE_DEPRECATED_OPENCL_1_1_APIS
#include <CL/cl.h>

#include <QList>
#include <QString>

// check_err.cpp
extern void CL_CHECK_ERR(const char *prefix, cl_int status);

// device.cpp
extern QList<QString> clhAvailableDeviceNames(cl_platform_id platform);

// platform.cpp
extern QList<QString> clhAvailablePlatformNames();
extern cl_platform_id clhSelectPlatform(int index);

#endif // CLHELPERS_CL_H
