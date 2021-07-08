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

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers

#include <d2d1_3.h>
#include <d3d11_2.h>
#include <d3d11on12.h>
#include <dwrite_2.h>
#include <dxgi1_4.h>
#include <microsoft.ui.xaml.media.dxinterop.h>
#include <microsoft.ui.xaml.window.h>
#include <pix.h>
#include <wincodec.h>

// Undefine GetCurrentTime macro to prevent
// conflict with Storyboard::GetCurrentTime
#undef GetCurrentTime

#include <winrt/Microsoft.System.h>
#include <winrt/Windows.ApplicationModel.Core.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Graphics.Display.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.System.Threading.h>
#include <winrt/Microsoft.UI.Xaml.h>
#include <winrt/Microsoft.UI.Xaml.Controls.h>
#include <winrt/Microsoft.UI.Xaml.Controls.Primitives.h>
#include <winrt/Microsoft.UI.Xaml.Markup.h>

namespace winrt
{
	using namespace winrt::Microsoft::UI::Xaml;
	using namespace winrt::Microsoft::UI::Xaml::Controls;
	using namespace winrt::Windows::ApplicationModel;
	using namespace winrt::Windows::Foundation;
	using namespace winrt::Windows::Storage;
	using namespace winrt::Windows::Storage::Streams;
	using namespace winrt::Windows::System::Threading;
}

#include "..\Common\d3dx12.h"
#include "..\Common\DirectXHelper.h"
#include "..\Common\StepTimer.h"
#include "..\Common\DeviceResources.h"
#include "..\Content\ShaderStructures.h"
#include "..\Content\Cube.h"
#include "..\Content\SampleTextRenderer.h"
#include "..\Content\Sample3DSceneRenderer.h"
