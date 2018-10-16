//
//  NBMediaPlayer.h
//  iOSNBMediaPlayer
//
//  Created by liu enbao on 17/09/2018.
//  Copyright © 2018 liu enbao. All rights reserved.
//

#import <Foundation/Foundation.h>

@class NBGLView;
@class NBAVPlayer;

/**
 * Media player delegate.
 */
@protocol NBAVPlayerDelegate <NSObject>

@required
/**
 * Called when the player prepared.
 *
 * @param player The shared media player instance.
 * @param arg Not use.
 */
- (void)mediaPlayer:(NBAVPlayer *)player didPrepared:(id)arg;

/**
 * Called when the player playback completed.
 *
 * @param player The shared media player instance.
 * @param arg Not use.
 */
- (void)mediaPlayer:(NBAVPlayer *)player playbackComplete:(id)arg;

/**
 * Called when the player have error occur.
 *
 * @param player The shared media player instance.
 * @param arg Contain the detail error information.
 */
- (void)mediaPlayer:(NBAVPlayer *)player error:(id)arg;

@optional

/**
 * Called when set the data source to player.
 *
 * You can tell media player manager what preference are you like in this call back method.
 * e.g. set `player.playDoblyAudioCodecs.
 *
 * @param player The shared media player instance.
 * @param arg Not use.
 */
- (void)mediaPlayer:(NBAVPlayer *)player setupManagerPreference:(id)arg;

/**
 * Called when the download rate change.
 *
 * This method is only useful for online media stream.
 *
 * @param player The shared media player instance.
 * @param arg *NSNumber* type, *int* value. The rate in KBytes/s.
 */
- (void)mediaPlayer:(NBAVPlayer *)player downloadRate:(id)arg;

/**
 * Called when the player seek completed.
 *
 * @param player The shared media player instance.
 * @param arg Not use.
 */
- (void)mediaPlayer:(NBAVPlayer *)player seekComplete:(id)arg;

/**
 * Called when the player buffering start.
 *
 * @param player The shared media player instance.
 * @param arg Not use.
 */
- (void)mediaPlayer:(NBAVPlayer *)player bufferingStart:(id)arg;

/**
 * Called when the player buffering progress changed.
 *
 * @param player The shared media player instance.
 * @param arg *NSNumber* type, *int* value. The progress percent of buffering.
 */
- (void)mediaPlayer:(NBAVPlayer *)player bufferingUpdate:(id)arg;

/**
 * Called when the player buffering end.
 *
 * @param player The shared media player instance.
 * @param arg Not use.
 */
- (void)mediaPlayer:(NBAVPlayer *)player bufferingEnd:(id)arg;

/**
 * Called when set the got the video size
 *
 * @param player The shared media player instance.
 * @param arg *NSArray* type. obj 0 *NSNubmer* type *int* value width of the video
 *                            obj 1 *NSNubmer* type *int* value height of the video
 */
- (void)mediaPlayer:(NBAVPlayer *)player videoSizeChanged:(id)arg;

@end

@interface NBAVPlayer : NSObject

@property (nonatomic, assign) id<NBAVPlayerDelegate> delegate;

- (void)setDataSource:(NSURL*)path params:(NSDictionary*)headers;

- (void)setVideoOutput:(NBGLView*)glView;

- (void)start;

- (void)stop;

- (void)pause;

- (void)seekTo:(int)msec;

- (void)reset;

- (void)prepareAsync;

- (int)getVideoWidth;

- (int)getVideoHeight;

- (BOOL)isPlaying;

- (int)getDuration;

- (int)getCurrentPosition;

// to release the all alloced object
- (void)releaseAll;

@end
