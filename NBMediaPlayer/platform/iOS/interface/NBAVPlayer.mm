//
//  NBMediaPlayer.m
//  iOSNBMediaPlayer
//
//  Created by liu enbao on 17/09/2018.
//  Copyright © 2018 liu enbao. All rights reserved.
//

#import "NBAVPlayer.h"

#include <NBMediaPlayer.h>
#include <NBLog.h>

NBLOG_DEFINE();

class MediaPlayerListener;

@interface NBAVPlayer () {
    NBMediaPlayer* mp;
    MediaPlayerListener* _playerListener;
    NBRendererTarget _renderTarget;
}

- (void)onNativeEventRecieved:(id)params;

@end

class MediaPlayerListener : public INBMediaPlayerLister {
public:
    MediaPlayerListener(__weak NBAVPlayer* mp)
    :mMP(mp) {
        
    }
    
public:
    virtual void sendEvent(int msg, int ext1=0, int ext2=0) {
        [mMP performSelectorOnMainThread:@selector(onNativeEventRecieved:) withObject:@[@(msg), @(ext1), @(ext2)] waitUntilDone:NO];
    }
    
private:
    __weak NBAVPlayer* mMP;
};

@implementation NBAVPlayer

@synthesize delegate = _delegate;

- (instancetype)init {
    if (self = [super init]) {
        NBLOG_INIT(NULL, NBLOG_MODE_CONSOLE);
        
        mp = new NBMediaPlayer();
        _playerListener = new MediaPlayerListener(self);
        mp->setListener(_playerListener);
    }
    return self;
}

- (void)setDataSource:(NSURL*)uri params:(NSDictionary*)params {
    mp->setDataSource([[uri absoluteString] UTF8String], NULL);
}

- (void)setVideoOutput:(NBGLView*)glView {
    _renderTarget.params = (__bridge void*)glView;
    mp->setVideoOutput(&_renderTarget);
}

- (void)prepareAsync {
    mp->prepareAsync();
}

- (void)start {
    mp->play();
}

- (void)pause {
    mp->pause();
}

- (void)seekTo:(int)seekPos {
    mp->seekTo(seekPos * 1000);
}

- (void)stop {
    mp->stop();
}

- (void)reset {
    mp->reset();
}

- (int)getDuration {
    int64_t duration = 0;
    mp->getDuration(&duration);
    return (int)(duration / 1000);
}

- (int)getCurrentPosition {
    int64_t position = 0;
    mp->getCurrentPosition(&position);
    return (int)(position / 1000);
}

- (BOOL)isPlaying {
    return mp->isPlaying() ? YES : NO;
}

- (int)getVideoWidth {
    int32_t width = -1;
    mp->getVideoWidth(&width);
    return width;
}

- (int)getVideoHeight {
    int32_t height = -1;
    mp->getVideoWidth(&height);
    return height;
}

// to release the all alloced object
- (void)releaseAll {
    if (mp != NULL) {
        delete mp;
        mp = NULL;
    }
    
    if (_playerListener != NULL) {
        delete _playerListener;
        _playerListener = NULL;
    }
}

- (void)onNativeEventRecieved:(id)params {
    int msg = [[params objectAtIndex:0] intValue];
    switch (msg) {
        case MEDIA_SET_VIDEO_SIZE:
            if ([self.delegate respondsToSelector:@selector(mediaPlayer:videoSizeChanged:)])
                [self.delegate mediaPlayer:self videoSizeChanged:@[[params objectAtIndex:1], [params objectAtIndex:2]]];
            break;
        case MEDIA_PREPARED:
            [self.delegate mediaPlayer:self didPrepared:nil];
            break;
        case MEDIA_PLAYBACK_COMPLETE:
            [self.delegate mediaPlayer:self playbackComplete:nil];
            break;
        case MEDIA_ERROR:
            [self.delegate mediaPlayer:self error:@([[params objectAtIndex:1] integerValue])];
            break;
        default:
            break;
    }
}

@end
