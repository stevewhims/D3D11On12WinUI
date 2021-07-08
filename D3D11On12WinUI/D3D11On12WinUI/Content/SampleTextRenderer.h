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
    // Renders a simple string to the screen using Direct2D.
    class SampleTextRenderer final
    {
        // data members

        DX::DeviceResources const& m_deviceResources;

        // DirectWrite and Direct2D data members

        winrt::com_ptr<ID2D1DrawingStateBlock> m_pD2D1StateBlock{ nullptr };
        winrt::com_ptr<ID2D1SolidColorBrush> m_pD2D1WhiteBrush{ nullptr };
        winrt::com_ptr<IDWriteTextFormat> m_pDWriteTextFormat{ nullptr };
        winrt::com_ptr<IDWriteTextLayout> m_pDWriteTextLayout{ nullptr };

    public:
        SampleTextRenderer(DX::DeviceResources const& deviceResources);

        // member functions

        void UpdateAndRender();
        void WindowIndependentSetup();
        void WindowIndependentReset();
    };
}
