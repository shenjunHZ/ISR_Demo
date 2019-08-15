#include <fstream>
#include "ISRService.hpp"

#define DEFAULT_INPUT_DEVID     (-1)
#define E_SR_NOACTIVEDEVICE 1
#define SR_MEMSET	memset

namespace
{
    constexpr auto DEFAULT_SESSION_PARA = "sub = iat, domain = iat, language = zh_cn, accent = mandarin, sample_rate = 16000, result_type = plain, result_encoding = gb2312";

    COORD begin_pos = { 0, 0 };
    COORD last_pos = { 0, 0 };

    std::atomic_bool keep_running = true;
} // namespace

namespace applications
{
    std::vector<HANDLE> ISRService::m_events{ nullptr, nullptr, nullptr };
    HANDLE ISRService::m_helperThreadHandle{ nullptr };

    ISRService::ISRService(Logger& logger, configuration::ISRLoginParams& loginParams,
        std::string& sessionParams)
        : m_logger{ logger }
        , m_loginParams{ loginParams }
        , m_sessionBeginParams{ sessionParams }
        , m_bLoginSuccess{ false }
    {

    }

    ISRService::~ISRService()
    {
        if (m_bLoginSuccess)
        {
            logoutSDK();
        }
    }

    bool ISRService::logoutSDK()
    {
        int ret = MSPLogout();
        if (MSP_SUCCESS != ret) //退出登录
        {
            LOG_ERROR_MSG("MSPLogin failed, error code: %d.", ret);
        }

        m_bLoginSuccess = false;
        LOG_INFO_MSG(m_logger, "MSPLogout success.");
        return true;
    }

    bool ISRService::loginSDK()
    {
        int ret = MSP_SUCCESS;
        std::string loginParams = "appid = " + m_loginParams.appId + ", work_dir = " + m_loginParams.workDir;
        LOG_DEBUG_MSG("login params: {}", loginParams);

        /* 用户登录 */
        ret = MSPLogin(NULL, NULL, loginParams.c_str()); //第一个参数是用户名，第二个参数是密码，第三个参数是登录参数，用户名和密码可在http://www.xfyun.cn注册获取
        if (MSP_SUCCESS != ret)
        {
            LOG_ERROR_MSG("MSPLogin failed, error code: {}.", ret);
            m_bLoginSuccess = false;
            return false;
        }
        LOG_INFO_MSG(m_logger, "MSPLogin success. login params: {}", loginParams.c_str());
        m_bLoginSuccess = true;
        return true;
    }

    bool ISRService::ISRAnalysis()
    {
        LOG_INFO_MSG(m_logger, "Start to record analysis.");
        if (m_loginParams.uploadOn)
        {
            LOG_DEBUG_MSG("Start to upload user words.");
            uploadUserWords();
        }

        startISRAnalysisMic();
        return true;
    }

    void ISRService::startISRAnalysisMic()
    {
        int errcode;
        int i = 0;

        DWORD waitres;
        char isquit = 0;

        configuration::ISRSpeechRecNotifier recnotifier =
        {
            std::bind(&ISRService::analysisResult, this, std::placeholders::_1, std::placeholders::_2),
            std::bind(&ISRService::speechBegin, this),
            std::bind(&ISRService::speechEnd, this, std::placeholders::_1)
        };

        errcode = ISRInit(configuration::ISRAudsrc::SR_MIC, DEFAULT_INPUT_DEVID, recnotifier);
        if (errcode)
        {
            LOG_DEBUG_MSG("speech recognizer init failed!");
            return;
        }

//         auto total = static_cast<int>(configuration::EventState::EVT_TOTAL);
//         for (i = 0; i < total; ++i)
//         {
//             m_events[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
//         }

//         m_helperThreadHandle = startHelperThread();
//         if (nullptr == m_helperThreadHandle)
//         {
//             LOG_DEBUG_MSG("create thread failed.");
//             exitAnalysisMic();
//             return;
//         }

//        showKeyHints();

//         while (keep_running)
//         {
//             waitres = WaitForMultipleObjects(total, &m_events[0], FALSE, INFINITE);
//             switch (waitres)
//             {
//             case WAIT_FAILED:
//             case WAIT_TIMEOUT:
//                 LOG_DEBUG_MSG("Why it happened !");
//                 break;
//             case WAIT_OBJECT_0 + static_cast<int>(configuration::EventState::EVT_START) :
//                 if (errcode = srStartListening())
//                 {
//                     LOG_DEBUG_MSG("start listen failed {}", errcode);
//                     isquit = 1;
//                 }
//                 break;
//             case WAIT_OBJECT_0 + static_cast<int>(configuration::EventState::EVT_STOP) :
//                 if (errcode = srStopListening())
//                 {
//                     LOG_DEBUG_MSG("stop listening failed {}", errcode);
//                     isquit = 1;
//                 }
//                 break;
//             case WAIT_OBJECT_0 + static_cast<int>(configuration::EventState::EVT_QUIT) :
//                 srStopListening();
//                 isquit = 1;
//                 break;
//             default:
//                 break;
//             }
//             if (isquit)
//                 break;
//         }
// 
//         exitAnalysisMic();
    }

    int ISRService::ISRInit(const configuration::ISRAudsrc audSrc, const int devId,
        configuration::ISRSpeechRecNotifier& notifier)
    {
        int errcode = 0;
        WAVEFORMATEX wavfmt = { WAVE_FORMAT_PCM, 1, 16000, 32000, 2, 16, sizeof(WAVEFORMATEX) };

        if (audSrc == configuration::ISRAudsrc::SR_MIC && m_sysRec && m_sysRec->getInputDevNum() == 0)
        {
            return -E_SR_NOACTIVEDEVICE;
        }

        if (m_sessionBeginParams.empty())
        {
            m_sessionBeginParams = DEFAULT_SESSION_PARA;
        }
        SR_MEMSET(&m_speechRec, 0, sizeof(configuration::ISRSpeechRec));
        m_speechRec.state = configuration::SRState::SR_STATE_INIT;
        m_speechRec.audSrc = audSrc;
        m_speechRec.ep_stat = MSP_EP_LOOKING_FOR_SPEECH;
        m_speechRec.rec_stat = MSP_REC_STATUS_SUCCESS;
        m_speechRec.audio_status = MSP_AUDIO_SAMPLE_FIRST;
        m_speechRec.sessionBeginParams = m_sessionBeginParams;
        m_speechRec.notif = std::move(notifier);

        updateFormatFromSessionparam(m_sessionBeginParams, wavfmt);
        m_sysRec = std::make_unique<WinRec>(std::make_unique<WAVEFORMATEX>(wavfmt));

        if (audSrc == configuration::ISRAudsrc::SR_MIC && m_sysRec)
        {
            errcode = m_sysRec->createRecorder(m_speechRec.isrRecorder, std::bind(&ISRService::iatCallback, this, std::placeholders::_1),
                static_cast<void*>(&m_speechRec));

            if (0 != errcode)
            {
                LOG_DEBUG_MSG("create recorder failed: {}", errcode);
                errcode = static_cast<int>(configuration::RecordErrorCode::RECORD_ERR_RECORDFAIL);
                destroyRecorder();
                return errcode;
            }

            errcode = m_sysRec->openRecorder(m_speechRec.isrRecorder, devId);
            if (0 != errcode)
            {
                LOG_DEBUG_MSG("recorder open failed: {}", errcode);
                errcode = static_cast<int>(configuration::RecordErrorCode::RECORD_ERR_RECORDFAIL);
                destroyRecorder();
                return errcode;
            }
        }

        return 0;
    }

    void ISRService::destroyRecorder()
    {
        if (m_sysRec)
        {
            m_sysRec->destroyRecorder(m_speechRec.isrRecorder);
        }

        m_speechRec.sessionBeginParams = "";
    }

    const char* ISRService::skipSpace(const char* s)
    {
        while (s && *s != ' ' && *s != '\0')
        {
            s++;
        }
        return s;
    }

    int ISRService::updateFormatFromSessionparam(std::string& session_para, WAVEFORMATEX& wavefmt)
    {
        const char *s;
        if ((s = strstr(session_para.c_str(), "sample_rate")))
        {
            if (s = strstr(s, "="))
            {
                s = skipSpace(s);
                if (s && *s)
                {
                    wavefmt.nSamplesPerSec = atoi(s);
                    wavefmt.nAvgBytesPerSec = wavefmt.nBlockAlign * wavefmt.nSamplesPerSec;
                    return 0;
                }
            }
        }

        return -1;
    }

    HANDLE ISRService::startHelperThread()
    {
        HANDLE hdl;

        hdl = (HANDLE)(_beginthreadex(NULL, 0, helperThreadProc, NULL, 0, NULL));

        return hdl;
    }

    /* helper thread: to listen to the keystroke */
    unsigned int  __stdcall ISRService::helperThreadProc(void* para)
    {
        int key;
        int quit = 0;

        do
        {
            key = _getch();
            switch (key)
            {
            case 'r':
            case 'R':
                SetEvent(m_events[static_cast<int>(configuration::EventState::EVT_START)]);
                break;
            case 's':
            case 'S':
                SetEvent(m_events[static_cast<int>(configuration::EventState::EVT_STOP)]);
                break;
            case 'q':
            case 'Q':
                quit = 1;
                SetEvent(m_events[static_cast<int>(configuration::EventState::EVT_QUIT)]);
                PostQuitMessage(0);
                break;
            default:
                break;
            }

            if (quit)
                break;
        } while (1);

        return 0;
    }

    void ISRService::exitAnalysisMic()
    {
        if (nullptr != m_helperThreadHandle)
        {
            WaitForSingleObject(m_helperThreadHandle, INFINITE);
            CloseHandle(m_helperThreadHandle);
        }

        auto total = static_cast<int>(configuration::EventState::EVT_TOTAL);
        for (int i = 0; i < total; ++i)
        {
            if (m_events[i])
            {
                CloseHandle(m_events[i]);
            }
        }

        ISRUninit();
    }

    void ISRService::ISRUninit()
    {
        if (nullptr != m_sysRec)
        {
            if (!m_sysRec->isRecordStopped(m_speechRec.isrRecorder))
            {
                m_sysRec->stopRecord(m_speechRec.isrRecorder);
            }
            m_sysRec->closeRecorder(m_speechRec.isrRecorder);
            m_sysRec->destroyRecorder(m_speechRec.isrRecorder);
        }

        m_speechRec.sessionBeginParams = "";
    }

    void ISRService::endSrOnError(const int& errorCode)
    {
        if (m_speechRec.audSrc == configuration::ISRAudsrc::SR_MIC)
        {
            m_sysRec->stopRecord(m_speechRec.isrRecorder);
        }

        if (!m_speechRec.sessionId.empty())
        {
            if (m_speechRec.notif.onSpeechEnd)
            {
                m_speechRec.notif.onSpeechEnd(static_cast<configuration::SpeechEndReason>(errorCode));
            }

            QISRSessionEnd(m_speechRec.sessionId.c_str(), "err");
            m_speechRec.sessionId = "";
        }
        m_speechRec.state = configuration::SRState::SR_STATE_INIT;
#ifdef __FILE_SAVE_VERIFY__
        safe_close_file();
#endif
    }

    int ISRService::uploadUserWords()
    {
        std::ifstream fsRead;
        fsRead.open(m_loginParams.uploadUserWords.c_str(), std::ios::in | std::ios::binary);

        if (!fsRead)
        {
            LOG_ERROR_MSG("open [userwords.txt] failed !");
            return -1;
        }

        fsRead.seekg(0, fsRead.end);
        std::streamoff len = fsRead.tellg(); //获取文件大小fs
        fsRead.seekg(0, fsRead.beg);

        std::string userwords((std::istreambuf_iterator<char>(fsRead)), (std::istreambuf_iterator<char>()));
        //        fsRead.read((char*)userwords.c_str(), len);
        //        int readLen = fsRead.gcount();

        if (userwords.size() != len)
        {
            LOG_ERROR_MSG("read [userwords.txt] failed !");
            fsRead.close();
            return -1;
        }

        int ret = 0;
        MSPUploadData("userwords", (void*)userwords.c_str(), len, "sub = uup, dtt = userword", &ret); //上传用户词表
        if (MSP_SUCCESS != ret)
        {
            LOG_ERROR_MSG("MSPUploadData failed ! errorCode: {}", ret);
            fsRead.close();
            return ret;
        }
        return ret;
    }

    /************************************speech to recognize*************************************************/

    int ISRService::srStartListening()
    {
        int ret;
        const char*	session_id = nullptr;
        int	errcode = MSP_SUCCESS;

        if (m_speechRec.state >= configuration::SRState::SR_STATE_STARTED)
        {
            LOG_DEBUG_MSG("already STARTED.");
            return static_cast<int>(configuration::RecordErrorCode::RECORD_ERR_ALREADY);
        }

        session_id = QISRSessionBegin(NULL, m_speechRec.sessionBeginParams.c_str(), &errcode);
        if (MSP_SUCCESS != errcode)
        {
            LOG_DEBUG_MSG("QISRSessionBegin failed! error code: {}", errcode);
            return errcode;
        }
        m_speechRec.sessionId = session_id;
        m_speechRec.ep_stat = MSP_EP_LOOKING_FOR_SPEECH;
        m_speechRec.rec_stat = MSP_REC_STATUS_SUCCESS;
        m_speechRec.audio_status = MSP_AUDIO_SAMPLE_FIRST;

        if (configuration::ISRAudsrc::SR_MIC == m_speechRec.audSrc && nullptr != m_sysRec)
        {
            ret = m_sysRec->startRecord(m_speechRec.isrRecorder);
            if (ret != 0)
            {
                LOG_DEBUG_MSG("start record failed: {}", ret);
                QISRSessionEnd(session_id, "start record fail");
                m_speechRec.sessionId = "";
                return static_cast<int>(configuration::RecordErrorCode::RECORD_ERR_RECORDFAIL);
            }
#ifdef __FILE_SAVE_VERIFY__
            open_stored_file(VERIFY_FILE_NAME);
#endif
        }

        m_speechRec.state = configuration::SRState::SR_STATE_STARTED;

        if (nullptr != m_speechRec.notif.onSpeechBegin)
        {
            m_speechRec.notif.onSpeechBegin();
        }

        return 0;
    }

    void ISRService::showKeyHints()
    {
        printf("\n\
----------------------------\n\
Press r to start speaking\n\
Press s to end your speaking\n\
Press q to quit\n\
----------------------------\n");
    }

    /* after stop_record, there are still some data callbacks */
    void ISRService::waitForRecStop(configuration::ISRRecorder& recorder, unsigned int timeout_ms /*= -1*/)
    {
        if (nullptr == m_sysRec)
        {
            return;
        }
        while (!m_sysRec->isRecordStopped(recorder))
        {
            Sleep(1);
            if (timeout_ms != (unsigned int)-1)
            {
                if (0 == timeout_ms--)
                {
                    break;
                }
            }
        }
    }

    int ISRService::srStopListening()
    {
        int ret = 0;
        const char* rslt = nullptr;

        if (m_speechRec.state < configuration::SRState::SR_STATE_STARTED)
        {
            LOG_ERROR_MSG("Not started or already stopped.");
            return 0;
        }

        if (m_speechRec.audSrc == configuration::ISRAudsrc::SR_MIC && nullptr != m_sysRec)
        {
            ret = m_sysRec->stopRecord(m_speechRec.isrRecorder);
#ifdef __FILE_SAVE_VERIFY__
            safe_close_file();
#endif
            if (ret != 0)
            {
                LOG_ERROR_MSG("Stop failed!");
                return static_cast<int>(configuration::RecordErrorCode::RECORD_ERR_RECORDFAIL);
            }
            waitForRecStop(m_speechRec.isrRecorder);
        }
        m_speechRec.state = configuration::SRState::SR_STATE_INIT;
        ret = QISRAudioWrite(m_speechRec.sessionId.c_str(), NULL, 0, MSP_AUDIO_SAMPLE_LAST, &m_speechRec.ep_stat, &m_speechRec.rec_stat);
        if (ret != 0)
        {
            LOG_ERROR_MSG("write LAST_SAMPLE failed: {}", ret);
            QISRSessionEnd(m_speechRec.sessionId.c_str(), "write err");
            return ret;
        }

        while (m_speechRec.rec_stat != MSP_REC_STATUS_COMPLETE)
        {
            rslt = QISRGetResult(m_speechRec.sessionId.c_str(), &m_speechRec.rec_stat, 0, &ret);
            if (MSP_SUCCESS != ret)
            {
                LOG_DEBUG_MSG("QISRGetResult failed! error code: {}", ret);
                endSrOnError(ret);
                return ret;
            }
            if (NULL != rslt && m_speechRec.notif.onAnalysisResult)
            {
                m_speechRec.notif.onAnalysisResult(rslt, m_speechRec.rec_stat == MSP_REC_STATUS_COMPLETE ? 1 : 0);
            }
            Sleep(100);
        }

        QISRSessionEnd(m_speechRec.sessionId.c_str(), "normal");
        m_speechRec.sessionId = "";
        return 0;
    }

    /************************************call back************************************************/
    void ISRService::analysisResult(const std::string& result, const char& isLast)
    {
        if (!result.empty())
        {
            m_result += result;
            showAnalysisResult(m_result, isLast);
        }
    }

    void ISRService::showAnalysisResult(const std::string& result, const char& isLast)
    {
        COORD current;
        CONSOLE_SCREEN_BUFFER_INFO info;
        HANDLE w = GetStdHandle(STD_OUTPUT_HANDLE);
        GetConsoleScreenBufferInfo(w, &info);
        current = info.dwCursorPosition;

        if (current.X == last_pos.X && current.Y == last_pos.Y)
        {
            SetConsoleCursorPosition(w, begin_pos);
        }
        else
        {
            /* changed by other routines, use the new pos as start */
            begin_pos = current;
        }
        if (isLast)
        {
            SetConsoleTextAttribute(w, FOREGROUND_GREEN);
        }
        LOG_DEBUG_MSG("Result: {}", result);
        if (isLast)
        {
            SetConsoleTextAttribute(w, info.wAttributes);
        }

        GetConsoleScreenBufferInfo(w, &info);
        last_pos = info.dwCursorPosition;
    }

    void ISRService::speechBegin()
    {
        m_result = "";

        LOG_INFO_MSG(m_logger, "Start Listening...");
    }

    void ISRService::speechEnd(const configuration::SpeechEndReason& reason)
    {
        if (configuration::SpeechEndReason::END_REASON_VAD_DETECT == reason)
        {
            LOG_INFO_MSG(m_logger, "Speaking done.");
        }
        else
        {
            LOG_INFO_MSG(m_logger, "Recognizer error: {}", static_cast<int>(reason));
        }
    }

    void ISRService::iatCallback(const std::string& data)
    {
        int errcode = 0;

        if (data.empty() || m_speechRec.ep_stat >= MSP_EP_AFTER_SPEECH)
        {
            return;
        }

#ifdef __FILE_SAVE_VERIFY__
        loopwrite_to_file(data, len);
#endif
        errcode = srWriteAudioData(data);
        if (errcode)
        {
            endSrOnError(errcode);
            return;
        }
    }

    int ISRService::srWriteAudioData(const std::string& data)
    {
        if (data.empty())
        {
            return 0;
        }

        int ret = QISRAudioWrite(m_speechRec.sessionId.c_str(), data.c_str(), data.size(), m_speechRec.audio_status, &m_speechRec.ep_stat, &m_speechRec.rec_stat);
        if (ret)
        {
            endSrOnError(ret);
            return ret;
        }
        m_speechRec.audio_status = MSP_AUDIO_SAMPLE_CONTINUE;

        if (MSP_REC_STATUS_SUCCESS == m_speechRec.rec_stat)
        { //已经有部分听写结果
            const char* rslt = QISRGetResult(m_speechRec.sessionId.c_str(), &m_speechRec.rec_stat, 0, &ret);
            if (MSP_SUCCESS != ret)
            {
                LOG_DEBUG_MSG("QISRGetResult failed! error code: {}", ret);
                endSrOnError(ret);
                return ret;
            }
            if (nullptr != rslt && m_speechRec.notif.onAnalysisResult)
            {
                m_speechRec.notif.onAnalysisResult(rslt, m_speechRec.rec_stat == MSP_REC_STATUS_COMPLETE ? 1 : 0);
            }
        }

        if (MSP_EP_AFTER_SPEECH == m_speechRec.ep_stat)
        {
            endSrOVad();
        }

        return 0;
    }

    void ISRService::endSrOVad()
    {
        int errcode;
        const char *rslt = nullptr;

        if (m_speechRec.audSrc == configuration::ISRAudsrc::SR_MIC)
        {
            m_sysRec->stopRecord(m_speechRec.isrRecorder);
        }

        while (m_speechRec.rec_stat != MSP_REC_STATUS_COMPLETE)
        {
            rslt = QISRGetResult(m_speechRec.sessionId.c_str(), &m_speechRec.rec_stat, 0, &errcode);
            if (rslt && m_speechRec.notif.onAnalysisResult)
            {
                m_speechRec.notif.onAnalysisResult(rslt, m_speechRec.rec_stat == MSP_REC_STATUS_COMPLETE ? 1 : 0);
            }

            Sleep(100); /* for cpu occupy, should sleep here */
        }

        if (!m_speechRec.sessionId.empty())
        {
            if (m_speechRec.notif.onSpeechEnd)
            {
                m_speechRec.notif.onSpeechEnd(configuration::SpeechEndReason::END_REASON_VAD_DETECT);
            }
            QISRSessionEnd(m_speechRec.sessionId.c_str(), "VAD Normal");
            m_speechRec.sessionId = "";
        }
        m_speechRec.state = configuration::SRState::SR_STATE_INIT;
#ifdef __FILE_SAVE_VERIFY__
        safe_close_file();
#endif
    }

    void ISRService::exitISRService()
    {
        keep_running = false;
    }

} // namespace applications