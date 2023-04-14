//
//  ViewController.m
//  SDLTestDemo
//
//  Created by 尹东博 on 2023/4/12.
//

#import "ViewController.h"
 
//#ifdef __cplusplus
//extern "C" {
//#endif
//
//#include "libavutil/opt.h"
//#include "libavcodec/avcodec.h"
//#include "libavformat/avformat.h"
//#include "libswscale/swscale.h"
//
//#include "SDL.h"
//#ifdef __cplusplus
//};
//#endif

#include "VideoPlayer.hpp"

@interface ViewController () {
    VideoPlayer *_player;
}
@property(nonatomic,copy)NSString * path;
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
}


@end
