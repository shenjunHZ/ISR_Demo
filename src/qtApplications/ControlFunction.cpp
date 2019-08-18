#include "ControlFunction.hpp"
#include "logger/Logger.hpp"
#include <QtCore/QUrl>
#include <QtGui/QDesktopServices>

namespace
{
    std::string getGnbCodeRepository(const configuration::AppConfiguration& config)
    {
        if (config.find(configuration::gnbCodeRepository) != config.end())
        {
            return config[configuration::gnbCodeRepository].as<std::string>();
        }
        return "";
    }

    std::string getDevelopmentCodeRepository(const configuration::AppConfiguration& config)
    {
        if (config.find(configuration::developmentCodeRepository) != config.end())
        {
            return config[configuration::developmentCodeRepository].as<std::string>();
        }
        return "";
    }

    std::string getUMLDocumentDirectory(const configuration::AppConfiguration& config)
    {
        if (config.find(configuration::UMLDocument) != config.end())
        {
            return config[configuration::UMLDocument].as<std::string>();
        }
        return "";
    }

    std::string getFeatureDocumentDirectory(const configuration::AppConfiguration& config)
    {
        if (config.find(configuration::featureDocument) != config.end())
        {
            return config[configuration::featureDocument].as<std::string>();
        }
        return "";
    }
}
namespace applications
{

    ControlFunction::ControlFunction(const configuration::AppConfiguration& config)
        : m_config{config}
    {

    }

    void ControlFunction::analysisResult(const std::string& result)
    {
        if (std::string::npos != result.find("基站代码"))
        {
            std::string repository = getGnbCodeRepository(m_config);
            if (!repository.empty())
            {
                if (! QDesktopServices::openUrl(QUrl(repository.c_str())))
                {
                    LOG_ERROR_MSG("open gnb code repository failed.");
                }
            }
        }
        else if (std::string::npos != result.find("开发代码"))
        {
            std::string repository = getDevelopmentCodeRepository(m_config);
            if (!repository.empty())
            {
                if (!QDesktopServices::openUrl(QUrl(repository.c_str())))
                {
                    LOG_ERROR_MSG("open development code repository failed.");
                }
            }
        }
        else if (std::string::npos != result.find("时序文档"))
        {
            std::string repository = getUMLDocumentDirectory(m_config);
            if (!repository.empty())
            {
                if (!QDesktopServices::openUrl(QUrl(repository.c_str())))
                {
                    LOG_ERROR_MSG("open UML document directory failed.");
                }
            }
        }
        else if (std::string::npos != result.find("开发文档"))
        {
            std::string repository = getFeatureDocumentDirectory(m_config);
            if (!repository.empty())
            {
                if (!QDesktopServices::openUrl(QUrl(repository.c_str())))
                {
                    LOG_ERROR_MSG("open feature document directory failed.");
                }
            }
        }
    }
} // namespace applications