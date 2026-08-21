// Copyright (c) Microsoft Corporation.  All rights reserved

#if !defined(__cplusplus)
    #error C++11 required
#endif

#pragma once

#include <XTaskQueue.h>

extern "C" 
{

enum class XGameActivationType : uint32_t
{
    Protocol = 0,
    File = 1,
    PendingGameInvite = 2,
    AcceptedGameInvite = 3
};

struct XGameActivationInfo
{
    XGameActivationType type;

    union
    {
        const char* protocolUri;
        const char* file;
        const char* inviteUri;
    };
};

typedef void CALLBACK XGameActivationCallback(
    _In_opt_ void* context,
    _In_ const XGameActivationInfo* activationInfo
    );

STDAPI XGameActivationRegisterForEvent(
    _In_opt_ XTaskQueueHandle queue,
    _In_opt_ void* context,
    _In_ XGameActivationCallback* callback,
    _Out_ XTaskQueueRegistrationToken* token
    ) noexcept;

STDAPI_(bool) XGameActivationUnregisterForEvent(
    _In_ XTaskQueueRegistrationToken token,
    _In_ bool wait
    ) noexcept;

STDAPI XGameActivationAcceptPendingInvite(
    _In_z_ const char* inviteUri
    ) noexcept;

}
