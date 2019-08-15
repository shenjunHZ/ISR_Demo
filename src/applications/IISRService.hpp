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
    };
} // namespace applications