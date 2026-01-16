#pragma once

#include <windows.h>
#include <wdf.h>
#include <iddcx.h>

// Tracing support
#define WPP_CONTROL_GUIDS \
    WPP_DEFINE_CONTROL_GUID( \
        VirMonDriverTraceGuid, \
        (B27B8B08, 0855, 41F2, 8560, 410057041539), \
        WPP_DEFINE_BIT(MYDRIVER_ALL_INFO) \
    )

extern "C" DRIVER_INITIALIZE DriverEntry;

EVT_WDF_DRIVER_DEVICE_ADD VirMonDriverDeviceAdd;
EVT_WDF_OBJECT_CONTEXT_CLEANUP VirMonDriverContextCleanup;
