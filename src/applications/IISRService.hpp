#pragma once

namespace applications
{
    class IISRService
    {
    public:
        virtual ~IISRService() = default;

        virtual bool loginSDK() = 0;
        virtual bool ISRAnalysis() = 0;

        virtual int srStartListening() = 0;
        virtual int srStopListening() = 0;
        virtual void setRegisterNotify(std::function<void(const std::string& result, const bool& isLast)>) = 0;
    };
} // namespace applications