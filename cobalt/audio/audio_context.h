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

#ifndef AUDIO_AUDIO_CONTEXT_H_
#define AUDIO_AUDIO_CONTEXT_H_

#include "base/callback.h"
#include "base/optional.h"
#include "cobalt/audio/async_audio_decoder.h"
#include "cobalt/audio/audio_buffer.h"
#include "cobalt/audio/audio_buffer_source_node.h"
#include "cobalt/audio/audio_destination_node.h"
#include "cobalt/dom/array_buffer.h"
#include "cobalt/dom/dom_exception.h"
#include "cobalt/dom/event_target.h"
#include "cobalt/script/callback_function.h"
#include "cobalt/script/script_object.h"

namespace cobalt {
namespace audio {

// This represents a set of AudioNode objects and their connections. It allows
// for arbitrary routing of signals to the AudioDestinationNode (what the user
// ultimately hears). Nodes are created from the cotnext and are then connected
// together. In most user cases, only a single AudioContext is used per
// document.
//   http://www.w3.org/TR/webaudio/#AudioContext-section
class AudioContext : public dom::EventTarget {
 public:
  // Type for decode success and error callbacks on JS side.
  //
  // The AudioBuffer is representing the decoded PCM audio data.
  typedef script::CallbackFunction<void(
      const scoped_refptr<AudioBuffer>& decoded_data)> DecodeSuccessCallback;
  typedef script::ScriptObject<DecodeSuccessCallback> DecodeSuccessCallbackArg;
  typedef DecodeSuccessCallbackArg::Reference DecodeSuccessCallbackReference;

  typedef script::CallbackFunction<void()> DecodeErrorCallback;
  typedef script::ScriptObject<DecodeErrorCallback> DecodeErrorCallbackArg;
  typedef DecodeErrorCallbackArg::Reference DecodeErrorCallbackReference;

  AudioContext();

  // Web API: AudioContext
  //
  // This is a time in seconds which starts at zero when the context is created
  // and increases in real-time. All scheduled times are relative to it. This is
  // not a "transport" time which can be started, paused, and re-positioned.
  // It is always moving forward.
  double current_time() const { return current_time_; }

  // The sample rate (in sample-frames per second) at which the AudioContext
  // handles audio. It is assumed that all AudioNodes in the context run at this
  // rate.
  float sample_rate() const { return sample_rate_; }

  // An AudioDestinationNode with a single input representing the final
  // destination for all audio. Usually this will represent the actual audio
  // hardware. All AudioNodes actively rendering audio will directly or
  // indirectly connect to destination.
  const scoped_refptr<AudioDestinationNode>& destination() const {
    return destination_;
  }

  // Asynchronous decodes the audio file data contained in the ArrayBuffer. The
  // ArrayBuffer can, for example, be loaded from an XMLHttpRequest's response
  // attribute after setting the responseType to "arraybuffer". Audio file data
  // can be in any of the formats supported by the audio element.
  void DecodeAudioData(const scoped_refptr<dom::ArrayBuffer>& audio_data,
                       const DecodeSuccessCallbackArg& success_handler);
  void DecodeAudioData(const scoped_refptr<dom::ArrayBuffer>& audio_data,
                       const DecodeSuccessCallbackArg& success_handler,
                       const DecodeErrorCallbackArg& error_handler);

  // Creates an AudioBufferSourceNode.
  scoped_refptr<AudioBufferSourceNode> CreateBufferSource();

  DEFINE_WRAPPABLE_TYPE(AudioContext);

 private:
  struct DecodeCallbackInfo
      : public base::RefCountedThreadSafe<DecodeCallbackInfo> {
    DecodeCallbackInfo(const AudioContext* const audio_context,
                       const DecodeSuccessCallbackArg& success_handler)
        : success_callback(audio_context, success_handler) {}

    DecodeCallbackInfo(const AudioContext* const audio_context,
                       const DecodeSuccessCallbackArg& success_handler,
                       const DecodeErrorCallbackArg& error_handler)
        : success_callback(audio_context, success_handler) {
      error_callback.emplace(audio_context, error_handler);
    }

    DecodeSuccessCallbackReference success_callback;
    base::optional<DecodeErrorCallbackReference> error_callback;
  };

  void DecodeFinish(const scoped_refptr<DecodeCallbackInfo>& info,
                    const scoped_refptr<AudioBuffer>& audio_data);

  float sample_rate_;
  double current_time_;

  scoped_refptr<AudioDestinationNode> destination_;

  AsyncAudioDecoder audio_decoder_;

  // The main message loop.
  scoped_refptr<base::MessageLoopProxy> const main_message_loop_;

  DISALLOW_COPY_AND_ASSIGN(AudioContext);
};

}  // namespace audio
}  // namespace cobalt

#endif  // AUDIO_AUDIO_CONTEXT_H_
