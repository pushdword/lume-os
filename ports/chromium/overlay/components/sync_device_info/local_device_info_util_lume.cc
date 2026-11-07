#include <unistd.h>

#include <array>
#include <string>

#include "base/system/sys_info.h"

namespace syncer
{

std::string GetPersonalizableDeviceNameInternal()
{
    std::array<char, 256> hostname{};
    if (gethostname(hostname.data(), hostname.size()) == 0)
    {
        hostname.back() = '\0';
        std::string name(hostname.data());
        if (!name.empty() && name != "localhost" && name != "localhost.localdomain")
        {
            return name;
        }
    }
    return base::SysInfo::OperatingSystemName();
}

} // namespace syncer
