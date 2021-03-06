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
    use of the MaidSafe Software.
*/

#include "maidsafe/common/test.h"
#include "maidsafe/common/asio_service.h"

#include "maidsafe/routing/routing_api.h"

#include "maidsafe/nfs/client/data_getter.h"

#include "maidsafe/vault/data_manager/service.h"
#include "maidsafe/vault/tests/tests_utils.h"

namespace maidsafe {

namespace vault {

namespace test {


class DataManagerServiceTest {
 public:
  DataManagerServiceTest() :
      pmid_(MakePmid()),
      routing_(pmid_),
      data_getter_(asio_service_, routing_),
      data_manager_service_(pmid_, routing_, data_getter_),
      asio_service_(2) {}

  typedef std::function<
      void(const std::pair<PmidName, GetResponseFromPmidNodeToDataManager::Contents>&)> Functor;

  void AddTask(Functor functor, uint32_t required, uint32_t task_id) {
    data_manager_service_.get_timer_.AddTask(detail::Parameters::kDefaultTimeout, functor,
                                             required, task_id);
  }

  template <typename ActionType>
  void Commit(const DataManager::Key& key, const ActionType& action) {
    data_manager_service_.db_.Commit(key, action);
  }

  DataManager::Value Get(const DataManager::Key& key) {
    return data_manager_service_.db_.Get(key);
  }

  template <typename UnresolvedActionType>
  std::vector<std::unique_ptr<UnresolvedActionType>> GetUnresolvedActions();

  template <typename UnresolvedActionType>
  void SendSync(const std::vector<UnresolvedActionType>& unresolved_actions,
                const std::vector<routing::GroupSource>& group_source);

 protected:
  passport::Pmid pmid_;
  routing::Routing routing_;
  nfs_client::DataGetter data_getter_;
  DataManagerService data_manager_service_;
  AsioService asio_service_;
};

template <typename UnresolvedActionType>
void  DataManagerServiceTest::SendSync(
    const std::vector<UnresolvedActionType>& /*unresolved_actions*/,
    const std::vector<routing::GroupSource>& /*group_source*/) {
  UnresolvedActionType::No_genereic_handler_is_available__Specialisation_is_required;
}

template <>
void DataManagerServiceTest::SendSync<DataManager::UnresolvedPut>(
    const std::vector<DataManager::UnresolvedPut>& unresolved_actions,
    const std::vector<routing::GroupSource>& group_source) {
  AddLocalActionAndSendGroupActions<DataManagerService, DataManager::UnresolvedPut,
                                    SynchroniseFromDataManagerToDataManager>(
      &data_manager_service_, data_manager_service_.sync_puts_, unresolved_actions, group_source);
}

template <>
void DataManagerServiceTest::SendSync<DataManager::UnresolvedDelete>(
    const std::vector<DataManager::UnresolvedDelete>& unresolved_actions,
    const std::vector<routing::GroupSource>& group_source) {
  AddLocalActionAndSendGroupActions<DataManagerService, DataManager::UnresolvedDelete,
                                    SynchroniseFromDataManagerToDataManager>(
      &data_manager_service_, data_manager_service_.sync_deletes_, unresolved_actions,
      group_source);
}

template <>
void DataManagerServiceTest::SendSync<DataManager::UnresolvedAddPmid>(
    const std::vector<DataManager::UnresolvedAddPmid>& unresolved_actions,
    const std::vector<routing::GroupSource>& group_source) {
  AddLocalActionAndSendGroupActions<DataManagerService, DataManager::UnresolvedAddPmid,
                                    SynchroniseFromDataManagerToDataManager>(
      &data_manager_service_, data_manager_service_.sync_add_pmids_, unresolved_actions,
      group_source);
}

template <>
void DataManagerServiceTest::SendSync<DataManager::UnresolvedRemovePmid>(
    const std::vector<DataManager::UnresolvedRemovePmid>& unresolved_actions,
    const std::vector<routing::GroupSource>& group_source) {
  AddLocalActionAndSendGroupActions<DataManagerService, DataManager::UnresolvedRemovePmid,
                                    SynchroniseFromDataManagerToDataManager>(
      &data_manager_service_, data_manager_service_.sync_remove_pmids_, unresolved_actions,
      group_source);
}

template <>
void DataManagerServiceTest::SendSync<DataManager::UnresolvedNodeDown>(
    const std::vector<DataManager::UnresolvedNodeDown>& unresolved_actions,
    const std::vector<routing::GroupSource>& group_source) {
  AddLocalActionAndSendGroupActions<DataManagerService, DataManager::UnresolvedNodeDown,
                                    SynchroniseFromDataManagerToDataManager>(
      &data_manager_service_, data_manager_service_.sync_node_downs_, unresolved_actions,
      group_source);
}

template <>
void DataManagerServiceTest::SendSync<DataManager::UnresolvedNodeUp>(
    const std::vector<DataManager::UnresolvedNodeUp>& unresolved_actions,
    const std::vector<routing::GroupSource>& group_source) {
  AddLocalActionAndSendGroupActions<DataManagerService, DataManager::UnresolvedNodeUp,
                                    SynchroniseFromDataManagerToDataManager>(
      &data_manager_service_, data_manager_service_.sync_node_ups_, unresolved_actions,
      group_source);
}

template <typename UnresolvedActionType>
std::vector<std::unique_ptr<UnresolvedActionType>> DataManagerServiceTest::GetUnresolvedActions() {
  UnresolvedActionType::No_genereic_handler_is_available__Specialisation_is_required;
  return std::vector<std::unique_ptr<UnresolvedActionType>>();
}

template <>
std::vector<std::unique_ptr<DataManager::UnresolvedPut>>
DataManagerServiceTest::GetUnresolvedActions<DataManager::UnresolvedPut>() {
  return data_manager_service_.sync_puts_.GetUnresolvedActions();
}

template <>
std::vector<std::unique_ptr<DataManager::UnresolvedDelete>>
DataManagerServiceTest::GetUnresolvedActions<DataManager::UnresolvedDelete>() {
  return data_manager_service_.sync_deletes_.GetUnresolvedActions();
}

template <>
std::vector<std::unique_ptr<DataManager::UnresolvedAddPmid>>
DataManagerServiceTest::GetUnresolvedActions<DataManager::UnresolvedAddPmid>() {
  return data_manager_service_.sync_add_pmids_.GetUnresolvedActions();
}

template <>
std::vector<std::unique_ptr<DataManager::UnresolvedRemovePmid>>
DataManagerServiceTest::GetUnresolvedActions<DataManager::UnresolvedRemovePmid>() {
  return data_manager_service_.sync_remove_pmids_.GetUnresolvedActions();
}

TEST_CASE_METHOD(DataManagerServiceTest, "data manager: check handlers availability",
                 "[Handler][DataManager][Service][Behavioural]") {
  SECTION("PutRequestFromMaidManagerToDataManager") {
    NodeId maid_node_id(NodeId::kRandomId), data_name_id;
    auto content(CreateContent<PutRequestFromMaidManagerToDataManager::Contents>());
    data_name_id = NodeId(content.data.name.raw_name.string());
    auto put_request(CreateMessage<PutRequestFromMaidManagerToDataManager>(content));
    auto group_source(CreateGroupSource(maid_node_id));
    CHECK_NOTHROW(GroupSendToGroup(&data_manager_service_, put_request, group_source,
                                     routing::GroupId(data_name_id)));
    CHECK(GetUnresolvedActions<DataManager::UnresolvedPut>().size() == 0);
  }

  SECTION("PutResponseFromPmidManagerToDataManager") {
    NodeId data_name_id, pmid_node_id(NodeId::kRandomId);
    auto content(CreateContent<PutResponseFromPmidManagerToDataManager::Contents>());
    data_name_id = NodeId(content.name.raw_name.string());
    auto group_source(CreateGroupSource(pmid_node_id));
    auto put_response(CreateMessage<PutResponseFromPmidManagerToDataManager>(content));
    CHECK_NOTHROW(GroupSendToGroup(&data_manager_service_, put_response, group_source,
                                     routing::GroupId(data_name_id)));
    CHECK(GetUnresolvedActions<DataManager::UnresolvedAddPmid>().size() == 1);
  }

  SECTION("PutFailureFromPmidManagerToDataManager") {
    NodeId data_name_id, pmid_node_id(NodeId::kRandomId);
    auto content(CreateContent<PutFailureFromPmidManagerToDataManager::Contents>());
    data_name_id = NodeId(content.name.raw_name.string());
    auto group_source(CreateGroupSource(pmid_node_id));
    auto put_failure(CreateMessage<PutFailureFromPmidManagerToDataManager>(content));
    CHECK_NOTHROW(GroupSendToGroup(&data_manager_service_, put_failure, group_source,
                                     routing::GroupId(data_name_id)));
    CHECK(GetUnresolvedActions<DataManager::UnresolvedRemovePmid>().size() == 1);
  }

  SECTION("nfs::GetRequestFromMaidNodeToDataManager") {
    NodeId data_name_id, maid_node_id(NodeId::kRandomId);
    auto content(CreateContent<nfs::GetRequestFromMaidNodeToDataManager::Contents>());
    data_name_id = NodeId(content.raw_name.string());
    auto get_request(CreateMessage<nfs::GetRequestFromMaidNodeToDataManager>(content));
    CHECK_NOTHROW(SingleSendsToGroup(&data_manager_service_, get_request,
                                       routing::SingleSource(maid_node_id),
                                       routing::GroupId(data_name_id)));
  }

  SECTION("nfs::GetRequestFromDataGetterToDataManager") {
    NodeId data_name_id, maid_node_id(NodeId::kRandomId);
    auto content(CreateContent<nfs::GetRequestFromDataGetterToDataManager::Contents>());
    data_name_id = NodeId(content.raw_name.string());
    auto get_request(CreateMessage<nfs::GetRequestFromDataGetterToDataManager>(content));
    CHECK_NOTHROW(SingleSendsToGroup(&data_manager_service_, get_request,
                                       routing::SingleSource(maid_node_id),
                                       routing::GroupId(data_name_id)));
  }

  SECTION("GetResponseFromPmidNodeToDataManager") {
    NodeId pmid_node_id(NodeId::kRandomId);
    auto content(CreateContent<GetResponseFromPmidNodeToDataManager::Contents>());
    auto get_response(CreateMessage<GetResponseFromPmidNodeToDataManager>(content));
    auto functor([=](const std::pair<PmidName, GetResponseFromPmidNodeToDataManager::Contents>&) {
                   LOG(kVerbose) << "functor called";
                 });

    AddTask(functor, 1, get_response.id.data);
    CHECK_NOTHROW(SingleSendsToSingle(&data_manager_service_, get_response,
                                        routing::SingleSource(pmid_node_id),
                                        routing::SingleId(routing_.kNodeId())));
  }

  SECTION("PutToCacheFromDataManagerToDataManager") {
    NodeId data_manager_id(NodeId::kRandomId);
    auto content(CreateContent<PutToCacheFromDataManagerToDataManager::Contents>());
    auto put_to_cache(CreateMessage<PutToCacheFromDataManagerToDataManager>(content));
    CHECK_NOTHROW(SingleSendsToSingle(&data_manager_service_, put_to_cache,
                                        routing::SingleSource(data_manager_id),
                                        routing::SingleId(routing_.kNodeId())));
  }

  SECTION("GetCachedResponseFromCacheHandlerToDataManager") {
    NodeId cache_handler_id(NodeId::kRandomId);
    auto content(CreateContent<GetCachedResponseFromCacheHandlerToDataManager::Contents>());
    auto get_cache_response(CreateMessage<GetCachedResponseFromCacheHandlerToDataManager>(content));
    CHECK_NOTHROW(SingleSendsToSingle(&data_manager_service_, get_cache_response,
                                        routing::SingleSource(cache_handler_id),
                                        routing::SingleId(routing_.kNodeId())));
  }

  SECTION("DeleteRequestFromMaidManagerToDataManager") {
    NodeId maid_node_id(NodeId::kRandomId);
    auto content(CreateContent<DeleteRequestFromMaidManagerToDataManager::Contents>());
    auto delete_request(CreateMessage<DeleteRequestFromMaidManagerToDataManager>(content));
    auto group_source(CreateGroupSource(maid_node_id));
    GroupSendToGroup(&data_manager_service_, delete_request, group_source,
                     routing::GroupId(NodeId(content.raw_name.string())));
    CHECK(GetUnresolvedActions<DataManager::UnresolvedDelete>().size() == 1);
  }
}

TEST_CASE_METHOD(DataManagerServiceTest,
                 "data manager: checking all sync message types are handled",
                 "[Sync][DataManager][Service][Behavioural]") {
  PmidName pmid_name(Identity(RandomString(64)));
  ActionDataManagerPut action_put;
  ImmutableData data(NonEmptyString(RandomString(kTestChunkSize)));
  DataManager::Key key(data.name());
  auto group_source(CreateGroupSource(data.name()));


  SECTION("Put") {
    Commit(key, ActionDataManagerAddPmid(pmid_name, kTestChunkSize));
    CHECK(Get(key).Subscribers() == 1);
    auto group_unresolved_action(
             CreateGroupUnresolvedAction<DataManager::UnresolvedPut>(key, action_put,
                                                                     group_source));
    SendSync<DataManager::UnresolvedPut>(group_unresolved_action, group_source);
    CHECK(Get(key).Subscribers() == 2);
  }

  SECTION("Delete") {
    // store key value in db
    Commit(key, ActionDataManagerAddPmid(pmid_name, kTestChunkSize));
    CHECK(Get(key).Subscribers() == 1);
    // key value is in db
    ActionDataManagerDelete action_delete((nfs::MessageId(RandomInt32())));
    auto group_unresolved_action(
             CreateGroupUnresolvedAction<DataManager::UnresolvedDelete>(key, action_delete,
                                                                      group_source));
    SendSync<DataManager::UnresolvedDelete>(group_unresolved_action, group_source);
    CHECK_THROWS(Get(key));
  }

  SECTION("AddPmid") {
    // check key value is not in db
    CHECK_THROWS(Get(key));
    // Sync AddPmid
    ActionDataManagerAddPmid action_add_pmid(pmid_name, kTestChunkSize);
    auto group_unresolved_action(
             CreateGroupUnresolvedAction<DataManager::UnresolvedAddPmid>(key, action_add_pmid,
                                                                         group_source));
    SendSync<DataManager::UnresolvedAddPmid>(group_unresolved_action, group_source);
    CHECK(Get(key).Subscribers() == 1);
  }

  SECTION("RemovePmid") {
    // store key value in db
    PmidName pmid_name_two(Identity(RandomString(64)));
    DataManager::Key key(data.name());
    Commit(key, ActionDataManagerAddPmid(pmid_name, kTestChunkSize));
    Commit(key, ActionDataManagerAddPmid(pmid_name_two, kTestChunkSize));
    auto value(Get(key));
    CHECK(value.Subscribers() == 1);
    CHECK(value.AllPmids().size() == 2);

    // Sync remove pmid
    ActionDataManagerRemovePmid action_remove_pmid(pmid_name_two);
    auto group_unresolved_action(
             CreateGroupUnresolvedAction<DataManager::UnresolvedRemovePmid>(key, action_remove_pmid,
                                                                            group_source));
    SendSync<DataManager::UnresolvedRemovePmid>(group_unresolved_action, group_source);
    CHECK(Get(key).AllPmids().size() == 1);
  }

  SECTION("NodeDown") {
    PmidName pmid_name_two(Identity(RandomString(64)));
    Commit(key, ActionDataManagerAddPmid(pmid_name, kTestChunkSize));
    Commit(key, ActionDataManagerAddPmid(pmid_name_two, kTestChunkSize));
    auto value(Get(key));
    CHECK(value.Subscribers() == 1);
    CHECK(value.AllPmids().size() == 2);

    // Sync node down
    ActionDataManagerNodeDown action_node_down(pmid_name_two);
    auto group_unresolved_action(
             CreateGroupUnresolvedAction<DataManager::UnresolvedNodeDown>(key, action_node_down,
                                                                          group_source));
    SendSync<DataManager::UnresolvedNodeDown>(group_unresolved_action, group_source);

    value = Get(key);
    CHECK(value.Subscribers() == 1);
    CHECK(value.AllPmids().size() == 2);
    CHECK(value.online_pmids().size() == 1);
  }

  SECTION("NodeUp") {
    // store key value in db
    PmidName pmid_name_two(Identity(RandomString(64)));
    Commit(key, ActionDataManagerAddPmid(pmid_name, kTestChunkSize));
    Commit(key, ActionDataManagerAddPmid(pmid_name_two, kTestChunkSize));
    auto value(Get(key));
    CHECK(value.Subscribers() == 1);
    CHECK(value.AllPmids().size() == 2);

    Commit(key, ActionDataManagerNodeDown(pmid_name_two));
    value = Get(key);
    CHECK(value.Subscribers() == 1);
    CHECK(value.AllPmids().size() == 2);
    CHECK(value.online_pmids().size() == 1);

    // Sync node up
    ActionDataManagerNodeUp action_node_up(pmid_name_two);
    auto group_source(CreateGroupSource(data.name()));
    auto group_unresolved_action(
             CreateGroupUnresolvedAction<DataManager::UnresolvedNodeUp>(key, action_node_up,
                                                                        group_source));
    SendSync<DataManager::UnresolvedNodeUp>(group_unresolved_action, group_source);

    value = Get(key);
    CHECK(value.Subscribers() == 1);
    CHECK(value.AllPmids().size() == 2);
    CHECK(value.online_pmids().size() == 2);
  }

  SECTION("SetPmidOnline") {}
  SECTION("SetPmidOffline") {}
}

}  //  namespace test

}  //  namespace vault

}  //  namespace maidsafe
