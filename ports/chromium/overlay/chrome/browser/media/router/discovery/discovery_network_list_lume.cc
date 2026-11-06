// Copyright 2017 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/discovery/discovery_network_list.h"

#include <algorithm>
#include <iterator>
#include <optional>
#include <string>

#include "base/containers/span.h"
#include "base/strings/string_number_conversions.h"
#include "lume/abi.h"
#include "lume/net_abi.h"
#include "lume/syscall_native.h"

namespace media_router
{

DiscoveryNetworkInfoListResult GetDiscoveryNetworkInfoList()
{
    lume_net_control request = {};
    request.version = LUME_NET_CONTROL_VERSION;
    if (lume_native_call(LUME_SYS_NETCTL, LUME_NET_ENUM, reinterpret_cast<uintptr_t>(&request),
                         sizeof(request)) < 0 ||
        request.count > LUME_NET_INTERFACES)
    {
        return std::nullopt;
    }

    std::vector<DiscoveryNetworkInfo> network_ids;
    for (const auto &source : base::span(request.data.interfaces).first(request.count))
    {
        const auto name_end = std::find(std::begin(source.name), std::end(source.name), '\0');
        if (!source.ifindex || name_end == std::begin(source.name) ||
            name_end == std::end(source.name))
        {
            return std::nullopt;
        }
        if ((source.flags & (LUME_IF_UP | LUME_IF_LINK)) != (LUME_IF_UP | LUME_IF_LINK))
        {
            continue;
        }
        if ((source.kind != LUME_IF_ETHERNET && source.kind != LUME_IF_WIFI) ||
            (source.mac[0] & 1) ||
            std::all_of(std::begin(source.mac), std::end(source.mac),
                        [](uint8_t value)
                        {
                            return value == 0;
                        }))
        {
            return std::nullopt;
        }

        std::string network_id;
        if (source.kind == LUME_IF_WIFI)
        {
            lume_net_control wireless = {};
            wireless.version = LUME_NET_CONTROL_VERSION;
            wireless.ifindex = source.ifindex;
            if (lume_native_call(LUME_SYS_NETCTL, LUME_NET_WIFI_STATE,
                                 reinterpret_cast<uintptr_t>(&wireless), sizeof(wireless)) < 0)
            {
                return std::nullopt;
            }
            const auto &state = wireless.data.wireless;
            if (state.state != LUME_WIFI_CONNECTED || state.ssid_bytes > sizeof(state.ssid))
            {
                return std::nullopt;
            }
            if (state.ssid_bytes)
            {
                network_id.assign(reinterpret_cast<const char *>(state.ssid), state.ssid_bytes);
            }
        }
        if (network_id.empty())
        {
            network_id = base::HexEncode(base::span(source.mac));
        }
        network_ids.emplace_back(std::string(source.name, name_end), network_id);
    }
    StableSortDiscoveryNetworkInfo(network_ids.begin(), network_ids.end());
    return network_ids;
}

} // namespace media_router
