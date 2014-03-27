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

#ifndef MAIDSAFE_VAULT_UNRESOLVED_KV_TRANSFER_H_
#define MAIDSAFE_VAULT_UNRESOLVED_KV_TRANSFER_H_

#include <cstdint>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "maidsafe/common/node_id.h"
#include "maidsafe/common/error.h"
#include "maidsafe/common/utils.h"

#include "maidsafe/nfs/types.h"

#include "maidsafe/routing/message.h"



namespace maidsafe {

namespace vault {

template <typename Key, typename Value>
struct UnresolvedKVTransfer {
 public:
  UnresolvedKVTransfer(const Key& key_in, const Value& value_in);

  void Merge(const UnresolvedKVTransfer& other);

  Key key;
  Value value;
};

template <typename Key, typename Value>
UnresolvedKVTransfer<Key, Value>::UnresolvedKVTransfer(const Key& key_in,
                                                        const Value& value_in)
  : key(key_in), value(value_in) {}

template <typename Key, typename Value>
void UnresolvedKVTransfer<Key, Value>::Merge(const UnresolvedKVTransfer& other) {
  value.Merge(other.value);
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_UNRESOLVED_KV_TRANSFER_ACTION_H_
