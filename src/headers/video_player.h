#pragma once

#include "local.h"
#include "helpers.h"

class VideoPlayer {

public:

    VideoPlayer();
    ~VideoPlayer();

    bool Open(const std::string& path);
    void Close();

    void Play();
    void Pause();
    void Stop();

    void Seek(double seconds);

    void Update(double dt);

    GLuint GetTexture() const {
        return texture;
    }

    int GetWidth() const {
        return width;
    }

    int GetHeight() const {
        return height;
    }

    double GetTime() const {
        return currentTime;
    }

    double GetDuration() const {
        return duration;
    }

    bool IsPlaying() const {
        return playing;
    }

    bool IsOpened() const {
        return formatContext != nullptr;
    }

    bool IsFinished() const {
        return finished;
    }

    AVCodecContext* audioCodecContext = nullptr;

    const AVCodec* audioCodec = nullptr;
    AVStream* audioStream = nullptr;

    int audioStreamIndex = -1;

    AVFrame* audioFrame = nullptr;

    SwrContext* swrContext = nullptr;

    SDL_AudioStream* audioOutput = nullptr;

    int audioSampleRate = 48000;
    int audioChannels = 2;

    AVSampleFormat audioSampleFormat = AV_SAMPLE_FMT_S16;

    std::vector<uint8_t> audioBuffer;

private:

    bool OpenDecoder();
    bool DecodeNextFrame(AVFrame* output);
    bool PresentFrame(AVFrame* frame);
    bool DecodeUntil(double targetTime);

    void CreateTexture();
    void DestroyTexture();

    bool DecodeAudioFrame();

    static std::string FFmpegError(int error);

private:

    AVFormatContext* formatContext = nullptr;
    AVCodecContext* codecContext = nullptr;

    const AVCodec* codec = nullptr;
    AVStream* videoStream = nullptr;

    AVPacket* packet = nullptr;

    AVFrame* decodeFrame = nullptr;
    AVFrame* pendingFrame = nullptr;

    SwsContext* swsContext = nullptr;

    int videoStreamIndex = -1;

    int width = 0;
    int height = 0;

    double timeBase = 0.0;
    double duration = 0.0;
    double currentTime = 0.0;

    bool playing = false;
    bool finished = false;
    bool endOfFile = false;
    bool hasPendingFrame = false;

    GLuint texture = 0;

    std::vector<uint8_t> rgbaBuffer;


};