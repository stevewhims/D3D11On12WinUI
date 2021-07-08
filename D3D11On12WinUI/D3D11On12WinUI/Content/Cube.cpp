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

#include "pch.h"

namespace winrt::D3D11On12WinUI
{
    // Creates and stores the cube's vertex and index data.
    Cube::Cube(Sample3DSceneRenderer& sample3DSceneRenderer) :
        m_sample3DSceneRenderer{ sample3DSceneRenderer }
    {
        constexpr float r{ 0.5f };
        std::array<float, 72> positions{ r, r, r, r, -r, r, -r, -r, r, -r, r, r, r, r, -r, r, -r, -r, r, -r, r, r, r, r, -r, r, -r, -r, -r, -r, r, -r, -r, r, r, -r, -r, r, r, -r, -r, r, -r, -r, -r, -r, r, -r, r, r, -r, r, r, r, -r, r, r, -r, r, -r, r, -r, r, r, -r, -r, -r, -r, -r, -r, -r, r };
        std::array<float, 72> normals{ 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 1, 0, -0, 1, 0, 0, 1, 0, -0, 1, 0, -0, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, -1, 0, -0, -1, -0, 0, -1, 0, -0, -1, 0, -0, 0, 1, 0, 0, 1, -0, 0, 1, 0, 0, 1, 0, -0, -1, 0, 0, -1, -0, -0, -1, 0, -0, -1, 0, };
        for (size_t ix{ 0 }; ix < 72; ix += 3)
        {
            m_vertices.push_back(VertexPositionNormalColor(
                DX::Vector3(positions[ix], positions[ix + 1], positions[ix + 2]),
                DX::Vector3(normals[ix], normals[ix + 1], normals[ix + 2])));
        }
        m_indices = std::array<uint16_t, 36>{ 0, 1, 2, 0, 2, 3, 4, 5, 6, 4, 6, 7, 8, 9, 10, 8, 10, 11, 12, 13, 14, 12, 14, 15, 16, 17, 18, 16, 18, 19, 20, 21, 22, 20, 22, 23 };
    }

    Cube::~Cube()
    {
        m_pD3D12WvpConstantBuffer->Unmap(0, nullptr);
        m_pMappedWvpConstantBuffer = nullptr;

        ReleaseBuffers();
    }

    // Create and upload data for the cube's geometry, etc.
    void Cube::CreateBuffers(winrt::com_ptr<::ID3D12GraphicsCommandList> const& pD3D12GraphicsCommandList)
    {
        DX::DeviceResources const& deviceResources{ m_sample3DSceneRenderer.DeviceResources() };

        D3D12_RESOURCE_DESC constantBufferDesc{ CD3DX12_RESOURCE_DESC::Buffer(DX::DeviceResources::NumFramebuffers() * s_alignedWvpConstantBufferSize) };
        winrt::check_hresult(deviceResources.ID3D12Device()->CreateCommittedResource(
            &Cube::s_heapPropertiesUpload,
            D3D12_HEAP_FLAG_NONE,
            &constantBufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            __uuidof(m_pD3D12WvpConstantBuffer),
            m_pD3D12WvpConstantBuffer.put_void()));

        // Map the constant buffers.
        D3D12_RANGE readRange{ CD3DX12_RANGE(0, 0) }; // We don't intend to read this resource on the CPU.
        winrt::check_hresult(m_pD3D12WvpConstantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_pMappedWvpConstantBuffer)));
        ::ZeroMemory(m_pMappedWvpConstantBuffer, (size_t)constantBufferDesc.Width);

        // Create a descriptor heap for the constant buffers.
        {
            D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDescription{};
            descriptorHeapDescription.NumDescriptors = DX::DeviceResources::NumFramebuffers();
            descriptorHeapDescription.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
            // This descriptor heap can be bound to the pipeline, and descriptors contained within it can be referenced by a root table.
            descriptorHeapDescription.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
            winrt::check_hresult(deviceResources.ID3D12Device()->CreateDescriptorHeap(&descriptorHeapDescription, _uuidof(m_pD3D12WvpCbvDescriptorHeap), m_pD3D12WvpCbvDescriptorHeap.put_void()));
        }

        // Create constant buffer views for accessing the upload buffer.
        D3D12_GPU_VIRTUAL_ADDRESS cbvGpuAddress{ m_pD3D12WvpConstantBuffer->GetGPUVirtualAddress() };
        CD3DX12_CPU_DESCRIPTOR_HANDLE cbvCpuHandle{ m_pD3D12WvpCbvDescriptorHeap->GetCPUDescriptorHandleForHeapStart() };
        m_cbvDescriptorSize = deviceResources.ID3D12Device()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptorHandleForHeapStart{ m_pD3D12WvpCbvDescriptorHeap->GetGPUDescriptorHandleForHeapStart() };

        for (std::size_t frameBufferIndex{ 0 }; frameBufferIndex < DX::DeviceResources::NumFramebuffers(); ++frameBufferIndex)
        {
            D3D12_CONSTANT_BUFFER_VIEW_DESC desc;
            desc.BufferLocation = cbvGpuAddress;
            desc.SizeInBytes = s_alignedWvpConstantBufferSize;
            deviceResources.ID3D12Device()->CreateConstantBufferView(&desc, cbvCpuHandle);

            m_gpuDescriptorHandleWvpCbv[frameBufferIndex] = CD3DX12_GPU_DESCRIPTOR_HANDLE
            (
                gpuDescriptorHandleForHeapStart,
                static_cast<int>(frameBufferIndex),
                m_cbvDescriptorSize
            );

            cbvGpuAddress += desc.SizeInBytes;
            cbvCpuHandle.Offset(m_cbvDescriptorSize);
        }

        winrt::com_ptr<::ID3D12Device> pD3D12Device{ m_sample3DSceneRenderer.DeviceResources().ID3D12Device() };

        D3D12_HEAP_PROPERTIES heapPropertiesDefault{ CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT) };

        // Create the vertex and index buffer resources in the GPU's default heap, and copy
        // vertex data into them using the upload heap. The upload resources mustn't be released
        // until after the GPU has finished using them.
        {
            const UINT vertexBufferSizeInBytes{ (UINT)(sizeof(VertexPositionNormalColor) * m_vertices.size()) };

            D3D12_RESOURCE_DESC vertexBufferDesc{ CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSizeInBytes) };
            winrt::check_hresult(pD3D12Device->CreateCommittedResource(
                &heapPropertiesDefault,
                D3D12_HEAP_FLAG_NONE,
                &vertexBufferDesc,
                D3D12_RESOURCE_STATE_COPY_DEST,
                nullptr,
                __uuidof(m_pD3D12VertexResource),
                m_pD3D12VertexResource.put_void()));

            winrt::check_hresult(pD3D12Device->CreateCommittedResource(
                &Cube::s_heapPropertiesUpload,
                D3D12_HEAP_FLAG_NONE,
                &vertexBufferDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                __uuidof(m_pD3D12VertexBufferUpload),
                m_pD3D12VertexBufferUpload.put_void()));

            {
                D3D12_SUBRESOURCE_DATA vertexData{};
                vertexData.pData = m_vertices.data();
                vertexData.RowPitch = vertexBufferSizeInBytes;
                vertexData.SlicePitch = vertexData.RowPitch;

                // Upload the vertex buffer to the GPU.
                ::UpdateSubresources(pD3D12GraphicsCommandList.get(), m_pD3D12VertexResource.get(), m_pD3D12VertexBufferUpload.get(), 0, 0, 1, &vertexData);

                D3D12_RESOURCE_BARRIER vertexBufferResourceBarrier
                {
                    CD3DX12_RESOURCE_BARRIER::Transition(
                        m_pD3D12VertexResource.get(),
                        D3D12_RESOURCE_STATE_COPY_DEST,
                        D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER)
                };
                pD3D12GraphicsCommandList->ResourceBarrier(1, &vertexBufferResourceBarrier);
            }

            // Set up m_d3d12VertexView now, and use it later in SetIAState() to call ID3D12GraphicsCommandList::IASetVertexBuffers.
            m_d3d12VertexView.BufferLocation = m_pD3D12VertexResource->GetGPUVirtualAddress();
            m_d3d12VertexView.StrideInBytes = sizeof(VertexPositionNormalColor);
            m_d3d12VertexView.SizeInBytes = vertexBufferSizeInBytes;
        }

        {
            const UINT indexBufferSizeInBytes{ (UINT)(sizeof(uint16_t) * m_indices.size()) };

            D3D12_RESOURCE_DESC indexBufferDesc{ CD3DX12_RESOURCE_DESC::Buffer(indexBufferSizeInBytes) };
            winrt::check_hresult(pD3D12Device->CreateCommittedResource(
                &heapPropertiesDefault,
                D3D12_HEAP_FLAG_NONE,
                &indexBufferDesc,
                D3D12_RESOURCE_STATE_COPY_DEST,
                nullptr,
                _uuidof(m_pD3D12IndexResource),
                m_pD3D12IndexResource.put_void()));

            winrt::check_hresult(pD3D12Device->CreateCommittedResource(
                &Cube::s_heapPropertiesUpload,
                D3D12_HEAP_FLAG_NONE,
                &indexBufferDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                _uuidof(m_pD3D12IndexBufferUpload),
                m_pD3D12IndexBufferUpload.put_void()));

            {
                D3D12_SUBRESOURCE_DATA indexData{};
                indexData.pData = m_indices.data();
                indexData.RowPitch = indexBufferSizeInBytes;
                indexData.SlicePitch = indexData.RowPitch;

                // Upload the index buffer to the GPU.
                ::UpdateSubresources(pD3D12GraphicsCommandList.get(), m_pD3D12IndexResource.get(), m_pD3D12IndexBufferUpload.get(), 0, 0, 1, &indexData);

                D3D12_RESOURCE_BARRIER indexBufferResourceBarrier
                {
                    CD3DX12_RESOURCE_BARRIER::Transition(
                        m_pD3D12IndexResource.get(),
                        D3D12_RESOURCE_STATE_COPY_DEST,
                        D3D12_RESOURCE_STATE_INDEX_BUFFER)
                };
                pD3D12GraphicsCommandList->ResourceBarrier(1, &indexBufferResourceBarrier);
            }

            // Set up m_indicesView now, and use it later in SetIAState() to call ID3D12GraphicsCommandList::IASetIndexBuffer.
            m_d3d12IndexView.BufferLocation = m_pD3D12IndexResource->GetGPUVirtualAddress();
            m_d3d12IndexView.SizeInBytes = indexBufferSizeInBytes;
            m_d3d12IndexView.Format = sizeof(uint16_t) == 4 ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
        }
    }

    void Cube::ReleaseBuffers()
    {
        ReleaseUploadBuffers();

        m_pD3D12IndexResource = nullptr;
        m_pD3D12VertexResource = nullptr;
        m_pD3D12WvpCbvDescriptorHeap = nullptr;
        m_pD3D12WvpConstantBuffer = nullptr;
    }

    void Cube::ReleaseUploadBuffers()
    {
        m_pD3D12VertexBufferUpload = nullptr;
        m_pD3D12IndexBufferUpload = nullptr;
    }

    void Cube::Render(winrt::com_ptr<::ID3D12GraphicsCommandList> const& pD3D12GraphicsCommandList)
    {
        DX::DeviceResources const& deviceResources{ m_sample3DSceneRenderer.DeviceResources() };

        // A command allocator can be reset only when its command lists have finished execution on the GPU.
        winrt::check_hresult(deviceResources.ID3D12CommandAllocator()->Reset());

        // The command list itself can be reset any time after ExecuteCommandLists is called.
        winrt::check_hresult(pD3D12GraphicsCommandList->Reset(deviceResources.ID3D12CommandAllocator(), m_sample3DSceneRenderer.GetD3D12PipelineState().get()));

        // Set the graphics root signature and descriptor heaps to be used by this frame.
        pD3D12GraphicsCommandList->SetGraphicsRootSignature(m_sample3DSceneRenderer.GetD3D12RootSignature().get());
        ID3D12DescriptorHeap* pHeaps{ m_pD3D12WvpCbvDescriptorHeap.get() };
        pD3D12GraphicsCommandList->SetDescriptorHeaps(1, &pHeaps);

        // Set the viewport and scissor rectangle.
        D3D12_VIEWPORT d3d12Viewport{ deviceResources.D3D12Viewport() };
        pD3D12GraphicsCommandList->RSSetViewports(1, &d3d12Viewport);
        D3D12_RECT d3d12ScissorRect{ deviceResources.D3D12ScissorRect() };
        pD3D12GraphicsCommandList->RSSetScissorRects(1, &d3d12ScissorRect);

        // Indicate that this resource will be in use as a render target.
        auto renderTargetResourceBarrier{ CD3DX12_RESOURCE_BARRIER::Transition(deviceResources.ID3D12RenderTarget(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET) };
        pD3D12GraphicsCommandList->ResourceBarrier(1, &renderTargetResourceBarrier);

        D3D12_CPU_DESCRIPTOR_HANDLE const& renderTargetView{ deviceResources.D3D12RenderTargetView() };
        D3D12_CPU_DESCRIPTOR_HANDLE const& depthStencilView{ deviceResources.D3D12DepthStencilView() };

        constexpr float clearColor4[]{ 0.f, 0.f, 0.f, 0.f };
        pD3D12GraphicsCommandList->ClearRenderTargetView(renderTargetView, clearColor4, 0, nullptr);
        pD3D12GraphicsCommandList->ClearDepthStencilView(depthStencilView, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);

        pD3D12GraphicsCommandList->OMSetRenderTargets(1, &renderTargetView, false, &depthStencilView);

        m_sample3DSceneRenderer.WorldTransform(m_worldTransform);

        // Update the constant buffer resource.
        unsigned int offset{ deviceResources.CurrentFrameIndex() * s_alignedWvpConstantBufferSize };
        unsigned char* destination{ m_pMappedWvpConstantBuffer + offset };
        memcpy(destination, &m_sample3DSceneRenderer.WvpConstantBufferData(), sizeof(m_sample3DSceneRenderer.WvpConstantBufferData()));

        // Bind the current frame's constant buffer to the pipeline.
        pD3D12GraphicsCommandList->SetGraphicsRootDescriptorTable(0, m_gpuDescriptorHandleWvpCbv[deviceResources.CurrentFrameIndex()]);

        ::PIXBeginEvent(pD3D12GraphicsCommandList.get(), 0, L"Render");
        {
            // Record drawing commands.
            pD3D12GraphicsCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            SetIAState(m_sample3DSceneRenderer.GetD3D12GraphicsCommandList().get());
            pD3D12GraphicsCommandList->DrawIndexedInstanced(36, 1, 0, 0, 0);
        }
        ::PIXEndEvent(pD3D12GraphicsCommandList.get());

        // Remain in RENDER_TARGET state. The ID3D11On12Device::ReleaseWrappedResources call
        // will take care of transitioning the render target to PRESENT.

        winrt::check_hresult(pD3D12GraphicsCommandList->Close());

        // Execute the command list.
        ID3D12CommandList* pCommandList{ pD3D12GraphicsCommandList.get() };
        deviceResources.ID3D12CommandQueue()->ExecuteCommandLists(1, &pCommandList);
    }

    void Cube::Rotation(DX::Vector3 const& rotation)
    {
        // the components of the vector represent right-hand rotation (counter-clockwise looking down a +ve axis toward the origin).
        DirectX::XMStoreFloat4x4(&m_worldTransform, DirectX::XMMatrixRotationRollPitchYawFromVector(rotation));
    }

    void Cube::SetIAState(ID3D12GraphicsCommandList* pD3D12GraphicsCommandList) const
    {
        pD3D12GraphicsCommandList->IASetVertexBuffers(0, 1, &m_d3d12VertexView);
        pD3D12GraphicsCommandList->IASetIndexBuffer(&m_d3d12IndexView);
    }
}
