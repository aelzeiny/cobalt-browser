// Copyright 2017 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/browser/download_manager.h"

#include "build/build_config.h"
#include "build/buildflag.h"

#if !BUILDFLAG(IS_COBALT)
#include "base/task/sequenced_task_runner.h"
#include "components/download/public/common/download_task_runner.h"  // nogncheck

namespace content {

// static
scoped_refptr<base::SequencedTaskRunner> DownloadManager::GetTaskRunner() {
  return download::GetDownloadTaskRunner();
}

}  // namespace content
#endif  // !BUILDFLAG(IS_COBALT)
