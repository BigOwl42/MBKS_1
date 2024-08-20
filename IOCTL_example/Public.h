/*++

Module Name:

    public.h

Abstract:

    This module contains the common declarations shared by driver
    and user applications.

Environment:

    user and kernel

--*/

//
// Define an Interface Guid so that app can find the device and talk to it.
//

DEFINE_GUID (GUID_DEVINTERFACE_ProcCreateFilter,
    0x97b729a5,0xd494,0x4075,0x8f,0xf6,0xc8,0xdd,0xc6,0xa7,0xfa,0x62);
// {97b729a5-d494-4075-8ff6-c8ddc6a7fa62}
