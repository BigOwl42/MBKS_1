/*++

Module Name:

    driver.h

Abstract:

    This file contains the driver definitions.

Environment:

    Kernel-mode Driver Framework

--*/

#include <fltKernel.h>
#include <dontuse.h>
#include <suppress.h>
#include <wdm.h>
#include <wchar.h>

EXTERN_C_START

//
// WDFDRIVER Events
//

DRIVER_INITIALIZE DriverEntry;
//EVT_WDF_DRIVER_DEVICE_ADD ProcCreateFilterEvtDeviceAdd;
//EVT_WDF_OBJECT_CONTEXT_CLEANUP ProcCreateFilterEvtDriverContextCleanup;

EXTERN_C_END
