// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef STORAGE_BROWSER_BLOB_FEATURES_H_
#define STORAGE_BROWSER_BLOB_FEATURES_H_

#include "base/component_export.h"
#include "base/features.h"
#include "build/build_config.h"

namespace features {

// Please keep features in alphabetical order.
// Enables blob URL fetches to fail when cross-partition.
COMPONENT_EXPORT(STORAGE_BROWSER)
BASE_DECLARE_FEATURE(kBlockCrossPartitionBlobUrlFetching);

#if BUILDFLAG(IS_COBALT)
// Cobalt (TV) memory experiment (see cobalt/COBALT_MEMORY_EXPERIMENTS.md):
// uses the Android formula (memory/100) for the in-memory blob cap instead of
// the desktop-shaped defaults. Declared here because storage/ cannot include
// cobalt/ headers; h5vcc experiment overrides are registered by feature NAME.
COMPONENT_EXPORT(STORAGE_BROWSER)
BASE_DECLARE_FEATURE(kCobaltMemBlobLimits);
#endif

// Please keep features in alphabetical order.

}  // namespace features

#endif  // STORAGE_BROWSER_BLOB_FEATURES_H_
