// SPDX-License-Identifier: BSD-3-Clause

#include "net/dns/dns_config_service.h"

#include <memory>

namespace net
{

std::unique_ptr<DnsConfigService> DnsConfigService::CreateSystemService()
{
    // The system resolver owns DNS queries and reads the active interface configuration.
    return nullptr;
}

} // namespace net
