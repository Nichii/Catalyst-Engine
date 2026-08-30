#pragma once

#include <windows.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <wrl/client.h>

#include <format>
#include <stdexcept>
#include <string_view>

inline void ThrowIfFailed(HRESULT result, std::string_view message)
{
	if (FAILED(result))
	{
		throw std::runtime_error(std::format("{} failed (HRESULT 0x{:08X})", message, static_cast<unsigned>(result)));
	}
}

inline void ThrowIfFailedDetailed(
    HRESULT result,
    const Microsoft::WRL::ComPtr<ID3DBlob>& error,
    std::string_view message)
{
    if (SUCCEEDED(result))
    {
        return;
    }

    std::string errorMessage;

    if (error && error->GetBufferPointer() && error->GetBufferSize() > 0)
    {
        errorMessage.assign(
            static_cast<const char*>(error->GetBufferPointer()),
            error->GetBufferSize());

        while (!errorMessage.empty() &&
            (errorMessage.back() == '\0' ||
                errorMessage.back() == '\r' ||
                errorMessage.back() == '\n'))
        {
            errorMessage.pop_back();
        }
    }

    if (!errorMessage.empty())
    {
        throw std::runtime_error(
            std::format(
                "{} failed (HRESULT 0x{:08X}): {}",
                message,
                static_cast<unsigned>(result),
                errorMessage));
    }

    ThrowIfFailed(result, message);
}