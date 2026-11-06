// SPDX-License-Identifier: BSD-3-Clause

#ifndef NET_BASE_NETWORK_CHANGE_NOTIFIER_LUME_H_
#define NET_BASE_NETWORK_CHANGE_NOTIFIER_LUME_H_

#include <array>
#include <atomic>
#include <cstdint>
#include <optional>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/timer/timer.h"
#include "net/base/network_change_notifier.h"

namespace net
{

class NetworkChangeNotifierLume final : public NetworkChangeNotifier
{
  public:
    NetworkChangeNotifierLume();
    ~NetworkChangeNotifierLume() override;

  private:
    struct InterfaceState
    {
        uint32_t index, kind, flags, family;
        std::array<uint8_t, 4> address, netmask, gateway, dns;
        bool operator==(const InterfaceState &) const = default;
    };

    struct Snapshot
    {
        std::vector<InterfaceState> interfaces;
        ConnectionType type = CONNECTION_NONE;
    };

    static std::optional<Snapshot> ReadSnapshot();
    void SchedulePoll();
    void OnSnapshot(std::optional<Snapshot> snapshot);
    ConnectionType GetCurrentConnectionType() const override;

    std::atomic<ConnectionType> connection_type_{CONNECTION_UNKNOWN};
    std::optional<Snapshot> previous_;
    bool poll_pending_ = false;
    base::RepeatingTimer timer_;
    base::WeakPtrFactory<NetworkChangeNotifierLume> weak_factory_{this};
};

} // namespace net

#endif
