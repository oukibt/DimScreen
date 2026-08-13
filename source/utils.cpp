#include "utils.h"

uint64_t Utils::GetCurrentTimestampUs()
{
	return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

uint32_t Utils::GetMonitorRefreshRate()
{
    DEVMODE dm{};
    dm.dmSize = sizeof(dm);

    if (!EnumDisplaySettings(
        nullptr,
        ENUM_CURRENT_SETTINGS,
        &dm
    ))
    {
        return 60u;
    }

    return (uint32_t)dm.dmDisplayFrequency;
}