// SPDX-License-Identifier: BSD-3-Clause

#include "net/base/network_change_notifier_lume.h"

#include <algorithm>
#include <utility>

#include "base/containers/span.h"
#include "base/functional/bind.h"
#include "base/location.h"
#include "base/task/thread_pool.h"
#include "base/time/time.h"
#include "lume/abi.h"
#include "lume/net_abi.h"
#include "lume/syscall_native.h"

namespace net
{

NetworkChangeNotifierLume::NetworkChangeNotifierLume()
{
    timer_.Start(FROM_HERE, base::Seconds(1), this, &NetworkChangeNotifierLume::SchedulePoll);
    SchedulePoll();
}

NetworkChangeNotifierLume::~NetworkChangeNotifierLume()
{
    ClearGlobalPointer();
}

std::optional<NetworkChangeNotifierLume::Snapshot> NetworkChangeNotifierLume::ReadSnapshot()
{
    lume_net_control request = {};
    request.version = LUME_NET_CONTROL_VERSION;
    if (lume_native_call(LUME_SYS_NETCTL, LUME_NET_ENUM, reinterpret_cast<uintptr_t>(&request),
                         sizeof(request)) < 0 ||
        request.count > LUME_NET_INTERFACES)
        return std::nullopt;

    Snapshot snapshot;
    bool connected = false;
    for (const auto &source : base::span(request.data.interfaces).first(request.count))
    {
        InterfaceState state{source.ifindex,
                             source.kind,
                             source.flags,
                             source.address_family,
                             std::to_array(source.address),
                             std::to_array(source.netmask),
                             std::to_array(source.gateway),
                             std::to_array(source.dns)};
        snapshot.interfaces.push_back(state);
        if ((source.flags & (LUME_IF_UP | LUME_IF_LINK)) != (LUME_IF_UP | LUME_IF_LINK) ||
            source.address_family != 4 ||
            !(source.address[0] || source.address[1] || source.address[2] || source.address[3]))
            continue;

        ConnectionType type = source.kind == LUME_IF_WIFI       ? CONNECTION_WIFI
                              : source.kind == LUME_IF_ETHERNET ? CONNECTION_ETHERNET
                                                                : CONNECTION_UNKNOWN;
        if (!connected)
            snapshot.type = type;
        else if (snapshot.type != type)
            snapshot.type = CONNECTION_UNKNOWN;
        connected = true;
    }
    std::sort(snapshot.interfaces.begin(), snapshot.interfaces.end(),
              [](const InterfaceState &left, const InterfaceState &right)
              {
                  return left.index < right.index;
              });
    return snapshot;
}

void NetworkChangeNotifierLume::SchedulePoll()
{
    if (poll_pending_)
        return;
    poll_pending_ = true;
    if (!base::ThreadPool::PostTaskAndReplyWithResult(
            FROM_HERE, {base::MayBlock(), base::TaskPriority::USER_VISIBLE},
            base::BindOnce(&NetworkChangeNotifierLume::ReadSnapshot),
            base::BindOnce(&NetworkChangeNotifierLume::OnSnapshot, weak_factory_.GetWeakPtr())))
        poll_pending_ = false;
}

void NetworkChangeNotifierLume::OnSnapshot(std::optional<Snapshot> snapshot)
{
    poll_pending_ = false;
    if (!snapshot)
        return;
    if (!previous_ || previous_->interfaces != snapshot->interfaces)
    {
        NotifyObserversOfIPAddressChange();
        NotifyObserversOfDNSChange();
    }
    if (connection_type_.exchange(snapshot->type, std::memory_order_acq_rel) != snapshot->type)
        NotifyObserversOfConnectionTypeChange();
    previous_ = std::move(snapshot);
}

NetworkChangeNotifier::ConnectionType NetworkChangeNotifierLume::GetCurrentConnectionType() const
{
    return connection_type_.load(std::memory_order_acquire);
}

} // namespace net
