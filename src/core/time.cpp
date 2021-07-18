//
// Created by jet on 4/9/21.
//

#include "core/time.hpp"
#include "stdafx.hpp"

namespace kuafu {
    float deltaTime;
    float prevTime;
    std::vector<uint32_t> allFrames;
    std::vector<float> frameTimes;

    uint32_t fps = 0;
    uint32_t frames = 0;
    float prevTime2 = 0.0F;

    float timeAtBenchmarkStart = 0.0F;
    float benchmarkLength = 0.0F;
    bool startedBenchmark = false;

    float percentile(const std::vector<float> &vec) {
        auto length = vec.size();
        float n = (length - 1) * 0.01 + 1.0;

        if (n == 1.0) {
            return vec[0];
        } else if (n == length) {
            return vec[length - 1];
        } else {
            auto k = static_cast<size_t>( n );
            float d = n - k;
            return vec[k - 1] + d * (vec[k] - vec[k - 1]);
        }
    }

    void Time::printBenchmarkResults() {
        if (allFrames.size() == 0) {
            return;
        }

        uint32_t sum = 0.0F;
        for (int value : allFrames) {
            sum += value;
        }

        float average = static_cast<float>( sum ) / static_cast<float>( allFrames.size());

        std::sort(frameTimes.begin(), frameTimes.end(), std::greater<float>());
        std::sort(allFrames.begin(), allFrames.end(), std::greater<uint32_t>());

//        KF_INFO( "Benchmark Results ( Length: ", benchmarkLength, " s.)" );
//        KF_INFO( "Average FPS          : ", average );
//        KF_INFO( "1% Min FPS           : ", 1.0F / percentile( frameTimes ) );
//        //KF_INFO( "Average frame time   : ", 1.0F / average );
        //KF_INFO( "Highest frame time   : ", frameTimes[0] );
        //KF_INFO( "1% (min frame time)  : ", percentile( frameTimes ) );
    }

    void Time::startBenchmark(float length) {
        allFrames.clear();
        allFrames.reserve(static_cast<size_t>( length ));

        frameTimes.clear();
        frameTimes.reserve(3600); // max one minute

        timeAtBenchmarkStart = getTime();
        benchmarkLength = length;
        startedBenchmark = true;
    }

    auto Time::getTime() -> float {
        return static_cast<float>( SDL_GetTicks()) / 1000.0F;
    }

    auto Time::getDeltaTime() -> float {
        return deltaTime;
    }

    auto Time::getFramesPerSecond() -> uint32_t {
        return fps;
    }

    void Time::update() {
        float current_time = static_cast<float>( SDL_GetTicks()) / 1000.0F;

        frames++;

        deltaTime = (current_time - prevTime);
        prevTime = current_time;

        // Print fps every full second.
        if (current_time - prevTime2 >= 1.0F) {
            fps = frames;

            std::cout << "FPS: " << frames;
//            KF_VERBOSE( "FPS: ", frames );
            allFrames.push_back(frames);

            frames = frames;
            frames = 0;
            prevTime2 = current_time;
        }

        if (startedBenchmark) {
            frameTimes.push_back(deltaTime);
        }

        if (getTime() >= timeAtBenchmarkStart + benchmarkLength && startedBenchmark) {
            startedBenchmark = false;
            printBenchmarkResults();
        }
    }
}
