// Copyright (c) Microsoft Corporation.  All rights reserved

#if !defined(__cplusplus)
    #error C++11 required
#endif

#pragma once

#include <XTaskQueue.h>

extern "C" 
{

typedef void CALLBACK XGameProtocolActivationCallback(
    _In_opt_ void* context,
    _In_ const char* protocolUri
    );

EXTERN_C __declspec(deprecated("Use XGameActivationRegisterForEvent instead of XGameProtocolRegisterForActivation. For more information, see the documentation."))
HRESULT STDAPICALLTYPE XGameProtocolRegisterForActivation(
    _In_opt_ XTaskQueueHandle queue,
    _In_opt_ void* context,
    _In_ XGameProtocolActivationCallback * callback,
    _Out_ XTaskQueueRegistrationToken* token
    ) noexcept;

EXTERN_C __declspec(deprecated("Use XGameActivationUnregisterForEvent instead of XGameProtocolUnregisterForActivation. For more information, see the documentation."))
bool STDAPICALLTYPE XGameProtocolUnregisterForActivation(
    _In_ XTaskQueueRegistrationToken token,
    _In_ bool wait
    ) noexcept;

}
