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

namespace winrt::D3D11On12WinUI
{
    // Represents a vertex containing position, normal, and color vectors.
    // Used to send per-vertex data to the vertex shader.
    struct VertexPositionNormalColor final
    {
        VertexPositionNormalColor(DX::Vector3 const& position,
            DX::Vector3 const& normal,
            DX::Vector3 const& color = DX::Vector3(1.f, 1.f, 1.f)) :
            Position{ position },
            Normal{ normal },
            Color{ color }
        {}

        DX::Vector3 Position;
        DX::Vector3 Normal;
        DX::Vector3 Color;

        static constexpr D3D12_INPUT_LAYOUT_DESC D3D12InputLayoutDesc()
        {
            return { VertexPositionNormalColor::s_d3d12VertexDesc.data(), (uint32_t)VertexPositionNormalColor::s_d3d12VertexDesc.size() };
        }

    private:
        static const std::array<D3D12_INPUT_ELEMENT_DESC, 3> s_d3d12VertexDesc;
    };

    // Constant buffer used to send world-view-projection matrices to the vertex shader.
    // See https://docs.microsoft.com/windows/desktop/direct3d9/transforms.
    struct WorldViewProjectionConstantBuffer final
    {
        DirectX::XMFLOAT4X4 world;
        DirectX::XMFLOAT4X4 view;
        DirectX::XMFLOAT4X4 projection;
    };
}
