#pragma once
#include <Windows.h>
#include <string>
#include <filesystem>
#include "intel_driver.h"
#include "../sdk/windows/ntstructs.h"

namespace service
{
	bool RegisterAndStart( const std::wstring& driver_path );
	bool StopAndRemove( const std::wstring& driver_name );
};