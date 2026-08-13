#pragma once

#include <cstdint>
#include <chrono>
#include <windows.h>

class Utils
{
public:
	static uint64_t GetCurrentTimestampUs();
	static uint32_t GetMonitorRefreshRate();
};