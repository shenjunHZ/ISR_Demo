//
// Created by shenjun on 18-9-27.
//
#include <spdlog/common.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <vector>
#include "logger/include/logger/LoggerConfiguration.hpp"

const std::vector<spdlog::sink_ptr> LoggerConfiguration::sinks = {std::make_shared<spdlog::sinks::stdout_sink_mt>()};

