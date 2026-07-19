#!/usr/bin/env bash
# One-time fixup script.
#
# Context: the assistant's shell tool was broken this session (root cause:
# disk filled up — each git worktree is a full Chromium checkout). The three
# opt/* implementations therefore live as uncommitted edits in two worktrees
# plus the main working tree. This script:
#   1. cleans up the failed third worktree checkout,
#   2. commits each implementation on a properly named opt/* branch,
#   3. removes the worktrees to free the disk space they consumed.
#
# Run from anywhere: bash /home/ahmed/cobalt/make-opt-branches.sh
# Review each `git diff` first if you want; the script only touches the
# files listed below. Delete this script afterwards.
set -euo pipefail
cd /home/ahmed/cobalt

TRAILER="Co-Authored-By: Claude Fable 5 <noreply@anthropic.com>"

echo "== 0. Clean up the failed (disk-full) third worktree =="
git worktree remove --force .claude/worktrees/opt+image-decode-cache-limit 2>/dev/null \
  || rm -rf .claude/worktrees/opt+image-decode-cache-limit
git worktree prune
git branch -D worktree-opt+image-decode-cache-limit 2>/dev/null || true

echo "== 1. opt/mse-video-buffer-budget =="
WT1=.claude/worktrees/opt+mse-video-buffer-budget
git -C "$WT1" add \
  starboard/android/shared/media_get_video_buffer_budget.cc \
  starboard/shared/starboard/media/media_get_video_buffer_budget.cc
git -C "$WT1" commit -m "starboard: Clamp 4K MSE video buffer budgets to 80MB

Lowers the 4K SDR budget from 100MB and the 4K HDR budget from 160MB
to 80MB on both the Android and shared Starboard implementations,
reducing the SourceBuffer memory ceiling during 4K playback by
20MB (SDR) / 80MB (HDR).

80MB is the floor validated by Chrome's field trials: pushing
smaller demuxer buffers to mid-range devices caused measurably more
rebuffers and less watch time on stable and was reverted in favor of
an 80MB limit judged sufficient for 4K60 (crbug.com/40264947).
Validate rebuffer ratio and seek latency on low-end network profiles
before lowering further.

$TRAILER"
git branch -m worktree-opt+mse-video-buffer-budget opt/mse-video-buffer-budget
git worktree remove "$WT1"

echo "== 2. opt/media-pool-release-on-hide =="
WT2=.claude/worktrees/opt+media-pool-release-on-hide
git -C "$WT2" add \
  media/starboard/decoder_buffer_allocator.cc \
  media/starboard/decoder_buffer_allocator.h
git -C "$WT2" commit -m "media: Release decoder buffer pool on hide by default

The renderer already calls ReleaseIdleMemory() when the page is
hidden, but the release was a no-op unless the web app opted in via
the h5vcc setting DecoderBuffer.ReleaseMemoryOnBackground. On
platforms that do not allocate the pool on demand (Android TV), the
pool therefore retained its grown capacity — up to the full video
buffer budget — for the process lifetime.

Gate the release on a new default-enabled feature instead, so hiding
the app frees the pool (once buffers drain) without requiring the
app's JS call. The h5vcc opt-in still works and now acts as a forced
enable. Kill switch:
--disable-features=CobaltReleaseIdleMediaBufferMemory

Expected saving: pool capacity (up to 80-160MB after a 4K session)
while the app is hidden, at the cost of re-growing the pool (a few
4MB allocations) on resume.

$TRAILER"
git branch -m worktree-opt+media-pool-release-on-hide opt/media-pool-release-on-hide
git worktree remove "$WT2"

echo "== 3. opt/image-decode-cache-limit (changes are in the main tree) =="
git switch -c opt/image-decode-cache-limit
git add \
  cc/tiles/image_decode_cache_utils.cc \
  cobalt/app/cobalt_switch_defaults_starboard.cc \
  cobalt/android/apk/app/src/main/java/dev/cobalt/coat/CommandLineOverrideHelper.java \
  cobalt/android/apk/app/src/test/java/dev/cobalt/coat/CommandLineOverrideHelperTest.java
git commit -m "cobalt: Fix inert image decode cache limit (128MB -> 24MB)

The default command lines enabled LimitImageDecodeCacheSize:mb/24 on
both platforms, but no feature with that name exists anywhere in the
tree, so the token was silently ignored and the decoded-image working
set ran with the 128MB IS_COBALT default — a quarter of the total
memory target on TV devices.

Replace the dead token with the switch that is actually consumed
(--decoded-image-working-set-budget-bytes=25165824) in both the
Starboard switch defaults and the Android CommandLineOverrideHelper,
lower the hardcoded IS_COBALT fallback to 24MB so future targets
cannot silently regress (this also covers tvOS), and update the Java
test that asserted the dead token.

Overflow beyond the working-set budget falls back to at-raster
decode rather than failing; verify scroll/decode latency on the
lowest-end SoC.

$TRAILER"
git switch -

echo "== Done =="
git branch --list 'opt/*'
git log --oneline -1 opt/mse-video-buffer-budget
git log --oneline -1 opt/media-pool-release-on-hide
git log --oneline -1 opt/image-decode-cache-limit
df -h /home | tail -1
