// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_FILTERS_BLOCKING_URL_PROTOCOL_H_
#define MEDIA_FILTERS_BLOCKING_URL_PROTOCOL_H_

#include "base/basictypes.h"
#include "base/callback.h"
#include "base/synchronization/waitable_event.h"
#include "media/filters/ffmpeg_glue.h"

namespace media {

class DataSource;

// An implementation of FFmpegURLProtocol that blocks until the underlying
// asynchronous DataSource::Read() operation completes.
//
// TODO(scherkus): Should be more thread-safe as |last_read_bytes_| is updated
// from multiple threads without any protection.
class MEDIA_EXPORT BlockingUrlProtocol : public FFmpegURLProtocol {
 public:
  // Implements FFmpegURLProtocol using the given |data_source|. |error_cb| is
  // fired any time DataSource::Read() returns an error.
  //
  // TODO(scherkus): After all blocking operations are isolated on a separate
  // thread we should be able to eliminate |error_cb|.
  BlockingUrlProtocol(const scoped_refptr<DataSource>& data_source,
                      const base::Closure& error_cb);
  virtual ~BlockingUrlProtocol();

  // Aborts any pending reads by returning a read error. After this method
  // returns all subsequent calls to Read() will immediately fail.
  //
  // TODO(scherkus): Currently this will cause |error_cb| to fire. Fix.
  void Abort();

  // FFmpegURLProtocol implementation.
  virtual int Read(int size, uint8* data) OVERRIDE;
  virtual bool GetPosition(int64* position_out) OVERRIDE;
  virtual bool SetPosition(int64 position) OVERRIDE;
  virtual bool GetSize(int64* size_out) OVERRIDE;
  virtual bool IsStreaming() OVERRIDE;

 private:
  // Sets |last_read_bytes_| and signals the blocked thread that the read
  // has completed.
  void SignalReadCompleted(int size);

  scoped_refptr<DataSource> data_source_;
  base::Closure error_cb_;

  // Used to convert an asynchronous DataSource::Read() into a blocking
  // FFmpegUrlProtocol::Read().
  base::WaitableEvent read_event_;

  // Read errors and aborts are unrecoverable. Any subsequent reads will
  // immediately fail when this is set to true.
  bool read_has_failed_;

  // Cached number of bytes last read from the data source.
  int last_read_bytes_;

  // Cached position within the data source.
  int64 read_position_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(BlockingUrlProtocol);
};

}  // namespace media

#endif  // MEDIA_FILTERS_BLOCKING_URL_PROTOCOL_H_
