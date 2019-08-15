#pragma once
#include "configurations/AppConfigurations.hpp"
#include "configurations/ConfigurationLoader.hpp"
#include "logger/Logger.hpp"
#include "qtApplications/MainUI.hpp"
#include "DefaultTimerService.hpp"
#include "IOService.hpp"
#include "TimerService.hpp"
#include "ISRService.hpp"

namespace applications
{
    class AppInstance final
    {
    public:
        AppInstance(Logger& logger, const configuration::AppConfiguration& config);
        ~AppInstance();

        void loopFuction();

    private:
        void initService();
        std::string prepareISRSessionParams(const configuration::AppConfiguration& config);

    private:
        bool m_bLoginSDK{false};
        std::unique_ptr<timerservice::IOService> m_ioService{};
        std::unique_ptr<timerservice::DefaultTimerService> m_timerService{};
        std::unique_ptr<ISRService> m_ISRService{};
        std::unique_ptr<MainUI> m_MainUI{};
        std::thread m_ISRAnalysisThread;
    };
} // namespace applications