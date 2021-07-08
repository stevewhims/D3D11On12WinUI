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

#pragma push_macro("GetCurrentTime")
#undef GetCurrentTime
#include "MainWindow.g.h"
#pragma pop_macro("GetCurrentTime")

namespace winrt::D3D11On12WinUI::implementation
{
    struct MainWindow : MainWindowT<MainWindow>
    {
        MainWindow();

        void OnAnimateButtonClick(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args);
        void OnDpiChanged();
        void OnSizeChanged(winrt::IInspectable const& sender, winrt::WindowSizeChangedEventArgs const& args);
        void OnSwapChainPanelLoaded(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args);
        void OnXamlRootChanged(winrt::XamlRoot const& sender, winrt::XamlRootChangedEventArgs const& args);

    private:
        HWND m_hWnd{ 0 };
        Sample3DSceneRenderer m_sample3DSceneRenderer;
    };
}

namespace winrt::D3D11On12WinUI::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
    {
    };
}
