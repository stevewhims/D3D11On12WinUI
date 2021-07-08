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

namespace DX
{
    DeviceResources::DeviceResources()
    {
        DeviceIndependentSetup();
    };

    ::ID3D11Resource* DeviceResources::AcquireWrappedRenderTarget()
    {
        // Acquire our wrapped render target resource for the current back buffer.
        ::ID3D11Resource* pWrappedRenderTarget{ m_pD3D11WrappedRenderTargets[m_currentBufferIndex].get() };
        m_pD3D11On12Device->AcquireWrappedResources(&pWrappedRenderTarget, 1);

        // Set the Direct2D target bitmap as the current target.
        m_pD2D1DeviceContext1->SetTarget(m_pD2D1TargetBitmap1s[m_currentBufferIndex].get());

        return pWrappedRenderTarget;
    }

    // Returns `true` if successful; returns `false` if device lost.
    bool DeviceResources::CreateSwapChain()
    {
        if (m_pDXGISwapChain3 == nullptr)
        {
            DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
            swapChainDesc.Width = (UINT)m_d3dRenderTargetSize.cx; // Match the size of the window.
            swapChainDesc.Height = (UINT)m_d3dRenderTargetSize.cy;
            swapChainDesc.Format = m_rtvFormat;
            swapChainDesc.Stereo = false;
            swapChainDesc.SampleDesc.Count = 1; // Don't use multi-sampling (we're not allowed to with DXGI_SWAP_EFFECT_FLIP_*).
            swapChainDesc.SampleDesc.Quality = 0;
            swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            swapChainDesc.BufferCount = NumFramebuffers(); // Use triple-buffering to minimize latency.
            swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // a Direct3D 12 app must use DXGI_SWAP_EFFECT_FLIP_*.
            swapChainDesc.Flags = 0;
            swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
            swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

            winrt::com_ptr<::IDXGISwapChain1> pDXGISwapChain1;
            winrt::check_hresult(
                m_pDXGIFactory4->CreateSwapChainForComposition(
                    m_pD3D12CommandQueue.get(),
                    &swapChainDesc,
                    nullptr,
                    pDXGISwapChain1.put()
                )
            );
            m_pDXGISwapChain3 = pDXGISwapChain1.as<::IDXGISwapChain3>();
        }
        else
        {
            HRESULT hr{ m_pDXGISwapChain3->ResizeBuffers(
                0, // 0 preserves the existing number of buffers in the swap chain.
                (UINT)m_d3dRenderTargetSize.cx, // Match the size of the window.
                (UINT)m_d3dRenderTargetSize.cy,
                DXGI_FORMAT_UNKNOWN, // DXGI_FORMAT_UNKNOWN preserves the existing format of the back buffer.
                0
            ) };

            if (hr == DXGI_ERROR_DEVICE_REMOVED)
            {
                hr = m_pD3D12Device->GetDeviceRemovedReason();
                return false;
            }
            else if (hr == DXGI_ERROR_DEVICE_RESET)
            {
                return false;
            }
            else
            {
                winrt::check_hresult(hr);
            }
        }

        // When the user sets the dpi higher than 96, the CompositionScaleX and
        // CompositionScaleY properties of the SwapChainPanel are set to a value
        // higher than 1. That doesn't affect the layout size of the SwapChainPanel,
        // and it doesn't affect the size of the swap chain itself, the render target,
        // etc., which all remain in raw pixels. But it means that what you render is
        // scaled up and out of view by the SwapChainPanel unless you compensate.
        // So here we scale the swap chain *down* by the amount the panel is scaling
        // itself *up*. Any Direct 2D content is scaled as expected to DIPs, and is
        // rasterized at full resolution.
        DXGI_MATRIX_3X2_F inverseScale{};
        inverseScale._11 = 96.f / m_dpi.x;
        inverseScale._22 = 96.f / m_dpi.y;
        auto pDXGISwapChain2{ m_pDXGISwapChain3.as<::IDXGISwapChain2>() };
        winrt::check_hresult(
            pDXGISwapChain2->SetMatrixTransform(&inverseScale)
        );

        // Create one render target view of the swap chain back buffer for each frame buffer.
        m_currentBufferIndex = m_pDXGISwapChain3->GetCurrentBackBufferIndex();

        D2D1_BITMAP_PROPERTIES1 bitmapProperties{
            D2D1::BitmapProperties1(
                D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
                D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
                m_dpi.x,
                m_dpi.y
            ) };

        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvDescriptorHandle(m_pD3D12RtvHeap->GetCPUDescriptorHandleForHeapStart());
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvDescriptorHandleForOffset{ rtvDescriptorHandle };

        for (UINT frameBufferIndex{ 0 }; frameBufferIndex < NumFramebuffers(); ++frameBufferIndex)
        {
            winrt::check_hresult(
                m_pDXGISwapChain3->GetBuffer(frameBufferIndex, __uuidof(m_pD3D12RenderTargets[frameBufferIndex]), m_pD3D12RenderTargets[frameBufferIndex].put_void())
            );

            m_pD3D12Device->CreateRenderTargetView(
                m_pD3D12RenderTargets[frameBufferIndex].get(),
                nullptr, // Default render target view description.
                rtvDescriptorHandleForOffset
            );
            rtvDescriptorHandleForOffset.Offset(m_rtvDescriptorSize);

            // Create a wrapped 11On12 resource of this back buffer. Since we're rendering
            // all Direct3D 12 content first, and then all Direct2D content, we specify
            // the In resource state as RENDER_TARGET (because Direct3D 12 will have last
            // used it in that state) and the Out resource state as PRESENT. When
            // ReleaseWrappedResources is called on the 11On12 device, the resource
            // transitions to the PRESENT state.
            D3D11_RESOURCE_FLAGS d3d11ResourceFlags{ D3D11_BIND_RENDER_TARGET };
            winrt::check_hresult(m_pD3D11On12Device->CreateWrappedResource(
                m_pD3D12RenderTargets[frameBufferIndex].get(),
                &d3d11ResourceFlags,
                D3D12_RESOURCE_STATE_RENDER_TARGET,
                D3D12_RESOURCE_STATE_PRESENT,
                __uuidof(m_pD3D11WrappedRenderTargets[frameBufferIndex]),
                m_pD3D11WrappedRenderTargets[frameBufferIndex].put_void()
            ));

            // Create a render target for Direct2D to draw directly to this back buffer.
            auto pDXGISurface{ m_pD3D11WrappedRenderTargets[frameBufferIndex].as<::IDXGISurface>() };
            winrt::check_hresult(
                m_pD2D1DeviceContext1->CreateBitmapFromDxgiSurface(
                    pDXGISurface.get(),
                    &bitmapProperties,
                    m_pD2D1TargetBitmap1s[frameBufferIndex].put()
                )
            );

            m_d3d12RenderTargetViews[frameBufferIndex] = CD3DX12_CPU_DESCRIPTOR_HANDLE(rtvDescriptorHandle, frameBufferIndex, m_rtvDescriptorSize);
        }

        // Create a depth stencil and view.
        {
            D3D12_HEAP_PROPERTIES depthHeapProperties{ CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT) };

            D3D12_RESOURCE_DESC depthResourceDesc{ CD3DX12_RESOURCE_DESC::Tex2D(
                m_dsvFormat,
                (UINT)m_d3dRenderTargetSize.cx,
                (UINT)m_d3dRenderTargetSize.cy,
                1,
                1) };
            depthResourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

            CD3DX12_CLEAR_VALUE depthOptimizedClearValue{ m_dsvFormat, 1.f, 0 };

            m_pD3D12DepthStencil = nullptr; // Clear before calling `put_void` again.

            winrt::check_hresult(m_pD3D12Device->CreateCommittedResource(
                &depthHeapProperties,
                D3D12_HEAP_FLAG_NONE,
                &depthResourceDesc,
                D3D12_RESOURCE_STATE_DEPTH_WRITE,
                &depthOptimizedClearValue,
                __uuidof(m_pD3D12DepthStencil),
                m_pD3D12DepthStencil.put_void()
            ));

            D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
            dsvDesc.Format = m_dsvFormat;
            dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
            dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

            m_pD3D12Device->CreateDepthStencilView(m_pD3D12DepthStencil.get(), &dsvDesc, m_d3d12DepthStencilView);
        }

        // Set the 3D rendering viewport and scissor rect to target the entire window.
        m_d3d12Viewport = {
            0.f,
            0.f,
            (float)m_d3dRenderTargetSize.cx,
            (float)m_d3dRenderTargetSize.cy,
            0.f,
            1.f
        };

        m_d3d12ScissorRect = {
            0L,
            0L,
            m_d3dRenderTargetSize.cx,
            m_d3dRenderTargetSize.cy,
        };

        return true;
    }

    void DeviceResources::ReleaseSwapChain()
    {
        m_pD3D12DepthStencil = nullptr;
        for (UINT frameBufferIndex{ 0 }; frameBufferIndex < NumFramebuffers(); ++frameBufferIndex)
        {
            m_pD2D1TargetBitmap1s[frameBufferIndex] = nullptr;
            m_pD3D11WrappedRenderTargets[frameBufferIndex] = nullptr;
            m_pD3D12RenderTargets[frameBufferIndex] = nullptr;
        }
        m_pDXGISwapChain3 = nullptr;
    }

    // Performs setup work that doesn't depend on the Direct3D device.
    void DeviceResources::DeviceIndependentSetup()
    {
        // Initialize Direct2D resources.
        D2D1_FACTORY_OPTIONS d2d1FactoryOptions{ D2D1_DEBUG_LEVEL_NONE };

#if defined (_DEBUG)
        // If the project is in a debug build, enable Direct2D debugging via SDK Layers.
        d2d1FactoryOptions.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#endif

        // Initialize the Direct2D Factory.
        winrt::check_hresult(
            ::D2D1CreateFactory(
                D2D1_FACTORY_TYPE_SINGLE_THREADED,
                __uuidof(m_pD2D1Factory3),
                &d2d1FactoryOptions,
                m_pD2D1Factory3.put_void()
            )
        );

        // Initialize the DirectWrite Factory.
        winrt::check_hresult(
            ::DWriteCreateFactory(
                DWRITE_FACTORY_TYPE_SHARED,
                __uuidof(m_pDWriteFactory2),
                reinterpret_cast<::IUnknown**>(m_pDWriteFactory2.put())
            )
        );

        // Initialize the Windows Imaging Component (WIC) factory.
        winrt::check_hresult(
            ::CoCreateInstance(
                CLSID_WICImagingFactory2,
                nullptr,
                CLSCTX_INPROC_SERVER,
                __uuidof(m_pWICImagingFactory2),
                m_pWICImagingFactory2.put_void()
            )
        );
    }

    // Gets and sets dpi; stores output size.
    void DeviceResources::DpiAndOutputSize(DirectX::XMFLOAT2 const& outputSize)
    {
        m_dpi.x = m_dpi.y = static_cast<float>(::GetDpiForWindow(m_hWnd));

        m_pD2D1DeviceContext1->SetDpi(m_dpi.x, m_dpi.y);

        if (outputSize.x != -1 && outputSize.y != -1)
        {
            // The Window gave us its Bounds in device-independent pixels (DIPs).
            OutputSize(outputSize, true);
        }
    }

    // Prepare to render the next frame.
    void DeviceResources::MoveToNextFrame()
    {
        // Put a signal on the queue for the old current frame's fence value.
        UINT64 const& fenceValueForOldCurrentBuffer{ m_fenceValues[m_currentBufferIndex] };
        winrt::check_hresult(m_pD3D12CommandQueue->Signal(m_pD3D12Fence.get(), fenceValueForOldCurrentBuffer));

        // From here on, "current" means "new current".
        m_currentBufferIndex = m_pDXGISwapChain3->GetCurrentBackBufferIndex();
        UINT64& fenceValueForNewCurrentBuffer{ m_fenceValues[m_currentBufferIndex] };

        // If the fence values of all the frames are still queued, then wait for the
        // oldest frame to become free (the value of fenceValueForNewCurrentBuffer
        // represents that oldest value, since we haven't updated that value yet).
        if (m_pD3D12Fence->GetCompletedValue() < fenceValueForNewCurrentBuffer)
        {
            // Tell the fence what event to signal when that oldest value is reached.
            winrt::check_hresult(m_pD3D12Fence->SetEventOnCompletion(fenceValueForNewCurrentBuffer, m_fenceEventHandle.get()));
            // Wait for the event.
            WaitForSingleObjectEx(m_fenceEventHandle.get(), INFINITE, FALSE);
        }

        // Set the fence value for the "new current" frame (which we'll signal the next time through this function).
        fenceValueForNewCurrentBuffer = fenceValueForOldCurrentBuffer + 1;
    }

    // Takes and stores an output size in either DIPs or raw pixels.
    void DeviceResources::OutputSize(DirectX::XMFLOAT2 const& outputSize, bool isInDIPs)
    {
        if (isInDIPs)
        {
            m_outputSizeInDIPs = outputSize;
            m_outputSizeInRawPixels = {
                DX::ConvertDIPsToPixels(outputSize.x, m_dpi.x),
                DX::ConvertDIPsToPixels(outputSize.y, m_dpi.y) };
        }
        else
        {
            m_outputSizeInRawPixels = outputSize;
            m_outputSizeInDIPs = {
                DX::ConvertPixelsToDIPs((LONG)outputSize.x, m_dpi.x),
                DX::ConvertPixelsToDIPs((LONG)outputSize.y, m_dpi.y) };
        }

        // Prevent zero-sized DirectX content from being created.
        m_outputSizeInRawPixels.x = std::max(1.f, m_outputSizeInRawPixels.x);
        m_outputSizeInRawPixels.y = std::max(1.f, m_outputSizeInRawPixels.y);
    }

    // Present the contents of the swap chain to the screen.
    // Returns `true` if successful; returns `false` if device lost.
    bool DeviceResources::Present()
    {
        DXGI_PRESENT_PARAMETERS parameters{};
        parameters.DirtyRectsCount = 0;
        parameters.pDirtyRects = nullptr;
        parameters.pScrollRect = nullptr;
        parameters.pScrollOffset = nullptr;

        // The first argument instructs DXGI to block until vertical sync, putting the application
        // to sleep until the next vertical sync. This ensures that we don't waste any cycles rendering
        // frames that will never be displayed to the screen.
        HRESULT hr{ m_pDXGISwapChain3->Present1(1, 0, &parameters) };

        // If the device was removed either by a disconnection or a driver upgrade, we 
        // must recreate all device resources.
        bool deviceLost{ false };
        if (hr == DXGI_ERROR_DEVICE_REMOVED)
        {
            deviceLost = true;
            hr = m_pD3D12Device->GetDeviceRemovedReason();
        }
        else if (hr == DXGI_ERROR_DEVICE_RESET)
        {
            deviceLost = true;
        }
        else
        {
            winrt::check_hresult(hr);
        }

        MoveToNextFrame();
        return !deviceLost;
    }

    // Returns `true` if successful; returns `false` if device lost.
    bool DeviceResources::ReleaseWrappedRenderTargetAndPresent(::ID3D11Resource* pWrappedRenderTargets)
    {
        // Release our wrapped render target resource. The act of releasing causes
        // the back buffer resource to transition to PRESENT, which is the state we
        // specified as the Out resource state when we created the wrapped resource.
        m_pD3D11On12Device->ReleaseWrappedResources(&pWrappedRenderTargets, 1);

        // Flush to submit the Direct3D 11 command list to the shared command queue.
        m_pD3D11DeviceContext->Flush();

        return Present();
    }

    // The XAML SwapChainPanel is the UI element through which the swap chain's
    // content is presented to the screen. Here, we connect the two together.
    winrt::fire_and_forget DeviceResources::SetSwapChainOnSwapChainPanelAsync()
    {
        co_await winrt::resume_foreground(m_window.DispatcherQueue());

        // Get the COM interface for the SwapChainPanel.
        auto panelNative{ m_swapChainPanel.as<ISwapChainPanelNative>() };
        winrt::check_hresult(
            panelNative->SetSwapChain(m_pDXGISwapChain3.get())
        );
    }

    // Takes and stores the window object, its HWND, and the swap chain panel.
    void DeviceResources::SetWindowAndSwapChainPanel(winrt::Window const& window, HWND hWnd, winrt::SwapChainPanel const& swapChainPanel)
    {
        m_window = window;
        m_hWnd = hWnd;
        m_swapChainPanel = swapChainPanel;
        OutputSize({ !m_window ? -1.f : m_window.Bounds().Width, m_window == nullptr ? -1.f : m_window.Bounds().Height }, true);
    }

    // Trims the graphics memory.
    void DeviceResources::Trim()
    {
        winrt::com_ptr<::IDXGIDevice3> dxgiDevice{ m_pDXGIFactory4.try_as<::IDXGIDevice3>() };
        if (dxgiDevice)
        {
            dxgiDevice->Trim();
        }
    }

    // Wait for the GPU to drain its work queue.
    void DeviceResources::WaitForGpu() const
    {
        // This function is called in two situations. The first, and simplest, situation is when uploading geometry to
        // the GPU (Sample3DSceneRenderer::WindowIndependentSetup). In that case, we use the current buffer's frame value
        // as a signal value, and then wait for the GPU to reach that signal. That way we know that the GPU has done all
        // the work we've asked it to do up to that point in time.
        // The other situation is when the window size changes (DeviceResources::WindowDependentSetup).
        // In that case, there's cleanup work to do after calling WaitForGpu(). See comments in DeviceResources::WindowDependentSetup.
        UINT64 const& fenceValueForCurrentBuffer{ m_fenceValues[m_currentBufferIndex] };

        // Put a signal on the queue for the current buffer's fence value.
        winrt::check_hresult(m_pD3D12CommandQueue->Signal(m_pD3D12Fence.get(), fenceValueForCurrentBuffer));
        // Tell the fence what event to signal when that value is reached.
        winrt::check_hresult(m_pD3D12Fence->SetEventOnCompletion(fenceValueForCurrentBuffer, m_fenceEventHandle.get()));
        // Wait for the event.
        ::WaitForSingleObjectEx(m_fenceEventHandle.get(), INFINITE, FALSE);
    }

    // Release window-dependent resources.
    void DeviceResources::WindowDependentReset()
    {
        ReleaseSwapChain();

        m_pD2D1DeviceContext1->SetTarget(nullptr);
    }

    // Create (or recreate) view-related state (called for changes in window size, dpi, device orientation, etc.).
    // That is, setup work that depends on the D3D device AND on the window.
    // Returns `true` if successful; returns `false` if device lost.
    bool DeviceResources::WindowDependentSetup()
    {
        // Clear the previous window-size-dependent content.
        m_pD2D1DeviceContext1->SetTarget(nullptr);
        for (UINT frameBufferIndex = 0; frameBufferIndex < NumFramebuffers(); ++frameBufferIndex)
        {
            m_pD2D1TargetBitmap1s[frameBufferIndex] = nullptr;
            m_pD3D11WrappedRenderTargets[frameBufferIndex] = nullptr;
            m_pD3D12RenderTargets[frameBufferIndex] = nullptr;
        }
        m_pD3D11DeviceContext->Flush();

        // We wait for the GPU to complete all the work we've asked it to do. That's done by using the
        // current buffer's frame value as a signal value, and then waiting for the GPU to reach that signal.
        // After that, we increment that fence value and set all the fence values in the array to that.
        // This gives us a clean slate of fence values that haven't yet been signaled.
        WaitForGpu();
        UINT64& fenceValueForCurrentBuffer{ m_fenceValues[m_currentBufferIndex] };
        ++fenceValueForCurrentBuffer;

        // Update the tracked fence values.
        for (UINT frameBufferIndex = 0; frameBufferIndex < NumFramebuffers(); ++frameBufferIndex)
        {
            m_fenceValues[frameBufferIndex] = fenceValueForCurrentBuffer;
        }

        if (m_outputSizeInRawPixels.x == 0 || m_outputSizeInRawPixels.y == 0) return true;

        m_d3dRenderTargetSize.cx = (UINT)m_outputSizeInRawPixels.x;
        m_d3dRenderTargetSize.cy = (UINT)m_outputSizeInRawPixels.y;

        // The window size has changed, so we need to re-create the swap chain.
        return CreateSwapChain();
    }

    // Release window-independent (device-dependent) resources.
    void DeviceResources::WindowIndependentReset()
    {
        Trim();
        ::CloseHandle(m_fenceEventHandle.get());
        m_pD3D12Fence = nullptr;
        for (auto& pD3D12CommandAllocator : m_pD3D12CommandAllocators)
        {
            pD3D12CommandAllocator = nullptr;
        }
        m_pD3D12DsvHeap = nullptr;
        m_pD3D12RtvHeap = nullptr;
        m_pD2D1DeviceContext1 = nullptr;
        m_pD2D1Device1 = nullptr;
        m_pD3D11DeviceContext = nullptr;
        m_pD3D12CommandQueue = nullptr;
        m_pD3D12Device = nullptr;
        m_pDXGIFactory4 = nullptr;
#if defined(_DEBUG)
        m_pD3DDebugger = nullptr;
#endif
    }

    // Does setup work that depends on the D3D device, but doesn't depend on the window.
    void DeviceResources::WindowIndependentSetup()
    {
#if defined(_DEBUG)
        // If the project is in a debug build, enable debugging via SDK Layers.
        if (SUCCEEDED(::D3D12GetDebugInterface(__uuidof(m_pD3DDebugger), m_pD3DDebugger.put_void())))
        {
            m_pD3DDebugger->EnableDebugLayer();
        }
#endif

        winrt::check_hresult(::CreateDXGIFactory1(__uuidof(m_pDXGIFactory4), m_pDXGIFactory4.put_void()));

        winrt::com_ptr<::IDXGIAdapter> pIDXGIAdapter;
        UINT adapterIndex{};
        HRESULT hr{ S_OK };

        do
        {
            winrt::check_hresult(m_pDXGIFactory4->EnumAdapters(adapterIndex, pIDXGIAdapter.put()));
            ++adapterIndex;

            m_pD3D12Device = nullptr;
            hr = ::D3D12CreateDevice(pIDXGIAdapter.get(), D3D_FEATURE_LEVEL_11_0, __uuidof(m_pD3D12Device), m_pD3D12Device.put_void());
            if (hr == DXGI_ERROR_UNSUPPORTED) continue;
            winrt::check_hresult(hr);
        } while (hr != S_OK);

        // Describe and create the command queue.
        D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
        commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

        winrt::check_hresult(
            m_pD3D12Device->CreateCommandQueue(
                &commandQueueDesc,
                _uuidof(m_pD3D12CommandQueue),
                m_pD3D12CommandQueue.put_void())
        );

        // Create an 11On12 device using the Direct3D 12 device and the command queue.

        // D3D11_CREATE_DEVICE_BGRA_SUPPORT supports surfaces with a different color channel ordering
        // than the API default. It's required for Direct2D interoperability with Direct3D resources.
        uint32_t createDeviceFlags{ D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_SINGLETHREADED };

#if defined (_DEBUG)
        if (DX::SdkLayersAvailable())
        {
            // If the project is in a debug build, then enable debugging via SDK Layers with this flag.
            createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
        }
#endif

        // Array with hardware feature levels in order of preference.
        D3D_FEATURE_LEVEL featureLevelsRequested[]{
            D3D_FEATURE_LEVEL_12_1,
            D3D_FEATURE_LEVEL_12_0,
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0
        };
        D3D12_FEATURE_DATA_FEATURE_LEVELS d3d12FeatureLevels{ sizeof(featureLevelsRequested) / sizeof(D3D_FEATURE_LEVEL), featureLevelsRequested, D3D_FEATURE_LEVEL_9_1 };
        winrt::check_hresult(m_pD3D12Device->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &d3d12FeatureLevels, sizeof(d3d12FeatureLevels)));

        winrt::com_ptr<::ID3D11Device> pD3D11Device;

        ::IUnknown* pCommandQueues{ m_pD3D12CommandQueue.get() };
        winrt::check_hresult(::D3D11On12CreateDevice(
            m_pD3D12Device.get(),
            createDeviceFlags,
            nullptr,
            0,
            &pCommandQueues,
            1,
            0,
            pD3D11Device.put(),
            m_pD3D11DeviceContext.put(),
            nullptr
        ));
        // Query the 11On12 device from the 11 device.
        m_pD3D11On12Device = pD3D11Device.as<::ID3D11On12Device>();

        // Create the Direct2D device object, and a corresponding context.
        winrt::com_ptr<::IDXGIDevice> pDXGIDevice{ m_pD3D11On12Device.as<::IDXGIDevice>() };

        winrt::check_hresult(
            m_pD2D1Factory3->CreateDevice(pDXGIDevice.get(), m_pD2D1Device1.put())
        );

        winrt::check_hresult(
            m_pD2D1Device1->CreateDeviceContext(
                D2D1_DEVICE_CONTEXT_OPTIONS_NONE,
                m_pD2D1DeviceContext1.put()
            )
        );

        // Create a descriptor heap for the render target view.
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
        rtvHeapDesc.NumDescriptors = NumFramebuffers();
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

        winrt::check_hresult(
            m_pD3D12Device->CreateDescriptorHeap(
                &rtvHeapDesc,
                _uuidof(m_pD3D12RtvHeap),
                m_pD3D12RtvHeap.put_void())
        );

        m_rtvDescriptorSize = m_pD3D12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        // Create a descriptor heap for the depth stencil view.
        D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc{};
        dsvHeapDesc.NumDescriptors = 1;
        dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

        winrt::check_hresult(
            m_pD3D12Device->CreateDescriptorHeap(
                &dsvHeapDesc,
                _uuidof(m_pD3D12DsvHeap),
                m_pD3D12DsvHeap.put_void())
        );
        m_d3d12DepthStencilView = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_pD3D12DsvHeap->GetCPUDescriptorHandleForHeapStart());

        // Create a command allocator for each frame buffer.
        for (auto& pD3D12CommandAllocator : m_pD3D12CommandAllocators)
        {
            winrt::check_hresult(
                m_pD3D12Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(pD3D12CommandAllocator), pD3D12CommandAllocator.put_void())
            );
        }

        // Create synchronization objects.
        winrt::check_hresult(m_pD3D12Device->CreateFence(m_fenceValues[m_currentBufferIndex], D3D12_FENCE_FLAG_NONE, _uuidof(m_pD3D12Fence), m_pD3D12Fence.put_void()));
        ++m_fenceValues[m_currentBufferIndex];

        m_fenceEventHandle.attach(::CreateEvent(nullptr, false, false, nullptr));
        winrt::check_bool(bool{ m_fenceEventHandle });
    }
}
