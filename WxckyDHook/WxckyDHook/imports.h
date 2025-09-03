#pragma once

#include <Windows.h>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
#include <winternl.h>
#include <errhandlingapi.h>
#include <mutex>

#include "minhook/MinHook.h"
#include "misc/xor.hpp"

namespace globals {
	std::string dumpPath = _XOR_("D:\\WxckyDHook\\");
	std::wstring dumpPathW = _XOR_(L"D:\\WxckyDHook\\");
	bool AllocateConsole = true;
}
