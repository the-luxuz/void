#include "headers/video_player.h"

#include <iostream>
#include <algorithm>
#include <cmath>
#include <cstring>

// Appointment

// Thanks god ai exist for things like this, bcs learning ffmpeg trash api and aplying this would take me weeks to do 
// and i fucking hate audio integration of this shit

// this is literally the wildest vibe coding i have never done before and i'm actually proud of avoiding this bullshit,
// bcs i don't fucking want in my entire life to learn this disgusting thing

VideoPlayer::VideoPlayer() {}

VideoPlayer::~VideoPlayer() {

    Close();
    DestroyTexture();
}

AVPixelFormat VideoPlayer::GetHardwareFormat(AVCodecContext* ctx, const AVPixelFormat* formats) {

    for (const AVPixelFormat* p = formats; *p != AV_PIX_FMT_NONE; ++p) {

        if (*p == AV_PIX_FMT_D3D11)
            return AV_PIX_FMT_D3D11;
    }

    std::cerr << "D3D11VA format not offered by decoder\n";

    return AV_PIX_FMT_NONE;
}

std::string VideoPlayer::FFmpegError(int error) {

    char buffer[AV_ERROR_MAX_STRING_SIZE];

    av_strerror(error, buffer, sizeof(buffer));

    return std::string(buffer);
}

bool VideoPlayer::Open(const std::string& path) {

    Close();

    int ret = avformat_open_input(&formatContext, path.c_str(), nullptr, nullptr);

    if (ret < 0) {

        std::cerr << "FFmpeg: avformat_open_input failed: " << FFmpegError(ret) << '\n';

        Close();
        return false;
    }

    ret = avformat_find_stream_info(formatContext, nullptr);

    if (ret < 0) {

        std::cerr << "FFmpeg: avformat_find_stream_info failed: " << FFmpegError(ret) << '\n';

        Close();
        return false;
    }

    videoStreamIndex = -1;
    audioStreamIndex = -1;

    videoStream = nullptr;
    audioStream = nullptr;

    for (unsigned int i = 0; i < formatContext->nb_streams; ++i) {

        AVStream* stream = formatContext->streams[i];

        if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && videoStreamIndex < 0) {

            videoStreamIndex = static_cast<int>(i);
            videoStream = stream;

        } else if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && audioStreamIndex < 0) {

            audioStreamIndex = static_cast<int>(i);
            audioStream = stream;
        }
    }

    if (videoStreamIndex < 0) {

        std::cerr << "FFmpeg: no video stream found\n";

        Close();
        return false;
    }

    timeBase = av_q2d(videoStream->time_base);

    if (videoStream->duration != AV_NOPTS_VALUE) {

        duration = videoStream->duration * av_q2d(videoStream->time_base);

    } else if (formatContext->duration != AV_NOPTS_VALUE) {

        duration = static_cast<double>(formatContext->duration) / AV_TIME_BASE;

    } else {

        duration = 0.0;
    }

    codec = avcodec_find_decoder(videoStream->codecpar->codec_id);

    if (!codec) {

        std::cerr << "FFmpeg: video decoder not found\n";

        Close();
        return false;
    }

    codecContext = avcodec_alloc_context3(codec);

    if (!codecContext) {
        std::cerr << "FFmpeg: failed to allocate video codec context\n";

        Close();
        return false;
    }

    ret = avcodec_parameters_to_context(
        codecContext,
        videoStream->codecpar
    );

    if (ret < 0) {
        std::cerr
            << "FFmpeg: failed to copy video codec parameters: "
            << FFmpegError(ret)
            << '\n';

        Close();
        return false;
    }

    codecContext->pkt_timebase = videoStream->time_base;

    codecContext->get_format = GetHardwareFormat;

    ret = av_hwdevice_ctx_create(
        &hwDeviceContext,
        AV_HWDEVICE_TYPE_D3D11VA,
        nullptr,
        nullptr,
        0
    );

    if (ret < 0) {
        std::cerr
            << "FFmpeg: failed to create D3D11VA device: "
            << FFmpegError(ret)
            << '\n';

        Close();
        return false;
    }

    codecContext->hw_device_ctx = av_buffer_ref(hwDeviceContext);

    if (!codecContext->hw_device_ctx) {
        std::cerr << "FFmpeg: failed to reference D3D11VA device\n";

        Close();
        return false;
    }

    ret = avcodec_open2(codecContext, codec, nullptr);

    if (ret < 0) {
        std::cerr
            << "FFmpeg: failed to open video decoder with D3D11VA: "
            << FFmpegError(ret)
            << '\n';

        Close();
        return false;
    }

    if (ret < 0) {
        std::cerr << "FFmpeg: failed to open video decoder with D3D11VA: "
            << FFmpegError(ret) << '\n';

        Close();
        return false;
    }

    if (audioStreamIndex >= 0) {
        audioCodec = avcodec_find_decoder(audioStream->codecpar->codec_id);

        if (!audioCodec) {

            std::cerr << "FFmpeg: audio decoder not found\n";

            Close();
            return false;
        }

        audioCodecContext = avcodec_alloc_context3(audioCodec);

        if (!audioCodecContext) {

            std::cerr << "FFmpeg: failed to allocate audio codec context\n";

            Close();
            return false;
        }

        ret = avcodec_parameters_to_context(audioCodecContext, audioStream->codecpar);

        if (ret < 0) {

            std::cerr << "FFmpeg: failed to copy audio codec parameters: " << FFmpegError(ret) << '\n';

            Close();
            return false;
        }

        audioCodecContext->pkt_timebase = audioStream->time_base;

        ret = avcodec_open2(audioCodecContext, audioCodec, nullptr);

        if (ret < 0) {

            std::cerr << "FFmpeg: failed to open audio decoder: " << FFmpegError(ret) << '\n';

            Close();
            return false;
        }

        audioFrame = av_frame_alloc();

        if (!audioFrame) {

            std::cerr << "FFmpeg: failed to allocate audio frame\n";

            Close();
            return false;
        }

        SDL_AudioSpec spec{};

        spec.format = SDL_AUDIO_S16;
        spec.channels = audioChannels;
        spec.freq = audioSampleRate;

        audioOutput = SDL_OpenAudioDeviceStream(
            SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
            &spec,
            nullptr,
            nullptr
        );

        if (!audioOutput) {

            std::cerr << "SDL: failed to open audio device: " << SDL_GetError() << '\n';

            Close();
            return false;
        }

        if (!SDL_ResumeAudioStreamDevice(audioOutput)) {

            std::cerr << "SDL: failed to start audio device: " << SDL_GetError() << '\n';

            Close();
            return false;
        }

        swrContext = swr_alloc();

        if (!swrContext) {

            std::cerr << "FFmpeg: failed to allocate SwrContext\n";

            Close();
            return false;
        }

        ret = av_opt_set_chlayout(swrContext, "in_chlayout", &audioCodecContext->ch_layout, 0);

        if (ret < 0) {

            std::cerr << "FFmpeg: failed to set input channel layout: " << FFmpegError(ret) << '\n';

            Close();
            return false;
        }

        AVChannelLayout stereo = AV_CHANNEL_LAYOUT_STEREO;

        ret = av_opt_set_chlayout(swrContext, "out_chlayout", &stereo, 0);

        if (ret < 0) {

            std::cerr << "FFmpeg: failed to set output channel layout: " << FFmpegError(ret) << '\n';

            Close();
            return false;
        }

        ret = av_opt_set_int(swrContext, "in_sample_rate", audioCodecContext->sample_rate, 0);

        if (ret < 0) {

            std::cerr << "FFmpeg: failed to set input sample rate\n";

            Close();
            return false;
        }

        ret = av_opt_set_int(swrContext, "out_sample_rate", audioSampleRate, 0);

        if (ret < 0) {

            std::cerr << "FFmpeg: failed to set output sample rate\n";

            Close();
            return false;
        }

        ret = av_opt_set_sample_fmt(swrContext, "in_sample_fmt", audioCodecContext->sample_fmt, 0);

        if (ret < 0) {

            std::cerr << "FFmpeg: failed to set input sample format\n";

            Close();
            return false;
        }

        ret = av_opt_set_sample_fmt(swrContext, "out_sample_fmt", audioSampleFormat, 0);

        if (ret < 0) {

            std::cerr << "FFmpeg: failed to set output sample format\n";

            Close();
            return false;
        }

        ret = swr_init(swrContext);

        if (ret < 0) {

            std::cerr << "FFmpeg: failed to initialize SwrContext: " << FFmpegError(ret) << '\n';

            Close();
            return false;
        }

        std::cout
            << "Audio initialized: "
            << audioCodecContext->sample_rate
            << " Hz -> "
            << audioSampleRate
            << " Hz, "
            << audioChannels
            << " channels\n";
    }

    width = codecContext->width;
    height = codecContext->height;

    if (width <= 0 || height <= 0) {

        std::cerr << "FFmpeg: invalid video dimensions\n";

        Close();
        return false;
    }

    packet = av_packet_alloc();
    decodeFrame = av_frame_alloc();
    pendingFrame = av_frame_alloc();
    hwTransferFrame = av_frame_alloc();

    if (!packet ||
        !decodeFrame ||
        !pendingFrame ||
        !hwTransferFrame) {
        std::cerr << "FFmpeg: failed to allocate frames\n";

        Close();
        return false;
    }

    int rgbaSize = av_image_get_buffer_size(AV_PIX_FMT_RGBA, width, height, 1);

    if (rgbaSize <= 0) {

        std::cerr << "FFmpeg: invalid RGBA buffer size\n";

        Close();
        return false;
    }

    rgbaBuffer.resize(static_cast<size_t>(rgbaSize));

    CreateTexture();

    if (texture == 0) {

        std::cerr << "OpenGL: failed to create video texture\n";

        Close();
        return false;
    }

    currentTime = 0.0;
    playing = false;
    finished = false;
    endOfFile = false;
    hasPendingFrame = false;

    return true;
}

void VideoPlayer::CreateTexture() {

    DestroyTexture();

    glGenTextures(1, &texture);

    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA8,
        width,
        height,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        nullptr
    );

    glBindTexture(GL_TEXTURE_2D, 0);
}

void VideoPlayer::DestroyTexture() {

    if (texture != 0) {

        glDeleteTextures(1, &texture);
        texture = 0;
    }
}

bool VideoPlayer::DecodeNextFrame(AVFrame* output) {

    if (!formatContext || !codecContext || !packet || !output)
        return false;

    while (true) {

        int ret = avcodec_receive_frame(codecContext, output);

        if (ret == 0) {

            static bool printed = false;

            if (!printed) {
                std::cout
                    << "VIDEO PIX FORMAT: "
                    << av_get_pix_fmt_name(
                        static_cast<AVPixelFormat>(output->format)
                    )
                    << '\n';

                printed = true;
            }

            return true;
        }

        if (ret == AVERROR_EOF)
            return false;

        if (ret != AVERROR(EAGAIN))
            return false;

        ret = av_read_frame(formatContext, packet);

        if (ret < 0) {

            if (ret == AVERROR_EOF) {

                ret = avcodec_send_packet(codecContext, nullptr);

                if (ret < 0 && ret != AVERROR_EOF && ret != AVERROR(EAGAIN))
                    return false;

                continue;
            }

            return false;
        }

        if (packet->stream_index == videoStreamIndex) {

            ret = avcodec_send_packet(codecContext, packet);

            av_packet_unref(packet);

            if (ret < 0)
                return false;

            continue;
        }

        if (packet->stream_index == audioStreamIndex) {

            ret = avcodec_send_packet(audioCodecContext, packet);

            av_packet_unref(packet);

            if (ret < 0)
                return false;

            DecodeAudioFrame();
            continue;
        }

        av_packet_unref(packet);
    }
}

bool VideoPlayer::PresentFrame(AVFrame* frame) {

    if (!frame || !texture)
        return false;

    AVFrame* cpuFrame = frame;

    // ------------------------------------------------------------
    // D3D11 GPU frame -> CPU frame

    if (frame->format == AV_PIX_FMT_D3D11) {

        av_frame_unref(hwTransferFrame);

        int ret = av_hwframe_transfer_data(
            hwTransferFrame,
            frame,
            0
        );

        if (ret < 0) {

            std::cerr
                << "FFmpeg: D3D11 frame transfer failed: "
                << FFmpegError(ret)
                << '\n';

            return false;
        }

        cpuFrame = hwTransferFrame;
    }

    // ------------------------------------------------------------
    // Crear SwsContext usando EL FORMATO DEL FRAME CPU

    AVPixelFormat sourceFormat = static_cast<AVPixelFormat>(cpuFrame->format);

    if (!swsContext) {

        swsContext = sws_getContext(
            cpuFrame->width,
            cpuFrame->height,
            sourceFormat,
            width,
            height,
            AV_PIX_FMT_RGBA,
            SWS_BILINEAR,
            nullptr,
            nullptr,
            nullptr
        );

        if (!swsContext) {

            std::cerr
                << "FFmpeg: failed to create SwsContext for CPU frame. Format: "
                << av_get_pix_fmt_name(sourceFormat)
                << '\n';

            return false;
        }
    }

    // ------------------------------------------------------------
    // Convertir a RGBA

    uint8_t* dstData[4] = {
        rgbaBuffer.data(),
        nullptr,
        nullptr,
        nullptr
    };

    int dstLinesize[4] = {
        width * 4,
        0,
        0,
        0
    };

    sws_scale(
        swsContext,
        cpuFrame->data,
        cpuFrame->linesize,
        0,
        cpuFrame->height,
        dstData,
        dstLinesize
    );

    // ------------------------------------------------------------
    // Subir RGBA a OpenGL

    glBindTexture(
        GL_TEXTURE_2D,
        texture
    );

    glPixelStorei(
        GL_UNPACK_ALIGNMENT,
        1
    );

    glTexSubImage2D(
        GL_TEXTURE_2D,
        0,
        0,
        0,
        width,
        height,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        rgbaBuffer.data()
    );

    glBindTexture(
        GL_TEXTURE_2D,
        0
    );

    return true;
}

bool VideoPlayer::DecodeUntil(double targetTime) {

    while (true) {

        AVFrame* frameToUse = nullptr;
        bool usingPending = false;

        if (hasPendingFrame) {

            frameToUse = pendingFrame;
            usingPending = true;

        } else {

            if (!DecodeNextFrame(decodeFrame))
                return false;

            frameToUse = decodeFrame;
        }

        int64_t pts = frameToUse->best_effort_timestamp;

        double frameTime = 0.0;

        if (pts != AV_NOPTS_VALUE)
            frameTime = pts * timeBase;

        if (frameTime <= targetTime) {

            PresentFrame(frameToUse);

            if (usingPending) {

                hasPendingFrame = false;
                av_frame_unref(pendingFrame);
            }

            continue;
        }

        if (usingPending)
            return true;

        av_frame_unref(pendingFrame);

        av_frame_move_ref(pendingFrame, decodeFrame);

        hasPendingFrame = true;

        return true;
    }
}

void VideoPlayer::Update(double dt) {

    if (!playing || !formatContext)
        return;

    currentTime += dt;

    if (duration > 0.0 && currentTime >= duration) {

        currentTime = duration;
        finished = true;
        playing = false;
    }

    DecodeUntil(currentTime);
}

void VideoPlayer::Play() {

    if (!formatContext)
        return;

    if (finished)
        Seek(0.0);

    playing = true;
    finished = false;
}

void VideoPlayer::Pause() {

    playing = false;
}

void VideoPlayer::Stop() {

    playing = false;
    Seek(0.0);
}

void VideoPlayer::Seek(double seconds) {

    if (!formatContext || !codecContext || !videoStream)
        return;

    seconds = std::clamp(seconds, 0.0, duration);

    int64_t timestamp = static_cast<int64_t>(seconds / timeBase);

    int ret = avformat_seek_file(
        formatContext,
        videoStreamIndex,
        INT64_MIN,
        timestamp,
        INT64_MAX,
        0
    );

    if (ret < 0)
        return;

    avcodec_flush_buffers(codecContext);

    av_frame_unref(decodeFrame);
    av_frame_unref(pendingFrame);

    hasPendingFrame = false;
    endOfFile = false;
    finished = false;

    currentTime = seconds;

    DecodeUntil(seconds);
}

void VideoPlayer::Close() {

    playing = false;

    if (swsContext) {

        sws_freeContext(swsContext);
        swsContext = nullptr;
    }

    if (pendingFrame)
        av_frame_free(&pendingFrame);

    if (decodeFrame)
        av_frame_free(&decodeFrame);

    if (packet)
        av_packet_free(&packet);

    if (hwTransferFrame)
        av_frame_free(&hwTransferFrame);

    if (codecContext)
        avcodec_free_context(&codecContext);

    if (hwDeviceContext)
        av_buffer_unref(&hwDeviceContext);

    if (formatContext)
        avformat_close_input(&formatContext);

    if (audioFrame)
        av_frame_free(&audioFrame);

    if (swrContext)
        swr_free(&swrContext);

    if (audioCodecContext)
        avcodec_free_context(&audioCodecContext);

    if (audioOutput) {
        SDL_DestroyAudioStream(audioOutput);
    }

    audioOutput = nullptr;
    audioCodec = nullptr;
    audioStream = nullptr;
    audioStreamIndex = -1;

    codec = nullptr;
    videoStream = nullptr;

    videoStreamIndex = -1;

    width = 0;
    height = 0;

    duration = 0.0;
    currentTime = 0.0;

    endOfFile = false;
    finished = false;
    hasPendingFrame = false;

    rgbaBuffer.clear();
    audioBuffer.clear();
}

bool VideoPlayer::DecodeAudioFrame() {
        
    if (!audioCodecContext || !audioFrame || !swrContext || !audioOutput)
        return false;

    while (true) {

        int ret = avcodec_receive_frame(audioCodecContext, audioFrame);

        if (ret == 0) {

            int64_t delay = swr_get_delay(
                swrContext,
                audioCodecContext->sample_rate
            );

            int outputSamples = static_cast<int>(
                av_rescale_rnd(
                    delay + audioFrame->nb_samples,
                    audioSampleRate,
                    audioCodecContext->sample_rate,
                    AV_ROUND_UP
                )
                );

            size_t bytes = static_cast<size_t>(
                outputSamples * audioChannels * sizeof(int16_t)
                );

            audioBuffer.resize(bytes);

            uint8_t* outputData[1] = {
                audioBuffer.data()
            };

            const uint8_t** inputData =
                const_cast<const uint8_t**>(audioFrame->extended_data);

            int converted = swr_convert(
                swrContext,
                outputData,
                outputSamples,
                inputData,
                audioFrame->nb_samples
            );

            if (converted < 0)
                return false;

            int outputBytes = converted * audioChannels * sizeof(int16_t);

            if (!SDL_PutAudioStreamData(audioOutput, audioBuffer.data(), outputBytes)) {

                std::cerr << "SDL_PutAudioStreamData: " << SDL_GetError() << '\n';
                return false;
            }

            continue;
        }

        if (ret == AVERROR(EAGAIN))
            return true;

        if (ret == AVERROR_EOF)
            return false;

        return false;
    }
}