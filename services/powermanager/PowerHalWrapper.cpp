/*
 * Copyright (C) 2020 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *            http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "HalWrapper"
#include <android/hardware/power/Boost.h>
#include <android/hardware/power/IPowerHintSession.h>
#include <android/hardware/power/Mode.h>
#include <powermanager/PowerHalWrapper.h>
#include <utils/Log.h>

#include <cinttypes>

using namespace android::hardware::power;
namespace Aidl = android::hardware::power;

namespace android {

namespace power {

// -------------------------------------------------------------------------------------------------

inline HalResult<void> toHalResult(const binder::Status& result) {
    if (result.isOk()) {
        return HalResult<void>::ok();
    }
    ALOGE("Power HAL request failed: %s", result.toString8().c_str());
    return HalResult<void>::fromStatus(result);
}

template <typename T>
template <typename R>
HalResult<T> HalResult<T>::fromReturn(hardware::Return<R>& ret, T data) {
    return ret.isOk() ? HalResult<T>::ok(data) : HalResult<T>::failed(ret.description());
}

template <typename T>
template <typename R>
HalResult<T> HalResult<T>::fromReturn(hardware::Return<R>& ret, V1_0::Status status, T data) {
    return ret.isOk() ? HalResult<T>::fromStatus(status, data)
                      : HalResult<T>::failed(ret.description());
}

// -------------------------------------------------------------------------------------------------

HalResult<void> HalResult<void>::fromStatus(status_t status) {
    if (status == android::OK) {
        return HalResult<void>::ok();
    }
    return HalResult<void>::failed(statusToString(status));
}

HalResult<void> HalResult<void>::fromStatus(binder::Status status) {
    if (status.exceptionCode() == binder::Status::EX_UNSUPPORTED_OPERATION) {
        return HalResult<void>::unsupported();
    }
    if (status.isOk()) {
        return HalResult<void>::ok();
    }
    return HalResult<void>::failed(std::string(status.toString8().c_str()));
}

template <typename R>
HalResult<void> HalResult<void>::fromReturn(hardware::Return<R>& ret) {
    return ret.isOk() ? HalResult<void>::ok() : HalResult<void>::failed(ret.description());
}
// -------------------------------------------------------------------------------------------------

HalResult<void> EmptyHalWrapper::setBoost(Boost boost, int32_t durationMs) {
    ALOGV("Skipped setBoost %s with duration %dms because Power HAL not available",
          toString(boost).c_str(), durationMs);
    return HalResult<void>::unsupported();
}

HalResult<void> EmptyHalWrapper::setMode(Mode mode, bool enabled) {
    ALOGV("Skipped setMode %s to %s because Power HAL not available", toString(mode).c_str(),
          enabled ? "true" : "false");
    return HalResult<void>::unsupported();
}

HalResult<sp<Aidl::IPowerHintSession>> EmptyHalWrapper::createHintSession(
        int32_t, int32_t, const std::vector<int32_t>& threadIds, int64_t) {
    ALOGV("Skipped createHintSession(task num=%zu) because Power HAL not available",
          threadIds.size());
    return HalResult<sp<Aidl::IPowerHintSession>>::unsupported();
}

HalResult<int64_t> EmptyHalWrapper::getHintSessionPreferredRate() {
    ALOGV("Skipped getHintSessionPreferredRate because Power HAL not available");
    return HalResult<int64_t>::unsupported();
}

// -------------------------------------------------------------------------------------------------

HalResult<void> HidlHalWrapperV1_0::setBoost(Boost boost, int32_t durationMs) {
    if (boost == Boost::INTERACTION) {
        return sendPowerHint(V1_3::PowerHint::INTERACTION, durationMs);
    } else {
        ALOGV("Skipped setBoost %s because Power HAL AIDL not available", toString(boost).c_str());
        return HalResult<void>::unsupported();
    }
}

HalResult<void> HidlHalWrapperV1_0::setMode(Mode mode, bool enabled) {
    uint32_t data = enabled ? 1 : 0;
    switch (mode) {
        case Mode::LAUNCH:
            return sendPowerHint(V1_3::PowerHint::LAUNCH, data);
        case Mode::LOW_POWER:
            return sendPowerHint(V1_3::PowerHint::LOW_POWER, data);
        case Mode::SUSTAINED_PERFORMANCE:
            return sendPowerHint(V1_3::PowerHint::SUSTAINED_PERFORMANCE, data);
        case Mode::VR:
            return sendPowerHint(V1_3::PowerHint::VR_MODE, data);
        case Mode::INTERACTIVE:
            return setInteractive(enabled);
        case Mode::DOUBLE_TAP_TO_WAKE:
            return setFeature(V1_0::Feature::POWER_FEATURE_DOUBLE_TAP_TO_WAKE, enabled);
        default:
            ALOGV("Skipped setMode %s because Power HAL AIDL not available",
                  toString(mode).c_str());
            return HalResult<void>::unsupported();
    }
}

HalResult<void> HidlHalWrapperV1_0::sendPowerHint(V1_3::PowerHint hintId, uint32_t data) {
    auto ret = mHandleV1_0->powerHint(static_cast<V1_0::PowerHint>(hintId), data);
    return HalResult<void>::fromReturn(ret);
}

HalResult<void> HidlHalWrapperV1_0::setInteractive(bool enabled) {
    auto ret = mHandleV1_0->setInteractive(enabled);
    return HalResult<void>::fromReturn(ret);
}

HalResult<void> HidlHalWrapperV1_0::setFeature(V1_0::Feature feature, bool enabled) {
    auto ret = mHandleV1_0->setFeature(feature, enabled);
    return HalResult<void>::fromReturn(ret);
}

HalResult<sp<hardware::power::IPowerHintSession>> HidlHalWrapperV1_0::createHintSession(
        int32_t, int32_t, const std::vector<int32_t>& threadIds, int64_t) {
    ALOGV("Skipped createHintSession(task num=%zu) because Power HAL not available",
          threadIds.size());
    return HalResult<sp<Aidl::IPowerHintSession>>::unsupported();
}

HalResult<int64_t> HidlHalWrapperV1_0::getHintSessionPreferredRate() {
    ALOGV("Skipped getHintSessionPreferredRate because Power HAL not available");
    return HalResult<int64_t>::unsupported();
}

// -------------------------------------------------------------------------------------------------

HalResult<void> HidlHalWrapperV1_1::sendPowerHint(V1_3::PowerHint hintId, uint32_t data) {
    auto handle = static_cast<V1_1::IPower*>(mHandleV1_0.get());
    auto ret = handle->powerHintAsync(static_cast<V1_0::PowerHint>(hintId), data);
    return HalResult<void>::fromReturn(ret);
}

// -------------------------------------------------------------------------------------------------

HalResult<void> HidlHalWrapperV1_2::sendPowerHint(V1_3::PowerHint hintId, uint32_t data) {
    auto handle = static_cast<V1_2::IPower*>(mHandleV1_0.get());
    auto ret = handle->powerHintAsync_1_2(static_cast<V1_2::PowerHint>(hintId), data);
    return HalResult<void>::fromReturn(ret);
}

HalResult<void> HidlHalWrapperV1_2::setBoost(Boost boost, int32_t durationMs) {
    switch (boost) {
        case Boost::CAMERA_SHOT:
            return sendPowerHint(V1_3::PowerHint::CAMERA_SHOT, durationMs);
        case Boost::CAMERA_LAUNCH:
            return sendPowerHint(V1_3::PowerHint::CAMERA_LAUNCH, durationMs);
        default:
            return HidlHalWrapperV1_1::setBoost(boost, durationMs);
    }
}

HalResult<void> HidlHalWrapperV1_2::setMode(Mode mode, bool enabled) {
    uint32_t data = enabled ? 1 : 0;
    switch (mode) {
        case Mode::CAMERA_STREAMING_SECURE:
        case Mode::CAMERA_STREAMING_LOW:
        case Mode::CAMERA_STREAMING_MID:
        case Mode::CAMERA_STREAMING_HIGH:
            return sendPowerHint(V1_3::PowerHint::CAMERA_STREAMING, data);
        case Mode::AUDIO_STREAMING_LOW_LATENCY:
            return sendPowerHint(V1_3::PowerHint::AUDIO_LOW_LATENCY, data);
        default:
            return HidlHalWrapperV1_1::setMode(mode, enabled);
    }
}

// -------------------------------------------------------------------------------------------------

HalResult<void> HidlHalWrapperV1_3::setMode(Mode mode, bool enabled) {
    uint32_t data = enabled ? 1 : 0;
    if (mode == Mode::EXPENSIVE_RENDERING) {
        return sendPowerHint(V1_3::PowerHint::EXPENSIVE_RENDERING, data);
    }
    return HidlHalWrapperV1_2::setMode(mode, enabled);
}

HalResult<void> HidlHalWrapperV1_3::sendPowerHint(V1_3::PowerHint hintId, uint32_t data) {
    auto handle = static_cast<V1_3::IPower*>(mHandleV1_0.get());
    auto ret = handle->powerHintAsync_1_3(hintId, data);
    return HalResult<void>::fromReturn(ret);
}

// -------------------------------------------------------------------------------------------------

HalResult<void> AidlHalWrapper::setBoost(Boost boost, int32_t durationMs) {
    std::unique_lock<std::mutex> lock(mBoostMutex);
    size_t idx = static_cast<size_t>(boost);

    // Quick return if boost is not supported by HAL
    if (idx >= mBoostSupportedArray.size() || mBoostSupportedArray[idx] == HalSupport::OFF) {
        ALOGV("Skipped setBoost %s because Power HAL doesn't support it", toString(boost).c_str());
        return HalResult<void>::unsupported();
    }

    if (mBoostSupportedArray[idx] == HalSupport::UNKNOWN) {
        bool isSupported = false;
        auto isSupportedRet = mHandle->isBoostSupported(boost, &isSupported);
        if (!isSupportedRet.isOk()) {
            ALOGE("Skipped setBoost %s because check support failed with: %s",
                  toString(boost).c_str(), isSupportedRet.toString8().c_str());
            // return HalResult::FAILED;
            return HalResult<void>::fromStatus(isSupportedRet);
        }

        mBoostSupportedArray[idx] = isSupported ? HalSupport::ON : HalSupport::OFF;
        if (!isSupported) {
            ALOGV("Skipped setBoost %s because Power HAL doesn't support it",
                  toString(boost).c_str());
            return HalResult<void>::unsupported();
        }
    }
    lock.unlock();

    return toHalResult(mHandle->setBoost(boost, durationMs));
}

HalResult<void> AidlHalWrapper::setMode(Mode mode, bool enabled) {
    std::unique_lock<std::mutex> lock(mModeMutex);
    size_t idx = static_cast<size_t>(mode);

    if (mHandleSeh != nullptr && mode == Mode::INTERACTIVE) {
        mHandleSeh->setInteractiveAsync(enabled, false);
    }

    // Quick return if mode is not supported by HAL
    if (idx >= mModeSupportedArray.size() || mModeSupportedArray[idx] == HalSupport::OFF) {
        ALOGV("Skipped setMode %s because Power HAL doesn't support it", toString(mode).c_str());
        return HalResult<void>::unsupported();
    }

    if (mModeSupportedArray[idx] == HalSupport::UNKNOWN) {
        bool isSupported = false;
        auto isSupportedRet = mHandle->isModeSupported(mode, &isSupported);
        if (!isSupportedRet.isOk()) {
            return HalResult<void>::failed(isSupportedRet.toString8().c_str());
        }

        mModeSupportedArray[idx] = isSupported ? HalSupport::ON : HalSupport::OFF;
        if (!isSupported) {
            ALOGV("Skipped setMode %s because Power HAL doesn't support it",
                  toString(mode).c_str());
            return HalResult<void>::unsupported();
        }
    }
    lock.unlock();

    return toHalResult(mHandle->setMode(mode, enabled));
}

HalResult<sp<Aidl::IPowerHintSession>> AidlHalWrapper::createHintSession(
        int32_t tgid, int32_t uid, const std::vector<int32_t>& threadIds, int64_t durationNanos) {
    sp<IPowerHintSession> appSession;
    return HalResult<sp<Aidl::IPowerHintSession>>::
            fromStatus(mHandle->createHintSession(tgid, uid, threadIds, durationNanos, &appSession),
                       appSession);
}

HalResult<int64_t> AidlHalWrapper::getHintSessionPreferredRate() {
    int64_t rate = -1;
    auto result = mHandle->getHintSessionPreferredRate(&rate);
    return HalResult<int64_t>::fromStatus(result, rate);
}

HalResult<void> HidlHalWrapperSeh::setBoost(Boost boost, int32_t durationMs) {
    if (boost == Boost::INTERACTION) {
        return sendPowerHint(V1_0::PowerHint::INTERACTION, durationMs);
    } else {
        ALOGV("Skipped setBoost %s because Power HAL AIDL not available", toString(boost).c_str());
        return HalResult<void>::unsupported();
    }
}

HalResult<void> HidlHalWrapperSeh::setMode(Mode mode, bool enabled) {
    uint32_t data = enabled ? 1 : 0;
    switch (mode) {
        case Mode::LAUNCH:
            return sendPowerHint(V1_0::PowerHint::LAUNCH, data);
        case Mode::LOW_POWER:
            return sendPowerHint(V1_0::PowerHint::LOW_POWER, data);
        case Mode::SUSTAINED_PERFORMANCE:
            return sendPowerHint(V1_0::PowerHint::SUSTAINED_PERFORMANCE, data);
        case Mode::VR:
            return sendPowerHint(V1_0::PowerHint::VR_MODE, data);
        case Mode::INTERACTIVE:
            return setInteractive(enabled);
        case Mode::DOUBLE_TAP_TO_WAKE:
            return setFeature(V1_0::Feature::POWER_FEATURE_DOUBLE_TAP_TO_WAKE, enabled);
        default:
            ALOGV("Skipped setMode %s because Power HAL AIDL not available",
                  toString(mode).c_str());
            return HalResult<void>::unsupported();
    }
}

HalResult<void> HidlHalWrapperSeh::sendPowerHint(V1_0::PowerHint hintId, uint32_t data) {
    if(mHandle11 != nullptr) {
        auto ret = mHandle11->powerHintAsync(hintId, data);
        return HalResult<void>::fromReturn(ret);
    } else {
        auto ret = mHandle10->powerHint(hintId, data);
        return HalResult<void>::fromReturn(ret);
    }
}

HalResult<void> HidlHalWrapperSeh::setInteractive(bool enabled) {
    if(mHandleSeh != nullptr) {
        mHandleSeh->setInteractiveAsync(enabled, false);
    }
    auto ret = mHandle10->setInteractive(enabled);
    return HalResult<void>::fromReturn(ret);
}

HalResult<void> HidlHalWrapperSeh::setFeature(V1_0::Feature feature, bool enabled) {
    auto ret = mHandle10->setFeature(feature, enabled);
    return HalResult<void>::fromReturn(ret);
}

HalResult<sp<Aidl::IPowerHintSession>> HidlHalWrapperSeh::createHintSession(
        int32_t, int32_t, const std::vector<int32_t>& threadIds, int64_t) {
    ALOGV("Skipped createHintSession(task num=%zu) because Power HAL not available",
          threadIds.size());
    return HalResult<sp<Aidl::IPowerHintSession>>::unsupported();
}

HalResult<int64_t> HidlHalWrapperSeh::getHintSessionPreferredRate() {
    ALOGV("Skipped getHintSessionPreferredRate because Power HAL not available");
    return HalResult<int64_t>::unsupported();
}

// -------------------------------------------------------------------------------------------------

} // namespace power

} // namespace android
