#pragma once

#include <initguid.h>
#include <audioclient.h>
#include <mmdeviceapi.h>

#include "hooks/audio/audio.h"

namespace hooks::audio {
    class LowLatencyAudioClient {
    public:
        static LowLatencyAudioClient *Create(IMMDevice *device);
        ~LowLatencyAudioClient();

    private:
        LowLatencyAudioClient(IAudioClient3* audioClient);
        IAudioClient3* audioClient;
    };
}
