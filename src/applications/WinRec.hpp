#pragma once
#include "ISysRec.hpp"
#include "configurations/AppConfigurations.hpp"

extern "C" {
#include <Windows.h>
#include <mmsystem.h> 
}

namespace applications
{
    class WinRec final : public ISysRec
    {
    public:
        WinRec(std::unique_ptr<WAVEFORMATEX> waveFormat);

        int getDefaultInputDev() override;
        unsigned int getInputDevNum() override;
        int startRecord(configuration::ISRRecorder& recorder) override;
        void closeRecorder(configuration::ISRRecorder& recorder)  override;
        void destroyRecorder(configuration::ISRRecorder& recorder) override;
        int createRecorder(configuration::ISRRecorder& recorder,
            std::function<void(const std::string& data)> on_data_ind, void* user_cb_para) override;
        int openRecorder(configuration::ISRRecorder& recorder, unsigned int dev) override;
        int stopRecord(configuration::ISRRecorder& recorder) override;
        int isRecordStopped(configuration::ISRRecorder& recorder) override;

    private:
        int startRecordInternal(HWAVEIN wi, WAVEHDR* header, unsigned int bufcount);
        void closeRecorderInternal(configuration::ISRRecorder& recorder);
        int preparRecBuffer(HWAVEIN wi, WAVEHDR** bufheader_out, unsigned int headercount, unsigned int bufsize);
        void WinRec::printWaveHeader(WAVEHDR& buf);

        static void dataProc(configuration::ISRRecorder& recorder, MSG& msg);
        static unsigned int __stdcall recordThreadProc(void* para);
        static int createCallbackThread(void* thread_proc_para, HANDLE* thread_hdl_out);

        int openRecDevice(int dev, HANDLE thread, HWAVEIN* wave_hdl_out);
        int openRecorderInternal(configuration::ISRRecorder& recorder, unsigned int dev);
        void freeRecBuffer(HWAVEIN wi, WAVEHDR* first_header, unsigned headercount);
        void closeRecDevice(HWAVEIN wi);
        void closeCallbackThread(HANDLE thread);
        void cleanUpRecorderResource(configuration::ISRRecorder& recorder);
        int stopRecordInternal(HWAVEIN wi);
        int isStoppedInternal(configuration::ISRRecorder& recorder);

    private:
        std::shared_ptr<WAVEFORMATEX> m_waveFormat{};

        static HANDLE m_msgqueueReadyEvt;
    };
} // namespace applications