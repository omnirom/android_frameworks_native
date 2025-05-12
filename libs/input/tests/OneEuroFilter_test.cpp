/**
 * Copyright 2024 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <input/OneEuroFilter.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <numeric>
#include <vector>

#include <gtest/gtest.h>

#include <input/Input.h>

namespace android {
namespace {

using namespace std::literals::chrono_literals;
using std::chrono::duration;

struct Sample {
    duration<double> timestamp{};
    double value{};

    friend bool operator<(const Sample& lhs, const Sample& rhs) { return lhs.value < rhs.value; }
};

/**
 * Generates a sinusoidal signal with the passed frequency and amplitude.
 */
std::vector<Sample> generateSinusoidalSignal(duration<double> signalDuration,
                                             double samplingFrequency, double signalFrequency,
                                             double amplitude) {
    std::vector<Sample> signal;
    const duration<double> samplingPeriod{1.0 / samplingFrequency};
    for (duration<double> timestamp{0.0}; timestamp < signalDuration; timestamp += samplingPeriod) {
        signal.push_back(
                Sample{timestamp,
                       amplitude * std::sin(2.0 * M_PI * signalFrequency * timestamp.count())});
    }
    return signal;
}

double meanAbsoluteError(const std::vector<Sample>& filteredSignal,
                         const std::vector<Sample>& signal) {
    if (filteredSignal.size() != signal.size()) {
        ADD_FAILURE() << "filteredSignal and signal do not have equal number of samples";
        return std::numeric_limits<double>::max();
    }
    std::vector<double> absoluteError;
    for (size_t sampleIndex = 0; sampleIndex < signal.size(); ++sampleIndex) {
        absoluteError.push_back(
                std::abs(filteredSignal[sampleIndex].value - signal[sampleIndex].value));
    }
    if (absoluteError.empty()) {
        ADD_FAILURE() << "Zero division. absoluteError is empty";
        return std::numeric_limits<double>::max();
    }
    return std::accumulate(absoluteError.begin(), absoluteError.end(), 0.0) / absoluteError.size();
}

double maxAbsoluteAmplitude(const std::vector<Sample>& signal) {
    if (signal.empty()) {
        ADD_FAILURE() << "Max absolute value amplitude does not exist. Signal is empty";
        return std::numeric_limits<double>::max();
    }
    std::vector<Sample> absoluteSignal;
    for (const Sample& sample : signal) {
        absoluteSignal.push_back(Sample{sample.timestamp, std::abs(sample.value)});
    }
    return std::max_element(absoluteSignal.begin(), absoluteSignal.end())->value;
}

} // namespace

class OneEuroFilterTest : public ::testing::Test {
protected:
    // The constructor's parameters are the ones that Chromium's using. The tuning was based on a 60
    // Hz sampling frequency. Refer to their one_euro_filter.h header for additional information
    // about these parameters.
    OneEuroFilterTest() : mFilter{/*minCutoffFreq=*/4.7, /*beta=*/0.01} {}

    std::vector<Sample> filterSignal(const std::vector<Sample>& signal) {
        std::vector<Sample> filteredSignal;
        for (const Sample& sample : signal) {
            filteredSignal.push_back(
                    Sample{sample.timestamp,
                           mFilter.filter(std::chrono::duration_cast<std::chrono::nanoseconds>(
                                                  sample.timestamp),
                                          sample.value)});
        }
        return filteredSignal;
    }

    OneEuroFilter mFilter;
};

TEST_F(OneEuroFilterTest, PassLowFrequencySignal) {
    const std::vector<Sample> signal =
            generateSinusoidalSignal(1s, /*samplingFrequency=*/60, /*signalFrequency=*/1,
                                     /*amplitude=*/1);

    const std::vector<Sample> filteredSignal = filterSignal(signal);

    // The reason behind using the mean absolute error as a metric is that, ideally, a low frequency
    // filtered signal is expected to be almost identical to the raw one. Therefore, the error
    // between them should be minimal. The constant is heuristically chosen.
    EXPECT_LT(meanAbsoluteError(filteredSignal, signal), 0.25);
}

TEST_F(OneEuroFilterTest, RejectHighFrequencySignal) {
    const std::vector<Sample> signal =
            generateSinusoidalSignal(1s, /*samplingFrequency=*/60, /*signalFrequency=*/22.5,
                                     /*amplitude=*/1);

    const std::vector<Sample> filteredSignal = filterSignal(signal);

    // The filtered signal should consist of values that are much closer to zero. The comparison
    // constant is heuristically chosen.
    EXPECT_LT(maxAbsoluteAmplitude(filteredSignal), 0.25);
}

} // namespace android
