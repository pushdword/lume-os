// SPDX-License-Identifier: BSD-3-Clause
#include "ui/events/devices/device_util_linux.h"

#include "base/files/file_path.h"

namespace ui
{

base::FilePath GetInputPathInSys(const base::FilePath &)
{
    return base::FilePath();
}

InputDeviceType GetInputDeviceTypeFromPath(const base::FilePath &)
{
    // Input paths do not expose bus metadata.
    return InputDeviceType::INPUT_DEVICE_UNKNOWN;
}

} // namespace ui
