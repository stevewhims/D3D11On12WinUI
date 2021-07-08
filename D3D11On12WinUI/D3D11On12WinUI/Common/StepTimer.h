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
    // Helper class for animation timing.
    class StepTimer final
    {
        LARGE_INTEGER m_initialTicks{};
        LARGE_INTEGER m_performanceCountStart{};
        LARGE_INTEGER m_performanceFrequency{};

    public:
        StepTimer()
        {
            ::QueryPerformanceCounter(&m_initialTicks);
            ::QueryPerformanceFrequency(&m_performanceFrequency);
        }

        float TotalSeconds()
        {
            LARGE_INTEGER previousPerformanceCountStart{ m_performanceCountStart };
            ::QueryPerformanceCounter(&m_performanceCountStart);

            LARGE_INTEGER elapsedMicroseconds;
            elapsedMicroseconds.QuadPart = m_performanceCountStart.QuadPart - m_initialTicks.QuadPart;
            elapsedMicroseconds.QuadPart *= (long long)1e6;
            elapsedMicroseconds.QuadPart /= m_performanceFrequency.QuadPart;
            return elapsedMicroseconds.QuadPart / 1.e6f;
        }
    };
}
