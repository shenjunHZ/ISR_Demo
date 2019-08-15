#include <fstream>
#include "ConfigurationLoader.hpp"
#include "AppConfigurations.hpp"
#include "logger/Logger.hpp"

namespace
{
    boost::program_options::options_description createProgramOptionsDescription()
    {
        auto description = boost::program_options::options_description();
        using boost::program_options::value;

        description.add_options()
            (configuration::ISRAppId, value<std::string>()->default_value("5d3807fe"), "ISR Register Id")
            (configuration::ISRWorkDir, value<std::string>()->default_value("./Windows_iat1226_5d3807fe/bin"), "ISR work directory")
            (configuration::ISRUploadOn, value<bool>()->default_value(false), "Does need ISR upload on key words")
            (configuration::ISRUploadUserWords, value<std::string>()->default_value("userwords.txt"), "ISR upload user words")
            (configuration::ISRSessionParams, value<std::string>()->default_value("sub = iat, domain = iat, language = zh_cn, accent = mandarin, sample_rate = 16000, result_type = plain, result_encoding = gb2312"), "ISR session begin params");

        return description;
    }

    boost::program_options::variables_map parseProgramOptions(std::ifstream& boostOptionsStream)
    {
        namespace po = boost::program_options;

        po::variables_map appConfig;
        po::store(po::parse_config_file(boostOptionsStream, createProgramOptionsDescription()), appConfig);
        try
        {
            po::notify(appConfig);
        }
        catch (const po::error& e)
        {
            LOG_ERROR_MSG("Parsing the config file failed: {}", e.what());
            throw;
        }
        return appConfig;
    }
} // namespace

namespace configuration
{
    AppConfiguration loadFromIniFile(const std::string& configFilename)
    {
        std::ifstream configFileStream{ configFilename };
        AppConfiguration configuration{ {parseProgramOptions(configFileStream)} };

        return configuration;
    }

} // namespace configuration
