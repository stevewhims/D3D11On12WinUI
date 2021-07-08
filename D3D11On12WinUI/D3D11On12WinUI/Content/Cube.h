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
    class Sample3DSceneRenderer;

    class Cube final
    {
        static constexpr UINT s_alignedWvpConstantBufferSize{ (sizeof(WorldViewProjectionConstantBuffer) + 255) & ~255 }; // A constant buffer must be 256-byte aligned.
        static inline D3D12_HEAP_PROPERTIES s_heapPropertiesUpload{ CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD) };

        // data members

        UINT m_cbvDescriptorSize{ 0 };
        std::array<uint16_t, 36> m_indices;
        unsigned char* m_pMappedWvpConstantBuffer{ nullptr };
        std::vector<VertexPositionNormalColor> m_vertices;
        Sample3DSceneRenderer & m_sample3DSceneRenderer;
        DirectX::XMFLOAT4X4 m_worldTransform;
        std::array<WorldViewProjectionConstantBuffer, DX::DeviceResources::NumFramebuffers()> m_wvpConstantBufferDatas;

        // Direct3D data members

        std::array<D3D12_GPU_DESCRIPTOR_HANDLE, DX::DeviceResources::NumFramebuffers()> m_gpuDescriptorHandleWvpCbv{};
        D3D12_INDEX_BUFFER_VIEW m_d3d12IndexView{};
        D3D12_VERTEX_BUFFER_VIEW m_d3d12VertexView{};
        D3D12_GPU_DESCRIPTOR_HANDLE m_d3d12WvpConstantBufferGPUHandle{};
        winrt::com_ptr<::ID3D12Resource> m_pD3D12IndexBufferUpload{};
        winrt::com_ptr<::ID3D12Resource> m_pD3D12IndexResource;
        winrt::com_ptr<::ID3D12Resource> m_pD3D12VertexBufferUpload{};
        winrt::com_ptr<::ID3D12Resource> m_pD3D12VertexResource;
        winrt::com_ptr<::ID3D12DescriptorHeap> m_pD3D12WvpCbvDescriptorHeap;
        winrt::com_ptr<::ID3D12Resource> m_pD3D12WvpConstantBuffer;

    public:
        Cube(Sample3DSceneRenderer& sample3DSceneRenderer);
        ~Cube();

        // member functions

        void CreateBuffers(winrt::com_ptr<::ID3D12GraphicsCommandList> const& pD3D12GraphicsCommandList);
        void ReleaseBuffers();
        void ReleaseUploadBuffers();
        void Render(winrt::com_ptr<::ID3D12GraphicsCommandList> const& pD3D12GraphicsCommandList);
        void SetIAState(ID3D12GraphicsCommandList* pD3D12GraphicsCommandList) const;

        // mutators

        void Rotation(DX::Vector3 const& rotation);
    };
}
