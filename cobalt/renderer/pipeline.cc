/*
 * Copyright 2014 Google Inc. All Rights Reserved.
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

#include "cobalt/renderer/pipeline.h"

#include "base/bind.h"
#include "base/debug/trace_event.h"

using cobalt::render_tree::Node;
using cobalt::render_tree::animations::NodeAnimationsMap;

namespace cobalt {
namespace renderer {

namespace {
// In order to put a bound on memory we set a maximum submission queue size that
// is empirically found to be a nice balance between animation smoothing and
// memory usage.
const size_t kMaxSubmissionQueueSize = 4u;

// How quickly the renderer time adjusts to changing submission times.
// 500ms is chosen as a default because it is fast enough that the user will not
// usually notice input lag from a slow timeline renderer, but slow enough that
// quick updates while a quick animation is playing should not jank.
const double kTimeToConvergeInMS = 500.0;
}  // namespace

Pipeline::Pipeline(const CreateRasterizerFunction& create_rasterizer_function,
                   const scoped_refptr<backend::RenderTarget>& render_target,
                   backend::GraphicsContext* graphics_context)
    : rasterizer_created_event_(true, false),
      render_target_(render_target),
      graphics_context_(graphics_context),
      submission_queue_(
          kMaxSubmissionQueueSize,
          base::TimeDelta::FromMillisecondsD(kTimeToConvergeInMS)),
      rasterizer_thread_(base::in_place, "Rasterizer") {
  TRACE_EVENT0("cobalt::renderer", "Pipeline::Pipeline()");
  // The actual Pipeline can be constructed from any thread, but we want
  // rasterizer_thread_checker_ to be associated with the rasterizer thread,
  // so we detach it here and let it reattach itself to the rasterizer thread
  // when CalledOnValidThread() is called on rasterizer_thread_checker_ below.
  rasterizer_thread_checker_.DetachFromThread();
#if defined(__LB_PS3__)
  // TODO(20953608) Implement a way to set this properly.
  const int kStackSize = 128 * 1024;
  rasterizer_thread_->StartWithOptions(
      base::Thread::Options(MessageLoop::TYPE_DEFAULT, kStackSize));
#else
  rasterizer_thread_->Start();
#endif
  rasterizer_thread_->message_loop()->PostTask(
      FROM_HERE,
      base::Bind(&Pipeline::InitializeRasterizerThread, base::Unretained(this),
                 create_rasterizer_function));
}

Pipeline::~Pipeline() {
  TRACE_EVENT0("cobalt::renderer", "Pipeline::~Pipeline()");
  // Submit a shutdown task to the rasterizer thread so that it can shutdown
  // anything that must be shutdown from that thread.
  rasterizer_thread_->message_loop()->PostTask(
      FROM_HERE,
      base::Bind(&Pipeline::ShutdownRasterizerThread, base::Unretained(this)));

  rasterizer_thread_ = base::nullopt;
}

render_tree::ResourceProvider* Pipeline::GetResourceProvider() {
  rasterizer_created_event_.Wait();
  return rasterizer_->GetResourceProvider();
}

void Pipeline::Submit(const Submission& render_tree_submission) {
  TRACE_EVENT0("cobalt::renderer", "Pipeline::Submit()");
  // Execute the actual set of the new render tree on the rasterizer tree.
  rasterizer_thread_->message_loop()->PostTask(
      FROM_HERE, base::Bind(&Pipeline::SetNewRenderTree, base::Unretained(this),
                            render_tree_submission));
}

void Pipeline::Clear() {
  TRACE_EVENT0("cobalt::renderer", "Pipeline::Clear()");
  base::WaitableEvent wait_event(true, false);
  rasterizer_thread_->message_loop()->PostTask(
      FROM_HERE,
      base::Bind(&Pipeline::ClearCurrentRenderTree, base::Unretained(this),
                 base::Bind(&base::WaitableEvent::Signal,
                            base::Unretained(&wait_event))));
  wait_event.Wait();
}

void Pipeline::RasterizeToRGBAPixels(
    const Submission& render_tree_submission,
    const RasterizationCompleteCallback& complete) {
  TRACE_EVENT0("cobalt::renderer", "Pipeline::RasterizeToRGBAPixels()");

  if (MessageLoop::current() != rasterizer_thread_->message_loop()) {
    rasterizer_thread_->message_loop()->PostTask(
        FROM_HERE,
        base::Bind(&Pipeline::RasterizeToRGBAPixels, base::Unretained(this),
                   render_tree_submission, complete));
    return;
  }
  // Create a new target that is the same dimensions as the display target.
  scoped_refptr<backend::RenderTarget> offscreen_target =
      graphics_context_->CreateOffscreenRenderTarget(
          render_target_->GetSurfaceInfo().size);

  // Rasterize this submission into the newly created target.
  RasterizeSubmissionToRenderTarget(render_tree_submission, offscreen_target);

  scoped_ptr<backend::Texture> texture =
      graphics_context_->CreateTextureFromOffscreenRenderTarget(
          offscreen_target);

  // Load the texture's pixel data into a CPU memory buffer and return it.
  complete.Run(graphics_context_->GetCopyOfTexturePixelDataAsRGBA(*texture),
               render_target_->GetSurfaceInfo().size);
}

void Pipeline::SetNewRenderTree(const Submission& render_tree_submission) {
  DCHECK(rasterizer_thread_checker_.CalledOnValidThread());
  DCHECK(render_tree_submission.render_tree.get());

  TRACE_EVENT0("cobalt::renderer", "Pipeline::SetNewRenderTree()");

  submission_queue_.PushSubmission(render_tree_submission);

  // Start the rasterization timer if it is not yet started.
  if (!rasterize_timer_) {
    // We submit render trees as fast as the rasterizer can consume them.
    // Practically, this will result in the rate being limited to the
    // display's refresh rate.
    rasterize_timer_.emplace(
        FROM_HERE, base::TimeDelta(),
        base::Bind(&Pipeline::RasterizeCurrentTree, base::Unretained(this)),
        true);
    rasterize_timer_->Reset();
  }
}

void Pipeline::ClearCurrentRenderTree(
    const base::Closure& clear_complete_callback) {
  DCHECK(rasterizer_thread_checker_.CalledOnValidThread());
  TRACE_EVENT0("cobalt::renderer", "Pipeline::ClearCurrentRenderTree()");

  submission_queue_.Reset();
  rasterize_timer_ = base::nullopt;

  if (!clear_complete_callback.is_null()) {
    clear_complete_callback.Run();
  }
}

void Pipeline::RasterizeCurrentTree() {
  DCHECK(rasterizer_thread_checker_.CalledOnValidThread());
  TRACE_EVENT0("cobalt::renderer", "Pipeline::RasterizeCurrentTree()");

  // Rasterize the last submitted render tree.
  RasterizeSubmissionToRenderTarget(submission_queue_.GetCurrentSubmission(),
                                    render_target_);
}

void Pipeline::RasterizeSubmissionToRenderTarget(
    const Submission& submission,
    const scoped_refptr<backend::RenderTarget>& render_target) {
  TRACE_EVENT0("cobalt::renderer",
               "Pipeline::RasterizeSubmissionToRenderTarget()");
  // Animate the render tree using the submitted animations.
  scoped_refptr<Node> animated_render_tree = submission.animations->Apply(
      submission.render_tree, submission.time_offset);

  // Rasterize the animated render tree.
  rasterizer_->Submit(animated_render_tree, render_target);

  if (!submission.on_rasterized_callback.is_null()) {
    submission.on_rasterized_callback.Run();
  }
}

void Pipeline::InitializeRasterizerThread(
    const CreateRasterizerFunction& create_rasterizer_function) {
  TRACE_EVENT0("cobalt::renderer", "Pipeline::InitializeRasterizerThread");
  DCHECK(rasterizer_thread_checker_.CalledOnValidThread());
  rasterizer_ = create_rasterizer_function.Run();
  rasterizer_created_event_.Signal();
}

void Pipeline::ShutdownRasterizerThread() {
  TRACE_EVENT0("cobalt::renderer", "Pipeline::ShutdownRasterizerThread()");
  DCHECK(rasterizer_thread_checker_.CalledOnValidThread());
  // Stop and shutdown the raterizer timer.
  rasterize_timer_ = base::nullopt;

  // Do not retain any more references to the current render tree (which
  // may refer to rasterizer resources) or animations which may refer to
  // render trees.
  submission_queue_.Reset();

  // Finally, destroy the rasterizer.
  rasterizer_.reset();
}

}  // namespace renderer
}  // namespace cobalt
