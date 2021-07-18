//
// Created by jet on 4/9/21.
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
        friend Window;

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
