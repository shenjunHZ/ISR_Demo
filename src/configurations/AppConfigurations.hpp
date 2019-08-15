#pragma once
#include <string>
#include <functional>

#define REGISTER_CONFIG_PREFIX "register"

namespace configuration
{
    constexpr auto ISRAppId = REGISTER_CONFIG_PREFIX ".ISRAppId";
    constexpr auto ISRWorkDir = REGISTER_CONFIG_PREFIX ".ISRWorkDir";
    constexpr auto ISRUploadOn = REGISTER_CONFIG_PREFIX ".ISRUploadOn";
    constexpr auto ISRUploadUserWords = REGISTER_CONFIG_PREFIX ".ISRUploadUserWords";
    constexpr auto ISRSessionParams = REGISTER_CONFIG_PREFIX ".ISRSessionParams";
    constexpr auto ISRVadEosParams = REGISTER_CONFIG_PREFIX ".ISRVadEos";

    // session params according to the sdk
    struct ISRLoginParams
    {
        std::string appId;
        std::string workDir;
        bool uploadOn;
        std::string uploadUserWords;

        ISRLoginParams()
        {
            appId = "5ba5af09";
            workDir = ".";
            uploadOn = false;
            uploadUserWords = "userwords.txt";
        }

        ISRLoginParams(const std::string& id, const std::string& dir, const bool& upload, const std::string& userWords)
        {
            appId = id;
            workDir = dir;
            uploadOn = upload;
            uploadUserWords = userWords;
        }
    };

    /*
    * sub:				请求业务类型
    * domain:			领域
    * language:			语言
    * accent:			方言
    * sample_rate:		音频采样率
    * result_type:		识别结果格式
    * result_encoding:	结果编码格式
    *
    */
    struct ISRSessionBeginParams
    {
        std::string sub;
        std::string domain;
        std::string language;
        std::string accent;
        int         sample_rate;
        std::string result_type;
        std::string result_encoding;

        ISRSessionBeginParams()
        {
            sub = "iat";
            domain = "iat";
            language = "zh_cn";
            accent = "mandarin";
            sample_rate = 16000;
            result_type = "plain";
            result_encoding = "gb2312";
        }
    };

    enum class ISRAudsrc
    {
        SR_MIC,	/* write data from mic */
        SR_USER	/* write data from user by calling API */
    };

    /* error code */
    enum class RecordErrorCode
    {
        RECORD_ERR_BASE = -1,
        RECORD_ERR_GENERAL = -2,
        RECORD_ERR_MEMFAIL = -3,
        RECORD_ERR_INVAL = -4,
        RECORD_ERR_NOT_READY = -5,
        RECORD_ERR_RECORDFAIL = -6,
        RECORD_ERR_ALREADY = -7
    };

    /* Do not change the sequence */
    enum class RecordState
    {
        RECORD_STATE_CREATED,	/* Init		*/
        RECORD_STATE_READY,		/* Opened	*/
        RECORD_STATE_STOPPING,	/* During Stop	*/
        RECORD_STATE_RECORDING,	/* Started	*/
    };

    enum class EventState
    {
        EVT_START = 0,
        EVT_STOP,
        EVT_QUIT,
        EVT_TOTAL
    };

    /* internal state */
    enum class SRState
    {
        SR_STATE_INIT,
        SR_STATE_STARTED
    };

    enum class SpeechEndReason
    {
        END_REASON_VAD_DETECT = 0  /* detected speech done  */
    };

    struct ISRSpeechRecNotifier
    {
        std::function<void(const std::string& result, const char& isLast)> onAnalysisResult{};
        std::function<void()> onSpeechBegin{};
        std::function<void(const SpeechEndReason& reason)> onSpeechEnd{};   /* 0 if VAD.  others, error : see E_SR_xxx and msp_errors.h  */
    };

    /* recorder object. */ // follow sdk define
    struct ISRRecorder
    {
        std::function<void(const std::string& data)> on_data_ind;
        RecordState state;		/* internal record state */
        void* user_cb_para;     // to do remove ?

        void* wavein_hdl;       // HWAVEIN
        void* rec_thread_hdl;   // thread call back handle
        void* bufheader;        // recorder buffer
        unsigned int bufcount;
    };

    struct ISRSpeechRec
    {
        ISRAudsrc audSrc;  /* from mic or manual  stream write */
        ISRSpeechRecNotifier notif;
        std::string sessionId;
        int ep_stat;
        int rec_stat;
        int audio_status;
        ISRRecorder isrRecorder;
        SRState state;
        std::string sessionBeginParams;
    };
} // namespace configuration
