//
//  VideoPlayer.cpp
//  SDLTestDemo
//
//  Created by 尹东博 on 2023/4/14.
//

#include "VideoPlayer.hpp"
#include <thread>

#define AUDIO_MAX_PKT_SIZE 1000
#define VIDEO_MAX_PKT_SIZE 500

#define SAMPLE_RATE 44100// 采样率
#define SAMPLE_FORMAT AUDIO_S16LSB// 采样格式
#define SAMPLE_SIZE SDL_AUDIO_BITSIZE(SAMPLE_FORMAT)// 采样大小
#define CHANNELS 2// 声道数
#define SAMPLES 512// 音频缓冲区的样本数量

 
VideoPlayer::VideoPlayer() {}
 
VideoPlayer::~VideoPlayer() {}


void VideoPlayer::setSelf(void *aSelf) {
    self = aSelf;
}

VideoPlayer::State VideoPlayer::getState() {
    return _state;
}

void VideoPlayer::play() {
    if (_state == VideoPlayer::Playing) {
        return;
    }
//    if (_state == Stopped) {
        std::thread([this](){
            readFile();
        }).detach();
//    } else {
//        setState(VideoPlayer::Playing);
//    }
}
void VideoPlayer::setFilename(const char *filename){ 
    printf("%s\n",filename);
    memcpy(_filename,filename,strlen(filename) + 1);
    cout << _filename << endl;
}

void VideoPlayer::readFile() {
    int ret = 0;
    ret = avformat_open_input(&_fmtCtx, _filename, nullptr, nullptr);
    ret = avformat_find_stream_info(_fmtCtx, nullptr);
    av_dump_format(_fmtCtx, 0, _filename, 0);
    
    _hasAudio = initAudioInfo();
    _hasVideo = initVideoInfo();
    
    SDL_PauseAudio(0);
}

#pragma mark - initSDL
int VideoPlayer::initSDL() {
    SDL_SetMainReady();
    SDL_AudioSpec spec;
    spec.freq = _aSwrOutSpec.sampleRate;
    spec.format = SAMPLE_FORMAT;
    spec.channels = _aSwrOutSpec.chs;
    spec.samples = SAMPLES;
    spec.callback = sdlAudioCallbackFunc;
    spec.userdata = this;
    if (SDL_OpenAudio(&spec, nullptr)) {
        return -1;
    }
    return 0;
}

void VideoPlayer::sdlAudioCallbackFunc(void *userdata, Uint8 *stream, int len){
    printf("sdlAudioCallbackFunc~~~");
    //回调函数在SDL开辟的子线程执行
//    VideoPlayer *player = (VideoPlayer *)userdata;
//    player->sdlAudioCallback(stream,len);
}

#pragma mark - 初始化 音频信息
int VideoPlayer::initAudioInfo() {
    int ret = initDecoder(&_aDecodeCtx, &_aStream, AVMEDIA_TYPE_AUDIO);
    /*
     _aDecodeCtx->sample_rate = 44100
     _aDecodeCtx->sample_fmt = AV_SAMPLE_FMT_FLTP
     _aDecodeCtx->channel_layout = 3
     _aDecodeCtx->channels = 2
     */
    ret = initSwr();
    ret = initSDL();
    return 1;
}

int VideoPlayer::initSwr() {
    _aSwrInSpec.sampleRate = _aDecodeCtx->sample_rate;
    _aSwrInSpec.sampleFmt = _aDecodeCtx->sample_fmt;
    _aSwrInSpec.chLayout =  static_cast<int>(_aDecodeCtx->channel_layout);
    _aSwrInSpec.chs = _aDecodeCtx->channels;
     
    _aSwrOutSpec.sampleRate = SAMPLE_RATE;
    _aSwrOutSpec.sampleFmt = AV_SAMPLE_FMT_S16;
    _aSwrOutSpec.chLayout = AV_CH_LAYOUT_STEREO;
    _aSwrOutSpec.chs = av_get_channel_layout_nb_channels(_aSwrOutSpec.chLayout);
    _aSwrOutSpec.bytesPerSampleFrame = _aSwrOutSpec.chs * av_get_bytes_per_sample(_aSwrOutSpec.sampleFmt);
     
    _aSwrCtx = swr_alloc_set_opts(nullptr,
                                  _aSwrOutSpec.chLayout, _aSwrOutSpec.sampleFmt, _aSwrOutSpec.sampleRate,
                                  _aSwrInSpec.chLayout, _aSwrInSpec.sampleFmt,  _aSwrInSpec.sampleRate,
                                  0, nullptr);
    if (!_aSwrCtx) {
        return -1;
    }
    int ret = swr_init(_aSwrCtx);
    _aSwrInFrame = av_frame_alloc();
    if(!_aSwrInFrame){
        return -1;
    }
    
    _aSwrOutFrame = av_frame_alloc();
    if(!_aSwrOutFrame){
        return -1;
    }
    ret = av_samples_alloc(_aSwrOutFrame->data,_aSwrOutFrame->linesize,_aSwrOutSpec.chs,4096,_aSwrOutSpec.sampleFmt,1);
    return 0;
}

#pragma mark - 初始化 视频信息
int VideoPlayer::initVideoInfo() {
    
    return 1;
}


#pragma mark - init Decoder
int VideoPlayer::initDecoder(AVCodecContext **decodeCtx, AVStream **stream, AVMediaType type) {
    int ret = av_find_best_stream(_fmtCtx, type, -1, -1, nullptr, 0);
    int streamIdx = ret;
    *stream = _fmtCtx->streams[streamIdx];
    AVCodec *decoder = nullptr;
    
    decoder = avcodec_find_decoder((*stream)->codecpar->codec_id);
    if (!decoder) {
        return -1;
    }
    *decodeCtx = avcodec_alloc_context3(decoder);
    if (!decodeCtx) {
        return -1;
    }
    
    ret = avcodec_parameters_to_context(*decodeCtx, (*stream)->codecpar);
    ret = avcodec_open2(*decodeCtx, decoder, nullptr);
    return 1;
}
