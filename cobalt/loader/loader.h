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

#ifndef LOADER_LOADER_H_
#define LOADER_LOADER_H_

#include "base/callback.h"
#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "cobalt/loader/decoder.h"
#include "cobalt/loader/fetcher.h"

namespace cobalt {
namespace loader {

// Loader class consists of a Fetcher and a Decoder, that loads and decodes a
// resource respectively. See the Loader design doc under ***REMOVED***cobalt.
class Loader {
 public:
  typedef base::Callback<void(const std::string&)> ErrorCallback;

  // This class is responsible for passing chunks of data from fetcher to
  // decoder and notifying fetching is done or aborted on error.
  class DecoderToFetcherAdapter : public Fetcher::Handler {
   public:
    DecoderToFetcherAdapter(Decoder* decoder, ErrorCallback error_callback)
        : decoder_(decoder), error_callback_(error_callback) {}
    void OnReceived(const char* data, size_t size) {
      decoder_->DecodeChunk(data, size);
    }
    void OnDone() { decoder_->Finish(); }
    void OnError(const std::string& error) { error_callback_.Run(error); }

   private:
    Decoder* decoder_;
    ErrorCallback error_callback_;
  };

  // The construction of Loader initiates the loading. It takes the ownership
  // of a Decoder and creates and manages a Fetcher using the given factory
  // method.
  Loader(base::Callback<scoped_ptr<Fetcher>(Fetcher::Handler*)> fetcher_creator,
         scoped_ptr<Decoder> decoder, ErrorCallback error_callback);
  virtual ~Loader() {}

 private:
  scoped_ptr<Decoder> decoder_;
  scoped_ptr<DecoderToFetcherAdapter> decoder_to_fetcher_adaptor_;
  scoped_ptr<Fetcher> fetcher_;
};

}  // namespace loader
}  // namespace cobalt

#endif  // LOADER_LOADER_H_
