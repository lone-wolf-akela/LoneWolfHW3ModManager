#include "pch.h"

#include <spdlog/spdlog.h>
#include <boost/dll/runtime_symbol_info.hpp>

#include "log.h"

std::shared_ptr<spdlog::logger> logger;

void LogInit()
{
	const auto exe_path = boost::dll::program_location().parent_path();

	logger = spdlog::basic_logger_mt("Mod Injector", (exe_path/"mod_injector.log").string(), true);
	logger->info("Initialize logging @ {}", exe_path.string());
}

void LogClose()
{
	logger->info("Close logging");
	spdlog::shutdown();
}