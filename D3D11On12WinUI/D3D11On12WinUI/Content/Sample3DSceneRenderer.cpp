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
    Sample3DSceneRenderer::Sample3DSceneRenderer()
    {
        m_pCube = std::make_unique<Cube>(*this);
        m_pCube->Rotation({ 0, 0, 0 });
        m_pSampleTextRenderer = std::make_unique<SampleTextRenderer>(m_deviceResources);
    }

    // Begin animating the cube.
    void Sample3DSceneRenderer::Animate() {
        m_animating = true;
        m_stepTimer = DX::StepTimer();
    };

    void Sample3DSceneRenderer::CreateBuffers()
    {
        m_pCube->CreateBuffers(m_pD3D12GraphicsCommandList);
    }

    void Sample3DSceneRenderer::ReleaseBuffers()
    {
        m_pCube->ReleaseBuffers();
    }

    // We queue dpi changes so that they happen on the right thread.
    void Sample3DSceneRenderer::OnDpiChanged(winrt::Rect const& bounds)
    {
        m_onDpiChangedQueued = true;
        m_queuedBounds = bounds;
    }

    // We queue size changes so that they happen on the right thread.
    void Sample3DSceneRenderer::OnSizeChanged(winrt::Rect const& bounds)
    {
        m_onSizeChangedQueued = true;
        m_queuedBounds = bounds;
    }

    void Sample3DSceneRenderer::Reset()
    {
        m_renderLoopWorkItem.Cancel();
        m_renderLoopWorkItem = nullptr;
        m_deviceResources.MoveToNextFrame();
        WindowDependentReset();
        WindowIndependentReset();
    }

    // We're independent of the main UI thread by this point.
    winrt::fire_and_forget Sample3DSceneRenderer::SetupAsync()
    {
        winrt::apartment_context capturedCallingContext;
        ShaderSetupAsync().get(); // Wait for the shaders to load before continuing setting up.
        co_await capturedCallingContext; // Switch back to the calling context.

        WindowIndependentSetup();
        WindowDependentSetup();
    }

    void Sample3DSceneRenderer::SetWindowAndSwapChainPanel(winrt::Window const& window, HWND hWnd, winrt::SwapChainPanel const& swapChainPanel)
    {
        m_deviceResources.SetWindowAndSwapChainPanel(window, hWnd, swapChainPanel);
    }

    // Asynchronously load shaders.
    winrt::IAsyncAction Sample3DSceneRenderer::ShaderSetupAsync()
    {
        m_fileBufferVS = co_await DX::ReadDataAsync(L"shader_vx_pos3norm3color3_phong.cso");
        m_fileBufferPS = co_await DX::ReadDataAsync(L"shader_px_pos3norm3color3_phong.cso");
    }

    void Sample3DSceneRenderer::StartRenderLoop(bool settingUp)
    {
        // If the render loop is already running, then don't start another.
        if (m_renderLoopWorkItem && m_renderLoopWorkItem.Status() == AsyncStatus::Started)
        {
            return;
        }

        // Create a task that will be run on a background thread.
        winrt::WorkItemHandler workItemHandler([this, settingUp](winrt::IAsyncAction const& workItem)
            {
                if (settingUp)
                {
                    SetupAsync();
                }

                // Calculate the updated frame, and render once per vertical blanking interval.
                while (workItem.Status() == AsyncStatus::Started)
                {
                    UpdateAndRender();
                }
            });

        // Run task on a dedicated high-priority background thread.
        m_renderLoopWorkItem = winrt::ThreadPool::RunAsync(workItemHandler, WorkItemPriority::High, WorkItemOptions::TimeSliced);
    }

    // Update the application state once per frame.
    void Sample3DSceneRenderer::UpdateAndRender()
    {
        if (m_onDpiChangedQueued)
        {
            m_onDpiChangedQueued = false;
            m_deviceResources.DpiAndOutputSize({ m_queuedBounds.Width, m_queuedBounds.Height });
            WindowDependentSetup();
        }
        if (m_onSizeChangedQueued)
        {
            m_onSizeChangedQueued = false;
            m_deviceResources.OutputSize({ m_queuedBounds.Width, m_queuedBounds.Height }, true);
            WindowDependentSetup();
        }

        if (m_shaderAndwindowIndependentSetupDone)
        {
            if (m_animating)
            {
                m_pCube->Rotation({ -sinf(m_stepTimer.TotalSeconds() / 3) / 2, sinf(m_stepTimer.TotalSeconds()), -sinf(m_stepTimer.TotalSeconds() / 3) / 4 });
            }

            m_pCube->Render(m_pD3D12GraphicsCommandList);

            ::ID3D11Resource* pWrappedRenderTarget = m_deviceResources.AcquireWrappedRenderTarget();

            m_pSampleTextRenderer->UpdateAndRender();

            if (!m_deviceResources.ReleaseWrappedRenderTargetAndPresent(pWrappedRenderTarget))
            {
                m_shaderAndwindowIndependentSetupDone = false;
                Reset();
                StartRenderLoop(true);
            }
        }
    }

    void Sample3DSceneRenderer::UpdateViewMatrix()
    {
        DirectX::XMStoreFloat4x4(
            &m_wvpConstantBufferData.view,
            DirectX::XMMatrixTranspose(DirectX::XMMatrixLookToRH(
                { -.3f, 0.f, 1.7f }, // EyePosition
                { 0.f, 0.f, -1.f }, // EyeDirection
                { 0.f, 1.f, 0.f })) // UpDirection
        );
    }

    void Sample3DSceneRenderer::WindowDependentReset()
    {
        m_deviceResources.WindowDependentReset();
    }

    // Initializes view parameters when the window size changes.
    void Sample3DSceneRenderer::WindowDependentSetup()
    {
        m_deviceResources.WindowDependentSetup();
        m_deviceResources.SetSwapChainOnSwapChainPanelAsync();

        DirectX::XMFLOAT2 outputSize{ m_deviceResources.OutputSizeInDIPs() };
        float aspectRatio{ outputSize.x / outputSize.y };
        float fovAngleY{ 65.f * DirectX::XM_PI / 180.f };

        // This sample makes use of a right-handed coordinate system using row-major matrices.
        DirectX::XMMATRIX projectionMatrix{ DirectX::XMMatrixPerspectiveFovRH(
            fovAngleY,
            aspectRatio,
            0.01f,
            100.f
        ) };

        DirectX::XMStoreFloat4x4(
            &m_wvpConstantBufferData.projection,
            XMMatrixTranspose(projectionMatrix));

        UpdateViewMatrix();
    }

    void Sample3DSceneRenderer::WindowIndependentReset()
    {
        ReleaseBuffers();
        m_pD3D12GraphicsCommandList = nullptr;
        m_pD3D12PipelineState = nullptr;
        m_pD3D12RootSignature = nullptr;
        m_pSampleTextRenderer->WindowIndependentReset();
        m_deviceResources.WindowIndependentReset();
    }

    // Does setup work that depends on the D3D device, but doesn't depend on the window.
    void Sample3DSceneRenderer::WindowIndependentSetup()
    {
        m_deviceResources.WindowIndependentSetup();
        m_pSampleTextRenderer->WindowIndependentSetup();

        auto pD3D12Device{ m_deviceResources.ID3D12Device() };

        // Create a root signature with a single constant buffer slot.
        {
            CD3DX12_DESCRIPTOR_RANGE range;
            CD3DX12_ROOT_PARAMETER parameter;

            range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
            parameter.InitAsDescriptorTable(1, &range, D3D12_SHADER_VISIBILITY_VERTEX);

            // Only the input assembler stage needs access to the constant buffer.
            D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags{
                D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
                D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
                D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
                D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
                D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS };

            CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
            rootSignatureDesc.Init(1, &parameter, 0, nullptr, rootSignatureFlags);

            winrt::com_ptr<::ID3DBlob> pSignature;
            winrt::com_ptr<::ID3DBlob> pError;

            winrt::check_hresult(
                ::D3D12SerializeRootSignature(
                    &rootSignatureDesc,
                    D3D_ROOT_SIGNATURE_VERSION_1,
                    pSignature.put(),
                    pError.put()
                )
            );

            winrt::check_hresult(
                pD3D12Device->CreateRootSignature(
                    0,
                    pSignature->GetBufferPointer(),
                    pSignature->GetBufferSize(),
                    _uuidof(m_pD3D12RootSignature),
                    m_pD3D12RootSignature.put_void()
                )
            );
        }

        {
            // Create the pipeline state now that the shaders are loaded.
            CD3DX12_RASTERIZER_DESC rasterizerDesc({
                /*FillMode*/ D3D12_FILL_MODE_SOLID,
                /*CullMode*/ D3D12_CULL_MODE_BACK,
                /*FrontCounterClockwise*/ FALSE,
                /*DepthBias*/ D3D12_DEFAULT_DEPTH_BIAS,
                /*DepthBiasClamp*/ D3D12_DEFAULT_DEPTH_BIAS_CLAMP,
                /*SlopeScaledDepthBias*/ D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,
                /*DepthClipEnable*/ TRUE,
                /*MultisampleEnable*/ FALSE,
                /*AntialiasedLineEnable*/ FALSE,
                /*ForcedSampleCount*/ 0 });

            CD3DX12_BLEND_DESC blendDesc(D3D12_DEFAULT);

            D3D12_GRAPHICS_PIPELINE_STATE_DESC d3d12GraphicsPipelineStateDesc{};
            d3d12GraphicsPipelineStateDesc.InputLayout = VertexPositionNormalColor::D3D12InputLayoutDesc();
            d3d12GraphicsPipelineStateDesc.pRootSignature = m_pD3D12RootSignature.get();
            d3d12GraphicsPipelineStateDesc.VS = { m_fileBufferVS.data(), m_fileBufferVS.Length() };
            d3d12GraphicsPipelineStateDesc.PS = { m_fileBufferPS.data(), m_fileBufferPS.Length() };
            d3d12GraphicsPipelineStateDesc.RasterizerState = rasterizerDesc;
            d3d12GraphicsPipelineStateDesc.BlendState = blendDesc;
            d3d12GraphicsPipelineStateDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
            d3d12GraphicsPipelineStateDesc.SampleMask = UINT_MAX;
            d3d12GraphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
            d3d12GraphicsPipelineStateDesc.NumRenderTargets = 1;
            d3d12GraphicsPipelineStateDesc.RTVFormats[0] = m_deviceResources.RTVFormat();
            d3d12GraphicsPipelineStateDesc.DSVFormat = m_deviceResources.DSVFormat();
            d3d12GraphicsPipelineStateDesc.SampleDesc.Count = 1;

            m_pD3D12PipelineState = nullptr;
            winrt::check_hresult(m_deviceResources.ID3D12Device()->CreateGraphicsPipelineState(&d3d12GraphicsPipelineStateDesc, __uuidof(m_pD3D12PipelineState), m_pD3D12PipelineState.put_void()));

            // Create and upload cube geometry resources to the GPU.

            // Create a command list.
            m_pD3D12GraphicsCommandList = nullptr;
            winrt::check_hresult(
                pD3D12Device->CreateCommandList(
                    0,
                    D3D12_COMMAND_LIST_TYPE_DIRECT,
                    m_deviceResources.ID3D12CommandAllocator(),
                    m_pD3D12PipelineState.get(),
                    __uuidof(m_pD3D12GraphicsCommandList),
                    m_pD3D12GraphicsCommandList.put_void()
                )
            );
        }

        CreateBuffers();

        // Close the command list, and execute it to begin the vertex/index buffer copy into the GPU's default heap.
        winrt::check_hresult(m_pD3D12GraphicsCommandList->Close());

        ID3D12CommandList* pCommandList{ m_pD3D12GraphicsCommandList.get() };
        m_deviceResources.ID3D12CommandQueue()->ExecuteCommandLists(1, &pCommandList);

        // Wait for the command list to finish executing, because the vertex/index
        // buffers need to be uploaded to the GPU before releasing the upload resources.
        m_deviceResources.WaitForGpu();
        m_pCube->ReleaseUploadBuffers();

        m_shaderAndwindowIndependentSetupDone = true;
    }
}
