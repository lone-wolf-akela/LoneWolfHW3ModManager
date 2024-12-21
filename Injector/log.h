#pragma once

#include <spdlog/sinks/basic_file_sink.h>

extern std::shared_ptr<spdlog::logger> logger;

void LogInit();

void LogClose();