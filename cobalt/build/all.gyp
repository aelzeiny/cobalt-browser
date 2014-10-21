# Copyright 2014 Google Inc. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# This files contains all targets that should be created by gyp_cobalt by
# default.

{
  'targets': [
    {
      'target_name': 'All',
      'type': 'none',
      'dependencies': [
        '<(DEPTH)/cobalt/browser/browser.gyp:*',
        '<(DEPTH)/cobalt/renderer/renderer.gyp:*',

        # Include the samples to make sure that they always compile and work.
        '<(DEPTH)/cobalt/samples/samples.gyp:*',

        # Include the skia sandbox app to ensure it compiles and works on
        # all platforms.
        '<(DEPTH)/cobalt/renderer/skia/sandbox/sandbox.gyp:*',
      ]
    }
  ],
}
