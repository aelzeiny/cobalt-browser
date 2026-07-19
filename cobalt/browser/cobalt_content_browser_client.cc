// Copyright 2024 The Cobalt Authors. All Rights Reserved.
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

#include "cobalt/browser/cobalt_content_browser_client.h"

#include <cstdlib>
#include <string>

#include "base/base_switches.h"
#include "base/command_line.h"
#include "base/debug/dump_without_crashing.h"
#include "base/feature_list.h"
#include "base/files/file_path.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/hash/hash.h"
#include "base/i18n/rtl.h"
#include "base/metrics/field_trial_params.h"
#include "base/metrics/histogram_functions.h"
#include "base/metrics/histogram_macros.h"
#include "base/path_service.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "base/timer/elapsed_timer.h"
#include "cc/base/switches.h"
#include "cobalt/browser/cobalt_browser_interface_binders.h"
#include "cobalt/browser/cobalt_browser_main_parts.h"
#include "cobalt/browser/cobalt_secure_navigation_throttle.h"
#include "cobalt/browser/cobalt_web_contents_observer.h"
#include "cobalt/browser/command_line_logger.h"
#include "cobalt/browser/constants/cobalt_experiment_names.h"
#include "cobalt/browser/features.h"
#include "cobalt/browser/global_features.h"
#include "cobalt/browser/h5vcc_settings_impl.h"
#include "cobalt/browser/idle_memory_purger.h"
#include "cobalt/browser/lifecycle/cobalt_lifecycle_manager.h"
#include "cobalt/browser/memory_experiments/memory_experiment_features.h"
#include "cobalt/browser/metrics/cobalt_metrics_services_manager_client.h"
#include "cobalt/browser/mojom/h5vcc_settings.mojom.h"
#include "cobalt/browser/switches.h"
#include "cobalt/browser/user_agent/user_agent_platform_info.h"
#include "cobalt/common/features/starboard_features_initialization.h"
#include "cobalt/media/service/platform_window_provider_service.h"
#include "cobalt/shell/browser/shell.h"
#include "cobalt/shell/common/shell_paths.h"
#include "cobalt/shell/common/shell_switches.h"
#include "cobalt/shell/common/url_constants.h"
#include "cobalt/version.h"
#include "components/embedder_support/user_agent_utils.h"
#include "components/metrics/metrics_state_manager.h"
#include "components/metrics_services_manager/metrics_services_manager.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/pref_service_factory.h"
#include "components/variations/pref_names.h"
#include "components/variations/service/variations_service.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_switch_dependent_feature_overrides.h"
#include "content/public/common/content_switches.h"
#include "gpu/command_buffer/service/gpu_switches.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"
#include "services/network/public/cpp/features.h"
#include "services/network/public/mojom/network_context.mojom.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_registry.h"
#include "third_party/blink/public/common/switches.h"
#include "third_party/blink/public/common/web_preferences/web_preferences.h"

#if BUILDFLAG(IS_STARBOARD)
#include "cobalt/browser/h5vcc_system/h5vcc_system_impl_base.h"
#endif

#if BUILDFLAG(USE_EVERGREEN)
#include "cobalt/updater/updater_module.h"  //nogncheck
#include "content/public/browser/storage_partition.h"
#endif  // BUILDFLAG(USE_EVERGREEN)

#if BUILDFLAG(IS_ANDROID)
#include "base/android/locale_utils.h"
#include "cobalt/android/browser_jni_headers/CobaltContentBrowserClient_jni.h"
#endif  // BUILDFLAG(IS_ANDROID)

#if !BUILDFLAG(IS_ANDROIDTV)
#if BUILDFLAG(IS_STARBOARD)
#include "starboard/extension/crash_handler.h"
#include "starboard/system.h"
#elif BUILDFLAG(IS_IOS_TVOS)
#include "cobalt/browser/cobalt_crash_annotations.h"  // nogncheck
#endif                                                // BUILDFLAG(IS_STARBOARD)
#endif  // !BUILDFLAG(IS_ANDROIDTV)

namespace cobalt {

namespace {

constexpr base::FilePath::CharType kCacheDirname[] = FILE_PATH_LITERAL("Cache");
constexpr base::FilePath::CharType kCookieFilename[] =
    FILE_PATH_LITERAL("Cookies");
constexpr base::FilePath::CharType kNetworkDataDirname[] =
    FILE_PATH_LITERAL("Network");
constexpr base::FilePath::CharType kNetworkPersistentStateFilename[] =
    FILE_PATH_LITERAL("Network Persistent State");
constexpr base::FilePath::CharType kSCTAuditingPendingReportsFileName[] =
    FILE_PATH_LITERAL("SCT Auditing Pending Reports");
constexpr base::FilePath::CharType kTransportSecurityPersisterFilename[] =
    FILE_PATH_LITERAL("TransportSecurity");
constexpr base::FilePath::CharType kTrustTokenFilename[] =
    FILE_PATH_LITERAL("Trust Tokens");

#if !BUILDFLAG(IS_ANDROIDTV)
// This value is expected by offline data processing and should not be changed.
constexpr const char kUserAgentAnnotationKey[] = "user_agent_string";
#endif  // !BUILDFLAG(IS_ANDROIDTV)

void BindPlatformWindowProviderService(
    uint64_t window_handle,
    mojo::PendingReceiver<cobalt::media::mojom::PlatformWindowProvider>
        receiver) {
  mojo::MakeSelfOwnedReceiver(
      std::make_unique<cobalt::media::PlatformWindowProviderService>(
          base::BindRepeating([](uint64_t handle) { return handle; },
                              window_handle)),
      std::move(receiver));
}

void ParseAndApplyH5vccSettings(std::string_view settings_value,
                                GlobalFeatures* global_features) {
  std::vector<std::string> pairs = base::SplitString(
      settings_value, ";", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  for (const std::string& pair : pairs) {
    size_t eq_pos = pair.find('=');
    if (eq_pos == std::string::npos || eq_pos == 0) {
      LOG(WARNING) << "Skipping value: pair=" << pair;
      continue;
    }
    std::string_view pair_view(pair);
    std::string_view key =
        base::TrimWhitespaceASCII(pair_view.substr(0, eq_pos), base::TRIM_ALL);
    std::string_view val_str =
        base::TrimWhitespaceASCII(pair_view.substr(eq_pos + 1), base::TRIM_ALL);
    int64_t int_val = 0;
    if (base::StringToInt64(val_str, &int_val)) {
      global_features->SetSettings(key, int_val);
    } else {
      global_features->SetSettings(key, std::string(val_str));
    }
  }
}

}  // namespace

void ParseAndApplyH5vccSettingsForTesting(std::string_view settings_value,
                                          GlobalFeatures* global_features) {
  ParseAndApplyH5vccSettings(settings_value, global_features);
}

#if BUILDFLAG(IS_ANDROID)
static void JNI_CobaltContentBrowserClient_FlushCookiesAndLocalStorage(
    JNIEnv*) {
  auto* client = CobaltContentBrowserClient::Get();
  if (!client) {
    return;
  }
  client->FlushCookiesAndLocalStorage(base::DoNothing());
}

static void JNI_CobaltContentBrowserClient_DispatchBlur(JNIEnv*) {
  auto* client = CobaltContentBrowserClient::Get();
  if (!client) {
    return;
  }
  client->DispatchBlur();
}

static void JNI_CobaltContentBrowserClient_DispatchFocus(JNIEnv*) {
  auto* client = CobaltContentBrowserClient::Get();
  if (!client) {
    return;
  }
  client->DispatchFocus();
}
#endif  // BUILDFLAG(IS_ANDROID)

std::string GetCobaltUserAgent() {
  const UserAgentPlatformInfo platform_info;
  static const std::string user_agent_str = platform_info.ToString();
  return user_agent_str;
}

blink::UserAgentMetadata GetCobaltUserAgentMetadata() {
  blink::UserAgentMetadata metadata;
  const UserAgentPlatformInfo platform_info;
  metadata.brand_version_list.emplace_back(platform_info.brand().value_or(""),
                                           COBALT_MAJOR_VERSION);
  metadata.brand_full_version_list.emplace_back(
      platform_info.brand().value_or(""), platform_info.cobalt_version());
  metadata.full_version = platform_info.cobalt_version();
  metadata.platform = "Starboard";
  metadata.architecture = embedder_support::GetCpuArchitecture();
  metadata.model = embedder_support::BuildModelInfo();

  metadata.bitness = embedder_support::GetCpuBitness();
  metadata.wow64 = embedder_support::IsWoW64();

  return metadata;
}

CobaltContentBrowserClient::CobaltContentBrowserClient(
    absl::optional<int64_t> startup_timestamp,
    const std::string& deep_link,
    bool is_visible)
    : startup_timestamp_(startup_timestamp),
      deep_link_(deep_link),
      is_visible_(is_visible) {
  COBALT_DETACH_FROM_THREAD(thread_checker_);
#if BUILDFLAG(IS_STARBOARD)
  // TODO: b/476434249 - Revisit if Cobalt supports multiple tabs/windows.
  ui::PlatformWindowStarboard::SetWindowCreatedCallback(
      base::BindRepeating(&CobaltContentBrowserClient::OnSbWindowCreated,
                          weak_factory_.GetWeakPtr()));
  ui::PlatformWindowStarboard::SetWindowDestroyedCallback(
      base::BindRepeating(&CobaltContentBrowserClient::OnSbWindowDestroyed,
                          weak_factory_.GetWeakPtr()));
#endif  // BUILDFLAG(IS_STARBOARD)
}

CobaltContentBrowserClient::~CobaltContentBrowserClient() {
#if BUILDFLAG(IS_STARBOARD)
  ui::PlatformWindowStarboard::ClearWindowCreatedCallback();
  ui::PlatformWindowStarboard::ClearWindowDestroyedCallback();
#endif  // BUILDFLAG(IS_STARBOARD)
}

// static
CobaltContentBrowserClient* CobaltContentBrowserClient::Get() {
  return static_cast<CobaltContentBrowserClient*>(
      content::ShellContentBrowserClient::Get());
}

std::unique_ptr<content::BrowserMainParts>
CobaltContentBrowserClient::CreateBrowserMainParts(
    bool /* is_integration_test */) {
  CHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  auto browser_main_parts =
      std::make_unique<CobaltBrowserMainParts>(deep_link_, is_visible_);
  set_browser_main_parts(browser_main_parts.get());
  return browser_main_parts;
}

std::unique_ptr<content::DevToolsManagerDelegate>
CobaltContentBrowserClient::CreateDevToolsManagerDelegate() {
#if defined(COBALT_IS_RELEASE_BUILD)
  return nullptr;
#else
  return content::ShellContentBrowserClient::CreateDevToolsManagerDelegate();
#endif
}

void CobaltContentBrowserClient::CreateThrottlesForNavigation(
    content::NavigationThrottleRegistry& registry) {
  content::NavigationHandle& navigation_handle = registry.GetNavigationHandle();
  registry.AddThrottle(
      std::make_unique<content::CobaltSecureNavigationThrottle>(
          &navigation_handle));
}

content::GeneratedCodeCacheSettings
CobaltContentBrowserClient::GetGeneratedCodeCacheSettings(
    content::BrowserContext* context) {
  // Default compiled javascript quota in Cobalt 25 is 3 MB:
  // https://github.com/youtube/cobalt/blob/3ccdb04a5e36c2597fe7066039037eabf4906ba5/cobalt/network/disk_cache/resource_type.cc#L72
  // When enable-optimized-v8-code-cache switch is set, increase to 5 MB for
  // YouTube TV.
  size_t size = 3 * 1024 * 1024;
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          "enable-optimized-v8-code-cache")) {
    size = 5 * 1024 * 1024;
  }
  base::FilePath cache_path;
  CHECK(base::PathService::Get(base::DIR_CACHE, &cache_path));
  return content::GeneratedCodeCacheSettings(/*enabled=*/true, size,
                                             cache_path);
}

std::string CobaltContentBrowserClient::GetApplicationLocale() {
  CHECK_CALLED_ON_VALID_THREAD(thread_checker_);
#if BUILDFLAG(IS_ANDROID)
  return base::android::GetDefaultLocaleString();
#else
  return base::i18n::GetConfiguredLocale();
#endif
}

std::string CobaltContentBrowserClient::GetUserAgent() {
  CHECK_CALLED_ON_VALID_THREAD(thread_checker_);
#if !defined(OFFICIAL_BUILD)
  const auto custom_ua = embedder_support::GetUserAgentFromCommandLine();
  if (custom_ua.has_value()) {
    return custom_ua.value();
  }
#endif  // !defined(OFFICIAL_BUILD)
  return GetCobaltUserAgent();
}

blink::UserAgentMetadata CobaltContentBrowserClient::GetUserAgentMetadata() {
  CHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  return GetCobaltUserAgentMetadata();
}

void CobaltContentBrowserClient::OverrideWebPreferences(
    content::WebContents* web_contents,
    content::SiteInstance& main_frame_site,
    blink::web_pref::WebPreferences* prefs) {
  CHECK_CALLED_ON_VALID_THREAD(thread_checker_);
#if !defined(COBALT_IS_RELEASE_BUILD)
  // Allow creating a ws: connection on a https: page to allow current
  // testing set up. See b/377410179.
  prefs->allow_running_insecure_content = true;
#endif  // !defined(COBALT_IS_RELEASE_BUILD)
  content::ShellContentBrowserClient::OverrideWebPreferences(
      web_contents, main_frame_site, prefs);
}

content::StoragePartitionConfig
CobaltContentBrowserClient::GetStoragePartitionConfigForSite(
    content::BrowserContext* browser_context,
    const GURL& site) {
  // Default to the browser-wide storage partition and override based on |site|
  // below.
  content::StoragePartitionConfig default_storage_partition_config =
      content::StoragePartitionConfig::CreateDefault(browser_context);

  return default_storage_partition_config;
}

void CobaltContentBrowserClient::ConfigureNetworkContextParams(
    content::BrowserContext* context,
    bool in_memory,
    const base::FilePath& relative_partition_path,
    network::mojom::NetworkContextParams* network_context_params,
    cert_verifier::mojom::CertVerifierCreationParams*
        cert_verifier_creation_params) {
  network_context_params->user_agent = GetCobaltUserAgent();
  network_context_params->enable_referrers = true;
  network_context_params->accept_language = GetApplicationLocale();

  auto cookie_manager_params = network::mojom::CookieManagerParams::New();
  cookie_manager_params->block_third_party_cookies = true;
  network_context_params->cookie_manager_params =
      std::move(cookie_manager_params);

  // Configure on-disk storage for non-off-the-record profiles. Off-the-record
  // profiles just use default behavior (in memory storage, default sizes).
  if (!in_memory) {
    network_context_params->file_paths =
        ::network::mojom::NetworkContextFilePaths::New();

    base::FilePath cache_path;
    CHECK(base::PathService::Get(base::DIR_CACHE, &cache_path));
    network_context_params->file_paths->http_cache_directory =
        cache_path.Append(kCacheDirname);

    // Runtime memory experiment ("CobaltMemCacheSweep"): bound the HTTP
    // cache instead of letting PreferredCacheSize() pick a desktop-shaped
    // size. This bounds the in-RAM cache index and open-entry overhead, and
    // limits flash wear on TV devices. When the experiment is disabled
    // (default), the field is left unset, matching upstream.
    if (base::FeatureList::IsEnabled(features::kCobaltMemCacheSweep)) {
      network_context_params->http_cache_max_size = 32 * 1024 * 1024;
    }

    base::FilePath user_data_dir =
        context->GetPath().Append(relative_partition_path);
    network_context_params->file_paths->data_directory =
        user_data_dir.Append(kNetworkDataDirname);
    network_context_params->file_paths->unsandboxed_data_path = user_data_dir;

    // Currently this just contains HttpServerProperties, but that will likely
    // change.
    network_context_params->file_paths->http_server_properties_file_name =
        base::FilePath(kNetworkPersistentStateFilename);
    network_context_params->file_paths->cookie_database_name =
        base::FilePath(kCookieFilename);
    network_context_params->file_paths->trust_token_database_name =
        base::FilePath(kTrustTokenFilename);

    // Always try to restore old session cookies.
    network_context_params->restore_old_session_cookies = true;
    network_context_params->persist_session_cookies = true;

    network_context_params->file_paths->transport_security_persister_file_name =
        base::FilePath(kTransportSecurityPersisterFilename);
    network_context_params->file_paths->sct_auditing_pending_reports_file_name =
        base::FilePath(kSCTAuditingPendingReportsFileName);
  }

  // Runtime memory experiment ("CobaltMemStripDesktop"): Domain
  // Reliability is desktop Google-services telemetry; keep it off on TV.
  // The mojom default is already false, but when the experiment is enabled
  // set it explicitly so the network service never creates a
  // DomainReliabilityMonitor for Cobalt. When disabled (default), the param
  // is left untouched, matching upstream.
  if (base::FeatureList::IsEnabled(features::kCobaltMemStripDesktop)) {
    network_context_params->enable_domain_reliability = false;
  }

  network_context_params->enable_certificate_reporting = true;

  network_context_params->sct_auditing_mode =
      network::mojom::SCTAuditingMode::kDisabled;

  // All consumers of the main NetworkContext must provide
  // NetworkAnonymizationKey / IsolationInfos, so storage can be isolated on a
  // per-site basis.
  network_context_params->require_network_anonymization_key = true;
}

bool CobaltContentBrowserClient::IsFirstPartySetsEnabled() {
  // Runtime memory experiment ("CobaltMemStripDesktop"): First-Party
  // Sets (Related Website Sets) is a desktop cookie feature with no use in a
  // single-app TV browser. There is no base::Feature for it in this tree;
  // the ContentBrowserClient default returns true, which makes
  // FirstPartySetsHandlerImplInstance load sets and exercise its sqlite
  // database. When the experiment is enabled, return false so the handler
  // stays disabled and the database is never opened. When disabled
  // (default), defer to the upstream default.
  //
  // Timing: the earliest callers in this tree are the network-service
  // creation (content/browser/network_service_instance_impl.cc) and
  // FirstPartySetsHandlerImplInstance creation during storage-partition
  // initialization -- both in browser-main-loop startup, after the feature
  // list was created in CobaltMainDelegate::PostEarlyInitialization(), so a
  // plain IsEnabled() check is safe here.
  if (base::FeatureList::IsEnabled(features::kCobaltMemStripDesktop)) {
    return false;
  }
  return ContentBrowserClient::IsFirstPartySetsEnabled();
}

void CobaltContentBrowserClient::OnWebContentsCreated(
    content::WebContents* web_contents) {
  CHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  if (web_contents->GetPrimaryMainFrame() &&
      web_contents->GetPrimaryMainFrame()->GetFrameName() ==
          content::kCobaltSplashMainFrameName) {
    // Don't observe WebContents if it's splash screen.
    VLOG(1) << "NativeSplash: Skip observing WebContents for "
               "kCobaltSplashMainFrameName.";
    return;
  }
  VLOG(1) << "NativeSplash: Observing main frame WebContents.";
  web_contents_observer_.reset(new CobaltWebContentsObserver(web_contents));
  // Runtime memory experiment ("CobaltMemIdlePurge"): sweep memory
  // caches whenever the user parks the app: no OS memory-pressure source
  // fires on Linux/Starboard, so without this the caches only ever grow over
  // multi-hour sessions. When the experiment is disabled (default), the
  // purger is never created and upstream behavior is unchanged.
  if (base::FeatureList::IsEnabled(features::kCobaltMemIdlePurge)) {
    idle_memory_purger_ = std::make_unique<IdleMemoryPurger>(web_contents);
  }
  // Initialize the lifecycle tracker for this WebContents to ensure we track
  // and register its frames (including the main frame) for lifecycle events
  // from the very start.
  CobaltLifecycleManager::GetInstance()->InitializeTracker(web_contents);
#if BUILDFLAG(USE_EVERGREEN)
  // Create the updater module singleton if not already created.
  auto* storage_partition =
      web_contents->GetPrimaryMainFrame()->GetStoragePartition();
  if (storage_partition && !updater::UpdaterModule::GetInstance()) {
    LOG(INFO) << "Creating UpdaterModule singleton.";
    updater::UpdaterModule::CreateInstance(
        storage_partition->GetURLLoaderFactoryForBrowserProcess(),
        GetUserAgent(), updater::kDefaultUpdateCheckDelay);
  }
#endif
}

void CobaltContentBrowserClient::RegisterBrowserInterfaceBindersForFrame(
    content::RenderFrameHost* render_frame_host,
    mojo::BinderMapWithContext<content::RenderFrameHost*>* map) {
  CHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  PopulateCobaltFrameBinders(startup_timestamp_, render_frame_host, map);
  ShellContentBrowserClient::RegisterBrowserInterfaceBindersForFrame(
      render_frame_host, map);
}

void CobaltContentBrowserClient::ExposeInterfacesToRenderer(
    service_manager::BinderRegistry* registry,
    blink::AssociatedInterfaceRegistry* associated_registry,
    content::RenderProcessHost* render_process_host) {
  ShellContentBrowserClient::ExposeInterfacesToRenderer(
      registry, associated_registry, render_process_host);
  registry->AddInterface<cobalt::mojom::H5vccSettings>(
      base::BindRepeating(&H5vccSettingsImpl::Create),
      base::SingleThreadTaskRunner::GetCurrentDefault());
}

void CobaltContentBrowserClient::WillCreateURLLoaderFactory(
    content::BrowserContext* browser_context,
    content::RenderFrameHost* frame,
    int render_process_id,
    URLLoaderFactoryType type,
    const url::Origin& request_initiator,
    const net::IsolationInfo& isolation_info,
    std::optional<int64_t> navigation_id,
    ukm::SourceIdObj ukm_source_id,
    network::URLLoaderFactoryBuilder& factory_builder,
    mojo::PendingRemote<network::mojom::TrustedURLLoaderHeaderClient>*
        header_client,
    bool* bypass_redirect_checks,
    bool* disable_secure_dns,
    network::mojom::URLLoaderFactoryOverridePtr* factory_override,
    scoped_refptr<base::SequencedTaskRunner> navigation_response_task_runner) {
  if (header_client) {
    mojo::MakeSelfOwnedReceiver(
        std::make_unique<browser::CobaltTrustedURLLoaderHeaderClient>(),
        header_client->InitWithNewPipeAndPassReceiver());
  }
}

void CobaltContentBrowserClient::DispatchBlur() {
  if (web_contents_observer_) {
    auto* web_contents = web_contents_observer_->web_contents();
    if (web_contents) {
      web_contents->GetRenderViewHost()->GetWidget()->Blur();
    }
  }
  auto start_time = std::make_unique<base::ElapsedTimer>();
  FlushCookiesAndLocalStorage(base::BindOnce(
      [](std::unique_ptr<base::ElapsedTimer> timer) {
        UMA_HISTOGRAM_TIMES("Cobalt.Storage.OnPause.FlushDuration",
                            timer->Elapsed());
      },
      std::move(start_time)));
}

void CobaltContentBrowserClient::DispatchFocus() {
  if (web_contents_observer_) {
    auto* web_contents = web_contents_observer_->web_contents();
    if (web_contents) {
      web_contents->GetRenderViewHost()->GetWidget()->Focus();
    }
  }
}

void CobaltContentBrowserClient::AddPendingWindowReceiver(
    mojo::PendingReceiver<cobalt::media::mojom::PlatformWindowProvider>
        receiver) {
  if (cached_sb_window_) {
    BindPlatformWindowProviderService(cached_sb_window_, std::move(receiver));
  } else {
    pending_window_receivers_.push_back(std::move(receiver));
  }
}

void CobaltContentBrowserClient::OnSbWindowCreated(SbWindow window) {
  // TODO: b/476434249 - Revisit if Cobalt supports multiple tabs/windows. This
  // assumes only single PlatformWindowStarboard() in Cobalt.
  CHECK(!cached_sb_window_);
  cached_sb_window_ = reinterpret_cast<uint64_t>(window);
#if BUILDFLAG(IS_STARBOARD)
  h5vcc_system::H5vccSystemImpl::SetPrimarySbWindow(window);
#endif
  for (auto& receiver : pending_window_receivers_) {
    BindPlatformWindowProviderService(cached_sb_window_, std::move(receiver));
  }
  pending_window_receivers_.clear();
}

void CobaltContentBrowserClient::OnSbWindowDestroyed(SbWindow window) {
  DCHECK_EQ(cached_sb_window_, reinterpret_cast<uint64_t>(window));
  cached_sb_window_ = 0;
#if BUILDFLAG(IS_STARBOARD)
  h5vcc_system::H5vccSystemImpl::SetPrimarySbWindow(kSbWindowInvalid);
#endif
}

void CobaltContentBrowserClient::FlushCookiesAndLocalStorage(
    base::OnceClosure callback) {
  if (!web_contents_observer_) {
    std::move(callback).Run();
    return;
  }
  auto* web_contents = web_contents_observer_->web_contents();
  CHECK(web_contents);
  content::RenderFrameHost* rfh = web_contents->GetPrimaryMainFrame();
  CHECK(rfh);
  auto* storage_partition = rfh->GetStoragePartition();
  CHECK(storage_partition);
  // Flushes localStorage.
  storage_partition->Flush();
  auto* cookie_manager = storage_partition->GetCookieManagerForBrowserProcess();
  CHECK(cookie_manager);
  cookie_manager->FlushCookieStore(std::move(callback));
}

void CobaltContentBrowserClient::SetUpCobaltFeaturesAndParams(
    base::FeatureList* feature_list) {
  // All Cobalt features are associated with the same field trial. This is for
  // easier feature param lookup.
  base::FieldTrial* cobalt_field_trial = base::FieldTrialList::CreateFieldTrial(
      kCobaltExperimentName, kCobaltGroupName);
  CHECK(cobalt_field_trial) << "Unexpected name conflict.";

  auto* global_features = GlobalFeatures::GetInstance();
  auto* experiment_config_manager =
      global_features->experiment_config_manager();
  auto config_type = experiment_config_manager->GetExperimentConfigType();
  if (config_type == ExperimentConfigType::kEmptyConfig) {
    return;
  }
  auto* experiment_config = global_features->experiment_config();
  const bool use_safe_config =
      (config_type == ExperimentConfigType::kSafeConfig);

  const base::Value::Dict& feature_map = experiment_config->GetDict(
      use_safe_config ? kSafeConfigFeatures : kExperimentConfigFeatures);
  const base::Value::Dict& param_map = experiment_config->GetDict(
      use_safe_config ? kSafeConfigFeatureParams
                      : kExperimentConfigFeatureParams);

  size_t features_applied = 0;
  for (const auto feature_name_and_value : feature_map) {
    if (feature_name_and_value.second.is_bool()) {
      auto override_value =
          feature_name_and_value.second.GetBool()
              ? base::FeatureList::OverrideState::OVERRIDE_ENABLE_FEATURE
              : base::FeatureList::OverrideState::OVERRIDE_DISABLE_FEATURE;
      feature_list->RegisterFieldTrialOverride(
          feature_name_and_value.first, override_value, cobalt_field_trial);
      features_applied++;
    } else {
      LOG(ERROR) << "Failed to apply override for feature "
                 << feature_name_and_value.first;
      base::debug::DumpWithoutCrashing();
    }
  }
  const bool has_invalid_feature_type = feature_map.size() != features_applied;
  base::UmaHistogramBoolean("Cobalt.Finch.HasInvalidFeatureType",
                            has_invalid_feature_type);
  base::UmaHistogramCounts100("Cobalt.Finch.NumFeaturesApplied",
                              static_cast<int>(features_applied));

  // Compound memory-experiment aliases (see cobalt/COBALT_MEMORY_EXPERIMENTS
  // .md): some experiments fan out to existing upstream features under their
  // own names. "CobaltMemAxAutodisable" and "CobaltMemParkableStrings" are
  // config-level aliases only -- they have no BASE_FEATURE declaration
  // anywhere and base::FeatureList::IsEnabled() is never called on them; the
  // fan-out below is their sole effect. "CobaltMemStripDesktop" additionally
  // exists as a declared feature (cobalt/browser/memory_experiments) that
  // ApplyMemoryExperimentSwitches() and other browser-side sites consume
  // directly.
  //
  // Explicit config entries win: any name the config set itself was already
  // registered above and must not be re-registered -- RegisterFieldTrialOverride()
  // DCHECKs when a feature name that already has a field-trial-associated
  // override is registered again (and RegisterOverride() is
  // first-override-wins regardless, so a second registration would be
  // ignored in release builds). IsFeatureOverridden() also covers overrides
  // that came from --enable-features/--disable-features or the
  // switch-dependent extra overrides, all of which were registered before
  // this function runs.
  const auto config_enables = [&feature_map](std::string_view feature_name) {
    return feature_map.FindBool(feature_name).value_or(false);
  };
  const auto register_compound_override =
      [&feature_list, &cobalt_field_trial](
          const std::string& feature_name,
          base::FeatureList::OverrideState override_state) {
        if (feature_list->IsFeatureOverridden(feature_name)) {
          return;
        }
        feature_list->RegisterFieldTrialOverride(feature_name, override_state,
                                                 cobalt_field_trial);
      };
  if (config_enables("CobaltMemStripDesktop")) {
    // Desktop-only allocation sources: Attribution Reporting and
    // FLEDGE/Protected Audience storage.
    register_compound_override(
        "ConversionMeasurement",
        base::FeatureList::OverrideState::OVERRIDE_DISABLE_FEATURE);
    register_compound_override(
        "InterestGroupStorage",
        base::FeatureList::OverrideState::OVERRIDE_DISABLE_FEATURE);
  }
  if (config_enables("CobaltMemAxAutodisable")) {
    // Tear down accessibility trees when no assistive technology consumes
    // accessibility events.
    register_compound_override(
        "AutoDisableAccessibility",
        base::FeatureList::OverrideState::OVERRIDE_ENABLE_FEATURE);
  }
  if (config_enables("CobaltMemParkableStrings")) {
    // Re-enable ParkableString aging in the (permanently) foreground TV app
    // so large strings such as JS source actually compress.
    register_compound_override(
        "LessAggressiveParkableString",
        base::FeatureList::OverrideState::OVERRIDE_DISABLE_FEATURE);
  }
  if (config_enables("CobaltMemCacheSweep")) {
    // Component-layer halves of the cache sweep: net/ (SSL client session
    // cache 1024 -> 32 entries) and sql/ (default page cache capped at 64
    // pages) cannot include the cobalt/ header that declares
    // "CobaltMemCacheSweep", and a feature name must have exactly one
    // BASE_FEATURE in the binary, so they declare distinct names that this
    // fan-out enables alongside the browser-side feature.
    register_compound_override(
        "CobaltMemCacheSweepNet",
        base::FeatureList::OverrideState::OVERRIDE_ENABLE_FEATURE);
    register_compound_override(
        "CobaltMemCacheSweepSql",
        base::FeatureList::OverrideState::OVERRIDE_ENABLE_FEATURE);
  }

  size_t params_applied = 0;
  base::FieldTrialParams params;
  for (const auto param_name_and_value : param_map) {
    if (param_name_and_value.second.is_string()) {
      params.emplace(param_name_and_value.first,
                     param_name_and_value.second.GetString());
      params_applied++;
    } else {
      LOG(ERROR) << "Failed to associate field trial param "
                 << param_name_and_value.first << " with string value "
                 << param_name_and_value.second;
      base::debug::DumpWithoutCrashing();
    }
  }
  const bool has_invalid_param_type = param_map.size() != params_applied;
  base::UmaHistogramBoolean("Cobalt.Finch.HasInvalidParamType",
                            has_invalid_param_type);
  base::UmaHistogramCounts100("Cobalt.Finch.NumParamsApplied",
                              static_cast<int>(params_applied));
  base::AssociateFieldTrialParams(kCobaltExperimentName, kCobaltGroupName,
                                  params);
}

void CobaltContentBrowserClient::ApplyMemoryExperimentSwitches() {
  // Bridges memory-experiment features whose consumers read command-line
  // switches (not base::FeatureList) into switch edits. This runs from
  // CreateFeatureListAndFieldTrials(), which the ContentMainDelegate invokes
  // in PostEarlyInitialization() (cobalt/app/cobalt_main_delegate.cc /
  // content/shell/app/shell_main_delegate.cc) -- i.e. before BrowserMain(),
  // before ShellBrowserMainParts::PreMainMessageLoopRun() starts the DevTools
  // HTTP handler (cobalt/shell/browser/shell_browser_main_parts.cc), and
  // before renderer/GPU/compositor initialization in single-process mode, so
  // all switch consumers below see the edited values.
  //
  // Note that base::CommandLine::AppendSwitch*() overwrites the stored value
  // for an existing switch key (last append wins for GetSwitchValue*()).
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();

  if (base::FeatureList::IsEnabled(features::kCobaltMemJsFlags)) {
    // Default --js-flags value applied by the Cobalt command-line
    // preprocessor. Keep in sync with GetCobaltParamSwitchDefaults() in
    // cobalt/app/cobalt_switch_defaults_starboard.cc. The string is
    // duplicated here because cobalt/app depends on cobalt/browser (not the
    // other way around), so this file cannot reference the preprocessor.
    static constexpr char kCobaltDefaultJsFlags[] =
        "--no-decommit-pooled-pages "
        "--optimize-for-size "
        "--initial-old-space-size=64 "
        "--max-old-space-size=512 "
        "--disable-optimizing-compilers "
        "--no-sparkplug "
        "--no-concurrent-marking";
    // The same defaults with the initial old space lowered to 16MB. A TV
    // app's live JS heap is typically 20-40MB; a small initial old space
    // triggers the first major GC earlier and lowers the plateau.
    static constexpr char kCobaltMemJsFlagsDefaults[] =
        "--no-decommit-pooled-pages "
        "--optimize-for-size "
        "--initial-old-space-size=16 "
        "--max-old-space-size=512 "
        "--disable-optimizing-compilers "
        "--no-sparkplug "
        "--no-concurrent-marking";
    const std::string current_js_flags =
        command_line->GetSwitchValueASCII(blink::switches::kJavaScriptFlags);
    std::string merged_js_flags;
    if (current_js_flags.empty() || current_js_flags == kCobaltDefaultJsFlags) {
      // No platform override: just swap the initial old space 64 -> 16.
      merged_js_flags = kCobaltMemJsFlagsDefaults;
    } else {
      // The platform provided its own --js-flags value. Merge
      // defaults-first-platform-wins: V8 parses flags left to right (and gin
      // splits --js-flags on spaces), so the platform's values override the
      // Cobalt defaults on a per-flag basis instead of silently replacing
      // the whole TV-tuned string.
      merged_js_flags =
          std::string(kCobaltMemJsFlagsDefaults) + " " + current_js_flags;
    }
    command_line->AppendSwitchASCII(blink::switches::kJavaScriptFlags,
                                    merged_js_flags);
  }

  if (base::FeatureList::IsEnabled(features::kCobaltMemImageCache)) {
    // Limit the decoded-image working set to 24MB
    // (cc::ImageDecodeCacheUtils reads this switch). The inert
    // "LimitImageDecodeCacheSize:mb/24" token that the switch defaults put
    // in --enable-features stays as-is; no feature by that name exists, so
    // it is ignored.
    command_line->AppendSwitchASCII(
        ::switches::kDecodedImageWorkingSetBudgetBytes, "25165824");
  }

  if (base::FeatureList::IsEnabled(features::kCobaltMemGpuBudget)) {
    // Lower the compositor GPU memory budget from the Cobalt default of 64MB
    // to 32MB.
    command_line->AppendSwitchASCII(::switches::kForceGpuMemAvailableMb, "32");
  }

  if (base::FeatureList::IsEnabled(features::kCobaltMemStripDesktop)) {
    // Make remote DevTools opt-in: drop the default --remote-debugging-port
    // and --remote-allow-origins so the DevTools HTTP server (and its
    // discovery/socket machinery) never starts. This runs before
    // ShellBrowserMainParts::PreMainMessageLoopRun() checks the switch.
    command_line->RemoveSwitch(::switches::kRemoteDebuggingPort);
    command_line->RemoveSwitch(::switches::kRemoteAllowOrigins);

    // TV devices have no FIDO transports; disabling the WebAuthn API (blink
    // runtime feature "WebAuth") prevents page-triggered device/fido
    // authenticator discovery churn. Preserve any existing value
    // (comma-joined list).
    std::string disable_blink_features =
        command_line->GetSwitchValueASCII(::switches::kDisableBlinkFeatures);
    if (!disable_blink_features.empty()) {
      disable_blink_features += ",";
    }
    disable_blink_features += "WebAuth";
    command_line->AppendSwitchASCII(::switches::kDisableBlinkFeatures,
                                    disable_blink_features);
  }

  if (base::FeatureList::IsEnabled(features::kCobaltMemThreadStacks)) {
    // Cap the default stack size of Chromium-created threads at 256KiB
    // instead of the 8MiB glibc default. The consumer
    // (base/threading/platform_thread_linux_base.cc) STRING-SCANS the raw
    // --enable-features switch value for "ReduceAndroidThreadStackSize"
    // rather than using base::FeatureList (threads are created before the
    // feature list exists), which is why this experiment must edit the
    // switch instead of relying on the field-trial override alone. Threads
    // created before this point (early browser threads) keep the platform
    // default stack size; only threads created afterwards are affected.
    // Appending here does not re-init the feature list; only the raw-switch
    // scanner observes the change.
    std::string enable_features =
        command_line->GetSwitchValueASCII(::switches::kEnableFeatures);
    if (enable_features.find("ReduceAndroidThreadStackSize") ==
        std::string::npos) {
      if (!enable_features.empty()) {
        enable_features += ",";
      }
      enable_features += "ReduceAndroidThreadStackSize";
      command_line->AppendSwitchASCII(::switches::kEnableFeatures,
                                      enable_features);
    }
  }
}

void CobaltContentBrowserClient::CreateFeatureListAndFieldTrials() {
  auto* global_features = GlobalFeatures::GetInstance();
  global_features->metrics_services_manager()->InstantiateFieldTrialList();
  // Mark the session as unclean at startup. If the session exits cleanly, it
  // will be marked as clean in CobaltMetricsServiceClient's destructor.
  global_features->metrics_services_manager_client()
      ->GetMetricsStateManager()
      ->LogHasSessionShutdownCleanly(false, false);

  auto feature_list = std::make_unique<base::FeatureList>();

  auto accessor = feature_list->ConstructAccessor();
  GlobalFeatures::GetInstance()->set_accessor(std::move(accessor));

  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();

  if (command_line.HasSwitch(switches::kEnableH5vccSettings)) {
    ParseAndApplyH5vccSettings(
        command_line.GetSwitchValueASCII(switches::kEnableH5vccSettings),
        global_features);
  }
  // Overrides for content/common and lower layers' switches.
  std::vector<base::FeatureList::FeatureOverrideInfo> feature_overrides =
      content::GetSwitchDependentFeatureOverrides(command_line);

  feature_list->InitFromCommandLine(
      command_line.GetSwitchValueASCII(::switches::kEnableFeatures),
      command_line.GetSwitchValueASCII(::switches::kDisableFeatures));

  // This needs to happen here: After the InitFromCommandLine() call,
  // because the explicit cmdline --disable-features and --enable-features
  // should take precedence over these extra overrides. Before the call to
  // SetInstance(), because overrides cannot be registered after the FeatureList
  // instance is set.
  feature_list->RegisterExtraFeatureOverrides(feature_overrides);

  SetUpCobaltFeaturesAndParams(feature_list.get());

  base::FeatureList::SetInstance(std::move(feature_list));
  UMA_HISTOGRAM_BOOLEAN(
      "Cobalt.Features.TestFinchFeature",
      base::FeatureList::IsEnabled(features::kTestFinchFeature));
  base::UmaHistogramSparse("Cobalt.Features.TestFinchFeatureParam",
                           base::Hash(features::kTestFinchFeatureParam.Get()));

  // Bridge memory-experiment features into command-line switch edits. Must
  // run after base::FeatureList::SetInstance() (it calls IsEnabled()), and
  // before the CobaltCommandLine logs below (so they show the final switch
  // values) and before InitializeStarboardFeatures().
  ApplyMemoryExperimentSwitches();

  LOG(INFO) << "CobaltCommandLine: enable_features=["
            << command_line.GetSwitchValueASCII(::switches::kEnableFeatures)
            << "], disable_features=["
            << command_line.GetSwitchValueASCII(::switches::kDisableFeatures)
            << "], enable_h5vcc_settings=["
            << command_line.GetSwitchValueASCII(switches::kEnableH5vccSettings)
            << "]";
  LOG(INFO) << "CobaltCommandLine: "
            << CommandLineSwitchesToString(
                   *base::CommandLine::ForCurrentProcess());

  // Push the initialized features and params down to Starboard.
  features::InitializeStarboardFeatures();
}

#if !BUILDFLAG(IS_ANDROIDTV)
// TODO: b/411198914 - Consider making this implementation platform-agnostic by
// using the mojom::CrashAnnotator interface.
void CobaltContentBrowserClient::SetUserAgentCrashAnnotation() {
  std::string user_agent_string = GetUserAgent();
  if (user_agent_string.empty()) {
    LOG(ERROR) << "Not setting the user agent annotation because the string is "
               << "empty";
    return;
  }

#if BUILDFLAG(IS_STARBOARD)
  auto crash_handler_extension =
      static_cast<const CobaltExtensionCrashHandlerApi*>(
          SbSystemGetExtension(kCobaltExtensionCrashHandlerName));
  if (crash_handler_extension && crash_handler_extension->version >= 2) {
    crash_handler_extension->SetString(kUserAgentAnnotationKey,
                                       user_agent_string.c_str());
  } else {
    LOG(ERROR) << "The plaform does not implement (the required version of) "
               << "the CrashHandler Starboard extension; not setting the user "
               << "agent annotation";
  }
#elif BUILDFLAG(IS_IOS_TVOS)
  cobalt::browser::CobaltCrashAnnotations::GetInstance()->SetAnnotation(
      kUserAgentAnnotationKey, user_agent_string);
#endif  // BUILDFLAG(IS_STARBOARD)
}
#endif  // !BUILDFLAG(IS_ANDROIDTV)

}  // namespace cobalt
