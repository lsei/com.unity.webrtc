#include "pch.h"
#include "UnityVideoEncoderFactory.h"

#include "DummyVideoEncoder.h"

#if defined(__APPLE__)
#import "sdk/objc/components/video_codec/RTCDefaultVideoEncoderFactory.h"
#import "sdk/objc/native/api/video_encoder_factory.h"
#endif

using namespace ::webrtc::H264;

namespace unity
{    
namespace webrtc
{

    bool IsFormatSupported(
        const std::vector<webrtc::SdpVideoFormat>& supported_formats,
        const webrtc::SdpVideoFormat& format)
    {
        for (const webrtc::SdpVideoFormat& supported_format : supported_formats)
        {
            if (cricket::IsSameCodec(format.name, format.parameters,
                supported_format.name,
                supported_format.parameters))
            {
                return true;
            }
        }
        return false;
    }

    webrtc::VideoEncoderFactory* CreateEncoderFactory()
    {
#if defined(__APPLE__)
        return webrtc::ObjCToNativeVideoEncoderFactory(
            [[RTCDefaultVideoEncoderFactory alloc] init]).release();
#endif
        return new webrtc::InternalEncoderFactory();
    }

    UnityVideoEncoderFactory::UnityVideoEncoderFactory(IVideoEncoderObserver* observer)
    : internal_encoder_factory_(CreateEncoderFactory())

    {
        m_observer = observer;
    }
    
    UnityVideoEncoderFactory::~UnityVideoEncoderFactory() = default;

    std::vector<webrtc::SdpVideoFormat> UnityVideoEncoderFactory::GetHardwareEncoderFormats() const
    {
#if defined(__APPLE__)
        auto formats = internal_encoder_factory_->GetSupportedFormats();
        std::vector<webrtc::SdpVideoFormat> filtered;
        std::copy_if(formats.begin(), formats.end(), std::back_inserter(filtered),
            [](webrtc::SdpVideoFormat format) {
                if(format.name.find("H264") == std::string::npos)
                    return false;
                return true;
            });
        return filtered;
#else
        return { webrtc::CreateH264Format(
            webrtc::H264::kProfileConstrainedBaseline,
            webrtc::H264::kLevel5_1, "1") };
#endif
    }


    std::vector<webrtc::SdpVideoFormat> UnityVideoEncoderFactory::GetSupportedFormats() const
    {
        // todo(kazuki): should support codec other than h264 like vp8, vp9 and av1.
        //
        // std::vector <webrtc::SdpVideoFormat> formats2 = internal_encoder_factory_->GetSupportedFormats();
        // formats.insert(formats.end(), formats2.begin(), formats2.end());
        return GetHardwareEncoderFormats();
    }

    webrtc::VideoEncoderFactory::CodecInfo UnityVideoEncoderFactory::QueryVideoEncoder(
        const webrtc::SdpVideoFormat& format) const
    {
        if (IsFormatSupported(GetHardwareEncoderFormats(), format))
        {
            return CodecInfo{ true, false };
        }
        RTC_DCHECK(IsFormatSupported(GetSupportedFormats(), format));
        return internal_encoder_factory_->QueryVideoEncoder(format);
    }

    std::unique_ptr<webrtc::VideoEncoder> UnityVideoEncoderFactory::CreateVideoEncoder(const webrtc::SdpVideoFormat& format)
    {
#if !defined(__APPLE__)
        if (IsFormatSupported(GetHardwareEncoderFormats(), format))
        {
            return std::make_unique<DummyVideoEncoder>(m_observer);
        }
#endif
        return internal_encoder_factory_->CreateVideoEncoder(format);
    }
}
}
