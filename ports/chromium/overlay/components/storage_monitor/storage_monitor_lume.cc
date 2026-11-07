#include "components/storage_monitor/storage_monitor.h"

namespace storage_monitor
{

namespace
{

class StorageMonitorLume final : public StorageMonitor
{
  public:
    StorageMonitorLume() = default;
    ~StorageMonitorLume() override = default;

    bool GetStorageInfoForPath(const base::FilePath &path, StorageInfo *device_info) const override
    {
        return false;
    }

  private:
    void Init() override
    {
        MarkInitialized();
    }
};

} // namespace

StorageMonitor *StorageMonitor::CreateInternal()
{
    return new StorageMonitorLume();
}

} // namespace storage_monitor
