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

#ifndef MAIDSAFE_VAULT_MAID_MANAGER_PMID_TOTALS_H_
#define MAIDSAFE_VAULT_MAID_MANAGER_PMID_TOTALS_H_

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "maidsafe/common/config.h"
#include "maidsafe/passport/types.h"
#include "maidsafe/routing/api_config.h"
#include "maidsafe/nfs/vault/pmid_registration.h"

#include "maidsafe/vault/pmid_manager/metadata.h"

namespace maidsafe {

namespace vault {

struct GetPmidTotalsOp {
  GetPmidTotalsOp(const MaidName& maid_account_name, const PmidName& pmid_account_name)
      : kMaidManagerName(maid_account_name),
        kPmidAccountName(pmid_account_name),
        mutex(),
        pmid_metadata() {}
  const MaidName kMaidManagerName;
  const PmidName kPmidAccountName;
  std::mutex mutex;
  std::vector<PmidManagerMetadata> pmid_metadata;
};

struct PmidTotals {
  PmidTotals();
  explicit PmidTotals(const std::string& serialised_pmid_registration_in);
  PmidTotals(const std::string& serialised_pmid_registration_in,
             const PmidManagerMetadata& pmid_metadata_in);
  PmidTotals(const PmidTotals& other);
  PmidTotals(PmidTotals&& other);
  PmidTotals& operator=(PmidTotals other);

  std::string Serialise() const;
  void ParseFromString(const std::string& serialised_pmid_total);

  std::string serialised_pmid_registration;
  PmidManagerMetadata pmid_metadata;
};

bool operator==(const PmidTotals& lhs, const PmidTotals& rhs);
void swap(PmidTotals& lhs, PmidTotals& rhs) MAIDSAFE_NOEXCEPT;

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_MAID_MANAGER_PMID_TOTALS_H_
