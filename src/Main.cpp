#include <stdlib.h>
#include <string>
#include <boost/program_options.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <QtWidgets/QApplication>
#include "logger/Logger.hpp"
#include "applications/AppInstance.hpp"
#include "configurations/ConfigurationLoader.hpp"
#include "applications/AppConfig.hpp"

void parseProgramOptions(int argc, char** argv, boost::program_options::variables_map& cmdParams)
{
    namespace po = boost::program_options;
    {
        po::options_description optDescr("General");
        {
            try
            {
                auto defaultConfigFilePath = po::value<std::string>()->default_value("configuration.ini");

                optDescr.add_options()("config-file,c", defaultConfigFilePath, "Configuration file path")(
                    "help,h", "Prints this help message");
            }
            catch (boost::bad_lexical_cast& e)
            {
                LOG_ERROR_MSG("parseProgramOptions:lexical_cast Failed in  default_value");
                std::cerr << e.what() << std::endl;
            }
        }

        try
        {
            po::store(po::parse_command_line(argc, argv, optDescr), cmdParams);
            po::notify(cmdParams);
        }
        catch (const po::error& e)
        {
            std::cerr << e.what() << std::endl;
            std::exit(EXIT_FAILURE);
        }

        if (cmdParams.count("help") != 0u)
        {
            std::cout << optDescr << std::endl;
            std::exit(EXIT_SUCCESS);
        }
    } // namespace po=boost::program_options;
}

void runApp(const configuration::AppConfiguration& configParams)
{
    auto& logger = logger::getLogger();
    LOG_INFO_MSG(logger, "Start to run app");

    applications::AppInstance appInstance(logger, configParams);

    //std::atomic_bool keepRunning(true);
    appInstance.loopFuction();
}

int main(int argc, char* argv[])
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
    QApplication app(argc, argv);
    applications::AppConfig appConfig(app);
    if (appConfig.runOnlyOne())
    {
        return 1;
    }

    boost::program_options::variables_map cmdParams;
    parseProgramOptions(argc, argv, cmdParams);

    try
    {
        configuration::AppConfiguration configParams =
            configuration::loadFromIniFile(cmdParams["config-file"].as<std::string>());
        runApp(configParams);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR_MSG(boost::diagnostic_information(e));
        // app.exec();
        appConfig.releaseShareMemory();
        return EXIT_FAILURE;
    }

    appConfig.releaseShareMemory();
    return EXIT_SUCCESS;
}

