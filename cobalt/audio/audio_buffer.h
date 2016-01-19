/*
 * Copyright 2015 Google Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef AUDIO_AUDIO_BUFFER_H_
#define AUDIO_AUDIO_BUFFER_H_

#include <vector>

#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"  // For scoped_array
#include "cobalt/dom/float32_array.h"
#include "cobalt/script/environment_settings.h"
#include "cobalt/script/wrappable.h"

namespace cobalt {
namespace audio {

// This represents a memory-resident audio asset (for one-shot sounds
// and other short audio clips). Its format is non-interleaved IEEE 32-bit
// linear PCM with a nominal range of -1 -> +1. It can contain one or more
// channels. Typically, it would be expected that the length of the PCM data
// would be fairly short (usually somewhat less than a minute). For longer
// sounds, such as music soundtracks, streaming should be used with the audio
// element and MediaElementAudioSourceNode.
// An AudioBuffer may be used by one or more AudioContexts.
//   https://www.w3.org/TR/webaudio/#AudioBuffer
class AudioBuffer : public script::Wrappable {
 public:
  // The audio data passed in |channels_data| stores multi-channel audio in
  // planar.  So for stereo audio, the first channel will be stored in the first
  // half of |channels_data| and the second channel will be stored in the second
  // half.
  AudioBuffer(script::EnvironmentSettings* settings, float sample_rate,
              int32 number_of_frames, int32 number_of_channels,
              scoped_array<uint8> channels_data);

  // Web API: AudioBuffer
  //
  // The sample-rate for the PCM audio data in samples per second.
  float sample_rate() const { return sample_rate_; }

  // Length of the PCM audio data in sample-frames.
  int32 length() const { return length_; }

  // Duration of the PCM audio data in seconds.
  double duration() const { return length() / sample_rate(); }

  // The number of discrete audio channels.
  int32 number_of_channels() const {
    return static_cast<int32>(channels_data_.size());
  }

  // Represents the PCM audio data for the specific channel.
  scoped_refptr<dom::Float32Array> GetChannelData(
      uint32 channel_index, script::ExceptionState* exception_state) const;

  DEFINE_WRAPPABLE_TYPE(AudioBuffer);

 private:
  typedef std::vector<scoped_refptr<dom::Float32Array> > Float32ArrayVector;

  float sample_rate_;
  int32 length_;

  Float32ArrayVector channels_data_;

  DISALLOW_COPY_AND_ASSIGN(AudioBuffer);
};

}  // namespace audio
}  // namespace cobalt

#endif  // AUDIO_AUDIO_BUFFER_H_
