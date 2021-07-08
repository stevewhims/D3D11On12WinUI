//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#pragma once

namespace DX
{
    // Function that asynchronously reads from a binary file.
    inline winrt::IAsyncOperation<winrt::IBuffer> ReadDataAsync(std::wstring const& filename)
    {
        auto installedLocation{ winrt::Package::Current().InstalledLocation() };
        winrt::StorageFile storageFile{ co_await installedLocation.GetFileAsync(filename) };
        co_return co_await winrt::FileIO::ReadBufferAsync(storageFile);
    }

    // Converts a length in device-independent pixels (DIPs) to a length in physical pixels.
    inline float ConvertDIPsToPixels(float dips, float dpi)
    {
        static constexpr float dipsPerLogicalInch{ 96.f };
        return floorf(dips * dpi / dipsPerLogicalInch + 0.5f); // Round to nearest integer.
    }

    // Converts a length in physical pixels to a length in device-independent pixels (DIPs).
    inline float ConvertPixelsToDIPs(LONG pixels, float dpi)
    {
        static constexpr float dipsPerLogicalInch{ 96.f };
        return (float)pixels * dipsPerLogicalInch / dpi;
    }

    // Represents a vector with three components.
    struct Vector3 final : public DirectX::XMFLOAT3
    {
        Vector3(float x = 0, float y = 0, float z = 0) :
            DirectX::XMFLOAT3{ x, y, z }
        {}

        // Conversion operator that lets a Vector3 be used wherever a DirectX::XMVECTOR is expected.
        operator const DirectX::XMVECTOR() const
        {
            return DirectX::XMLoadFloat3(this);
        }
    };

    inline Vector3 operator - (Vector3 const& lvec, Vector3 const& rvec)
    {
        return Vector3{
            lvec.x - rvec.x,
            lvec.y - rvec.y,
            lvec.z - rvec.z };
    }

    inline bool operator == (Vector3 const& lvec, Vector3 const& rvec)
    {
        return (
            lvec.x == rvec.x &&
            lvec.y == rvec.y &&
            lvec.z == rvec.z);
    }

#if defined (_DEBUG)
    // Check for SDK Layer support.
    inline bool SdkLayersAvailable()
    {
        HRESULT hr{ ::D3D11CreateDevice(
            nullptr,
            D3D_DRIVER_TYPE_NULL,		// There's no need to create a real hardware device.
            0,
            D3D11_CREATE_DEVICE_DEBUG,	// Check for the SDK layers.
            nullptr,					// Any feature level will do.
            0,
            D3D11_SDK_VERSION,
            nullptr,					// No need to keep the Direct3D device reference.
            nullptr,					// No need to know the feature level.
            nullptr						// No need to keep the Direct3D device context reference.
        ) };

        return SUCCEEDED(hr);
    }
#endif
}
