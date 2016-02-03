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

#include "cobalt/webdriver/session_driver.h"

#include "base/logging.h"
#include "cobalt/base/log_message_handler.h"

namespace cobalt {
namespace webdriver {
namespace {

const char kBrowserLog[] = "browser";

protocol::LogEntry::LogLevel SeverityToLogLevel(int severity) {
  switch (severity) {
    case logging::LOG_INFO:
      return protocol::LogEntry::kInfo;
    case logging::LOG_WARNING:
    case logging::LOG_ERROR:
    case logging::LOG_ERROR_REPORT:
      return protocol::LogEntry::kWarning;
    case logging::LOG_FATAL:
      return protocol::LogEntry::kSevere;
  }
  return protocol::LogEntry::kInfo;
}

}  // namespace

SessionDriver::SessionDriver(
    const protocol::SessionId& session_id,
    const NavigateCallback& navigate_callback,
    const CreateWindowDriverCallback& create_window_driver_callback)
    : session_id_(session_id),
      capabilities_(protocol::Capabilities::CreateActualCapabilities()),
      navigate_callback_(navigate_callback),
      create_window_driver_callback_(create_window_driver_callback),
      next_window_id_(0),
      logging_callback_id_(0) {
  logging_callback_id_ = base::LogMessageHandler::GetInstance()->AddCallback(
      base::Bind(&SessionDriver::LogMessageHandler, base::Unretained(this)));
  window_driver_ = create_window_driver_callback_.Run(GetUniqueWindowId());
}

SessionDriver::~SessionDriver() {
  // No more calls to LogMessageHandler will be made after this, so we can
  // safely be destructed.
  base::LogMessageHandler::GetInstance()->RemoveCallback(logging_callback_id_);
}

WindowDriver* SessionDriver::GetWindow(const protocol::WindowId& window_id) {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (protocol::WindowId::IsCurrent(window_id) ||
      window_driver_->window_id() == window_id) {
    return window_driver_.get();
  } else {
    return NULL;
  }
}

util::CommandResult<protocol::Capabilities> SessionDriver::GetCapabilities() {
  return util::CommandResult<protocol::Capabilities>(capabilities_);
}

util::CommandResult<protocol::WindowId>
SessionDriver::GetCurrentWindowHandle() {
  return util::CommandResult<protocol::WindowId>(window_driver_->window_id());
}

util::CommandResult<std::vector<protocol::WindowId> >
SessionDriver::GetWindowHandles() {
  typedef util::CommandResult<std::vector<protocol::WindowId> > CommandResult;
  // There is only one window, so just return a list of that.
  std::vector<protocol::WindowId> window_handles;
  window_handles.push_back(window_driver_->window_id());
  return CommandResult(window_handles);
}

util::CommandResult<void> SessionDriver::Navigate(const GURL& url) {
  DCHECK(thread_checker_.CalledOnValidThread());

  protocol::WindowId current_id = window_driver_->window_id();
  // Destroy the window_driver here to ensure there are no handles to anything
  // that will be destroyed when navigating.
  window_driver_.reset();
  base::WaitableEvent finished_event(true, false);
  navigate_callback_.Run(url, base::Bind(&base::WaitableEvent::Signal,
                                         base::Unretained(&finished_event)));
  // TODO(***REMOVED***): Implement timeout logic.
  finished_event.Wait();
  // Create a new WindowDriver using the same ID. Even though we've created a
  // new Window and WindowDriver, it should appear as though the navigation
  // happened within the same window.
  window_driver_ = create_window_driver_callback_.Run(current_id);

  return util::CommandResult<void>(protocol::Response::kSuccess);
}

util::CommandResult<std::vector<std::string> > SessionDriver::GetLogTypes() {
  DCHECK(thread_checker_.CalledOnValidThread());
  typedef util::CommandResult<std::vector<std::string> > CommandResult;

  std::vector<std::string> log_types;
  // Only "browser" log is supported.
  log_types.push_back(kBrowserLog);
  return CommandResult(log_types);
}

util::CommandResult<std::vector<protocol::LogEntry> > SessionDriver::GetLog(
    const protocol::LogType& type) {
  DCHECK(thread_checker_.CalledOnValidThread());
  typedef util::CommandResult<std::vector<protocol::LogEntry> > CommandResult;
  // Return an empty log vector for unsupported log types.
  CommandResult result((LogEntryVector()));
  if (type.type() == kBrowserLog) {
    base::AutoLock auto_lock(log_lock_);
    result = CommandResult(log_entries_);
    log_entries_.clear();
  }
  return result;
}

util::CommandResult<std::string> SessionDriver::GetAlertText() {
  return util::CommandResult<std::string>(
      protocol::Response::kNoAlertOpenError);
}

util::CommandResult<void> SessionDriver::SwitchToWindow(
    const protocol::WindowId& window_id) {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (window_id == window_driver_->window_id()) {
    return util::CommandResult<void>(protocol::Response::kSuccess);
  } else {
    return util::CommandResult<void>(protocol::Response::kNoSuchWindow);
  }
}

protocol::WindowId SessionDriver::GetUniqueWindowId() {
  DCHECK(thread_checker_.CalledOnValidThread());
  std::string window_id = base::StringPrintf("window-%d", next_window_id_++);
  return protocol::WindowId(window_id);
}

bool SessionDriver::LogMessageHandler(int severity, const char* file, int line,
                                      size_t message_start,
                                      const std::string& str) {
  // Could be called from an arbitrary thread.
  base::Time log_time = base::Time::Now();
  protocol::LogEntry::LogLevel level = SeverityToLogLevel(severity);

  base::AutoLock auto_lock(log_lock_);
  log_entries_.push_back(protocol::LogEntry(log_time, level, str));
  // Don't capture this log entry - give other handlers a shot at it.
  return false;
}

}  // namespace webdriver
}  // namespace cobalt
