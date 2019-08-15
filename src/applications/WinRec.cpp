#include "WinRec.hpp"
#include "logger/Logger.hpp"

namespace
{
    const int BUF_COUNT = 4;
    const int FRAME_CNT = 10;
    const int SAMPLE_RATE = 16000;
    const int SAMPLE_BIT_SIZE = 16;
} // namespace

namespace applications
{
    HANDLE WinRec::m_msgqueueReadyEvt{ nullptr };

    WinRec::WinRec(std::unique_ptr<WAVEFORMATEX> waveFormat)
        : m_waveFormat{ std::move(waveFormat) }
    {

    }

    unsigned int WinRec::getInputDevNum()
    {
        return waveInGetNumDevs();
    }

    int WinRec::getDefaultInputDev()
    {
        return WAVE_MAPPER;
    }

    int WinRec::startRecord(configuration::ISRRecorder& recorder)
    {
        int ret;

        if (recorder.state < configuration::RecordState::RECORD_STATE_READY)
        {
            return static_cast<int>(configuration::RecordErrorCode::RECORD_ERR_NOT_READY);
        }
        if (recorder.state == configuration::RecordState::RECORD_STATE_RECORDING)
        {
            return 0;
        }

        ret = startRecordInternal(static_cast<HWAVEIN>(recorder.wavein_hdl), reinterpret_cast<WAVEHDR*>(recorder.bufheader), recorder.bufcount);
        if (ret == 0)
        {
            recorder.state = configuration::RecordState::RECORD_STATE_RECORDING;
        }
        return ret;
    }

    void WinRec::closeRecorder(configuration::ISRRecorder& recorder)
    {
        if (recorder.state < configuration::RecordState::RECORD_STATE_READY)
        {
            return;
        }
        if (recorder.state == configuration::RecordState::RECORD_STATE_RECORDING)
        {
            stopRecord(recorder);
        }
        closeRecorderInternal(recorder);

        recorder.state = configuration::RecordState::RECORD_STATE_CREATED;
    }

    void WinRec::destroyRecorder(configuration::ISRRecorder& recorder)
    {
        LOG_DEBUG_MSG("Recorder destroy.");
    }

    int WinRec::startRecordInternal(HWAVEIN wi, WAVEHDR* header, unsigned int bufcount)
    {
        MMRESULT res;
        unsigned int i;

        if (bufcount < 2)
            return -1;

        /* must put at least one buffer into the driver first.
        and this buffer must has been allocated and prepared. */
        for (i = 0; i < bufcount; ++i)
        {
            if ((header->dwFlags & WHDR_INQUEUE) == 0)
            {
                header->dwUser = i + 1;
                res = waveInAddBuffer(wi, header, sizeof(WAVEHDR));
                if (res != MMSYSERR_NOERROR)
                {
                    waveInReset(wi);
                    return 0 - res;
                }
            }
            header++;
        }
        res = waveInStart(wi);
        if (MMSYSERR_NOERROR != res)
        {
            waveInReset(wi);
            return 0 - res;
        }

        return 0;
    }

    void WinRec::closeRecorderInternal(configuration::ISRRecorder& recorder)
    {
        if (recorder.wavein_hdl)
        {
            closeRecDevice(static_cast<HWAVEIN>(recorder.wavein_hdl));

            if (recorder.rec_thread_hdl)
            {
                closeCallbackThread(static_cast<HWAVEIN>(recorder.rec_thread_hdl));
                recorder.rec_thread_hdl = nullptr;
            }
            if (recorder.bufheader)
            {
                freeRecBuffer(static_cast<HWAVEIN>(recorder.wavein_hdl), static_cast<WAVEHDR*>(recorder.bufheader), recorder.bufcount);
                recorder.bufheader = nullptr;
                recorder.bufcount = 0;
            }
            recorder.wavein_hdl = nullptr;
        }
    }

    int WinRec::preparRecBuffer(HWAVEIN wi, WAVEHDR** bufheader_out, unsigned int headercount, unsigned int bufsize)
    {
        int ret = 0;
        unsigned int i = 0;
        WAVEHDR *header;
        MMRESULT res;

        /* at least doubel buffering */
        if (headercount < 2 || bufheader_out == nullptr)
        {
            return static_cast<int>(configuration::RecordErrorCode::RECORD_ERR_INVAL);
        }

        //header = (WAVEHDR *)malloc(sizeof(WAVEHDR)* headercount);
        header = new WAVEHDR[headercount];
        if (!header)
        {
            return static_cast<int>(configuration::RecordErrorCode::RECORD_ERR_MEMFAIL);
        }
        memset(header, 0, sizeof(WAVEHDR) * headercount);

        for (i = 0; i < headercount; ++i)
        {
            //(header + i)->lpData = (LPSTR)malloc(bufsize);
            (header + i)->lpData = new char[bufsize];

            if ((header + i)->lpData == nullptr)
            {
                freeRecBuffer(wi, header, headercount);
                return ret;
            }
            (header + i)->dwBufferLength = bufsize;
            (header + i)->dwFlags = 0;
            (header + i)->dwUser = i + 1; /* my usage: if 0, indicate it's not used */

            res = waveInPrepareHeader(wi, header + i, sizeof(WAVEHDR));
            if (res != MMSYSERR_NOERROR)
            {
                freeRecBuffer(wi, header, headercount);
                return ret;
            }
        }

        *bufheader_out = header;
        return ret;
    }

    void WinRec::printWaveHeader(WAVEHDR& buf)
    {
        LOG_DEBUG_MSG("========================================");
        LOG_DEBUG_MSG("Len = {}, Rec = {}, Flag = {}",
            static_cast<unsigned long>(buf.dwBufferLength), static_cast<unsigned long>(buf.dwBytesRecorded), static_cast<unsigned long>(buf.dwFlags));
        LOG_DEBUG_MSG("========================================");
    }

    /*******************************************************
        for open device and create buffer
    *******************************************************/
    // create recorder
    using namespace configuration;

    int WinRec::createRecorder(configuration::ISRRecorder& recorder, std::function<void(const std::string& data)> on_data_ind, void* user_cb_para)
    {
        recorder.on_data_ind = on_data_ind;
        recorder.user_cb_para = user_cb_para;
        recorder.state = RecordState::RECORD_STATE_CREATED;

        return 0;
    }

    int WinRec::openRecorder(configuration::ISRRecorder& recorder, unsigned int dev)
    {
        if (recorder.state >= RecordState::RECORD_STATE_READY)
        {
            return 0;
        }

        int ret = openRecorderInternal(recorder, dev);
        if (ret == 0)
        {
            recorder.state = RecordState::RECORD_STATE_READY;
        }
        return ret;
    }

    void WinRec::dataProc(configuration::ISRRecorder& recorder, MSG& msg)
    {
        HWAVEIN whdl = (HWAVEIN)msg.wParam;
        WAVEHDR* buf = (WAVEHDR *)msg.lParam;

        //LOG_DEBUG_MSG("recorder data....");
        //printWaveHeader(*buf);

        /* dwUser should be index + 1 */
        if (buf->dwUser > recorder.bufcount)
        {
            LOG_ERROR_MSG("data_proc: something wrong. maybe buffer is reset.");
            return;
        }

        std::string recordData(buf->lpData, buf->dwBytesRecorded);
        //recorder.on_data_ind(recordData, recorder.user_cb_para);
        recorder.on_data_ind(recordData);

        switch (recorder.state)
        {
        case RecordState::RECORD_STATE_RECORDING:
            // after copied, put it into the queue of driver again.
            waveInAddBuffer(whdl, buf, sizeof(WAVEHDR));
            break;
        case RecordState::RECORD_STATE_STOPPING:
        default:
            /* from this flag, can check if the whole data is processed after stopping */
            buf->dwUser = 0;
            break;
        }
    }

    unsigned int  __stdcall WinRec::recordThreadProc(void* para)
    {
        MSG msg;
        BOOL bRet;
        ISRRecorder* recorder = static_cast<ISRRecorder*>(para);

        /* trigger the message queue generator */
        PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);
        SetEvent(m_msgqueueReadyEvt);

        while ((bRet = GetMessage(&msg, NULL, 0, 0)) != 0)
        {
            if (bRet == -1)
            {
                continue;
            }

            switch (msg.message) {
            case MM_WIM_OPEN:
                LOG_DEBUG_MSG("record opened....");
                break;
            case MM_WIM_CLOSE:
                LOG_DEBUG_MSG("record closed....");
                PostQuitMessage(0);
                break;
            case MM_WIM_DATA:
                dataProc(*recorder, msg);
                break;
            default:
                break;
            }
        }

        return 0;
    }

    int WinRec::createCallbackThread(void* thread_proc_para, HANDLE* thread_hdl_out)
    {
        int ret = 0;

        HANDLE rec_thread_hdl = 0;
        unsigned int rec_thread_id;

        /* For indicating the thread's life stage.
           . signaled after the call back thread started and create its message
             queue.
           . will be close after callback thread exit.
           . must be manual reset. once signaled, keep in signaled state */
        m_msgqueueReadyEvt = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (nullptr == m_msgqueueReadyEvt)
        {
            return -1;
        }

        rec_thread_hdl = (HANDLE)_beginthreadex(NULL, 0, WinRec::recordThreadProc, thread_proc_para, 0, &rec_thread_id);
        if (rec_thread_hdl == 0)
        {
            /* close the event handle */
            CloseHandle(m_msgqueueReadyEvt);
            m_msgqueueReadyEvt = nullptr;

            return -1;
        }


        *thread_hdl_out = rec_thread_hdl;

        /* wait the message queue of the new thread has been created */
        WaitForSingleObject(m_msgqueueReadyEvt, INFINITE);

        return 0;
    }

    int WinRec::openRecDevice(int dev, HANDLE thread, HWAVEIN* wave_hdl_out)
    {
        MMRESULT res;
        HWAVEIN wi = nullptr;

        if (thread == nullptr)
        {
            return 0 - static_cast<int>(RecordErrorCode::RECORD_ERR_INVAL);
        }

        if (nullptr == m_waveFormat)
        {
            m_waveFormat = std::make_shared<WAVEFORMATEX>();
            m_waveFormat->wFormatTag = WAVE_FORMAT_PCM;
            m_waveFormat->nChannels = 1;
            m_waveFormat->nSamplesPerSec = SAMPLE_RATE;
            m_waveFormat->nAvgBytesPerSec = SAMPLE_RATE * (SAMPLE_BIT_SIZE >> 3);
            m_waveFormat->nBlockAlign = 2;
            m_waveFormat->wBitsPerSample = SAMPLE_BIT_SIZE;
            m_waveFormat->cbSize = sizeof(WAVEFORMATEX);
        }
        WAVEFORMATEX* final_fmt = m_waveFormat.get();

        res = waveInOpen((LPHWAVEIN)&wi, dev, final_fmt, GetThreadId(thread), (DWORD_PTR)0, CALLBACK_THREAD);
        if (res != MMSYSERR_NOERROR)
        {
            return 0 - res;
        }

        *wave_hdl_out = wi;
        return 0;
    }

    int WinRec::openRecorderInternal(configuration::ISRRecorder& recorder, unsigned int dev)
    {
        unsigned int buf_size;
        int ret = 0;

        recorder.bufcount = BUF_COUNT;
        recorder.wavein_hdl = nullptr;
        recorder.rec_thread_hdl = nullptr;

        ret = createCallbackThread(static_cast<void*>(&recorder), &recorder.rec_thread_hdl);
        if (ret != 0)
        {
            cleanUpRecorderResource(recorder);
            return ret;
        }

        ret = openRecDevice(dev, static_cast<HANDLE>(recorder.rec_thread_hdl), reinterpret_cast<HWAVEIN *>(&recorder.wavein_hdl));
        if (ret != 0)
        {
            cleanUpRecorderResource(recorder);
            return ret;
        }

        if (nullptr != m_waveFormat)
        {
            buf_size = m_waveFormat->nBlockAlign *(m_waveFormat->nSamplesPerSec / 50) * FRAME_CNT; // 200ms
        }
        else
        {
            buf_size = FRAME_CNT * 20 * 16 * 2;  // 16khz, 16bit, 200ms;
        }

        ret = preparRecBuffer(static_cast<HWAVEIN>(recorder.wavein_hdl), reinterpret_cast<WAVEHDR**>(&recorder.bufheader), recorder.bufcount, buf_size);
        if (ret != 0)
        {
            cleanUpRecorderResource(recorder);
        }

        return ret;
    }

    /*******************************************************
            for close device and handle
    *******************************************************/
    // free record buffer
    void WinRec::freeRecBuffer(HWAVEIN wi, WAVEHDR* first_header, unsigned headercount)
    {
        unsigned int i;
        WAVEHDR *header;
        if (nullptr == first_header || 0 == headercount)
        {
            return;
        }

        header = first_header;
        for (i = 0; i < headercount; ++i)
        {
            if (header->lpData)
            {
                if (WHDR_PREPARED & header->dwFlags)
                {
                    waveInUnprepareHeader(wi, header, sizeof(WAVEHDR));
                }
                delete[](header->lpData);
                header->lpData = nullptr;
            }
            header++;
        }
        delete[](first_header);
        first_header = nullptr;
    }

    // close record device
    void WinRec::closeRecDevice(HWAVEIN wi)
    {
        if (nullptr != wi)
        {
            waveInClose(wi);
        }
    }

    // close handle
    void WinRec::closeCallbackThread(HANDLE thread)
    {
        if (nullptr == thread)
        {
            return;
        }

        if (nullptr != m_msgqueueReadyEvt)
        {
            /* if quit before the thread ready */
            WaitForSingleObject(m_msgqueueReadyEvt, INFINITE);
            CloseHandle(m_msgqueueReadyEvt);
            m_msgqueueReadyEvt = nullptr;

            PostThreadMessage(GetThreadId(thread), WM_QUIT, 0, 0);
            WaitForSingleObject(thread, INFINITE);
            CloseHandle(thread);
        }
    }

    // clean up resource
    void WinRec::cleanUpRecorderResource(configuration::ISRRecorder& recorder)
    {
        if (recorder.bufheader)
        {
            freeRecBuffer(static_cast<HWAVEIN>(recorder.wavein_hdl), static_cast<WAVEHDR*>(recorder.bufheader), recorder.bufcount);
            recorder.bufheader = nullptr;
            recorder.bufcount = 0;
        }
        if (recorder.wavein_hdl)
        {
            closeRecDevice(static_cast<HWAVEIN>(recorder.wavein_hdl));
            recorder.wavein_hdl = nullptr;
        }
        if (recorder.rec_thread_hdl)
        {
            closeCallbackThread(recorder.rec_thread_hdl);
            recorder.rec_thread_hdl = nullptr;
        }
    }

    // stop record
    int WinRec::stopRecord(configuration::ISRRecorder& recorder)
    {
        int ret = -1;

        if (recorder.state < configuration::RecordState::RECORD_STATE_RECORDING)
        {
            return 0;
        }

        recorder.state = configuration::RecordState::RECORD_STATE_STOPPING;
        ret = stopRecordInternal(static_cast<HWAVEIN>(recorder.wavein_hdl));
        if (ret == 0)
        {
            recorder.state = configuration::RecordState::RECORD_STATE_READY;
        }
        return ret;
    }

    int WinRec::stopRecordInternal(HWAVEIN wi)
    {
        MMRESULT res;
        //res = waveInStop(wi); /* some buffer may be still in the driver's queue */
        res = waveInReset(wi);

        if (MMSYSERR_NOERROR != res)
        {
            return 0 - res;
        }

        return 0;
    }

    // is stop record
    int WinRec::isRecordStopped(configuration::ISRRecorder& recorder)
    {
        if (recorder.state == configuration::RecordState::RECORD_STATE_RECORDING)
        {
            return 0;
        }

        return isStoppedInternal(recorder);
    }

    int WinRec::isStoppedInternal(configuration::ISRRecorder& recorder)
    {
        unsigned int i;
        WAVEHDR* header;

        header = static_cast<WAVEHDR*>(recorder.bufheader);
        /* after close, already free */
        if (nullptr == header || recorder.bufcount == 0 /*|| rec->using_flags == NULL*/)
        {
            return 1;
        }

        for (i = 0; i < recorder.bufcount; ++i)
        {
            if ((header)->dwFlags & WHDR_INQUEUE)
            {
                return 0;
            }
            /* after stop, we called the waveInReset to return all buffers */
            /* dwUser, see data_proc; */
            if (0 != header->dwUser)
            {
                return 0;
            }

            header++;
        }

        return 1;
    }

} // namespace applications