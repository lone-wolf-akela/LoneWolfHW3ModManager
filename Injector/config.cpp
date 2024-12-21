#include "pch.h"

#include <filesystem>
#include <boost/nowide/convert.hpp>
#include <nlohmann/json.hpp>

#include "log.h"
#include "config.h"


std::unordered_map<std::wstring, std::wstring> file_mapping;

std::unordered_map<std::wstring, std::unordered_set<std::wstring>> directory_mapping;

namespace
{
	constexpr int InjectorVersion = 1;

	void CreateNonExistsFolder()
	{
		for (const auto& virtual_path : file_mapping | std::views::keys)
		{
			const auto path = std::filesystem::path(virtual_path).parent_path();
			if (!exists(path))
			{
				create_directories(path);
			}
		}
	}

	void AddDirectoryMapping(const std::wstring& virtual_path, const std::wstring& real_path)
	{
		auto [it, _] = directory_mapping.insert({ virtual_path, {} });
		it->second.insert(real_path);
	}

	void AddFileMapping(const std::wstring& virtual_path, const std::wstring& real_path)
	{
		const auto normalized_virtual_path = GetNormalizedPath(virtual_path);
		const auto normalized_real_path = GetNormalizedPath(real_path);
		file_mapping[normalized_virtual_path] = normalized_real_path;

		const auto normalized_virtual_path_parent = std::filesystem::path(normalized_virtual_path).parent_path();
		AddDirectoryMapping(normalized_virtual_path_parent/"*.*", normalized_real_path);

		logger->info("Mapping: {} -> {}",
			boost::nowide::narrow(normalized_virtual_path), boost::nowide::narrow(normalized_real_path));
	}

	void ClearMapping()
	{
		file_mapping.clear();
		directory_mapping.clear();
	}

	enum class MessageType : std::uint8_t
	{
		ClientHello = 0,
		ServerHello = 1,
		RequestMappingList = 2,
		SetMappingList = 3,
	};

	struct MessageBase
	{
		MessageType type;
		MessageBase(MessageType type) : type(type) {}
	};

	struct ClientHelloMessage : MessageBase
	{
		int client_version;
		ClientHelloMessage() : MessageBase(MessageType::ClientHello), client_version(InjectorVersion) {}

		friend void to_json(nlohmann::json& j, const ClientHelloMessage& obj)
		{
			j["type"] = obj.type;
			j["client_version"] = obj.client_version;
		}

		friend void from_json(const nlohmann::json& j, ClientHelloMessage& obj)
		{
			try
			{
				obj.type = j.at("type");
				obj.client_version = j.at("client_version");
			}
			catch (const nlohmann::json::out_of_range&)
			{
				// skip
			}
		}
	};

	struct ServerHelloMessage : MessageBase
	{
		int server_version;
		ServerHelloMessage() : MessageBase(MessageType::ServerHello), server_version(0) {}

		friend void to_json(nlohmann::json& j, const ServerHelloMessage& obj)
		{
			j["type"] = obj.type;
			j["server_version"] = obj.server_version;
		}

		friend void from_json(const nlohmann::json& j, ServerHelloMessage& obj)
		{
			try
			{
				obj.type = j.at("type");
				obj.server_version = j.at("server_version");
			}
			catch (const nlohmann::json::out_of_range&)
			{
				// skip
			}
		}
	};

	struct RequestMappingListMessage : MessageBase
	{
		RequestMappingListMessage() : MessageBase(MessageType::RequestMappingList) {}

		friend void to_json(nlohmann::json& j, const RequestMappingListMessage& obj)
		{
			j["type"] = obj.type;
		}

		friend void from_json(const nlohmann::json& j, RequestMappingListMessage& obj)
		{
			try
			{
				obj.type = j.at("type");
			}
			catch (const nlohmann::json::out_of_range&)
			{
				// skip
			}
		}
	};

	struct Mapping
	{
		std::wstring virtual_path;
		std::wstring real_path;

		friend void to_json(nlohmann::json& j, const Mapping& obj)
		{
			j["virtual_path"] = boost::nowide::narrow(obj.virtual_path);
			j["real_path"] = boost::nowide::narrow(obj.real_path);
		}

		friend void from_json(const nlohmann::json& j, Mapping& obj)
		{
			try 
			{
				obj.virtual_path = boost::nowide::widen(j.at("virtual_path").get<std::string>());
				obj.real_path = boost::nowide::widen(j.at("real_path").get<std::string>());
			}
			catch (const nlohmann::json::out_of_range&)
			{
				// skip
			}
		}
	};

	struct SetMappingListMessage : MessageBase
	{
		std::vector<Mapping> mappings;
		SetMappingListMessage() : MessageBase(MessageType::SetMappingList) {}

		friend void to_json(nlohmann::json& j, const SetMappingListMessage& obj)
		{
			j["type"] = obj.type;
			j["mappings"] = obj.mappings;
		}

		friend void from_json(const nlohmann::json& j, SetMappingListMessage& obj)
		{
			try
			{
				obj.type = j.at("type");
				obj.mappings = j.at("mappings");
			}
			catch (const nlohmann::json::out_of_range&)
			{
				// skip
			}
		}
	};

	template<typename T>
	void PipeSendMessage(HANDLE hPipe, const T& message)
	{
		const nlohmann::json message_json = message;
		const auto str = message_json.dump();
		const std::uint32_t message_len = static_cast<std::uint32_t>(str.length());
		DWORD dwWritten;
		WriteFile(hPipe, &message_len, sizeof(message_len), &dwWritten, nullptr);
		WriteFile(hPipe, str.c_str(), message_len, &dwWritten, nullptr);
	}

	template<typename T>
	T PipeReceiveMessage(HANDLE hPipe)
	{
		std::uint32_t message_len;
		DWORD dwRead;
		ReadFile(hPipe, &message_len, sizeof(message_len), &dwRead, nullptr);
		std::string message(message_len, '\0');
		ReadFile(hPipe, message.data(), message_len, &dwRead, nullptr);
		auto message_json = nlohmann::json::parse(message);
		return message_json.get<T>();
	}
}


void ConfigInit()
{
	const HANDLE hPipe = CreateFile(
	 L"\\\\.\\pipe\\LoneWolfHW3ModManager",
        GENERIC_READ | GENERIC_WRITE,
        0,
        nullptr,
        OPEN_EXISTING,
        0,
        nullptr);
	if (hPipe == INVALID_HANDLE_VALUE)
	{
		logger->info("Cannot connect to mod manager.");
		return;
	}
	logger->info("Mod manager connected.");

	PipeSendMessage(hPipe, ClientHelloMessage());

	auto server_hello = PipeReceiveMessage<ServerHelloMessage>(hPipe);
	if (server_hello.type != MessageType::ServerHello)
	{
		logger->info("Invalid message type (not server_hello) received.");
		CloseHandle(hPipe);
		return;
	}
	logger->info("Server version: {}", server_hello.server_version);

	PipeSendMessage(hPipe, RequestMappingListMessage());
	logger->info("Requesting mapping list");

	const auto mapping_list = PipeReceiveMessage<SetMappingListMessage>(hPipe);
	if (mapping_list.type != MessageType::SetMappingList)
	{
		logger->info("Invalid message type (not mapping_list) received.");
		CloseHandle(hPipe);
		return;
	}

	ClearMapping();
	for (const auto& mapping : mapping_list.mappings)
	{
		AddFileMapping(mapping.virtual_path, mapping.real_path);
	}

	CreateNonExistsFolder();

	logger->info("Mapping list setup complete. Disconnect from mod manager.");
	CloseHandle(hPipe);
}

static const auto locale = _create_locale(LC_ALL, "en-US");

std::wstring GetNormalizedPath(const std::wstring& path)
{
	std::filesystem::path p = path;
	p = p.lexically_normal();
	auto str = p.wstring();
	_wcsupr_s_l(str.data(), str.size() + 1, locale);
	return str;
}