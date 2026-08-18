#include "headers/video_player.h"

#include <iostream>
#include <algorithm>
#include <cmath>
#include <cstring>

VideoPlayer::VideoPlayer() {}

VideoPlayer::~VideoPlayer() {

    Close();
    DestroyTexture();
}

std::string VideoPlayer::FFmpegError(int error) {

    char buffer[AV_ERROR_MAX_STRING_SIZE];

    av_strerror(
        error,
        buffer,
        sizeof(buffer)
    );

    return std::string(buffer);
}

bool VideoPlayer::Open(const std::string& path) {

    Close();

    int ret = avformat_open_input(&formatContext, path.c_str(), nullptr, nullptr);

    if (ret < 0) {

        return false;
    }

    ret = avformat_find_stream_info(
        formatContext,
        nullptr
    );

    if (ret < 0) {

        Close();
        return false;
    }

    videoStreamIndex = -1;

    for (unsigned int i = 0; i < formatContext->nb_streams; ++i) {

        AVStream* stream = formatContext->streams[i];

        if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {

            videoStreamIndex = static_cast<int>(i);

            videoStream = stream;
            break;
        }
    }

    if (videoStreamIndex < 0) {

        Close();
        return false;
    }

    timeBase = av_q2d(videoStream->time_base);

    if (formatContext->duration != AV_NOPTS_VALUE)
        duration = static_cast<double>(formatContext->duration) / AV_TIME_BASE;
    else
        duration = 0.0;

    codec = avcodec_find_decoder(videoStream->codecpar->codec_id);

    if (!codec) {

        Close();
        return false;
    }

    codecContext = avcodec_alloc_context3(codec);

    if (!codecContext) {

        Close();
        return false;
    }

    ret = avcodec_parameters_to_context(codecContext, videoStream->codecpar);

    if (ret < 0) {

        Close();
        return false;
    }

    codecContext->pkt_timebase = videoStream->time_base;

    ret = avcodec_open2(
        codecContext,
        codec,
        nullptr
    );

    if (ret < 0) {

        Close();
        return false;
    }

    width = codecContext->width;
    height = codecContext->height;

    packet = av_packet_alloc();
    decodeFrame = av_frame_alloc();
    pendingFrame = av_frame_alloc();

    if (!packet || !decodeFrame || !pendingFrame) {


        Close();
        return false;
    }

    swsContext = sws_getContext(
                width,
                height,
                codecContext->pix_fmt,
                width,
                height,
                AV_PIX_FMT_RGBA,
                SWS_BILINEAR,
                nullptr,
                nullptr,
                nullptr
            );

    if (!swsContext) {

        Close();
        return false;
    }

    int rgbaSize = av_image_get_buffer_size(AV_PIX_FMT_RGBA, width, height, 1);

    if (rgbaSize <= 0) {

        Close();
        return false;
    }

    rgbaBuffer.resize(static_cast<size_t>(rgbaSize));

    CreateTexture();

    currentTime = 0.0;
    playing = false;
    finished = false;
    endOfFile = false;
    hasPendingFrame = false;

    return true;
}

void VideoPlayer::CreateTexture() {

    DestroyTexture();

    glGenTextures(
        1,
        &texture
    );

    glBindTexture(
        GL_TEXTURE_2D,
        texture
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MIN_FILTER,
        GL_LINEAR
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MAG_FILTER,
        GL_LINEAR
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_WRAP_S,
        GL_CLAMP_TO_EDGE
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_WRAP_T,
        GL_CLAMP_TO_EDGE
    );

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

    glBindTexture(
        GL_TEXTURE_2D,
        0
    );
}

void VideoPlayer::DestroyTexture() {

    if (texture != 0) {

        glDeleteTextures(
            1,
            &texture
        );

        texture = 0;
    }
}

bool VideoPlayer::DecodeNextFrame(AVFrame* output) {

    if (!formatContext || !codecContext || !packet || !output) {

        return false;
    }

    while (true) {

        int ret =
            avcodec_receive_frame(
                codecContext,
                output
            );

        if (ret == 0) {

            return true;
        }

        if (ret == AVERROR_EOF) {

            return false;
        }

        if (ret != AVERROR(EAGAIN)) {

            return false;
        }

        ret = av_read_frame(
                formatContext,
                packet
            );

        if (ret < 0) {

            if (ret == AVERROR_EOF) {

                ret = avcodec_send_packet(
                        codecContext,
                        nullptr
                    );

                if (ret < 0 && ret != AVERROR_EOF) {

                    return false;
                }

                continue;
            }


            return false;
        }

        if (packet->stream_index != videoStreamIndex) {

            av_packet_unref(packet);
            continue;
        }

        ret = avcodec_send_packet(
                codecContext,
                packet
            );

        av_packet_unref(packet);

        if (ret < 0) {

            return false;
        }
    }
}

bool VideoPlayer::PresentFrame(AVFrame* frame) {

    if (!frame || !swsContext || !texture) {

        return false;
    }

    static int frameCount = 0;

    if ((frameCount++ % 30) == 0) {

        double pts = 0.0;

        if (frame->best_effort_timestamp != AV_NOPTS_VALUE)
            pts =
            frame->best_effort_timestamp *
            timeBase;
    }

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
        frame->data,
        frame->linesize,
        0,
        height,
        dstData,
        dstLinesize
    );

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

        }
        else {

            if (!DecodeNextFrame(decodeFrame)) {

                return false;
            }

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

        if (usingPending) {


            return true;
        }

        av_frame_unref(pendingFrame);

        av_frame_move_ref(
            pendingFrame,
            decodeFrame
        );

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

    if (!formatContext || !codecContext || !videoStream) {

        return;
    }

    seconds = std::clamp(
            seconds,
            0.0,
            duration
        );

    int64_t timestamp = static_cast<int64_t>(seconds / timeBase);

    int ret =
        avformat_seek_file(
            formatContext,
            videoStreamIndex,
            INT64_MIN,
            timestamp,
            INT64_MAX,
            0
        );

    if (ret < 0) {

        return;
    }

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

        sws_freeContext(
            swsContext
        );

        swsContext = nullptr;
    }

    if (pendingFrame) {

        av_frame_free(
            &pendingFrame
        );
    }

    if (decodeFrame) {

        av_frame_free(
            &decodeFrame
        );
    }

    if (packet) {

        av_packet_free(
            &packet
        );
    }

    if (codecContext) {

        avcodec_free_context(
            &codecContext
        );
    }

    if (formatContext) {

        avformat_close_input(
            &formatContext
        );
    }

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
}