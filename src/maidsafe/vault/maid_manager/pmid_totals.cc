/*  Copyright 2013 MaidSafe.net limited

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

#include "maidsafe/vault/maid_manager/maid_manager.pb.h"
#include "maidsafe/vault/maid_manager/pmid_totals.h"

#include <string>
#include <utility>

namespace maidsafe {

namespace vault {

PmidTotals::PmidTotals() : serialised_pmid_registration(), pmid_metadata() {}

PmidTotals::PmidTotals(const std::string& serialised_pmid_registration_in)
    : serialised_pmid_registration(serialised_pmid_registration_in), pmid_metadata() {}

PmidTotals::PmidTotals(const std::string& serialised_pmid_registration_in,
                       const PmidManagerMetadata& pmid_metadata_in)
    : serialised_pmid_registration(serialised_pmid_registration_in),
      pmid_metadata(pmid_metadata_in) {}

PmidTotals::PmidTotals(const PmidTotals& other)
    : serialised_pmid_registration(other.serialised_pmid_registration),
      pmid_metadata(other.pmid_metadata) {}

PmidTotals::PmidTotals(PmidTotals&& other)
    : serialised_pmid_registration(std::move(other.serialised_pmid_registration)),
      pmid_metadata(std::move(other.pmid_metadata)) {}

PmidTotals& PmidTotals::operator=(PmidTotals other) {
  swap(*this, other);
  return *this;
}

std::string PmidTotals::Serialise() const {
  protobuf::PmidTotal pmid_total_proto;
  pmid_total_proto.set_serialised_pmid_registration(serialised_pmid_registration);
  pmid_total_proto.set_serialised_pmid_metadata(pmid_metadata.Serialise());
  return pmid_total_proto.SerializeAsString();
}

void PmidTotals::ParseFromString(const std::string& serialised_pmid_total) {
  protobuf::PmidTotal pmid_total_proto;
  pmid_total_proto.ParseFromString(serialised_pmid_total);
  serialised_pmid_registration = pmid_total_proto.serialised_pmid_registration();
  pmid_metadata = PmidManagerMetadata(pmid_total_proto.serialised_pmid_metadata());
}

bool operator==(const PmidTotals& lhs, const PmidTotals& rhs) {
  return lhs.serialised_pmid_registration == rhs.serialised_pmid_registration &&
         lhs.pmid_metadata == rhs.pmid_metadata;
}

void swap(PmidTotals& lhs, PmidTotals& rhs) MAIDSAFE_NOEXCEPT {
  using std::swap;
  swap(lhs.serialised_pmid_registration, rhs.serialised_pmid_registration);
  swap(lhs.pmid_metadata, rhs.pmid_metadata);
}

}  // namespace vault

}  // namespace maidsafe
