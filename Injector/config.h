#pragma once

#include <unordered_map>
#include <string>
#include <unordered_set>

extern std::unordered_map<std::wstring, std::wstring> file_mapping;

extern std::unordered_map<std::wstring, std::unordered_set<std::wstring>> directory_mapping;

void ConfigInit();
std::wstring GetNormalizedPath(const std::wstring& path);