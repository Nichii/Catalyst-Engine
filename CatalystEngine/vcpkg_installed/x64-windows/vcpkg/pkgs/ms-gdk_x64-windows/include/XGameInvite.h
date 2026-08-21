// Copyright (c) Microsoft Corporation.  All rights reserved

#if !defined(__cplusplus)
    #error C++11 required
#endif

#pragma once

#include <XTaskQueue.h>

extern "C" 
{

typedef void CALLBACK XGameInviteEventCallback(
    _In_opt_ void* context,
    _In_ const char* inviteUri
    );

EXTERN_C __declspec(deprecated("Use XGameActivationRegisterForEvent instead of XGameInviteRegisterForEvent. For more information, see the documentation."))
HRESULT STDAPICALLTYPE XGameInviteRegisterForEvent(
    _In_opt_ XTaskQueueHandle queue,
    _In_opt_ void* context,
    _In_ XGameInviteEventCallback* callback,
    _Out_ XTaskQueueRegistrationToken* token
    ) noexcept;

EXTERN_C __declspec(deprecated("Use XGameActivationUnregisterForEvent instead of XGameInviteUnregisterForEvent. For more information, see the documentation."))
bool STDAPICALLTYPE XGameInviteUnregisterForEvent(
    _In_ XTaskQueueRegistrationToken token,
    _In_ bool wait
    ) noexcept;

EXTERN_C __declspec(deprecated("Use XGameActivationRegisterForEvent instead of XGameInviteRegisterForPendingEvent. For more information, see the documentation."))
HRESULT STDAPICALLTYPE XGameInviteRegisterForPendingEvent(
    _In_opt_ XTaskQueueHandle queue,
    _In_opt_ void* context,
    _In_ XGameInviteEventCallback* callback,
    _Out_ XTaskQueueRegistrationToken* token
    ) noexcept;

EXTERN_C __declspec(deprecated("Use XGameActivationUnregisterForEvent instead of XGameInviteUnregisterForPendingEvent. For more information, see the documentation."))
bool STDAPICALLTYPE XGameInviteUnregisterForPendingEvent(
    _In_ XTaskQueueRegistrationToken token,
    _In_ bool wait
    ) noexcept;

EXTERN_C __declspec(deprecated("Use XGameActivationAcceptPendingInvite instead of XGameInviteAcceptPendingInvite. For more information, see the documentation."))
HRESULT STDAPICALLTYPE XGameInviteAcceptPendingInvite(
    _In_z_ const char* inviteUri
    ) noexcept;

}
