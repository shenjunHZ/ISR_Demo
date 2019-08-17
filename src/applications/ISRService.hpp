#pragma once
#include "configurations/AppConfigurations.hpp"
#include "logger/Logger.hpp"
#include "IISRService.hpp"
#include "WinRec.hpp"

extern "C" {
#include "qisr.h"
#include "msp_cmn.h"
#include "msp_errors.h"
#include <conio.h>
}

namespace applications
{
    class ISRService final : public IISRService
    {
    public:
        ISRService::ISRService(Logger& logger, configuration::ISRLoginParams& loginParams,
            std::string& sessionParams);
        ~ISRService();

        bool loginSDK() override;
        bool ISRAnalysis() override;

        int srStartListening() override;
        int srStopListening() override;

        void exitAnalysisMic();

        void setRegisterNotify(std::function<void(const std::string&, const bool&)> registerNotify);

    private:
        bool logoutSDK();
        void startISRAnalysisMic();
        int ISRInit(const configuration::ISRAudsrc audSrc, const int devId, configuration::ISRSpeechRecNotifier& notifier);
        void destroyRecorder();
        const char* skipSpace(const char* s);
        int updateFormatFromSessionparam(std::string& session_para, WAVEFORMATEX& wavefmt);

        void ISRUninit();
        void endSrOnError(const int& errorCode);
        int uploadUserWords();
        
        void waitForRecStop(configuration::ISRRecorder& recorder, unsigned int timeout_ms = -1);
        
        void analysisResult(const std::string& result, const char& isLast);
        void showAnalysisResult(const std::string& result, const char& isLast);
        void speechBegin();
        void speechEnd(const configuration::SpeechEndReason& reason);
        void iatCallback(const std::string& data);
        int srWriteAudioData(const std::string& data);
        void endSrOVad();

    private:
        Logger& m_logger;
        configuration::ISRLoginParams m_loginParams;
        std::string m_sessionBeginParams;
        bool m_bLoginSuccess;
        std::unique_ptr<ISysRec> m_sysRec{};
        configuration::ISRSpeechRec m_speechRec;
        std::string m_result{};

        std::function<void(const std::string&, const bool&)> registerNotify{};
    };
} // namespace applications