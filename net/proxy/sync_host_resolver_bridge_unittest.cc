// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/proxy/sync_host_resolver_bridge.h"

#include "base/thread.h"
#include "net/base/address_list.h"
#include "net/base/net_errors.h"
#include "net/base/net_log.h"
#include "net/base/test_completion_callback.h"
#include "net/proxy/proxy_info.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace net {

namespace {

// This implementation of HostResolver allows blocking until a resolve request
// has been received. The resolve requests it receives will never be completed.
class BlockableHostResolver : public HostResolver {
 public:
  BlockableHostResolver()
      : event_(true, false),
        was_request_cancelled_(false) {
  }

  virtual int Resolve(const RequestInfo& info,
                      AddressList* addresses,
                      CompletionCallback* callback,
                      RequestHandle* out_req,
                      const BoundNetLog& net_log) {
    EXPECT_TRUE(callback);
    EXPECT_TRUE(out_req);
    *out_req = reinterpret_cast<RequestHandle*>(1);  // Magic value.

    // Indicate to the caller that a request was received.
    event_.Signal();

    // We return ERR_IO_PENDING, as this request will NEVER be completed.
    // Expectation is for the caller to later cancel the request.
    return ERR_IO_PENDING;
  }

  virtual void CancelRequest(RequestHandle req) {
    EXPECT_EQ(reinterpret_cast<RequestHandle*>(1), req);
    was_request_cancelled_ = true;
  }

  virtual void AddObserver(Observer* observer) {
    NOTREACHED();
  }

  virtual void RemoveObserver(Observer* observer) {
    NOTREACHED();
  }

  // Waits until Resolve() has been called.
  void WaitUntilRequestIsReceived() {
    event_.Wait();
  }

  bool was_request_cancelled() const {
    return was_request_cancelled_;
  }

 private:
  // Event to notify when a resolve request was received.
  base::WaitableEvent event_;
  bool was_request_cancelled_;
};

// This implementation of ProxyResolver simply does a synchronous resolve
// on |host_resolver| in response to GetProxyForURL().
class SyncProxyResolver : public ProxyResolver {
 public:
  explicit SyncProxyResolver(HostResolver* host_resolver)
      : ProxyResolver(false), host_resolver_(host_resolver) {}

  virtual int GetProxyForURL(const GURL& url,
                             ProxyInfo* results,
                             CompletionCallback* callback,
                             RequestHandle* request,
                             const BoundNetLog& net_log) {
    EXPECT_FALSE(callback);
    EXPECT_FALSE(request);

    // Do a synchronous host resolve.
    HostResolver::RequestInfo info(url.host(), 80);
    AddressList addresses;
    int rv =
        host_resolver_->Resolve(info, &addresses, NULL, NULL, BoundNetLog());

    EXPECT_EQ(ERR_ABORTED, rv);

    return rv;
  }

  virtual void CancelRequest(RequestHandle request) {
    NOTREACHED();
  }

 private:
  virtual int SetPacScript(const GURL& pac_url,
                           const std::string& bytes_utf8,
                           CompletionCallback* callback) {
    NOTREACHED();
    return OK;
  }

  scoped_refptr<HostResolver> host_resolver_;
};

// This helper thread is used to create the circumstances for the deadlock.
// It is analagous to the "IO thread" which would be main thread running the
// network stack.
class IOThread : public base::Thread {
 public:
  explicit IOThread(HostResolver* async_resolver)
      : base::Thread("IO-thread"), async_resolver_(async_resolver) {
  }

  virtual ~IOThread() {
    Stop();
  }

 protected:
  virtual void Init() {
    // Create a synchronous host resolver that operates the async host
    // resolver on THIS thread.
    scoped_refptr<SyncHostResolverBridge> sync_resolver =
        new SyncHostResolverBridge(async_resolver_, message_loop());

    proxy_resolver_.reset(
        new SingleThreadedProxyResolverUsingBridgedHostResolver(
            new SyncProxyResolver(sync_resolver),
            sync_resolver));

    // Start an asynchronous request to the proxy resolver
    // (note that it will never complete).
    proxy_resolver_->GetProxyForURL(GURL("http://test/"), &results_,
                                    &callback_, &request_, BoundNetLog());
  }

  virtual void CleanUp() {
    // Cancel the outstanding request (note however that this will not
    // unblock the PAC thread though).
    proxy_resolver_->CancelRequest(request_);

    // Delete the single threaded proxy resolver.
    proxy_resolver_.reset();
  }

 private:
  HostResolver* async_resolver_;
  scoped_ptr<ProxyResolver> proxy_resolver_;

  // Data for the outstanding request to the single threaded proxy resolver.
  TestCompletionCallback callback_;
  ProxyInfo results_;
  ProxyResolver::RequestHandle request_;
};

// Test that a deadlock does not happen during shutdown when a host resolve
// is outstanding on the SyncHostResolverBridge.
// This is a regression test for http://crbug.com/41244.
TEST(SingleThreadedProxyResolverWithBridgedHostResolverTest, ShutdownDeadlock) {
  // This (async) host resolver will outlive the thread that is operating it
  // synchronously.
  scoped_refptr<BlockableHostResolver> host_resolver =
      new BlockableHostResolver();

  {
    IOThread io_thread(host_resolver.get());
    base::Thread::Options options;
    options.message_loop_type = MessageLoop::TYPE_IO;
    ASSERT_TRUE(io_thread.StartWithOptions(options));

    // Wait until the host resolver receives a request (this means that the
    // PAC thread is now blocked, waiting for the async response from the
    // host resolver.
    host_resolver->WaitUntilRequestIsReceived();

    // Now upon exitting this scope, the IOThread is destroyed -- this will
    // stop the IOThread, which will in turn delete the
    // SingleThreadedProxyResolver, which in turn will stop its internal
    // PAC thread (which is currently blocked waiting on the host resolve which
    // is running on IOThread).
  }

  // During the teardown sequence of the single threaded proxy resolver,
  // the outstanding host resolve should have been cancelled.
  EXPECT_TRUE(host_resolver->was_request_cancelled());
}

}  // namespace

}  // namespace net

