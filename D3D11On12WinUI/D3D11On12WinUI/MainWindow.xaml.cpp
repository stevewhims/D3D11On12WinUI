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
#include "MainWindow.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

using namespace winrt;
using namespace Microsoft::UI::Xaml;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace winrt::D3D11On12WinUI::implementation
{
    MainWindow::MainWindow()
    {
        InitializeComponent();
        swapChainPanel().Loaded({ this, &MainWindow::OnSwapChainPanelLoaded });

        winrt::D3D11On12WinUI::MainWindow projectedThis{ *this };
        auto windowNative{ projectedThis.as<::IWindowNative>() };
        windowNative->get_WindowHandle(&m_hWnd);
        ::SetWindowTextW(m_hWnd, L"D3D11On12WinUI");

        m_sample3DSceneRenderer.SetWindowAndSwapChainPanel(*this, m_hWnd, swapChainPanel());
        m_sample3DSceneRenderer.StartRenderLoop();

        SizeChanged({ this, &MainWindow::OnSizeChanged });
    }

    void MainWindow::OnAnimateButtonClick(winrt::IInspectable const& /* sender */, winrt::RoutedEventArgs const& /* args */)
    {
        animateButton().IsEnabled(false);
        animateButton().Content(box_value(L"Cube is animated"));
        m_sample3DSceneRenderer.Animate();
    }

    void MainWindow::OnDpiChanged()
    {
        m_sample3DSceneRenderer.OnDpiChanged(Bounds());
    }

    void MainWindow::OnSizeChanged(winrt::IInspectable const& /*sender*/, winrt::WindowSizeChangedEventArgs const& /* args */)
    {
        m_sample3DSceneRenderer.OnSizeChanged(Bounds());
    }

    void MainWindow::OnSwapChainPanelLoaded(winrt::IInspectable const& /* sender */, winrt::RoutedEventArgs const& /* args */)
    {
        auto xamlRoot{ swapChainPanel().XamlRoot() };
        xamlRoot.Changed({ this, &MainWindow::OnXamlRootChanged });
    }

    void MainWindow::OnXamlRootChanged(winrt::XamlRoot const& /* sender */, winrt::XamlRootChangedEventArgs const& /* args */)
    {
        OnDpiChanged();
    }
}
