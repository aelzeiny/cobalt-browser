// Copyright 2025 The Cobalt Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// clang-format off
#include "cobalt/shell/browser/shell_platform_delegate.h"
// clang-format on

#include <memory>

#include "base/containers/contains.h"
#include "base/memory/ptr_util.h"
#include "build/build_config.h"
#include "build/buildflag.h"
#include "cobalt/browser/lifecycle/cobalt_lifecycle_manager.h"
#include "cobalt/shell/browser/shell.h"
#include "cobalt/shell/browser/shell_platform_data_aura.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "ui/aura/env.h"
#include "ui/aura/window.h"
#include "ui/aura/window_event_dispatcher.h"
#include "ui/aura/window_tree_host.h"
#include "ui/aura/window_tree_host_platform.h"
#include "ui/compositor/compositor.h"
#include "ui/platform_window/platform_window.h"

#if defined(USE_AURA) && BUILDFLAG(IS_STARBOARD)
#include "ui/ozone/platform/starboard/platform_window_starboard.h"
#endif

namespace content {

namespace {

// Zero-views helper setting transparent background on WebContents so SbPlayer
// punch-out video is not occluded by an opaque quad.
class WebContentsSetBackgroundColor
    : public content::WebContentsObserver,
      public content::WebContentsUserData<WebContentsSetBackgroundColor> {
 public:
  static void CreateForWebContentsWithColor(content::WebContents* web_contents,
                                            SkColor color) {
    if (!web_contents || FromWebContents(web_contents)) {
      return;
    }
    web_contents->SetUserData(
        UserDataKey(), base::WrapUnique(new WebContentsSetBackgroundColor(
                           web_contents, color)));
  }

  void RenderFrameCreated(
      content::RenderFrameHost* render_frame_host) override {
    if (!render_frame_host->GetParentOrOuterDocument() &&
        render_frame_host->GetView()) {
      render_frame_host->GetView()->SetBackgroundColor(color_);
    }
  }

 private:
  friend class content::WebContentsUserData<WebContentsSetBackgroundColor>;
  WebContentsSetBackgroundColor(content::WebContents* web_contents,
                                SkColor color)
      : content::WebContentsObserver(web_contents),
        content::WebContentsUserData<WebContentsSetBackgroundColor>(
            *web_contents),
        color_(color) {
    if (web_contents->GetPrimaryMainFrame() &&
        web_contents->GetPrimaryMainFrame()->GetView()) {
      web_contents->GetPrimaryMainFrame()->GetView()->SetBackgroundColor(
          color_);
    }
  }

  SkColor color_;
  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

WEB_CONTENTS_USER_DATA_KEY_IMPL(WebContentsSetBackgroundColor);

#if defined(USE_AURA)
bool CheckAndHandleRevealState(content::WebContents* web_contents) {
  if (!web_contents) {
    return false;
  }
  gfx::NativeView window = web_contents->GetNativeView();
  if (!window) {
    return false;
  }
  aura::WindowTreeHost* host = window->GetHost();
  if (!host) {
    return false;
  }
  auto* host_platform = static_cast<aura::WindowTreeHostPlatform*>(host);
  ui::PlatformWindow* platform_window = host_platform->platform_window();
  if (!platform_window) {
    return false;
  }

#if BUILDFLAG(IS_STARBOARD)
  auto* pw_starboard =
      static_cast<ui::PlatformWindowStarboard*>(platform_window);
  bool is_waiting = pw_starboard->IsWaitingForRevealAck() ||
                    content::Shell::GetPlatform()->IsWaitingForRevealAck();
  if (is_waiting && !pw_starboard->IsWaitingForRevealAck()) {
    pw_starboard->SetWaitingForRevealAck(true);
  }
  return is_waiting;
#else
  return content::Shell::GetPlatform()->IsWaitingForRevealAck();
#endif
}
#endif  // defined(USE_AURA)

}  // namespace

struct ShellPlatformDelegate::ShellData {
  gfx::NativeWindow window;
  gfx::Size initial_size_;
};

struct ShellPlatformDelegate::PlatformData {
  std::unique_ptr<ShellPlatformDataAura> aura;
  gfx::Size default_window_size;
};

ShellPlatformDelegate::ShellPlatformDelegate() = default;

ShellPlatformDelegate::~ShellPlatformDelegate() {
  cobalt::CobaltLifecycleManager::GetInstance()->RemoveObserver(
      static_cast<cobalt::CobaltLifecycleManagerObserver*>(this));
}

ShellPlatformDataAura* ShellPlatformDelegate::GetShellPlatformDataAura() {
  return platform_ ? platform_->aura.get() : nullptr;
}

void ShellPlatformDelegate::Initialize(const gfx::Size& default_window_size,
                                       bool is_visible) {
  is_visible_ = is_visible;
  platform_ = std::make_unique<PlatformData>();
  platform_->default_window_size = default_window_size;
  platform_->aura = nullptr;
}

void ShellPlatformDelegate::CreatePlatformWindow(
    Shell* shell,
    const gfx::Size& initial_size) {
  DCHECK(!base::Contains(shell_data_map_, shell));
  ShellData& shell_data = shell_data_map_[shell];

  shell_data.initial_size_ = initial_size;

  if (IsVisible()) {
    CreatePlatformWindowInternal(shell, initial_size);
  } else {
    shell_data.window = nullptr;
  }
}

void ShellPlatformDelegate::CreatePlatformWindowInternal(
    Shell* shell,
    const gfx::Size& initial_size) {
#if BUILDFLAG(IS_STARBOARD)
  if (!platform_->aura) {
    // Use initial_size when provided, falling back to default_window_size.
    platform_->aura = std::make_unique<ShellPlatformDataAura>(
        !initial_size.IsEmpty() ? initial_size
                                : platform_->default_window_size);
  }
  // Resize the Aura window to the actual native Starboard window bounds
  // queried via SbWindowGetSize() rather than overwriting it with initial_size.
  platform_->aura->ResizeWindow(
      platform_->aura->host()->GetBoundsInPixels().size());
#else
  if (!platform_->aura) {
    platform_->aura =
        std::make_unique<ShellPlatformDataAura>(platform_->default_window_size);
  }
  platform_->aura->ResizeWindow(initial_size);
#endif

  ShellData& shell_data = shell_data_map_.at(shell);
  shell_data.window = platform_->aura->host()->window();
}

gfx::NativeWindow ShellPlatformDelegate::GetNativeWindow(Shell* shell) {
  DCHECK(base::Contains(shell_data_map_, shell));
  ShellData& shell_data = shell_data_map_[shell];
  return shell_data.window;
}

void ShellPlatformDelegate::CleanUp(Shell* shell) {
  DCHECK(base::Contains(shell_data_map_, shell));
  shell_data_map_.erase(shell);
}

void ShellPlatformDelegate::SetContents(Shell* shell) {
  aura::Window* content = shell->web_contents()->GetNativeView();
  if (!platform_ || !platform_->aura) {
    return;
  }
  aura::Window* parent = platform_->aura->host()->window();
  if (parent) {
    if (!parent->Contains(content)) {
      parent->AddChild(content);
    } else {
      parent->StackChildAtTop(content);
    }
  }

  WebContentsSetBackgroundColor::CreateForWebContentsWithColor(
      shell->web_contents(), SK_ColorTRANSPARENT);

  bool is_waiting = CheckAndHandleRevealState(shell->web_contents());
  if (!is_waiting) {
    content->Show();
    if (shell->web_contents()->GetRenderWidgetHostView()) {
      shell->web_contents()->GetRenderWidgetHostView()->Focus();
    }
  }
}

void ShellPlatformDelegate::RevealShell(Shell* shell) {
  DCHECK(base::Contains(shell_data_map_, shell));
  ShellData& shell_data = shell_data_map_.at(shell);
  if (!shell_data.window) {
    CreatePlatformWindowInternal(shell, shell_data.initial_size_);
    SetContents(shell);
  } else {
#if defined(USE_AURA) && BUILDFLAG(IS_STARBOARD)
    if (platform_ && platform_->aura && platform_->aura->host()) {
      auto* host_platform =
          static_cast<aura::WindowTreeHostPlatform*>(platform_->aura->host());
      auto* platform_window = static_cast<ui::PlatformWindowStarboard*>(
          host_platform->platform_window());
      if (platform_window) {
        platform_window->Restore();
      }
    }
#endif
  }
}

#if defined(USE_AURA) && BUILDFLAG(IS_STARBOARD)
void ShellPlatformDelegate::MapWindowShell(Shell* shell) {
  if (!platform_ || !platform_->aura || !platform_->aura->host()) {
    return;
  }
  auto* host =
      static_cast<aura::WindowTreeHostPlatform*>(platform_->aura->host());
  if (host->compositor()) {
    host->compositor()->SetVisible(true);
  }
  host->Show();
  if (host->window()) {
    host->window()->Show();
  }
  if (host->platform_window()) {
    host->platform_window()->Show(true);
  }
}
#endif  // defined(USE_AURA) && BUILDFLAG(IS_STARBOARD)

void ShellPlatformDelegate::ConcealShell(Shell* shell) {
  if (shell && shell->web_contents()) {
    shell->web_contents()->WasHidden();
  }
  if (!platform_ || !platform_->aura || !platform_->aura->host()) {
    return;
  }
  if (platform_->aura->host()->compositor()) {
    platform_->aura->host()->compositor()->SetVisible(false);
  }
  platform_->aura->host()->Hide();
}

#if defined(USE_AURA) && BUILDFLAG(IS_STARBOARD)
void ShellPlatformDelegate::DidCreateOrAttachWebContents(
    Shell* shell,
    WebContents* web_contents) {
  if (!is_visible_) {
    TrackPreviouslyVisibleWebContents(web_contents);
  }
  if (is_visible_ && !IsWaitingForRevealAck() && platform_ && platform_->aura) {
    platform_->aura->ShowWindow();
  }
}
#endif  // defined(USE_AURA) && BUILDFLAG(IS_STARBOARD)

void ShellPlatformDelegate::LoadSplashScreenContents(Shell* shell) {
  if (!shell || !shell->splash_screen_web_contents()) {
    return;
  }
  aura::Window* splash_view =
      shell->splash_screen_web_contents()->GetNativeView();
  if (!platform_ || !platform_->aura || !splash_view) {
    return;
  }
  aura::Window* parent = platform_->aura->host()->window();
  if (parent && !parent->Contains(splash_view)) {
    parent->AddChild(splash_view);
  }
  WebContentsSetBackgroundColor::CreateForWebContentsWithColor(
      shell->splash_screen_web_contents(), SK_ColorTRANSPARENT);
  splash_view->Show();
  platform_->aura->host()->Show();
}

void ShellPlatformDelegate::UpdateContents(Shell* shell) {
  SetContents(shell);
}

void ShellPlatformDelegate::ResizeWebContent(Shell* shell,
                                             const gfx::Size& content_size) {
  if (shell && shell->web_contents() &&
      shell->web_contents()->GetRenderWidgetHostView()) {
    shell->web_contents()->GetRenderWidgetHostView()->SetSize(content_size);
  }
}

void ShellPlatformDelegate::SetTitle(Shell* shell,
                                     const std::u16string& title) {}

void ShellPlatformDelegate::MainFrameCreated(Shell* shell) {}

bool ShellPlatformDelegate::DestroyShell(Shell* shell) {
  return false;  // Shell destroys itself.
}

}  // namespace content
