#pragma once
#include <string>
#include <boost/program_options.hpp>
#include <boost/program_options/parsers.hpp>
#include "AppConfigurations.hpp"

namespace configuration
{
    using AppConfiguration = boost::program_options::variables_map;

    AppConfiguration loadFromIniFile(const std::string& configFilename);
} // namespace configuration