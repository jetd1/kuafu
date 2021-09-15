//
// Modified by Jet <i@jetd.me> based on Rayex source code.
// Original copyright notice:
//
// Copyright (c) 2021 Christian Hilpert
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the author be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose
// and to alter it and redistribute it freely, subject to the following
// restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//
#pragma once

#include "stdafx.hpp"

namespace kuafu {
class Window;

/// Used to keep track of the application's timing.
/// @ingroup BASE
/// @todo Average FPS are pointless. Implement minimum FPS and frametimes instead.
class Time {
public:
    friend class Kuafu;

    friend class Context;

    /// Prints the average fps as well as the time since recording.
    static void printBenchmarkResults();

    static void startBenchmark(float length);

    /// @return Returns the time passed since application start in seconds.
    static auto getTime() -> float;

    /// @return Returns the time passed between the current and the last frame.
    static auto getDeltaTime() -> float;

    static auto getFramesPerSecond() -> uint32_t;

private:
    /// Updates the timing.
    ///
    /// Prints the current fps every three seconds.
    /// @note This function will be called every tick.
    static void update();
};
}
