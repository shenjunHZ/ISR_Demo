#include <string>
#include "AppInstance.hpp"
#include "DefaultTimerService.hpp"

namespace
{
    std::string getISRAppId(const configuration::AppConfiguration& config)
    {
        if (config.find(configuration::ISRAppId) != config.end())
        {
            return config[configuration::ISRAppId].as<std::string>();
        }
        return "5d3807fe";
    }

    std::string getISRWorkDir(const configuration::AppConfiguration& config)
    {
        if (config.find(configuration::ISRWorkDir) != config.end())
        {
            return config[configuration::ISRWorkDir].as<std::string>();
        }
        return "./Windows_iat1226_5d3807fe/bin";
    }

    bool getISRUploadOn(const configuration::AppConfiguration& config)
    {
        if (config.find(configuration::ISRUploadOn) != config.end())
        {
            return config[configuration::ISRUploadOn].as<bool>();
        }
        return false;
    }

    std::string getISRUploadUserWords(const configuration::AppConfiguration& config)
    {
        if (config.find(configuration::ISRUploadUserWords) != config.end())
        {
            return config[configuration::ISRUploadUserWords].as<std::string>();
        }
        return "userwords.txt";
    }

    std::string getISRSessionParams(const configuration::AppConfiguration& config)
    {
        if (config.find(configuration::ISRSessionParams) != config.end())
        {
            return config[configuration::ISRSessionParams].as<std::string>();
        }
        return "sub = iat, domain = iat, language = zh_cn, accent = mandarin, sample_rate = 16000, result_type = plain, result_encoding = gb2312";
    }

    std::string getISRVadEosParams(const configuration::AppConfiguration& config)
    {
        if (config.find(configuration::ISRVadEosParams) != config.end())
        {
            return config[configuration::ISRVadEosParams].as<std::string>();
        }
        return "";
    }

}

namespace applications
{
    AppInstance::AppInstance(Logger& logger, const configuration::AppConfiguration& config)
        : m_ioService{ std::make_unique<timerservice::IOService>() }
        , m_timerService{ std::make_unique<timerservice::DefaultTimerService>(*m_ioService) }

    {
        std::string strId = getISRAppId(config);
        std::string strDir = getISRWorkDir(config);
        bool uploadOn = getISRUploadOn(config);
        std::string userWords = getISRUploadUserWords(config);
        configuration::ISRLoginParams loginParams{ std::move(strId), std::move(strDir), std::move(uploadOn), std::move(userWords) };
        //configuration::ISRSessionBeginParams sessionParams;
        std::string sessionParams = prepareISRSessionParams(config);

        m_ISRService = std::make_unique<ISRService>(logger, std::move(loginParams), std::move(sessionParams), config);
        m_MainUI = std::make_unique<MainUI>(*m_ISRService, config);
        m_MainUI->init();

        initService();
    }

    AppInstance::~AppInstance()
    {
        if (nullptr != m_ioService)
        {
            m_ioService->stop();
        }
    }

    void AppInstance::initService()
    {
        if (m_ISRService)
        {
            m_bLoginSDK = m_ISRService->loginSDK();
        }
    }

    void AppInstance::loopFuction()
    {
        if (! m_ISRService)
        {
            return;
        }

        m_ISRService->ISRAnalysis();

        int dlgCode = m_MainUI->exec();

        m_ISRService->exitAnalysisMic();

        if (QDialog::Rejected == dlgCode)
        {
            return;
        }

        return;
    }

    std::string AppInstance::prepareISRSessionParams(const configuration::AppConfiguration& config)
    {
        std::string ISRSessionParams = "";
        ISRSessionParams = getISRSessionParams(config) + getISRVadEosParams(config);

        return ISRSessionParams;
    }

} // namespace applications