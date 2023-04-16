//
//  ViewController.m
//  SDLTestDemo
//
//  Created by 尹东博 on 2023/4/12.
//

#import "ViewController.h"
#import "OpenGLView20.h"
#include "VideoPlayer.hpp"

@interface ViewController () <
UIGestureRecognizerDelegate
> {
    VideoPlayer *_player;
    OpenGLView20 *_myview;
}
@property (nonatomic, strong) NSString * path;
 
@property (nonatomic, strong) UIButton *playBtn;
@property (nonatomic, strong) UIButton *stopBtn;
@property (nonatomic, strong) UIButton *muteBtn;
@property (nonatomic, strong) UISlider *timeSlider;
@property (nonatomic, strong) UISlider *volumnSlider;
@property (nonatomic, strong) UILabel *volumnLabel;
@property (nonatomic, strong) UITapGestureRecognizer *tapGesture;
@property (nonatomic, strong) UITapGestureRecognizer *volumnTapGesture;
@property (nonatomic, strong) UILabel *durationLabel;
@property (nonatomic, strong) UILabel *label;
@property (nonatomic, strong) UILabel *timeLabel;
@property (nonatomic, strong) UIView *blackView;
@property (nonatomic, strong) UIView *menuView;
@end

@implementation ViewController

  
- (void)viewDidLoad {
    [super viewDidLoad];


    _player = new VideoPlayer();
    _path =  [[NSBundle mainBundle] pathForResource:@"MyHeartWillGoOn" ofType:@"mp4"];
    const char *filename = [_path cStringUsingEncoding:NSUTF8StringEncoding];
    __weak typeof(self) weakSelf = self;
    dispatch_async(dispatch_get_global_queue(0, 0), ^{    //传入文件路径
        __strong typeof(weakSelf) strongSelf = weakSelf;
        _player->setSelf((__bridge void *)strongSelf);
        _player->setFilename(filename);
        _player->readFile();
    });
    
    //渲染View
    _myview = [[OpenGLView20 alloc]initWithFrame:CGRectMake(0, 0, self.view.frame.size.width, self.view.frame.size.height - 200)];
    [self.view addSubview:_myview];
    
    //菜单View
    self.menuView = [[UIView alloc] initWithFrame:CGRectMake(0, CGRectGetMaxY(_myview.frame), self.view.frame.size.width, 200)];
    [self.view addSubview:self.menuView];
    
    //播放/暂停按钮
    self.playBtn = [UIButton buttonWithType:UIButtonTypeCustom];
    self.playBtn.frame = CGRectMake(2, 15, 40, 30);
    self.playBtn.layer.cornerRadius = 10;
    [self.playBtn setTitle:@"播放" forState:UIControlStateNormal];
    [self.playBtn setTintColor:[UIColor whiteColor]];
    [self.playBtn setBackgroundColor:[UIColor cyanColor]];
    [self.playBtn setTitleColor:[UIColor blackColor] forState:UIControlStateNormal];
    [self.playBtn addTarget:self action:@selector(playBtnClick:) forControlEvents:UIControlEventTouchUpInside];
    [self.menuView addSubview:self.playBtn];
    
    //停止按钮
    self.stopBtn = [UIButton buttonWithType:UIButtonTypeCustom];
    self.stopBtn.frame = CGRectMake(CGRectGetMaxX(self.playBtn.frame)+6, 15, 40, 30);
    self.stopBtn.layer.cornerRadius = 10;
    [self.stopBtn setTitle:@"停止" forState:UIControlStateNormal];
    [self.stopBtn setTintColor:[UIColor whiteColor]];
    [self.stopBtn setBackgroundColor:[UIColor cyanColor]];
    [self.stopBtn setTitleColor:[UIColor blackColor] forState:UIControlStateNormal];
    [self.stopBtn addTarget:self action:@selector(stopBtnClick:) forControlEvents:UIControlEventTouchUpInside];
    [self.menuView addSubview:self.stopBtn];
    
    //播放进度条
    /// 创建Slider 设置Frame
    UISlider *timeSlider = [[UISlider alloc] initWithFrame:CGRectMake(CGRectGetMaxX(_stopBtn.frame)+6, 15, self.view.frame.size.width - CGRectGetMaxX(self.stopBtn.frame)-10, 50)];
    timeSlider.center = CGPointMake(timeSlider.center.x, self.playBtn.center.y);
    self.timeSlider = timeSlider;
    /// 添加Slider
    [self.menuView addSubview:timeSlider];
    timeSlider.tag = 100;
    /// 属性配置
    // minimumValue  : 当值可以改变时，滑块可以滑动到最小位置的值，默认为0.0
    timeSlider.minimumValue = 0.0;
    // maximumValue : 当值可以改变时，滑块可以滑动到最大位置的值，默认为1.0
    timeSlider.maximumValue = 99.0;
    // 当前值，这个值是介于滑块的最大值和最小值之间的，如果没有设置边界值，默认为0-1；
    timeSlider.value = 0.0;
    // continuous : 如果设置YES，在拖动滑块的任何时候，滑块的值都会改变。默认设置为YES
    [timeSlider setContinuous:YES];
    // minimumTrackTintColor : 小于滑块当前值滑块条的颜色，默认为蓝色
    timeSlider.minimumTrackTintColor = [UIColor blueColor];
    // maximumTrackTintColor: 大于滑块当前值滑块条的颜色，默认为白色
    timeSlider.maximumTrackTintColor = [UIColor grayColor];
    // thumbTintColor : 当前滑块的颜色，默认为白色
    timeSlider.thumbTintColor = [UIColor cyanColor];
    /// 事件监听
    [timeSlider addTarget:self action:@selector(_sliderValueDidChanged:) forControlEvents:UIControlEventValueChanged];
    [timeSlider addTarget:self action:@selector(_sliderTouchDown:) forControlEvents:UIControlEventTouchDown];
    [timeSlider addTarget:self action:@selector(_sliderTouchUp:) forControlEvents:UIControlEventTouchUpInside];
    
    _tapGesture = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(actionTapGesture:)];
    _tapGesture.delegate = self;
    [timeSlider addGestureRecognizer:_tapGesture];

    //时间Label
    self.timeLabel = [[UILabel alloc] init];
    self.timeLabel.font = [UIFont fontWithName:@"PingFangSC-Regular" size:13];
    self.timeLabel.textColor = [UIColor blackColor];
    self.timeLabel.text = @"00:00:00";
    self.timeLabel.textAlignment = NSTextAlignmentCenter;
    [self.menuView addSubview:self.timeLabel];
    
    CGFloat lbX = CGRectGetMinX(timeSlider.frame);
    CGFloat lbY = CGRectGetMaxY(timeSlider.frame)+2;
    CGFloat lbW = 58;
    CGFloat lbH = 34;
    self.timeLabel.frame = CGRectMake(lbX, lbY, lbW, lbH);
    
    //中间分割线/Label
    self.label = [[UILabel alloc] init];
    self.label.font = [UIFont fontWithName:@"PingFangSC-Regular" size:13];
    self.label.textColor = [UIColor blackColor];
    self.label.text = @"/";
    self.label.textAlignment = NSTextAlignmentCenter;
    [self.menuView addSubview:self.label];
    
    CGFloat labelX = CGRectGetMaxX(self.timeLabel.frame);
    CGFloat labelY = CGRectGetMaxY(timeSlider.frame)+2;
    CGFloat labelW = 6;
    CGFloat lableH = 34;
    self.label.frame = CGRectMake(labelX, labelY, labelW, lableH);
    self.label.center = CGPointMake(self.label.center.x, self.timeLabel.center.y);
    
    //总时长Label
    self.durationLabel = [[UILabel alloc] init];
    self.durationLabel.font = [UIFont fontWithName:@"PingFangSC-Regular" size:13];
    self.durationLabel.textColor = [UIColor blackColor];
    self.durationLabel.text = @"00:00:00";
    self.durationLabel.textAlignment = NSTextAlignmentCenter;
    [self.menuView addSubview:self.durationLabel];
    
    CGFloat durationLabelX = CGRectGetMaxX(self.label.frame);
    CGFloat durationLabelY = CGRectGetMaxY(timeSlider.frame)+2;
    CGFloat durationLabelW = 58;
    CGFloat durationLabelH = 34;
    self.durationLabel.frame = CGRectMake(durationLabelX, durationLabelY, durationLabelW, durationLabelH);
    
    //静音按钮
    self.muteBtn = [UIButton buttonWithType:UIButtonTypeCustom];
    self.muteBtn.frame = CGRectMake(CGRectGetMaxX(self.durationLabel.frame), CGRectGetMinY(self.durationLabel.frame), 40, 30);
    self.muteBtn.layer.cornerRadius = 10;
    [self.muteBtn setTitle:@"静音" forState:UIControlStateNormal];
    [self.muteBtn setTintColor:[UIColor whiteColor]];
    [self.muteBtn setBackgroundColor:[UIColor cyanColor]];
    [self.muteBtn setTitleColor:[UIColor blackColor] forState:UIControlStateNormal];
    [self.muteBtn addTarget:self action:@selector(muteBtnClick:) forControlEvents:UIControlEventTouchUpInside];
    [self.menuView addSubview:self.muteBtn];
    
    //静音滑动条
    /// 创建Slider 设置Frame
    UISlider *volumnSlider = [[UISlider alloc] initWithFrame:CGRectMake(CGRectGetMaxX(_muteBtn.frame), CGRectGetMinY(self.durationLabel.frame), 100, 50)];
    volumnSlider.center = CGPointMake(volumnSlider.center.x, self.muteBtn.center.y);
    self.volumnSlider = volumnSlider;
    /// 添加Slider
    [self.menuView addSubview:volumnSlider];
    volumnSlider.tag = 101;
    /// 属性配置
    // minimumValue  : 当值可以改变时，滑块可以滑动到最小位置的值，默认为0.0
    volumnSlider.minimumValue = VideoPlayer::Volumn::Min;
    // maximumValue : 当值可以改变时，滑块可以滑动到最大位置的值，默认为1.0
    volumnSlider.maximumValue = VideoPlayer::Volumn::Max;
    // 当前值，这个值是介于滑块的最大值和最小值之间的，如果没有设置边界值，默认为0-1；
    volumnSlider.value = volumnSlider.maximumValue/2;
    // continuous : 如果设置YES，在拖动滑块的任何时候，滑块的值都会改变。默认设置为YES
    [volumnSlider setContinuous:YES];
    // minimumTrackTintColor : 小于滑块当前值滑块条的颜色，默认为蓝色
    volumnSlider.minimumTrackTintColor = [UIColor blueColor];
    // maximumTrackTintColor: 大于滑块当前值滑块条的颜色，默认为白色
    volumnSlider.maximumTrackTintColor = [UIColor grayColor];
    // thumbTintColor : 当前滑块的颜色，默认为白色
    volumnSlider.thumbTintColor = [UIColor cyanColor];
    /// 事件监听
    [volumnSlider addTarget:self action:@selector(_sliderValueDidChanged:) forControlEvents:UIControlEventValueChanged];
    [volumnSlider addTarget:self action:@selector(_sliderTouchDown:) forControlEvents:UIControlEventTouchDown];
    [volumnSlider addTarget:self action:@selector(_sliderTouchUp:) forControlEvents:UIControlEventTouchUpInside];
    
    _volumnTapGesture = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(actionVolumnTapGesture:)];
    _volumnTapGesture.delegate = self;
    [volumnSlider addGestureRecognizer:_volumnTapGesture];
    
    //volumnLabel
    self.volumnLabel = [[UILabel alloc] init];
    self.volumnLabel.font = [UIFont fontWithName:@"PingFangSC-Regular" size:13];
    self.volumnLabel.textColor = [UIColor blackColor];
    self.volumnLabel.text = [NSString stringWithFormat:@"%ld",lround(volumnSlider.value)];
    self.volumnLabel.textAlignment = NSTextAlignmentLeft;
    [self.menuView addSubview:self.volumnLabel];
    
    CGFloat volumnLabelX = CGRectGetMaxX(self.volumnSlider.frame);
    CGFloat volumnLabelY = CGRectGetMaxY(timeSlider.frame);
    CGFloat volumnLabelW = 30;
    CGFloat volumnLabelH = 22;
    self.volumnLabel.frame = CGRectMake(volumnLabelX, volumnLabelY, volumnLabelW, volumnLabelH);
    self.volumnLabel.center = CGPointMake(self.volumnLabel.center.x, self.volumnSlider.center.y);
}

#pragma mark 视频文件适配屏幕尺寸
-(void)adapterScreenWidth:(int)videoWidth Height:(int)videoHeight{
    //计算最终的尺寸
    //组件/屏幕的尺寸
    int w = self.view.frame.size.width;
    int h = self.view.frame.size.height - 200;
    //计算rect
    int dx = 0;
    int dy = 0;
    int dw = videoWidth;
    int dh = videoHeight;
    //计算目标尺寸
    //图片的宽高比大于播放器的宽高比，则缩放图片到图片的宽度等于播放器的宽度
    //图片的高宽比大于播放器的高宽比，则缩放图片到图片的高度等于播放器的高度
    /*
    if(dw/dh > w/h){
        dw = w;
        dh = 计算出来
    }else{
        dh = h;
        dw = 计算出来
    }
     */
    if(dw > w || dh > h){ //图片的宽度大于播放器宽度或图片的高度大于播放器的高度都要进行缩放
        if(dw * h > w * dh){//视频的宽高比 > 播放器的宽高比   视频的宽度=播放器的宽度 等比例压缩求出缩放后的视频的高度
            dh = dh * w/dw;
            dw = w;
        }else{//视频的高宽比 > 播放器的高宽比 视频的高度= 播放器的高度  等比例压缩求出缩放后的视频的宽度
            dw = dw * h/dh;
            dh = h;
        }
    }
    //居中
    dx = (w-dw) >> 1;
    dy = (h-dh) >> 1;
    
    _myview.frame = CGRectMake(dx, dy, dw, dh);
    _menuView.frame = CGRectMake(0, CGRectGetMaxY(_myview.frame), _menuView.frame.size.width, _menuView.frame.size.height);
}
- (void)actionVolumnTapGesture:(UITapGestureRecognizer *)sender{
    CGPoint touchPoint = [sender locationInView:_volumnSlider];
    CGFloat value = (_volumnSlider.maximumValue - _volumnSlider.minimumValue) * (touchPoint.x / _volumnSlider.frame.size.width );
    [_volumnSlider setValue:value animated:YES];
    self.volumnLabel.text = [NSString stringWithFormat:@"%ld",lround(value)];
    _player->setVolumn(value);
}
- (void)actionTapGesture:(UITapGestureRecognizer *)sender {
    CGPoint touchPoint = [sender locationInView:_timeSlider];
    CGFloat value = (_timeSlider.maximumValue - _timeSlider.minimumValue) * (touchPoint.x / _timeSlider.frame.size.width );
    [_timeSlider setValue:value animated:YES];
    NSLog(@"当前滑块点按所处的位置:%lf",value);
    _player->setTime(lround(value));
}
-(void)_sliderTouchUp:(UISlider *)slider{
    //时间滑动条
    if (slider.tag == 100) {
        _tapGesture.enabled = YES;
    }else if(slider.tag == 101){//音量滑动条
        _volumnTapGesture.enabled = YES;
    }
//    NSLog(@"_sliderTouchUp当前滑块点击的位置:%lf",slider.value);
}
-(void)_sliderTouchDown:(UISlider *)slider{
    //时间滑动条
    if (slider.tag == 100) {
        _tapGesture.enabled = NO;
    }else if(slider.tag == 101){//音量滑动条
        _volumnTapGesture.enabled = NO;
    }
//    NSLog(@"_sliderTouchDown当前滑块点击的位置:%lf",slider.value);
}

-(void)_sliderValueDidChanged:(UISlider *)slider{
    if(slider.tag == 100){//播放进度
        NSLog(@"当前视频滑块所处的位置:%lf",slider.value);
        _player->setTime(lround(slider.value));
    }else if (slider.tag == 101) {//音量
        NSLog(@"当前音量滑块所处的位置:%lf",slider.value);
        self.volumnLabel.text = [NSString stringWithFormat:@"%ld",lround(slider.value)];
        _player->setVolumn(lround(slider.value));
    }
}

#pragma mark 音视频播放音频时间变化 时间Label和进度条变化
-(void)timeChanged{
    //音频播放时间设置滚动条和label
    self.timeSlider.value = _player->getTime();
    self.timeLabel.text = [self getTimeText:_player->getTime()];
}
#pragma mark 音视频播放音频时间变化
void timeChanged(void *hhObjectInstance){
    dispatch_async(dispatch_get_main_queue(), ^{
        [(__bridge id)hhObjectInstance timeChanged];
    });
}
#pragma mark 格式化时间 秒->hh:mm:ss
-(NSString *)getTimeText:(int)value{
    NSString *str_hour = [NSString stringWithFormat:@"%02d",value/3600];
    NSString *str_minute = [NSString stringWithFormat:@"%02d",(value%3600)/60];
    NSString *str_second = [NSString stringWithFormat:@"%02d",value%60];
    NSString *format_time = [NSString stringWithFormat:@"%@:%@:%@",str_hour,str_minute,str_second];
    return format_time;
}
#pragma mark 音视频解码器初始化完毕 timeLabel赋值
-(void)initFinished{
    self.timeSlider.minimumValue = 0;//设置slider进度条的伸缩范围
    self.timeSlider.maximumValue = _player->getDuration();
    NSLog(@"初始化完成后滑块最大值:%lf,视频总时长:%d",self.timeSlider.maximumValue,_player->getDuration());
    self.durationLabel.text = [self getTimeText:_player->getDuration()];//显示时间到label上面
}
#pragma mark 音视频解码器初始化完毕
void initFinished(void *hhObjectInstance){
    dispatch_async(dispatch_get_main_queue(), ^{
        [(__bridge id)hhObjectInstance initFinished];
    });
}
#pragma mark 播放器静音
-(void)muteBtnClick:(UIButton *)sender{
    if(_player->isMute()){
        _player->setMute(false);
        [self.muteBtn setTitle:@"静音" forState:UIControlStateNormal];
    }else{
        _player->setMute(true);
        [self.muteBtn setTitle:@"开音" forState:UIControlStateNormal];
    }
}
#pragma mark - 播放器播放暂停
-(void)playBtnClick:(UIButton *)sender{
    VideoPlayer::State state = _player->getState(); //获取当前播放器状态
    if(state == VideoPlayer::Playing){
       _player->pause();
    }else{
       _player->play();
    }
}
#pragma mark - 音视频解码器停止
-(void)stopBtnClick:(UIButton *)sender{
    _player->stop();
}
#pragma mark - 播放器状态改变 调整UI
-(void)stateChanged{
    if(_player == nullptr) {
        NSLog(@"_player已经销毁了");
        return;
    }
    VideoPlayer::State state = _player->getState();
    if(state == VideoPlayer::Playing){
        [self.playBtn setTitle:@"暂停" forState:UIControlStateNormal];
    }else{
        [self.playBtn setTitle:@"播放" forState:UIControlStateNormal];
    }
    if(state == VideoPlayer::Stopped){
        self.stopBtn.enabled = NO;
        self.timeSlider.enabled = NO;
        self.volumnSlider.enabled = NO;
        self.muteBtn.enabled = NO;
        self.timeLabel.text = [self getTimeText:0];
        self.timeSlider.value = 0;
        
       //显示打开文件的页面:最好黑屏
//        [self showES:nil width:768 height:432];
        self.blackView = [[UIView alloc] initWithFrame:CGRectMake(0, 0, _myview.frame.size.width, _myview.frame.size.height)];
        self.blackView.backgroundColor = [UIColor blackColor];
        [_myview addSubview:self.blackView];
        
    }else{
        self.playBtn.enabled = YES;
        self.stopBtn.enabled = YES;
        self.timeSlider.enabled = YES;
        self.volumnSlider.enabled = YES;
        self.muteBtn.enabled = YES;
        //显示播放的页面:正常点击播放重新开始渲染
        [self.blackView removeFromSuperview];
    }
}
#pragma mark - 播放器状态改变
void stateChanged(void *hhObjectInstance){
    dispatch_async(dispatch_get_main_queue(), ^{
        [(__bridge id)hhObjectInstance stateChanged];
    });
}
#pragma mark - OPENGL进行渲染 传入AVFrame帧中的数据 宽 高
-(void)showES:(void *)data width:(uint32_t) w height:(uint32_t) h{
    // w 1610 \ 和918
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        [self adapterScreenWidth:w Height:h];
    });
    [_myview setVideoSize:w height:h];
    [_myview displayYUV420pData:data width:w height:h];
}
#pragma mark - C++视频解码后回调OC调用OPENGL进行渲染
void playerDoDraw(void *hhObjectInstance,void *data, uint32_t w, uint32_t h) {
    dispatch_async(dispatch_get_main_queue(), ^{
        NSLog(@"----------------------主线程开始渲染------------------------%@",[NSThread currentThread]);
        [(__bridge id)hhObjectInstance showES:data width:w height:h];
    });
}
#pragma mark - C++ 回调OC方法
int playerDoSomethingWith(void *self, void *aParameter) {
    // 通过将self指针桥接为oc 对象来调用oc方法
    return [(__bridge id)self doSomethingWith:aParameter];
}

-(int)doSomethingWith:(void *)aParameter {
    //将void *指针强转为对应的类型
    int* param = (int *)aParameter;
    return *param / 2 ;
}


#pragma mark - 音视频播放失败 弹窗
-(void)playFailed {
    UIAlertController *alertController = [UIAlertController alertControllerWithTitle:@"提示" message:@"哦豁，播放失败!" preferredStyle:UIAlertControllerStyleAlert];
    UIAlertAction *cancelAction = [UIAlertAction actionWithTitle:@"Cancel" style:UIAlertActionStyleCancel handler:^(UIAlertAction * _Nonnull action) {
        NSLog(@"点击了Cancel");
    }];
    UIAlertAction *okAction = [UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault handler:^(UIAlertAction * _Nonnull action) {
        NSLog(@"点击了OK");
    }];
    [alertController addAction:okAction];
    [alertController addAction:cancelAction];
    [self presentViewController:alertController animated:YES completion:nil];
}

#pragma mark 音视频播放失败
void playFailed(void *hhObjectInstance) {
    dispatch_async(dispatch_get_main_queue(), ^{
        [(__bridge id)hhObjectInstance playFailed];
    });
}

-(void)dealloc {
    NSLog(@"%s", __func__);
    delete _player;
    _player = nullptr;
}
@end
