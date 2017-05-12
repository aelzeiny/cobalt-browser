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

{
  'targets': [
    {
      'target_name': 'renderer_backend',
      'type': 'static_library',
      'sources': [
        'render_target.cc',
        'render_target.h',
      ],

      'dependencies': [
        '<(DEPTH)/cobalt/base/base.gyp:base',
        '<(DEPTH)/cobalt/math/math.gyp:math',
      ],
      'conditions': [
        ['OS=="starboard"', {
          'dependencies': [
            '<(DEPTH)/cobalt/renderer/backend/starboard/platform_backend.gyp:renderer_platform_backend',
          ],
        }, {
          'dependencies': [
            '<(DEPTH)/cobalt/renderer/backend/<(actual_target_arch)/platform_backend.gyp:renderer_platform_backend',
          ],
        }],
      ],
    },
  ],
}
