// Copyright 2018 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sql/sql_features.h"

#include "base/feature_list.h"

namespace sql::features {

// Use a fixed memory-map size instead of using the heuristic.
BASE_FEATURE(kSqlFixedMmapSize,
             "SqlFixedMmapSize",
             base::FEATURE_DISABLED_BY_DEFAULT);

// Explicitly unlock the database on close to ensure lock is released.
BASE_FEATURE(kUnlockDatabaseOnClose,
             "UnlockDatabaseOnClose",
             base::FEATURE_DISABLED_BY_DEFAULT);

#if BUILDFLAG(IS_COBALT)
// Cobalt (TV) memory experiment: sql/-layer half of the "CobaltMemCacheSweep"
// experiment (see cobalt/COBALT_MEMORY_EXPERIMENTS.md). Caps SQLite's default
// per-connection page cache at 64 pages when the caller did not request a
// size. Declared under a distinct name because sql/ cannot include the
// cobalt/ header that declares "CobaltMemCacheSweep" and a feature name must
// have exactly one BASE_FEATURE in the binary; the browser client fans the
// config's "CobaltMemCacheSweep" out to this name.
BASE_FEATURE(kCobaltMemCacheSweepSql,
             "CobaltMemCacheSweepSql",
             base::FEATURE_DISABLED_BY_DEFAULT);
#endif

}  // namespace sql::features
