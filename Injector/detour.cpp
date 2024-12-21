#include "pch.h"

#include <filesystem>
#include <type_traits>
#include <ranges>
#include <MinHook.h>

#include "config.h"
#include "detour.h"

#include <boost/nowide/config.hpp>
#include <boost/nowide/convert.hpp>

#include "log.h"

namespace
{
	HANDLE(WINAPI* fpCreateFileW)(
		LPCWSTR lpFileName,
		DWORD dwDesiredAccess,
		DWORD dwShareMode,
		LPSECURITY_ATTRIBUTES lpSecurityAttributes,
		DWORD dwCreationDisposition,
		DWORD dwFlagsAndAttributes,
		HANDLE hTemplateFile
		) = nullptr;

	HANDLE (WINAPI* fpFindFirstFileW)(
		LPCWSTR lpFileName,
		LPWIN32_FIND_DATAW lpFindFileData
    );

	BOOL (WINAPI* fpFindNextFileW)(
		HANDLE hFindFile,
		LPWIN32_FIND_DATAW lpFindFileData
    );

	BOOL (WINAPI* fpFindClose)(
		HANDLE hFindFile
    );

	HANDLE WINAPI DetourCreateFileW(
		LPCWSTR lpFileName,
		DWORD dwDesiredAccess,
		DWORD dwShareMode,
		LPSECURITY_ATTRIBUTES lpSecurityAttributes,
		DWORD dwCreationDisposition,
		DWORD dwFlagsAndAttributes,
		HANDLE hTemplateFile
	)
	{
		std::wstring file_name = GetNormalizedPath(lpFileName);

		if (const auto it = file_mapping.find(file_name); it != file_mapping.end())
		{
			logger->info("CreateFileW redirect: {} -> {}", 
				boost::nowide::narrow(lpFileName), 
				boost::nowide::narrow(it->second));
			lpFileName = it->second.c_str();
		}
		return fpCreateFileW(
			lpFileName,
			dwDesiredAccess,
			dwShareMode,
			lpSecurityAttributes,
			dwCreationDisposition,
			dwFlagsAndAttributes,
			hTemplateFile
		);
	}

	struct DetourFindFileTask
	{
		std::size_t index;
		std::vector<WIN32_FIND_DATAW> file_data_list;
	};

	std::unordered_map<HANDLE, DetourFindFileTask> find_file_override_task;

	std::vector<WIN32_FIND_DATAW> GenerateFindFileData(const std::unordered_set<std::wstring>& files)
	{
		std::vector<WIN32_FIND_DATAW> file_data_list;
		for (const auto& file : files)
		{
			WIN32_FIND_DATAW file_data = {};
			const HANDLE handle = fpFindFirstFileW(file.c_str(), &file_data);
			if (handle != INVALID_HANDLE_VALUE)
			{
				file_data_list.push_back(file_data);
				fpFindClose(handle);
			}
		}
		return file_data_list;
	}

	std::tuple<HANDLE, std::vector<WIN32_FIND_DATAW>> ListAllFileData(LPCWSTR file_path)
	{
		std::vector<WIN32_FIND_DATAW> file_data_list;
		WIN32_FIND_DATAW file_data;
		const HANDLE handle = fpFindFirstFileW(file_path, &file_data); 
		if (handle != INVALID_HANDLE_VALUE)
		{
			file_data_list.push_back(file_data);
			while (fpFindNextFileW(handle, &file_data))
			{
				file_data_list.push_back(file_data);
			}
		}
		return { handle, file_data_list };
	}

	BOOL WINAPI DetourFindNextFileW(
		HANDLE hFindFile,
		LPWIN32_FIND_DATAW lpFindFileData
    )
	{
		if (const auto it = find_file_override_task.find(hFindFile); it != find_file_override_task.end())
		{
			if (it->second.index < it->second.file_data_list.size())
			{
				*lpFindFileData = it->second.file_data_list[it->second.index];
				it->second.index += 1;
				return TRUE;
			}
			return FALSE;
		}
		return fpFindNextFileW(hFindFile, lpFindFileData);
	}

	HANDLE WINAPI DetourFindFirstFileW(
		LPCWSTR lpFileName,
		LPWIN32_FIND_DATAW lpFindFileData
    )
	{
		std::wstring directory_name = GetNormalizedPath(lpFileName);

		if (const auto it = directory_mapping.find(directory_name); it != directory_mapping.end())
		{
			{
				auto file_list = it->second
						| std::views::transform([](const auto& s) { return boost::nowide::narrow(s); })
						| std::views::join_with(',');
				std::string file_list_str;
				std::ranges::copy(file_list, std::back_inserter(file_list_str));

				logger->info("Injecting files into FindFirstFileW result: {} <- [{}]", 
					boost::nowide::narrow(lpFileName), file_list_str);
			}
			DetourFindFileTask task;
			task.file_data_list = GenerateFindFileData(it->second);
			const auto [handle, original_files] = ListAllFileData(lpFileName);
			task.file_data_list.insert_range(task.file_data_list.end(), original_files);
			task.index = 0;
			find_file_override_task.insert({ handle, task });
			DetourFindNextFileW(handle, lpFindFileData);
			return handle;
		}
		return fpFindFirstFileW(lpFileName, lpFindFileData);
	}

	BOOL WINAPI DetourFindClose(
		HANDLE hFindFile
    )
	{
		if (const auto it = find_file_override_task.find(hFindFile); it != find_file_override_task.end())
		{
			find_file_override_task.erase(it);
		}
		return fpFindClose(hFindFile);
	}
}

void DetourInit()
{
	MH_Initialize();

	MH_CreateHook(CreateFileW, DetourCreateFileW, reinterpret_cast<LPVOID*>(&fpCreateFileW));
	MH_EnableHook(CreateFileW);

	MH_CreateHook(FindFirstFileW, DetourFindFirstFileW, reinterpret_cast<LPVOID*>(&fpFindFirstFileW));
	MH_EnableHook(FindFirstFileW);

	MH_CreateHook(FindNextFileW, DetourFindNextFileW, reinterpret_cast<LPVOID*>(&fpFindNextFileW));
	MH_EnableHook(FindNextFileW);

	MH_CreateHook(FindClose, DetourFindClose, reinterpret_cast<LPVOID*>(&fpFindClose));
	MH_EnableHook(FindClose);
}

void DetourClose()
{
	MH_DisableHook(FindClose);
	MH_DisableHook(FindNextFileW);
	MH_DisableHook(FindFirstFileW);
	MH_DisableHook(CreateFileW);
	MH_Uninitialize();
}