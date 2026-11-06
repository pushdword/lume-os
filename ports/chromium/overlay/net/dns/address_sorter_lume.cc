// SPDX-License-Identifier: BSD-3-Clause

#include "net/dns/address_sorter.h"

#include <algorithm>
#include <memory>
#include <utility>

namespace net
{
namespace
{

class AddressSorterLume final : public AddressSorter
{
  public:
    void Sort(const std::vector<IPEndPoint> &endpoints, const NetworkAnonymizationKey &,
              handles::NetworkHandle target_network, CallbackType callback) const override
    {
        auto sorted = endpoints;
        if (target_network != handles::kInvalidNetworkHandle)
        {
            std::move(callback).Run(false, std::move(sorted));
            return;
        }
        // IPv4 is the native transport. Preserve resolver order within each address family.
        std::stable_partition(sorted.begin(), sorted.end(),
                              [](const IPEndPoint &endpoint)
                              {
                                  return endpoint.address().IsIPv4();
                              });
        std::move(callback).Run(true, std::move(sorted));
    }
};

} // namespace

std::unique_ptr<AddressSorter> AddressSorter::CreateAddressSorter()
{
    return std::make_unique<AddressSorterLume>();
}

} // namespace net
