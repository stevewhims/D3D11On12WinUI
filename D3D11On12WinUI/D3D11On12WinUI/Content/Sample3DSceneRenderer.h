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
    class Sample3DSceneRenderer final
    {
        // data members

        bool m_animating{ false };
        UINT m_cbvDescriptorSize{ 0 };
        DX::DeviceResources m_deviceResources;
        winrt::IBuffer m_fileBufferPS{ nullptr };
        winrt::IBuffer m_fileBufferVS{ nullptr };
        bool m_onDpiChangedQueued{ false };
        bool m_onSizeChangedQueued{ false };
        std::unique_ptr<Cube> m_pCube{ nullptr };
        std::unique_ptr<SampleTextRenderer> m_pSampleTextRenderer{ nullptr };
        winrt::Rect m_queuedBounds{ 0.f, 0.f, 0.f, 0.f };
        winrt::IAsyncAction m_renderLoopWorkItem{ nullptr };
        bool m_shaderAndwindowIndependentSetupDone{ false };
        DX::StepTimer m_stepTimer;
        WorldViewProjectionConstantBuffer m_wvpConstantBufferData;

        // Direct3D data members

        winrt::com_ptr<::ID3D12GraphicsCommandList> m_pD3D12GraphicsCommandList;
        winrt::com_ptr<::ID3D12PipelineState> m_pD3D12PipelineState;
        winrt::com_ptr<::ID3D12RootSignature> m_pD3D12RootSignature;

        // member functions

        void CreateBuffers();
        void ReleaseBuffers();
        void Reset();
        winrt::fire_and_forget SetupAsync();
        winrt::IAsyncAction ShaderSetupAsync();
        void UpdateAndRender();
        void UpdateViewMatrix();
        void WindowIndependentReset();
        void WindowIndependentSetup();
        void WindowDependentReset();
        void WindowDependentSetup();

    public:
        Sample3DSceneRenderer();

        // member functions

        void Animate();
        void OnDpiChanged(winrt::Rect const& bounds);
        void OnSizeChanged(winrt::Rect const& bounds);
        void SetWindowAndSwapChainPanel(winrt::Window const& window, HWND hWnd, winrt::SwapChainPanel const& swapChainPanel);
        void StartRenderLoop(bool settingUp = true);

        // accessors

        DX::DeviceResources const& DeviceResources() const { return m_deviceResources; };
        WorldViewProjectionConstantBuffer const& WvpConstantBufferData() { return m_wvpConstantBufferData; }

        // Direct3D accessors

        winrt::com_ptr<::ID3D12GraphicsCommandList> const& GetD3D12GraphicsCommandList() const { return m_pD3D12GraphicsCommandList; }
        winrt::com_ptr<::ID3D12PipelineState> const& GetD3D12PipelineState() const { return m_pD3D12PipelineState; }
        winrt::com_ptr<::ID3D12RootSignature> const& GetD3D12RootSignature() const { return m_pD3D12RootSignature; }

        // mutators

        void WorldTransform(DirectX::XMFLOAT4X4 const& worldTransform) { m_wvpConstantBufferData.world = worldTransform; }
    };
}
