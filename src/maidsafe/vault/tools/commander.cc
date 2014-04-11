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

#include "maidsafe/vault/tools/commander.h"
#include "maidsafe/common/error.h"

namespace maidsafe {

namespace vault {

namespace tools {

namespace {

PmidVector GetJustPmids(const KeyChainVector& keychains) {
  PmidVector pmids;
  for (auto& keychain : keychains)
    pmids.push_back(keychain.pmid);
  return pmids;
}

}  // namespace

namespace fs = boost::filesystem;
namespace po = boost::program_options;

bool SelectedOperationsContainer::InvalidOptions(
    const po::variables_map& variables_map,
    const std::vector<boost::asio::ip::udp::endpoint>& peer_endpoints) {
  do_create = variables_map.count("create") != 0;
  do_load = variables_map.count("load") != 0;
  if (!do_create && !do_load)
    do_load = variables_map.at("key_index").as<int>() != -1;
  do_delete = variables_map.count("delete") != 0;
  do_bootstrap = variables_map.count("bootstrap") != 0;
  do_store = variables_map.count("store") != 0;
  do_verify = variables_map.count("verify") != 0;
  do_test = variables_map.count("test") != 0;
  do_test_with_delete = variables_map.count("test_with_delete") != 0;
  do_generate_chunks = variables_map.count("generate_chunks") != 0;
  do_test_store_chunk = variables_map.count("test_store_chunk") != 0;
  do_test_fetch_chunk = variables_map.count("test_fetch_chunk") != 0;
  do_test_delete_chunk = variables_map.count("test_delete_chunk") != 0;
  do_test_version = variables_map.count("version_test") != 0;
  do_print = variables_map.count("print") != 0;

  return NoOptionsSelected() || ConflictedOptions(peer_endpoints);
}

bool SelectedOperationsContainer::ConflictedOptions(
    const std::vector<boost::asio::ip::udp::endpoint>& peer_endpoints) const {
  if (!do_create && !do_load && !do_delete && !do_generate_chunks && !do_bootstrap && !do_test)
    return true;
  if (do_create && do_load)
    return true;
  if (do_create && do_delete)
    return true;
  if (do_load && do_delete)
    return true;
  if (do_delete && do_print)
    return true;
  if (!(do_create || do_load) && do_print)
    return true;
  if (do_bootstrap && (do_store || do_verify || do_test || do_test_with_delete))
    return true;
  if (peer_endpoints.empty() &&
      !do_create && !do_load && !do_delete && !do_generate_chunks && !do_bootstrap)
    return true;
  return false;
}

bool SelectedOperationsContainer::NoOptionsSelected() const {
  return !(do_create || do_load || do_bootstrap || do_store || do_verify || do_test || do_delete ||
           do_test_with_delete || do_generate_chunks || do_test_store_chunk ||
           do_test_fetch_chunk || do_test_delete_chunk || do_test_version || do_print);
}

Commander::Commander(size_t pmids_count)
    : pmids_count_(pmids_count),
      key_index_(-1),
      chunk_set_count_(-1),
      chunk_index_(0),
      all_keychains_(),
      pmids_from_file_(),
      keys_path_(),
      peer_endpoints_(),
      selected_ops_() {
  routing::Parameters::append_local_live_port_endpoint = true;
}

void Commander::AnalyseCommandLineOptions(int argc, char* argv[]) {
  po::options_description cmdline_options;
  cmdline_options.add(AddGenericOptions("Commands"))
      .add(AddConfigurationOptions("Configuration options"));
  CheckOptionValidity(cmdline_options, argc, argv);
  ChooseOperations();
}

boost::asio::ip::udp::endpoint Commander::GetBootstrapEndpoint(const std::string& peer) {
  size_t delim = peer.rfind(':');
  boost::asio::ip::udp::endpoint ep;
  ep.port(boost::lexical_cast<uint16_t>(peer.substr(delim + 1)));
  ep.address(boost::asio::ip::address::from_string(peer.substr(0, delim)));
  LOG(kInfo) << "Going to bootstrap off endpoint " << ep;
  return ep;
}

po::options_description Commander::AddGenericOptions(const std::string& title) {
  po::options_description generic_options(title);
  generic_options.add_options()("help,h", "Print this help message.")(
      "create,c", "Create keys and write to file.")("load,l", "Load keys from file.")(
      "delete,d", "Delete keys file.")("print,p", "Print the list of keys available.")(
      "bootstrap,b", "Run boostrap nodes only, using first 2 keys.")(
      "store,s", "Store keys on network.")(
      "verify,v", "Verify keys are available on network.")(
      "test,t", "Run simple test that stores and retrieves chunks.")(
      "test_with_delete,w", "Run simple test that stores and deletes chunks.")(
      "generate_chunks,g", "Generate a set of chunks for later on tests")(
      "test_store_chunk,1", "Run a simple test that stores a chunk from file")(
      "test_fetch_chunk,2", "Run a simple test that retrieves a chunk(recorded in file)")(
      "test_delete_chunk,3", "Run a simple test that removes a chunk(recorded in file)")(
      "version_test,4", "Run a simple test about handling version)");
  return generic_options;
}

po::options_description Commander::AddConfigurationOptions(const std::string& title) {
  po::options_description config_file_options(title);
  boost::system::error_code error_code;
  config_file_options.add_options()(
      "peer", po::value<std::string>(),
              "Endpoint of bootstrap node, if attaching to running network.")(
      "pmids_count,n", po::value<size_t>(&pmids_count_)->default_value(pmids_count_),
                       "Number of keys to create")(
      "keys_path", po::value<std::string>()->default_value(fs::path(
                       fs::temp_directory_path(error_code) / "key_directory.dat").string()),
      "Path to keys file")(
      "chunk_path", po::value<std::string>()->default_value(
                        fs::path(fs::temp_directory_path(error_code) / "keys_chunks").string()),
      "Path to chunk directory")(
      "key_index,k", po::value<int>(&key_index_)->default_value(key_index_),
                     "The index of key to be used as client during chunk store test")(
      "chunk_set_count", po::value<int>(&chunk_set_count_)->default_value(chunk_set_count_),
                         "Num of test rounds(default is infinite);Or num of generate chunks")(
      "chunk_index", po::value<int>(&chunk_index_)->default_value(chunk_index_),
                     "Index of the chunk to be used during tests, default is 0");
  return config_file_options;
}

void Commander::CheckOptionValidity(po::options_description& cmdline_options, int argc,
                                    char* argv[]) {
  po::command_line_parser parser(argc, argv);
  po::variables_map variables_map;
  po::store(parser.options(cmdline_options).allow_unregistered().run(), variables_map);
  po::notify(variables_map);
  keys_path_ = maidsafe::GetPathFromProgramOptions("keys_path", variables_map, false, true);
  if (variables_map.count("peer"))
    peer_endpoints_.push_back(GetBootstrapEndpoint(variables_map.at("peer").as<std::string>()));

  if (variables_map.count("help") || selected_ops_.InvalidOptions(variables_map, peer_endpoints_)) {
    std::cout << cmdline_options << "Options order: [c|l|d] p [b|(s|v)|t]" << std::endl;
    if (!variables_map.count("help")) {
      std::cout << "Invalid command line options.\n";
      BOOST_THROW_EXCEPTION(MakeError(CommonErrors::invalid_parameter));
    }
  }
}

void Commander::ChooseOperations() {
  HandleKeyOperations();
  if (selected_ops_.do_bootstrap)
    HandleSetupBootstraps();
  if (selected_ops_.do_store)
    HandleStorePublicKeys(key_index_);
  if (selected_ops_.do_verify)
    HandleVerifyStoredPublicKeys(key_index_);
  if (selected_ops_.do_test)
    HandleDoTest(key_index_);
  if (selected_ops_.do_test_with_delete)
    HandleDoTestWithDelete(key_index_);
  if (selected_ops_.do_generate_chunks)
    HandleGenerateChunks();
  if (selected_ops_.do_test_store_chunk)
    HandleStoreChunk(key_index_);
  if (selected_ops_.do_test_fetch_chunk)
    HandleFetchChunk(key_index_);
  if (selected_ops_.do_test_delete_chunk)
    HandleDeleteChunk(key_index_);
  if (selected_ops_.do_test_version)
    HandleTestVersion(key_index_);
}

void Commander::CreateKeys() {
  all_keychains_.clear();
  for (size_t i = 0; i < pmids_count_; ++i) {
    passport::Anmaid anmaid;
    passport::Maid maid(anmaid);
    passport::Anpmid anpmid;
    passport::Pmid pmid(anpmid);
    all_keychains_.push_back(passport::detail::AnmaidToPmid(anmaid, maid, anpmid, pmid));
  }
  LOG(kInfo) << "Created " << all_keychains_.size() << " pmids.";
  if (maidsafe::passport::detail::WriteKeyChainList(keys_path_, all_keychains_)) {
    std::cout << "Wrote keys to " << keys_path_ << '\n';
  } else {
    std::cout << "Could not write keys to " << keys_path_ << '\n';
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::invalid_parameter));
  }
}

void Commander::HandleKeyOperations() {
  if (selected_ops_.do_create) {
    CreateKeys();
  } else if (selected_ops_.do_load) {
    all_keychains_ = maidsafe::passport::detail::ReadKeyChainList(keys_path_);
    for (auto& key_chain : all_keychains_)
      pmids_from_file_.push_back(passport::PublicPmid(key_chain.pmid));
    LOG(kInfo) << "Loaded " << all_keychains_.size() << " pmids from " << keys_path_;
  } else if (selected_ops_.do_delete) {
    HandleDeleteKeyFile();
  }

  if (selected_ops_.do_print) {
    for (size_t i(0); i < all_keychains_.size(); ++i)
      std::cout << '\t' << i << "\t ANMAID " << HexSubstr(all_keychains_.at(i).anmaid.name().value)
                << "\t MAID " << HexSubstr(all_keychains_.at(i).maid.name().value) << "\t PMID "
                << HexSubstr(all_keychains_.at(i).pmid.name().value)
                << (i < 2 ? " (bootstrap)" : "") << std::endl;
  }
}

void Commander::HandleSetupBootstraps() {
  if (all_keychains_.size() < 2) {
    all_keychains_.clear();
    all_keychains_ = maidsafe::passport::detail::ReadKeyChainList(keys_path_);
    LOG(kInfo) << "Loaded " << all_keychains_.size() << " pmids from " << keys_path_;
  }
  assert(all_keychains_.size() >= 2);
  NetworkGenerator generator;
  generator.SetupBootstrapNodes(GetJustPmids(all_keychains_));
}

void Commander::HandleStorePublicKeys(size_t client_index) {
  try {
//     boost::system::error_code error_code;
//     fs::path target_key_file(fs::current_path(error_code) / keys_path_.filename());
//     KeyChainVector keys_to_store = maidsafe::passport::detail::ReadKeyChainList(target_key_file);
    KeyStorer storer(all_keychains_.at(client_index), peer_endpoints_,
                     pmids_from_file_, all_keychains_);
    storer.Store();
  } catch (const std::exception& e) {
    std::cout << "Failed storing key chain : " << boost::diagnostic_information(e) << std::endl;
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::invalid_parameter));
  }
  std::cout << "Keys Stored" << std::endl;
}

void Commander::HandleVerifyStoredPublicKeys(size_t /*client_index*/) {
  size_t failures(0);
  for (auto& keychain : all_keychains_) {
    try {
      KeyVerifier verifier(keychain, peer_endpoints_, pmids_from_file_);
      verifier.Verify();
    }
    catch (const std::exception& e) {
      std::cout << "Failed verifying key chain with PMID " << HexSubstr(keychain.pmid.name().value)
                << ": " << boost::diagnostic_information(e) << '\n';
      ++failures;
    }
  }
  if (failures) {
    std::cout << "Could not verify " << std::to_string(failures) << " out of "
              << std::to_string(all_keychains_.size()) << '\n';
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::invalid_parameter));
  }
}

void Commander::HandleDoTest(size_t client_index) {
  assert(client_index > 1);
  LOG(kVerbose) << "constructing chunk_storer ......";
  DataChunkStorer chunk_storer(all_keychains_.at(client_index), peer_endpoints_,
                               pmids_from_file_, false);
  LOG(kVerbose) << "testing store " << chunk_set_count_ << " chunks ......";
  chunk_storer.Test(chunk_set_count_);
}

void Commander::HandleDoTestWithDelete(size_t client_index) {
  assert(client_index > 1);
  DataChunkStorer chunk_storer(all_keychains_.at(client_index), peer_endpoints_,
                               pmids_from_file_, false);
  chunk_storer.TestWithDelete(chunk_set_count_);
}

void Commander::HandleStoreChunk(size_t client_index) {
  assert(client_index > 1);
  DataChunkStorer chunk_storer(all_keychains_.at(client_index), peer_endpoints_,
                               pmids_from_file_, true);
  chunk_storer.TestStoreChunk(chunk_index_);
}

void Commander::HandleFetchChunk(size_t client_index) {
  assert(client_index > 1);
  DataChunkStorer chunk_storer(all_keychains_.at(client_index), peer_endpoints_,
                               pmids_from_file_, true);
  chunk_storer.TestFetchChunk(chunk_index_);
}

void Commander::HandleDeleteChunk(size_t client_index) {
  assert(client_index > 1);
  DataChunkStorer chunk_storer(all_keychains_.at(client_index), peer_endpoints_,
                               pmids_from_file_, true);
  chunk_storer.TestDeleteChunk(chunk_index_);
}

void Commander::HandleTestVersion(size_t client_index) {
  assert(client_index > 1);
  DataChunkStorer chunk_storer(all_keychains_.at(client_index), peer_endpoints_,
                               pmids_from_file_, true);
  chunk_storer.TestVersion();
}

void Commander::HandleGenerateChunks() {
  boost::filesystem::path store_path(boost::filesystem::temp_directory_path() / "Chunks");
  boost::system::error_code error_code;
  if (!fs::exists(store_path, error_code)) {
    if (!fs::create_directories(store_path, error_code)) {
      LOG(kError) << "Can't create store path at " << store_path << ": " << error_code.message();
      BOOST_THROW_EXCEPTION(MakeError(CommonErrors::uninitialised));
      return;
    }
  }

  assert(chunk_set_count_ > 0);
  for (int i(0); i < chunk_set_count_; ++i) {
    ImmutableData::serialised_type content(NonEmptyString(RandomString(1 << 18)));  // 256 KB
    ImmutableData::Name name(Identity(crypto::Hash<crypto::SHA512>(content.data.string())));
    ImmutableData chunk_data(name, content);

    fs::path chunk_file(store_path / HexEncode(chunk_data.name()->string()));
    if (!WriteFile(chunk_file, content->string()))
      LOG(kError) << "Can't store chunk " << HexSubstr(chunk_data.name()->string());
  }
}

void Commander::HandleDeleteKeyFile() {
  if (fs::remove(keys_path_))
    std::cout << "Deleted " << keys_path_ << std::endl;
}

}  // namespace tools

}  // namespace vault

}  // namespace maidsafe
