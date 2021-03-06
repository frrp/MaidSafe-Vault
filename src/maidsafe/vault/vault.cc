/*  Copyright 2012 MaidSafe.net limited

    This MaidSafe Software is licensed to you under (1) the MaidSafe.net Commercial License,
    version 1.0 or later, or (2) The General Public License (GPL), version 3, depending on which
    licence you accepted on initial access to the Software (the "Licences").

    By contributing code to the MaidSafe Software, or to this project generally, you agree to be
    bound by the terms of the MaidSafe Contributor Agreement, version 1.0, found in the root
    directory of this project at LICENSE, COPYING and CONTRIBUTOR respectively and also
    available at: http://www.maidsafe.net/licenses

    Unless required by applicable law or agreed to in writing, the MaidSafe Software distributed
    under the GPL Licence is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS
    OF ANY KIND, either express or implied.

    See the Licences for the specific language governing permissions and limitations relating to
    use of the MaidSafe Software.                                                                 */

#include "maidsafe/vault/vault.h"

#include "maidsafe/routing/routing_api.h"
#include "maidsafe/routing/node_info.h"

#include "maidsafe/vault/parameters.h"

namespace maidsafe {

namespace vault {

Vault::Vault(const passport::Pmid& pmid, const boost::filesystem::path& vault_root_dir,
             std::function<void(boost::asio::ip::udp::endpoint)> on_new_bootstrap_endpoint,
             const std::vector<passport::PublicPmid>& pmids_from_file,
             const std::vector<boost::asio::ip::udp::endpoint>& peer_endpoints)
    : network_health_mutex_(),
      network_health_condition_variable_(),
      network_health_(-1),
      on_new_bootstrap_endpoint_(on_new_bootstrap_endpoint),
      routing_(new routing::Routing(pmid)),
      pmids_from_file_(pmids_from_file),
      data_getter_(asio_service_, *routing_),
      public_pmid_helper_(),
      maid_manager_service_(
          std::move(std::unique_ptr<MaidManagerService>(new MaidManagerService(pmid, *routing_,
                                                                               data_getter_)))),
      version_handler_service_(std::move(
          std::unique_ptr<VersionHandlerService>(new VersionHandlerService(pmid, *routing_)))),
      data_manager_service_(std::move(std::unique_ptr<DataManagerService>(
          new DataManagerService(pmid, *routing_, data_getter_)))),
      pmid_manager_service_(
          std::move(std::unique_ptr<PmidManagerService>(new PmidManagerService(pmid, *routing_)))),
      pmid_node_service_(std::move(std::unique_ptr<PmidNodeService>(
          new PmidNodeService(pmid, *routing_, data_getter_, vault_root_dir)))),
      // FIXME need to specialise
      cache_service_(std::move(std::unique_ptr<CacheHandlerService>(
          new CacheHandlerService(*routing_, vault_root_dir)))),
      demux_(maid_manager_service_, version_handler_service_, data_manager_service_,
             pmid_manager_service_, pmid_node_service_, data_getter_),
      asio_service_(2),
      getting_keys_()
#ifdef TESTING
      ,
      pmids_mutex_()
#endif
{
  // TODO(Fraser#5#): 2013-03-29 - Prune all empty dirs.
  InitRouting(peer_endpoints);
}

Vault::~Vault() {
  // call stop on all components
  routing_.reset();
}

#ifdef TESTING
void Vault::AddPublicPmid(const passport::PublicPmid& public_pmid) {
  std::lock_guard<std::mutex> lock(pmids_mutex_);
  pmids_from_file_.push_back(public_pmid);
}
#endif

void Vault::InitRouting(const std::vector<boost::asio::ip::udp::endpoint>& peer_endpoints) {
  routing::Functors functors(InitialiseRoutingCallbacks());
  routing_->Join(functors, peer_endpoints);

  std::unique_lock<std::mutex> lock(network_health_mutex_);
#ifdef TESTING
  if (!network_health_condition_variable_.wait_for(lock, std::chrono::minutes(5), [this] {
         return network_health_ >= detail::Parameters::kMinNetworkHealth;
       }))
    BOOST_THROW_EXCEPTION(MakeError(VaultErrors::failed_to_join_network));
#else
  network_health_condition_variable_.wait(
      lock, [this] { return network_health_ >= detail::Parameters::kMinNetworkHealth; });
#endif
}

routing::Functors Vault::InitialiseRoutingCallbacks() {
  routing::Functors functors;
  functors.typed_message_and_caching.single_to_single.message_received = [this](
      const routing::SingleToSingleMessage& message) { OnMessageReceived(message); };  // NOLINT
  functors.typed_message_and_caching.single_to_group.message_received = [this](
      const routing::SingleToGroupMessage& message) { OnMessageReceived(message); };  // NOLINT
  functors.typed_message_and_caching.group_to_single.message_received = [this](
      const routing::GroupToSingleMessage& message) { OnMessageReceived(message); };  // NOLINT
  functors.typed_message_and_caching.group_to_group.message_received = [this](
      const routing::GroupToGroupMessage& message) { OnMessageReceived(message); };  // NOLINT
  functors.typed_message_and_caching.single_to_group_relay.message_received = [this](
      const routing::SingleToGroupRelayMessage& message) { OnMessageReceived(message); };  // NOLINT
  functors.typed_message_and_caching.single_to_single.get_cache_data = [this](
      const routing::SingleToSingleMessage& message) { return OnGetFromCache(message); };  // NOLINT
  functors.typed_message_and_caching.single_to_group.get_cache_data = [this](
      const routing::SingleToGroupMessage& message) { return OnGetFromCache(message); };  // NOLINT
  functors.typed_message_and_caching.group_to_single.get_cache_data = [this](
      const routing::GroupToSingleMessage& message) { return OnGetFromCache(message); };  // NOLINT
  functors.typed_message_and_caching.group_to_group.get_cache_data = [this](
      const routing::GroupToGroupMessage& message) { return OnGetFromCache(message); };  // NOLINT
  functors.typed_message_and_caching.single_to_single.put_cache_data = [this](
      const routing::SingleToSingleMessage& message) { OnStoreInCache(message); };  // NOLINT
  functors.typed_message_and_caching.single_to_group.put_cache_data = [this](
      const routing::SingleToGroupMessage& message) { OnStoreInCache(message); };  // NOLINT
  functors.typed_message_and_caching.group_to_single.put_cache_data = [this](
      const routing::GroupToSingleMessage& message) { OnStoreInCache(message); };  // NOLINT
  functors.typed_message_and_caching.group_to_group.put_cache_data = [this](
      const routing::GroupToGroupMessage& message) { OnStoreInCache(message); };  // NOLINT

  functors.network_status = [this](const int&
                                   network_health) { OnNetworkStatusChange(network_health); };  // NOLINT
  functors.close_node_replaced = [this](const std::vector<routing::NodeInfo>&
                                        new_close_nodes) { OnCloseNodeReplaced(new_close_nodes); };  // NOLINT
  functors.matrix_changed = [this](std::shared_ptr<routing::MatrixChange> matrix_change) {
    OnMatrixChanged(matrix_change);
  };
  functors.request_public_key = [this](
    const NodeId& node_id,
    const routing::GivePublicKeyFunctor& give_key) {
      nfs::detail::DoGetPublicKey(data_getter_, node_id, give_key, pmids_from_file_,
                                  public_pmid_helper_);
    };
  functors.new_bootstrap_endpoint = [this](const boost::asio::ip::udp::endpoint& endpoint) {
                                        OnNewBootstrapEndpoint(endpoint);
                                    };
  return functors;
}

void Vault::OnNetworkStatusChange(int network_health) {
  asio_service_.service().post([=] { DoOnNetworkStatusChange(network_health); });
}

void Vault::DoOnNetworkStatusChange(int network_health) {
  if (network_health >= 0) {
    if (network_health >= network_health_)
      LOG(kVerbose) << "Init - " << DebugId(routing_->kNodeId()) << " - Network health is "
                    << network_health << "% (was " << network_health_ << "%)";
    else
      LOG(kWarning) << "Init - " << DebugId(routing_->kNodeId()) << " - Network health is "
                    << network_health << "% (was " << network_health_ << "%)";
  } else {
    LOG(kWarning) << "Init - " << DebugId(routing_->kNodeId()) << " - Network is down ("
                  << network_health << ")";
  }

  {
    std::lock_guard<std::mutex> lock(network_health_mutex_);
    network_health_ = network_health;
  }
  network_health_condition_variable_.notify_one();
  // TODO(Team) : actions when network is down/up per persona
}

void Vault::OnCloseNodeReplaced(const std::vector<routing::NodeInfo>& /*new_close_nodes*/) {}

void Vault::OnMatrixChanged(std::shared_ptr<routing::MatrixChange> matrix_change) {
//   LOG(kVerbose) << "OnMatrixChanged ";
//   matrix_change->Print();
//   data_manager_service_.HandleChurnEvent(matrix_change);
//   asio_service_.service().post([=] { maid_manager_service_.HandleChurnEvent(matrix_change); });
//  asio_service_.service().post([=] { version_handler_service_.HandleChurnEvent(matrix_change); });
  asio_service_.service().post([=] { data_manager_service_.HandleChurnEvent(matrix_change); });
  asio_service_.service().post([=] { pmid_manager_service_.HandleChurnEvent(matrix_change); });
}

void Vault::OnNewBootstrapEndpoint(const boost::asio::ip::udp::endpoint& endpoint) {
  asio_service_.service().post([=] { on_new_bootstrap_endpoint_(endpoint); });
}

}  // namespace vault

}  // namespace maidsafe
