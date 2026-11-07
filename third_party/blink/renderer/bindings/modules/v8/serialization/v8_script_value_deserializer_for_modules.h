// Copyright 2016 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_BINDINGS_MODULES_V8_SERIALIZATION_V8_SCRIPT_VALUE_DESERIALIZER_FOR_MODULES_H_
#define THIRD_PARTY_BLINK_RENDERER_BINDINGS_MODULES_V8_SERIALIZATION_V8_SCRIPT_VALUE_DESERIALIZER_FOR_MODULES_H_

#include "build/build_config.h"
#include "third_party/blink/renderer/bindings/core/v8/serialization/v8_script_value_deserializer.h"
#include "third_party/blink/renderer/modules/modules_export.h"

namespace blink {

#if !BUILDFLAG(IS_COBALT)
class AudioData;
#endif
class CropTarget;
class CryptoKey;
#if !BUILDFLAG(IS_COBALT)
class EncodedAudioChunk;
class EncodedVideoChunk;
#endif
class FileSystemHandle;
class MediaSourceHandleImpl;
class RestrictionTarget;
class RTCEncodedAudioFrame;
class RTCEncodedVideoFrame;
class RTCDataChannel;
#if !BUILDFLAG(IS_COBALT)
class VideoFrame;
#endif

// Extends V8ScriptValueSerializer with support for modules/ types.
class MODULES_EXPORT V8ScriptValueDeserializerForModules final
    : public V8ScriptValueDeserializer {
 public:
  using V8ScriptValueDeserializer::V8ScriptValueDeserializer;

  static bool ExecutionContextExposesInterface(ExecutionContext*,
                                               SerializationTag interface_tag);

 protected:
  ScriptWrappable* ReadDOMObject(SerializationTag, ExceptionState&) override;

 private:
  bool ReadOneByte(uint8_t* byte) {
    const void* data;
    if (!ReadRawBytes(1, &data))
      return false;
    *byte = *reinterpret_cast<const uint8_t*>(data);
    return true;
  }
  CryptoKey* ReadCryptoKey();
  FileSystemHandle* ReadFileSystemHandle(SerializationTag tag);
  RTCEncodedAudioFrame* ReadRTCEncodedAudioFrame();
  RTCEncodedVideoFrame* ReadRTCEncodedVideoFrame();
#if !BUILDFLAG(IS_COBALT)
  AudioData* ReadAudioData();
  VideoFrame* ReadVideoFrame();
  EncodedAudioChunk* ReadEncodedAudioChunk();
  EncodedVideoChunk* ReadEncodedVideoChunk();
#endif
  MediaStreamTrack* ReadMediaStreamTrack();
  RTCDataChannel* ReadRTCDataChannel();
  CropTarget* ReadCropTarget();
  RestrictionTarget* ReadRestrictionTarget();
  MediaSourceHandleImpl* ReadMediaSourceHandle();
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_BINDINGS_MODULES_V8_SERIALIZATION_V8_SCRIPT_VALUE_DESERIALIZER_FOR_MODULES_H_
