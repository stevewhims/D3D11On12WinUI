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
    class DeviceResources final
    {
        static constexpr UINT s_numFramebuffers{ 3 }; // Use triple-buffering.

        // data members

        UINT m_currentBufferIndex{ 0 };
        SIZE m_d3dRenderTargetSize{ 0L, 0L };
        DirectX::XMFLOAT2 m_dpi{ 96.f, 96.f };
        winrt::handle m_fenceEventHandle{ 0 };
        std::array<UINT64, s_numFramebuffers> m_fenceValues{};
        HWND m_hWnd{ 0 };
        DirectX::XMFLOAT2 m_outputSizeInDIPs{ 0.f, 0.f };
        DirectX::XMFLOAT2 m_outputSizeInRawPixels{ 0.f, 0.f };
        UINT m_rtvDescriptorSize{ 0 };
        winrt::SwapChainPanel m_swapChainPanel{ nullptr };
        winrt::Window m_window{ nullptr };

        // Direct3D and DXGI data members

        D3D12_CPU_DESCRIPTOR_HANDLE m_d3d12DepthStencilView{};
        std::array<D3D12_CPU_DESCRIPTOR_HANDLE, DeviceResources::s_numFramebuffers> m_d3d12RenderTargetViews{};
        D3D12_VIEWPORT m_d3d12Viewport{};
        D3D12_RECT m_d3d12ScissorRect{};
        DXGI_FORMAT m_dsvFormat{ DXGI_FORMAT_D32_FLOAT };
        winrt::com_ptr<::ID3D11DeviceContext> m_pD3D11DeviceContext{ nullptr };
        winrt::com_ptr<::ID3D11On12Device> m_pD3D11On12Device{ nullptr };
        std::array< winrt::com_ptr<::ID3D11Resource>, s_numFramebuffers> m_pD3D11WrappedRenderTargets{};
        std::array<winrt::com_ptr<::ID3D12CommandAllocator>, s_numFramebuffers> m_pD3D12CommandAllocators{};
        winrt::com_ptr<::ID3D12CommandQueue> m_pD3D12CommandQueue{ nullptr };
        winrt::com_ptr<::ID3D12Fence> m_pD3D12Fence{ nullptr };
#if defined (_DEBUG)
        winrt::com_ptr<::ID3D12Debug> m_pD3DDebugger{ nullptr };
#endif
        winrt::com_ptr<::ID3D12Resource> m_pD3D12DepthStencil{ nullptr };
        winrt::com_ptr<::ID3D12Device> m_pD3D12Device{ nullptr };
        winrt::com_ptr<::ID3D12DescriptorHeap> m_pD3D12DsvHeap{ nullptr };
        std::array<winrt::com_ptr<::ID3D12Resource>, DeviceResources::s_numFramebuffers> m_pD3D12RenderTargets{};
        winrt::com_ptr<::ID3D12DescriptorHeap> m_pD3D12RtvHeap{ nullptr };
        winrt::com_ptr<::IDXGIFactory4> m_pDXGIFactory4{ nullptr };
        winrt::com_ptr<::IDXGISwapChain3> m_pDXGISwapChain3{ nullptr };
        DXGI_FORMAT m_rtvFormat{ DXGI_FORMAT_B8G8R8A8_UNORM };

        // Direct2D and DirectWrite data members

        winrt::com_ptr<::ID2D1Device1> m_pD2D1Device1{ nullptr };
        winrt::com_ptr<::ID2D1DeviceContext1> m_pD2D1DeviceContext1{ nullptr };
        winrt::com_ptr<::ID2D1Factory3> m_pD2D1Factory3{ nullptr };
        winrt::com_ptr<::IDWriteFactory2> m_pDWriteFactory2{ nullptr };
        std::array< winrt::com_ptr<::ID2D1Bitmap1>, DeviceResources::s_numFramebuffers> m_pD2D1TargetBitmap1s{};
        winrt::com_ptr<::IWICImagingFactory2> m_pWICImagingFactory2{ nullptr };

        // member functions

        bool CreateSwapChain();
        void DeviceIndependentSetup();
        bool Present();
        void ReleaseSwapChain();

    public:
        DeviceResources();

        // member functions

        ::ID3D11Resource* AcquireWrappedRenderTarget();
        void DpiAndOutputSize(DirectX::XMFLOAT2 const& outputSize);
        void MoveToNextFrame();
        void OutputSize(DirectX::XMFLOAT2 const& outputSize, bool isInDIPs);
        bool ReleaseWrappedRenderTargetAndPresent(::ID3D11Resource* pWrappedRenderTargets);
        winrt::fire_and_forget SetSwapChainOnSwapChainPanelAsync();
        void SetWindowAndSwapChainPanel(winrt::Window const& window, HWND hWnd, winrt::SwapChainPanel const& swapChainPanel);
        void Trim();
        void WaitForGpu() const;
        void WindowDependentReset();
        bool WindowDependentSetup();
        void WindowIndependentReset();
        void WindowIndependentSetup();

        // accessors

        unsigned int CurrentFrameIndex() const { return m_currentBufferIndex; }
        DirectX::XMFLOAT2 const& Dpi() const { return m_dpi; }
        static constexpr UINT NumFramebuffers(){ return DeviceResources::s_numFramebuffers; }
        DirectX::XMFLOAT2 const& OutputSizeInDIPs() const { return m_outputSizeInDIPs; }

        // Direct3D and DXGI accessors

        ID3D12CommandQueue* ID3D12CommandQueue() const { return m_pD3D12CommandQueue.get(); }
        ID3D12CommandAllocator* ID3D12CommandAllocator() const { return m_pD3D12CommandAllocators[m_currentBufferIndex].get(); }
        DXGI_FORMAT DSVFormat() const { return m_dsvFormat; }
        D3D12_CPU_DESCRIPTOR_HANDLE const& D3D12DepthStencilView() const { return m_d3d12DepthStencilView; }
        winrt::com_ptr<::ID3D12Device> ID3D12Device() const { return m_pD3D12Device; };
        DXGI_FORMAT RTVFormat() const { return m_rtvFormat; }
        D3D12_CPU_DESCRIPTOR_HANDLE const& D3D12RenderTargetView() const { return m_d3d12RenderTargetViews[m_currentBufferIndex]; }
        D3D12_RECT const& D3D12ScissorRect() const { return m_d3d12ScissorRect; }
        D3D12_VIEWPORT const& D3D12Viewport() const { return m_d3d12Viewport; }
        ::ID3D12Resource* ID3D12RenderTarget() const { return m_pD3D12RenderTargets[m_currentBufferIndex].get(); }

        // Direct2D accessors

        ::ID2D1DeviceContext1* ID2D1DeviceContext1() const { return m_pD2D1DeviceContext1.get(); }
        ::ID2D1Factory3* ID2D1Factory3() const { return m_pD2D1Factory3.get(); }
        ::IDWriteFactory2* IDWriteFactory2() const { return m_pDWriteFactory2.get(); }
    };
}
