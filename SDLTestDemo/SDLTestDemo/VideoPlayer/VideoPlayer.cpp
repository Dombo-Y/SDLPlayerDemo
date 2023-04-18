//
//  VideoPlayer.cpp
//  SDLTestDemo
//
//  Created by 尹东博 on 2023/4/14.
//

#include "VideoPlayer.hpp"
#include <thread>
#include <unistd.h>

#include "PlayerC_interface.h"

#define AUDIO_MAX_PKT_SIZE 1000
#define VIDEO_MAX_PKT_SIZE 500

#define SAMPLE_RATE 44100// 采样率
#define SAMPLE_FORMAT AUDIO_S16LSB// 采样格式
#define SAMPLE_SIZE SDL_AUDIO_BITSIZE(SAMPLE_FORMAT)// 采样大小
#define CHANNELS 2// 声道数
#define SAMPLES 512// 音频缓冲区的样本数量


#define ERROR_BUF \
    char errbuf[1024];\
    av_strerror(ret,errbuf,sizeof(errbuf));

#define CODE(func,code) \
    if(ret < 0) { \
    ERROR_BUF;\
    printf("%s error %s",#func,errbuf);\
    cout << #func << "error" << errbuf;\
    code;\
    }

#define END(func) CODE(func,fataError();return;)

#define RET(func) CODE(func,return ret;)

#define CONTINUE(func) CODE(func,continue;)

#define BREAK(func) CODE(func,break;)

int  audioBuffIndex = 0;

#pragma mark - 构造析构
VideoPlayer::VideoPlayer() {
    avformat_network_init();// 初始化网络库以及网络加密相关协议
}

VideoPlayer::~VideoPlayer(){
    stop();//窗口关闭停掉子线程
    SDL_Quit();//若不该为Stopped状态，线程还在后台执行未停止
}

#define MEN_ITEM_SIZE (20 * 1024 * 102)

#pragma mark - State
void VideoPlayer:: play() {
    if(_state == VideoPlayer::Playing) return;
    if(_state == Stopped){
        std::thread([this](){
            readFile();
        }).detach();
    }else{
        setState(VideoPlayer::Playing);//改变状态
    }
}

void VideoPlayer::pause(){
    if(_state != VideoPlayer::Playing) return;
    setState(VideoPlayer::Paused);
}

void VideoPlayer::stop(){
    if(_state == VideoPlayer::Stopped) return;
    _state = Stopped;//改变状态
    playerfree(); //释放资源
    stateChanged(self);//通知外界
}

bool VideoPlayer::isPlaying(){
    return _state == VideoPlayer::Playing;
}

#pragma mark - State Set
VideoPlayer::State VideoPlayer::getState(){
     return _state;
}

//总时长四舍五入
int VideoPlayer::getTime(){
    return round(_aTime);
}

void VideoPlayer::setTime(int seekTime){
   _seekTime = seekTime;
   cout << "setTime" << seekTime << endl;
}

void VideoPlayer::setVolumn(int volumn){
    _volumn = volumn;
}

int VideoPlayer::getVolume(){
    return _volumn;
}

void VideoPlayer::setMute(bool mute){
    _mute = mute;
}

bool VideoPlayer::isMute(){
    return _mute;
}

#pragma mark - 公有方法
void VideoPlayer::setFilename(const char *filename) {
    printf("%s\n",filename);
    memcpy(_filename,filename,strlen(filename) + 1);
    cout << _filename << endl;
}

int VideoPlayer::getDuration() {
    return _fmtCtx ? round(_fmtCtx->duration * av_q2d(AV_TIME_BASE_Q)):0; //ffmpeg时间转现实时间，取整
}

#pragma mark - 公有方法
void VideoPlayer::readFile() {
    int ret = 0;
//    const char * str = "rtmp://58.200.131.2:1935/livetv/cctv1";
    ret = avformat_open_input(&_fmtCtx,_filename,nullptr,nullptr); //获取AVFormatContext
    ret = avformat_find_stream_info(_fmtCtx,nullptr); // 获取视频文件信息，不调用回出现什么问题
    // avformat_find_stream_info 会带来很大延迟
    av_dump_format(_fmtCtx,0,_filename,0);
    fflush(stderr);
     
    for (int i = 0; i< _fmtCtx->nb_streams; i++) {
        AVStream *in_stream = _fmtCtx->streams[i];
        if (AVMEDIA_TYPE_AUDIO == in_stream->codecpar->codec_type) {
            if (AV_SAMPLE_FMT_FLTP == in_stream -> codecpar ->format) {
                
            }else if (AV_SAMPLE_FMT_S16P == in_stream -> codecpar-> format) {
                
            } else if (AV_CODEC_ID_H264) {
                
            }else {
                
            }
            printf("-------  video info :\n");
            printf("fps:%lffps \n", av_q2d(in_stream->avg_frame_rate));
            printf("width:%d   height:%d\n", in_stream->codecpar->width, in_stream->codecpar->height);
            if (in_stream -> duration != AV_NOPTS_VALUE) {
                int duration_video = (in_stream->duration) * av_q2d(in_stream->time_base);
                printf("video duration: %02d:%02d:%02d\n",
                       duration_video / 3600,
                       (duration_video % 3600)/60 ,
                       (duration_video % 60));
            }
        }
    }
    
    _hasAudio = initAudioInfo() >= 0;
    _hasVideo = initVideoInfo() >= 0;

    if(!_hasAudio && !_hasVideo) {
        fataError();
        return;
    }

    initFinished(self); //初始化完毕，发送信号
    setState(VideoPlayer::Playing);
    SDL_PauseAudio(0);

    std::thread([this](){ //视频解码子线程开始工作:开启新的线程去解码视频数据
        decodeVideo();
    }).detach();

//    AVPacket *pkt =  av_packet_alloc();
    AVPacket pkt;
    while(_state != Stopped) {
        //处理seek操作
        if(_seekTime >= 0){
            int streamIdx;
            if(_hasAudio){//优先使用音频流索引
            cout << "seek优先使用，音频流索引" << _aStream->index << endl;
              streamIdx = _aStream->index;
            }else{
            cout << "seek优先使用，视频流索引" << _vStream->index << endl;
              streamIdx = _vStream->index;
            }
            //现实时间 -> 时间戳
            AVRational time_base = _fmtCtx->streams[streamIdx]->time_base;
            int64_t ts = _seekTime/av_q2d(time_base);
            ret = av_seek_frame(_fmtCtx,streamIdx,ts,AVSEEK_FLAG_BACKWARD);
            if(ret < 0){//seek失败
                cout << "seek失败" << _seekTime << ts << streamIdx << endl;
                _seekTime = -1;
            }else{//seek成功
                cout << "------------seek成功-----------seekTime:" << _seekTime << "--ffmpeg时间戳:" << ts << "--流索引:" << streamIdx << endl;
                //记录seek到了哪一帧，有可能是P帧或B,会导致seek向前找到I帧，此时就会比实际seek的值要提前几帧，现象是调到seek的帧时会快速的闪现I帧到seek的帧
                //清空之前读取的数据包
                clearAudioPktList();
                clearVideoPktList();
                _vSeekTime = _seekTime;
                _aSeekTime = _seekTime;
                _seekTime = -1;
                _aTime = 0;//恢复时钟
                _vTime = 0;
            }
        }

        int vSize = static_cast<int>(_vPktList.size());
        int aSize = static_cast<int>(_aPktList.size());
        if(vSize >= VIDEO_MAX_PKT_SIZE || aSize >= AUDIO_MAX_PKT_SIZE){
            SDL_Delay(10); //不要将文件中的压缩数据一次性读取到内存中，控制下大小
            continue;
        }
        ret = av_read_frame(_fmtCtx,&pkt); // 很重要的这个～～～从码流中读取音视频流（是如何进行区分的呢）
        if(ret == 0){
            if(pkt.stream_index == _aStream->index){//读取到的是音频数据
                addAudioPkt(pkt);
            }else if(pkt.stream_index == _vStream->index){//读取到的是视频数据
                addVideoPkt(pkt);
            }
            else{//如果不是音频、视频流，直接释放，防止内存泄露
                av_packet_unref(&pkt);
            }
        }else if(ret == AVERROR_EOF){//读到了文件尾部
            if(vSize == 0 && aSize == 0){
                cout << "AVERROR_EOF读取到文件末尾了-------vSize=0--aSize=0--------" << endl;
                _fmtCtxCanFree = true;
                break;
            }
        }else{
            ERROR_BUF;
            cout << "av_read_frame error" << errbuf;
            continue;
        }
    }
    if(_fmtCtxCanFree){//正常读取到文件尾部
        cout << "AVERROR_EOF------------读取到文件末尾了---------------" << _fmtCtxCanFree << endl;
        stop();
    }else{
        //标记一下:_fmtCtx可以释放了
        _fmtCtxCanFree = true;
    }
}


void VideoPlayer::setState(State state) {
    cout << "setState() ~~~~~~" << endl;
    if(state == _state) return;
    _state = state;
    //通知播放器进行UI改变
    stateChanged(self);
}

void VideoPlayer::playerfree() {
    cout << "playerfree() ~~~~~~" << endl;
    while (_hasAudio && !_aCanFree);
    while (_hasVideo && !_vCanFree);
    while (!_fmtCtxCanFree);

    _seekTime = -1;
    avformat_close_input(&_fmtCtx);
    _fmtCtxCanFree = false;
    freeAudio();
    freeVideo();
}

void VideoPlayer::fataError() {
    cout << "fataError() ~~~~~~" << endl;
    _state = Playing;
    stop();
    setState(Stopped);
    playFailed(self);
    playerfree();
}

#pragma mark - init Decoder
int VideoPlayer::initDecoder(AVCodecContext **decodeCtx,AVStream **stream,AVMediaType type) {
    int ret = av_find_best_stream(_fmtCtx,type,-1,-1,nullptr,0);
    RET(av_find_best_stream);
    int streamIdx = ret;
    *stream = _fmtCtx->streams[streamIdx];
    if(!*stream){
        cout << "stream is empty" << endl;
        return -1;
    }
    AVCodec *decoder = nullptr;
    decoder = avcodec_find_decoder((*stream)->codecpar->codec_id);
    if(!decoder){
        cout << "decoder not found" <<(*stream)->codecpar->codec_id << endl;
        return -1;
    }
    *decodeCtx = avcodec_alloc_context3(decoder);
    if(!decodeCtx){
        cout << "avcodec_alloc_context3 error" << endl;
        return -1;
    }
    ret = avcodec_parameters_to_context(*decodeCtx,(*stream)->codecpar);
    RET(avcodec_parameters_to_context);
    ret = avcodec_open2(*decodeCtx,decoder,nullptr);
    RET(avcodec_open2);
    return 0;
}

#pragma mark - 初始化 音频信息
int VideoPlayer::initAudioInfo() {
    int ret = initDecoder(&_aDecodeCtx,&_aStream,AVMEDIA_TYPE_AUDIO);
    RET(initDecoder);
    ret = initSwr();
    RET(initSwr);
    ret = initSDL();
    RET(initSDL);

    return 0;
}

int VideoPlayer::initSwr() {
    _aSwrInSpec.sampleRate = _aDecodeCtx->sample_rate;
    _aSwrInSpec.sampleFmt = _aDecodeCtx->sample_fmt;
    _aSwrInSpec.chLayout = static_cast<int>(_aDecodeCtx->channel_layout);
    _aSwrInSpec.chs = _aDecodeCtx->channels;
 
    _aSwrOutSpec.sampleRate = SAMPLE_RATE;
    _aSwrOutSpec.sampleFmt = AV_SAMPLE_FMT_S16;
    _aSwrOutSpec.chLayout = AV_CH_LAYOUT_STEREO;
    _aSwrOutSpec.chs = av_get_channel_layout_nb_channels(_aSwrOutSpec.chLayout);
    _aSwrOutSpec.bytesPerSampleFrame = _aSwrOutSpec.chs * av_get_bytes_per_sample(_aSwrOutSpec.sampleFmt);
    
    _aSwrCtx = swr_alloc_set_opts(nullptr,
                                  _aSwrOutSpec.chLayout,_aSwrOutSpec.sampleFmt,_aSwrOutSpec.sampleRate,
                                  _aSwrInSpec.chLayout, _aSwrInSpec.sampleFmt,_aSwrInSpec.sampleRate,
                                  0, nullptr);
    if (!_aSwrCtx) {
        cout << "swr_alloc_set_opts error" << endl;
        return -1;
    }
    int ret = swr_init(_aSwrCtx);
    RET(swr_init);
  
    _aSwrInFrame = av_frame_alloc();//初始化输入Frame
    if(!_aSwrInFrame){
        cout << "av_frame_alloc error" << endl;
        return -1;
    }
 
    _aSwrOutFrame = av_frame_alloc(); //初始化输出Frame
    if(!_aSwrOutFrame){
        cout << "av_frame_alloc error" << endl;
        return -1;
    }
    
    ret = av_samples_alloc(_aSwrOutFrame->data,_aSwrOutFrame->linesize,_aSwrOutSpec.chs,1024,_aSwrOutSpec.sampleFmt,1);
    RET(av_samples_alloc);
    return 0;
}

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
        cout << "SDL_OpenAudio Error " << SDL_GetError() << endl;
        return -1;
    }
    return 0;
}

void VideoPlayer::sdlAudioCallbackFunc(void *userdata, Uint8 *stream, int len) {
    VideoPlayer *player = (VideoPlayer *)userdata;
    player->sdlAudioCallback(stream,len);
}
 
void VideoPlayer::addAudioPkt(AVPacket &pkt) {
    _aMutex.lock();
    _aPktList.push_back(pkt);
    audioBuffIndex ++;
    _aMutex.signal();
    _aMutex.unlock();
    cout << " audioBuffIndex ===" << audioBuffIndex << endl;
}

void VideoPlayer::clearAudioPktList() {
    _aMutex.lock();
    for(AVPacket &pkt:_aPktList){
        av_packet_unref(&pkt);
    }
    _aPktList.clear();
    _aMutex.unlock();
}

void VideoPlayer::freeAudio() {
    _aTime = 0;
    _aSwrOutIdx = 0;
    _aSwrOutSize = 0;
    _aStream = nullptr;
    _aCanFree = false;
    _aSeekTime = -1;

    clearAudioPktList();
    avcodec_free_context(&_aDecodeCtx);
    av_frame_free(&_aSwrInFrame);
    swr_free(&_aSwrCtx);
    if(_aSwrOutFrame){
        av_freep(&_aSwrOutFrame->data[0]);
        av_frame_free(&_aSwrOutFrame);
    }
     
    SDL_PauseAudio(1);
    SDL_CloseAudio();
}

void VideoPlayer::sdlAudioCallback(Uint8 *stream, int len) {
    SDL_memset(stream,0,len);
    while (len > 0) {
        if(_state == Paused) break;
        if(_state == Stopped){
            _aCanFree = true;
             break;
        }
        if(_aSwrOutIdx >= _aSwrOutSize){
            _aSwrOutSize = decodeAudio();//全新PCM的数据大小
            _aSwrOutIdx = 0; //索引清0
            if(_aSwrOutSize <= 0){ //没有解码出PCM数据那就静音处理
                _aSwrOutSize = 1024;  //出错了，或者还没有解码出PCM数据，假定1024个字节静音处理
                memset(_aSwrOutFrame->data[0],0,_aSwrOutSize); //给PCM填充0(静音)
            }
        }
        int fillLen = _aSwrOutSize - _aSwrOutIdx;//本次需要填充到stream中的PCM数据大小
        fillLen = std::min(fillLen,len);
        int volumn = _mute ? 0:(_volumn * 1.0/Max) * SDL_MIX_MAXVOLUME;//获取音量
        SDL_MixAudio(stream,_aSwrOutFrame->data[0]+_aSwrOutIdx,fillLen,volumn); //将一个pkt包解码后的pcm数据填充到SDL的音频缓冲区
        len -= fillLen; //移动偏移量
        stream += fillLen;
        _aSwrOutIdx += fillLen;
        cout << "_aSwrOutIdx == " << _aSwrOutIdx <<endl;
    }
}

int VideoPlayer::decodeAudio(){
    _aMutex.lock();
    if(_aPktList.empty()){
        cout << "_aPktList为空" << _aPktList.size() << endl;
        _aMutex.unlock();
        return 0;
    }
    
    AVPacket &pkt = _aPktList.front();
    if(pkt.pts != AV_NOPTS_VALUE){ //音频包应该在多少秒播放
      _aTime = av_q2d(_aStream->time_base) * pkt.pts;
        timeChanged(self);//通知外界:播放时间发生了改变
    }
    
    //如果是视频，不能在这个位置判断(不能提前释放pkt,不然会导致B帧、P帧解码失败，画面撕裂)
    if(_aSeekTime >= 0){
        if(_aTime < _aSeekTime){ //发现音频的时间是早于seekTime的，就丢弃，防止到seekTime的位置闪现
            av_packet_unref(&pkt); //释放pkt
            _aPktList.pop_front();
             _aMutex.unlock();
            return 0;
        }else{
            _aSeekTime = -1;
        }
    }
 
    int ret = avcodec_send_packet(_aDecodeCtx, &pkt);
    av_packet_unref(&pkt);
    _aPktList.pop_front();
    _aMutex.unlock();
    RET(avcodec_send_packet);
 
    ret = avcodec_receive_frame(_aDecodeCtx, _aSwrInFrame); // 获取解码后的数据
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
        return 0;
    } else RET(avcodec_receive_frame);
   
    int64_t outSamples = av_rescale_rnd(_aSwrOutSpec.sampleRate, _aSwrInFrame->nb_samples, _aSwrInSpec.sampleRate, AV_ROUND_UP);
    ret = swr_convert(_aSwrCtx,_aSwrOutFrame->data, (int)outSamples,(const uint8_t **) _aSwrInFrame->data, _aSwrInFrame->nb_samples);
    RET(swr_convert);
    //ret为每一个声道的样本数 * 声道数 * 每一个样本的大小 = 重采样后的pcm的大小
    return  ret * _aSwrOutSpec.bytesPerSampleFrame;
}


#pragma mark - 初始化 视频信息
int VideoPlayer::initVideoInfo() {
    int ret = initDecoder(&_vDecodeCtx,&_vStream,AVMEDIA_TYPE_VIDEO);
    RET(initDecoder);
    ret = initSws();
    RET(initSws);
    return 0;
}
 
int VideoPlayer::initSws() {
    _vSwsInFrame = av_frame_alloc();
    if(!_vSwsInFrame){
        cout << "av_frame_alloc error" << endl;
        return -1;
    }
    return 0;
}

void VideoPlayer::addVideoPkt(AVPacket &pkt) {
    _vMutex.lock();
    _vPktList.push_back(pkt);
    _vMutex.signal();
    _vMutex.unlock();
}

void VideoPlayer::clearVideoPktList() {
    _vMutex.lock();
    for(AVPacket &pkt:_vPktList){
        av_packet_unref(&pkt);
    }
    _vPktList.clear();
    _vMutex.unlock();
}

void VideoPlayer::freeVideo() {
    _vTime = 0;
    _vCanFree = false;
    clearVideoPktList();
    avcodec_free_context(&_vDecodeCtx);
    av_frame_free(&_vSwsInFrame);
    if(_vSwsOutFrame) {
        av_freep(&_vSwsOutFrame->data[0]);
        av_frame_free(&_vSwsOutFrame);
    }
    sws_freeContext(_vSwsCtx);
    _vSwsCtx = nullptr;
    _vStream = nullptr;
    _vSeekTime = -1;
}

void VideoPlayer::decodeVideo() {
    while (true) {
        if(_state == Paused && _vSeekTime == -1) continue;
        //如果是停止状态，会调用free，就不用再去解码，重采样，渲染，导致访问释放了的内存空间，会闪退
        if(_state == Stopped){
            _vCanFree = true;
            break;
        }
        _vMutex.lock();
        if(_vPktList.empty()){
            _vMutex.unlock();
            continue;
        }
        AVPacket pkt = _vPktList.front();
        _vPktList.pop_front();
        _vMutex.unlock();
 
        int ret = avcodec_send_packet(_vDecodeCtx,&pkt);
        if(pkt.dts != AV_NOPTS_VALUE){
//            cout << "视频时间基分子:" << _vStream->time_base.num << "---视频时间基分母:" << _vStream->time_base.den << endl;
            _vTime = av_q2d(_vStream->time_base) * pkt.pts;
//            cout << "当前视频时间"<< _vTime << "seek时间" << _vSeekTime << endl;
        }
 
        av_packet_unref(&pkt);
        CONTINUE(avcodec_send_packet);
        int loopIndex = 0;
        while (true) {
            ret = avcodec_receive_frame(_vDecodeCtx,_vSwsInFrame);
            if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF){
                break;//结束本次循环，重新从_vPktList取出包进行解码
            }else BREAK(avcodec_receive_frame);
            loopIndex ++;
//            cout<< "loopIndex =============== " << loopIndex << endl;
            
            //一定要在解码成功后，再进行下面的判断,防止seek时，到达的是p帧，但前面的I帧已经被释放了，无法参考，这一帧的解码就会出现撕裂现象
            //发现视频的时间是早于seekTime的，就丢弃，防止到seekTime的位置闪现
            if(_vSeekTime >= 0){
                if(_vTime < _vSeekTime){
                    continue;
                }else{
                    _vSeekTime = -1;
                }
            }
            
            char *buf = (char *)malloc(_vSwsInFrame->width * _vSwsInFrame->height * 3 / 2);
            AVPicture *pict;
            int w, h;
            char *y, *u, *v;
            pict = (AVPicture *)_vSwsInFrame;//这里的frame就是解码出来的AVFrame
            w = _vSwsInFrame->width;
            h = _vSwsInFrame->height;
            y = buf;
            u = y + w * h;
            v = u + w * h / 4;
            for (int i=0; i<h; i++)
                memcpy(y + w * i, pict->data[0] + pict->linesize[0] * i, w);
            for (int i=0; i<h/2; i++)
                memcpy(u + w / 2 * i, pict->data[1] + pict->linesize[1] * i, w / 2);
            for (int i=0; i<h/2; i++)
                memcpy(v + w / 2 * i, pict->data[2] + pict->linesize[2] * i, w / 2);
            if(_hasAudio){//有音频
                //如果视频包多早解码出来，就要等待对应的音频时钟到达
                //有可能点击停止的时候，正在循环里面，停止后sdl free掉了，就不会再从音频list中取出包，_aClock就不会增大，下面while就死循环了，一直出不来，所以加Playing判断
                printf("vTime=%lf, aTime=%lf, vTime-aTime=%lf\n", _vTime, _aTime, _vTime - _aTime);
                while(_vTime > _aTime && _state == Playing){//音视频同步
//                    cout<< "音视频当然要同步啦～～～～～～～" << endl;
                }
            }else{
                //TODO 没有音频的情况
            }
            playerDoDraw(self,buf,_vSwsInFrame->width,_vSwsInFrame->height);
            //TODO ---- 啥时候释放 若立即释放 会崩溃 原因是渲染并没有那么快，OPENGL还没有渲染完毕，但是这块内存已经被free掉了
            
            //放到OPGLES glview中等待一帧渲染完毕后，再释放，此处不能释放
//            cout<< "buf ~~~ size :" << sizeof(buf) << endl;
//            free(buf);
//            cout << "渲染了一帧" << _vTime << "音频时间" << _aTime << endl;
            //子线程把这一块数据_vSwsOutFrame->data[0]直接发送到主线程，给widget里面的image里面的bits指针指向去绘制图片，主线程也会访问这一块内存数据，子线程也会访问，有可能子线程正往里面写着，主线程就拿去用了，会导致数据错乱，崩溃
            //将像素转换后的图片数据拷贝一份出来,OPENGL渲染是一样的道理，需要先保存新的一份字节数据
        }
    }
}



#pragma mark 调用C函数
int VideoPlayer::someMethod (void *objectiveCObject, void *aParameter) {
    return playerDoSomethingWith(objectiveCObject, aParameter);
}

void VideoPlayer::setSelf(void *aSelf) {
    self = aSelf;
}
