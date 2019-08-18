#pragma once
#include "configurations/ConfigurationLoader.hpp"

namespace applications
{
    class ControlFunction
    {
    public:
        ControlFunction(const configuration::AppConfiguration& config);
        ~ControlFunction() = default;

        void analysisResult(const std::string& result);

    private:
        const configuration::AppConfiguration& m_config;
    };
}