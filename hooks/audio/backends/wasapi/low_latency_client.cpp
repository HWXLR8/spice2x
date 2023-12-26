#include "low_latency_client.h"
#include "util/logging.h"

#define PRINT_FAILED_RESULT(name, ret) \
    log_warning("audio::lowlatency", "{} failed, hr={}", name, FMT_HRESULT(ret))

namespace hooks::audio {
    bool LOW_LATENCY_SHARED_WASAPI = false;
}

using namespace hooks::audio;

LowLatencyAudioClient::LowLatencyAudioClient(IAudioClient3* audioClient) : audioClient(audioClient) {}

LowLatencyAudioClient::~LowLatencyAudioClient() {
    log_info("audio::lowlatency", "stopping");
    this->audioClient->Stop();
    this->audioClient->Release();
    this->audioClient = nullptr;
}

LowLatencyAudioClient *LowLatencyAudioClient::Create(IMMDevice *device) {
    HRESULT ret;
    UINT32 minPeriod;
    UINT32 defaultPeriod;
    UINT32 fundamentalPeriod;
    UINT32 maxPeriod;
    PWAVEFORMATEX pFormat;
    IAudioClient3* audioClient;

    ret = device->Activate(IID_IAudioClient3, CLSCTX_ALL, NULL, reinterpret_cast<void**>(&audioClient));
    if (FAILED(ret)) {
        PRINT_FAILED_RESULT("IMMDevice::Activate(IID_IAudioClient3...)", ret);
        log_warning("audio::lowlatency", "note that only Windows 10 and above supports IAudioClient3");
        return nullptr;
    }

    ret = audioClient->GetMixFormat(&pFormat);
    if (FAILED(ret)) {
        PRINT_FAILED_RESULT("IAudioClient3::GetMixFormat", ret);
        audioClient->Release();
        return nullptr;
    }

    ret = audioClient->GetSharedModeEnginePeriod(pFormat, &defaultPeriod, &fundamentalPeriod, &minPeriod, &maxPeriod);
    if (FAILED(ret)) {
        PRINT_FAILED_RESULT("IAudioClient3::GetSharedModeEnginePeriod", ret);
        audioClient->Release();
        return nullptr;
    }

    ret = audioClient->InitializeSharedAudioStream(0, minPeriod, pFormat, NULL);
    if (FAILED(ret)) {
        PRINT_FAILED_RESULT("IAudioClient3::InitializeSharedAudioStream", ret);
        audioClient->Release();
        return nullptr;
    }

    ret = audioClient->Start();
    if (FAILED(ret)) {
        PRINT_FAILED_RESULT("IAudioClient3::Start", ret);
        audioClient->Release();
        return nullptr;
    }

    log_info("audio::lowlatency", "low latency shared mode audio client initialized successfully");
    log_info("audio::lowlatency", "this is NOT used to output sound, but rather to reduce buffer sizes when the game requests an audio client at a later point");
    log_info("audio::lowlatency", "... sample rate         : {} Hz", pFormat->nSamplesPerSec);
    log_info("audio::lowlatency", "... min buffer size     : {} samples ({} ms)", minPeriod, 1000.0f * minPeriod / pFormat->nSamplesPerSec);
    log_info("audio::lowlatency", "... max buffer size     : {} samples ({} ms)", maxPeriod, 1000.0f * maxPeriod / pFormat->nSamplesPerSec);
    log_info("audio::lowlatency", "... default buffer size : {} samples ({} ms)", defaultPeriod, 1000.0f * defaultPeriod / pFormat->nSamplesPerSec);
    log_info("audio::lowlatency", "... Windows will use minimum buffer size (instead of default) for shared mode audio clients from now on");
    return new LowLatencyAudioClient(audioClient);
}
