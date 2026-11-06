// SPDX-License-Identifier: BSD-3-Clause
#include "media/audio/audio_manager_base.h"

#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace media
{
namespace
{

class AudioManagerLume final : public AudioManagerBase
{
  public:
    AudioManagerLume(std::unique_ptr<AudioThread> thread, AudioLogFactory *log_factory)
        : AudioManagerBase(std::move(thread), log_factory)
    {
    }

    bool HasAudioOutputDevices() override
    {
        return false;
    }

    bool HasAudioInputDevices() override
    {
        return false;
    }

    const std::string_view GetName() override
    {
        return "Lume";
    }

    AudioOutputStream *MakeLinearOutputStream(const AudioParameters &, const LogCallback &) override
    {
        return nullptr;
    }

    AudioOutputStream *MakeLowLatencyOutputStream(const AudioParameters &, const std::string &,
                                                  const LogCallback &) override
    {
        return nullptr;
    }

    AudioInputStream *MakeLinearInputStream(const AudioParameters &, const std::string &,
                                            const LogCallback &) override
    {
        return nullptr;
    }

    AudioInputStream *MakeLowLatencyInputStream(const AudioParameters &, const std::string &,
                                                const LogCallback &) override
    {
        return nullptr;
    }

    AudioParameters GetInputStreamParameters(const std::string &) override
    {
        return AudioParameters::UnavailableDeviceParams();
    }

  protected:
    AudioParameters GetPreferredOutputStreamParameters(const std::string &,
                                                       const AudioParameters &) override
    {
        return AudioParameters::UnavailableDeviceParams();
    }

    std::string GetDefaultInputDeviceID() override
    {
        return {};
    }

    std::string GetDefaultOutputDeviceID() override
    {
        return {};
    }

    std::string GetCommunicationsInputDeviceID() override
    {
        return {};
    }

    std::string GetCommunicationsOutputDeviceID() override
    {
        return {};
    }
};

} // namespace

std::unique_ptr<AudioManager> CreateAudioManager(std::unique_ptr<AudioThread> audio_thread,
                                                 AudioLogFactory *audio_log_factory)
{
    return std::make_unique<AudioManagerLume>(std::move(audio_thread), audio_log_factory);
}

} // namespace media
