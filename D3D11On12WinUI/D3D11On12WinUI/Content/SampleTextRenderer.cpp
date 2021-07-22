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
    SampleTextRenderer::SampleTextRenderer(DX::DeviceResources const& deviceResources) :
        m_deviceResources{ deviceResources }
    {
    }

    // Update the text to be displayed, and render a frame to the screen.
    void SampleTextRenderer::UpdateAndRender()
    {
        std::wstring text{ L"Direct3D 11-on-12 and WinUI XAML sample" };
        DirectX::XMFLOAT2 outputSizeInDIPs{ m_deviceResources.OutputSizeInDIPs() };

        m_pDWriteTextLayout = nullptr;
        winrt::check_hresult(
            m_deviceResources.IDWriteFactory2()->CreateTextLayout(
                text.c_str(),
                (uint32_t)text.length(),
                m_pDWriteTextFormat.get(),
                outputSizeInDIPs.x, // Max width of the input text.
                outputSizeInDIPs.y, // Max height of the input text.
                m_pDWriteTextLayout.put()
            )
        );

        ID2D1DeviceContext1* pContext{ m_deviceResources.ID2D1DeviceContext1() };

        pContext->SaveDrawingState(m_pD2D1StateBlock.get());
        pContext->BeginDraw();

        pContext->DrawTextLayout(
            D2D1::Point2F(3.f, 0.f),
            m_pDWriteTextLayout.get(),
            m_pD2D1WhiteBrush.get()
        );

        // Ignore D2DERR_RECREATE_TARGET here. This error indicates that the device
        // is lost. It will be handled during the next call to Present.
        HRESULT hr{ pContext->EndDraw() };
        if (hr != D2DERR_RECREATE_TARGET && hr != S_OK)
        {
            if (hr != E_NOINTERFACE) winrt::check_hresult(hr);
        }

        pContext->RestoreDrawingState(m_pD2D1StateBlock.get());
    }

    // Initialize Direct2D resources used for text rendering.
    void SampleTextRenderer::WindowIndependentSetup()
    {
        winrt::check_hresult(
            m_deviceResources.IDWriteFactory2()->CreateTextFormat(
                L"Segoe UI",
                nullptr,
                DWRITE_FONT_WEIGHT_REGULAR,
                DWRITE_FONT_STYLE_NORMAL,
                DWRITE_FONT_STRETCH_NORMAL,
                16.f,
                L"en-US",
                m_pDWriteTextFormat.put()
            )
        );

        winrt::check_hresult(
            m_deviceResources.ID2D1Factory3()->CreateDrawingStateBlock(m_pD2D1StateBlock.put())
        );

        winrt::check_hresult(
            m_deviceResources.ID2D1DeviceContext1()->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), m_pD2D1WhiteBrush.put())
        );
   }

    // Uninitialize Direct2D resources ready for reinitialization.
    void SampleTextRenderer::WindowIndependentReset()
    {
        m_pD2D1WhiteBrush = nullptr;
        m_pD2D1StateBlock = nullptr;
        m_pDWriteTextFormat = nullptr;
        m_pDWriteTextLayout = nullptr;
    }
}
