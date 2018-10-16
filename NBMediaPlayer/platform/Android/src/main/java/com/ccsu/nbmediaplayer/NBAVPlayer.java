package com.ccsu.nbmediaplayer;

import android.content.ContentResolver;
import android.content.Context;
import android.content.res.AssetFileDescriptor;
import android.database.Cursor;
import android.media.AudioTrack;
import android.media.TimedText;
import android.net.Uri;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.os.PowerManager;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;

import java.io.FileDescriptor;
import java.io.IOException;
import java.lang.ref.WeakReference;
import java.util.Map;

public class NBAVPlayer {

    // the debug file
    private static final String TAG = "NBAVPlayer";

    public static final boolean METADATA_UPDATE_ONLY = true;
    public static final boolean METADATA_ALL = false;
    public static final boolean APPLY_METADATA_FILTER = true;
    public static final boolean BYPASS_METADATA_FILTER = false;

    private long mNativeContext;  //accessed by native methods

    private SurfaceHolder mSurfaceHolder;
    private AudioTrack mAudioTrack = null;
    private EventHandler mEventHandler;
    private PowerManager.WakeLock mWakeLock = null;

    private boolean mScreenOnWhilePlaying;
    private boolean mStayAwake;

    private static final int INVOKE_ID_GET_TRACK_INFO = 1;
    private static final int INVOKE_ID_ADD_EXTERNAL_SOURCE = 2;
    private static final int INVOKE_ID_ADD_EXTERNAL_SOURCE_FD = 3;
    private static final int INVOKE_ID_SELECT_TRACK = 4;
    private static final int INVOKE_ID_DESELECT_TRACK = 5;
    private static final int INVOKE_ID_SET_VIDEO_SCALE_MODE = 6;
    public static final int VIDEO_SCALING_MODE_SCALE_TO_FIT = 1;
    public static final int VIDEO_SCALING_MODE_SCALE_TO_FIT_WITH_CROPPING = 2;

    public static final String MEDIA_MIMETYPE_TEXT_SUBRIP = "application/x-subrip";


    private static final int MEDIA_NOP = 0;
    private static final int MEDIA_PREPARED = 1;
    private static final int MEDIA_PLAYBACK_COMPLETE = 2;
    private static final int MEDIA_BUFFERING_UPDATE = 3;
    private static final int MEDIA_SEEK_COMPLETE = 4;
    private static final int MEDIA_SET_VIDEO_SIZE = 5;
    private static final int MEDIA_PLAYBACK_BUFFERING_UPDATE = 6;
    private static final int MEDIA_SET_AUDIO_PARAMS = 7;

    private static final int MEDIA_ERROR = 100;
    private static final int MEDIA_INFO = 200;
    private static final int MEDIA_TRACKING_INFOMATION = 500;

    private OnPreparedListener mOnPreparedListener;
    private OnCompletionListener mOnCompletionListener;
    private OnBufferingUpdateListener mOnBufferingUpdateListener;
    private OnPlaybackBufferingUpdateListener mOnPlaybackBufferingUpdateListener;
    private OnSeekCompleteListener mOnSeekCompleteListener;
    private OnVideoSizeChangedListener mOnVideoSizeChangedListener;
    private OnErrorListener mOnErrorListener;
    private OnInfoListener mOnInfoListener;

    public static final int MEDIA_ERROR_UNKNOWN = 1;
    public static final int MEDIA_ERROR_SERVER_DIED = 100;
    public static final int MEDIA_ERROR_NOT_VALID_FOR_PROGRESSIVE_PLAYBACK = 200;
    public static final int MEDIA_ERROR_IO = -1004;
    public static final int MEDIA_ERROR_MALFORMED = -1007;
    public static final int MEDIA_ERROR_UNSUPPORTED = -1010;
    public static final int MEDIA_ERROR_TIMED_OUT = -110;

    public static final int MEDIA_INFO_UNKNOWN = 1;
    public static final int MEDIA_INFO_STARTED_AS_NEXT = 2;
    public static final int MEDIA_INFO_VIDEO_RENDERING_START = 3;
    public static final int MEDIA_INFO_VIDEO_TRACK_LAGGING = 700;
    public static final int MEDIA_INFO_BUFFERING_START = 701;
    public static final int MEDIA_INFO_BUFFERING_END = 702;
    public static final int MEDIA_INFO_NETWORK_BANDWIDTH = 703;
    public static final int MEDIA_INFO_DECODE_MODE_NOT_SUPPORT = 707;
    public static final int MEDIA_INFO_BAD_INTERLEAVING = 800;
    public static final int MEDIA_INFO_NOT_SEEKABLE = 801;
    public static final int MEDIA_INFO_METADATA_UPDATE = 802;
    public static final int MEDIA_INFO_TIMED_TEXT_ERROR = 900;
    public static final int MEDIA_INFO_VIDEO_START = 1000;
    public static final int MEDIA_INFO_VIDEO_END = 1001;
    public static final int MEDIA_INFO_VIDEO_PLAYING_START = 1002;

    public static final int MEDIA_DECODE_AUTOSELECT = 0;
    public static final int MEDIA_DECODE_HARDWARE = 1;
    public static final int MEDIA_DECODE_SOFTWARE = 2;

    static {
        try {
            System.loadLibrary("nbavplayer_jni");
        } catch (UnsatisfiedLinkError use) {
            Log.e(TAG, "WARNING: Could not load nbavplayer_jni.so");
        }

        native_init();
    }

    public NBAVPlayer(Context context) {
        Looper looper;
        if ((looper = Looper.myLooper()) != null)
            this.mEventHandler = new EventHandler(this, looper);
        else if ((looper = Looper.getMainLooper()) != null)
            this.mEventHandler = new EventHandler(this, looper);
        else {
            this.mEventHandler = null;
        }

        WeakReference<NBAVPlayer> paramObject = new WeakReference<NBAVPlayer>(this);
        native_setup(paramObject, context);
    }

    public void setVideoScalingMode(int mode) {
        if (!isVideoScalingModeSupported(mode)) {
            String msg = "Scaling mode " + mode + " is not supported";
            throw new IllegalArgumentException(msg);
        }
    }

    public static NBAVPlayer create(Context context, Uri uri) {
        try {
            NBAVPlayer mp = new NBAVPlayer(context);
            mp.setDataSource(context, uri);
            mp.prepare();
            return mp;
        } catch (IOException ex) {
            Log.d("MediaPlayer", "create failed:", ex);
        } catch (IllegalArgumentException ex) {
            Log.d("MediaPlayer", "create failed:", ex);
        } catch (SecurityException ex) {
            Log.d("MediaPlayer", "create failed:", ex);
        }
        return null;
    }

    public static NBAVPlayer create(Context context, int resid) {
        try {
            AssetFileDescriptor afd = context.getResources().openRawResourceFd(resid);
            if (afd == null) {
                return null;
            }
            NBAVPlayer mp = new NBAVPlayer(context);
            mp.setDataSource(afd.getFileDescriptor(), afd.getStartOffset(), afd.getLength());
            afd.close();
            mp.prepare();
            return mp;
        } catch (IOException ex) {
            Log.d("MediaPlayer", "create failed:", ex);
        } catch (IllegalArgumentException ex) {
            Log.d("MediaPlayer", "create failed:", ex);
        } catch (SecurityException ex) {
            Log.d("MediaPlayer", "create failed:", ex);
        }

        return null;
    }

    public void setDataSource(Context context, Uri uri) throws IOException, IllegalArgumentException, SecurityException, IllegalStateException {
        setDataSource(context, uri, null);
    }

    public void setDataSource(Context context, Uri uri, Map<String, String> headers) throws IOException, IllegalArgumentException, SecurityException, IllegalStateException {
        String scheme = uri.getScheme();
        if ((scheme == null) || (scheme.equals("file"))) {
            setDataSource(uri.getPath());
            return;
        }
        if (scheme.equals("content")) {
            String[] proj = { "_data" };
            Cursor cursor = context.getContentResolver().query(uri, proj, null, null, null);
            if (cursor != null) {
                int column_index = cursor.getColumnIndex("_data");
                if ((column_index != -1) && (cursor.moveToFirst())) {
                    String filepath = cursor.getString(column_index);
                    Log.d("MediaPlayer", "resolve content. " + uri + " => " + filepath);
                    setDataSource(filepath);
                    return;
                }
            }

        }

        AssetFileDescriptor fd = null;
        try {
            ContentResolver resolver = context.getContentResolver();
            fd = resolver.openAssetFileDescriptor(uri, "r");
            if (fd == null) {
                return;
            }
            // Note: using getDeclaredLength so that our behavior is the same
            // as previous versions when the content provider is returning
            // a full file.
            if (fd.getDeclaredLength() < 0) {
                setDataSource(fd.getFileDescriptor());
            } else {
                setDataSource(fd.getFileDescriptor(), fd.getStartOffset(),fd.getDeclaredLength());
            }
            return;
        } catch (SecurityException ex) {
        } catch (IOException ex) {
        } finally {
            if (fd != null) {
                fd.close();
            }
        }
    }

    public void setDataSource(String path) throws IOException, IllegalArgumentException, SecurityException, IllegalStateException {
        setDataSource(path, null, null);
    }

    public void setDataSource(String path, Map<String, String> headers) throws IOException, IllegalArgumentException, SecurityException, IllegalStateException {
        String[] keys = null;
        String[] values = null;

        Log.d("DataSource", "The datasource path is : " + path);

        if (headers != null) {
            keys = new String[headers.size()];
            values = new String[headers.size()];

            int i = 0;
            for (Map.Entry entry : headers.entrySet()) {
                keys[i] = ((String) entry.getKey());
                values[i] = ((String) entry.getValue());
                ++i;
            }
        }
        setDataSource(path, keys, values);
    }

    private void setDataSource(String path, String[] keys, String[] values) throws IOException, IllegalArgumentException, SecurityException, IllegalStateException {
        _setDataSource(path, keys, values);
    }

    public void setDataSource(FileDescriptor fd) throws IOException, IllegalArgumentException, IllegalStateException {
        setDataSource(fd, 0L, 576460752303423487L);
    }

    public void setSurface(Surface surface) {
        _setVideoSurface(surface);
    }

    public void start() throws IllegalStateException {
        stayAwake(true);
        _start();
    }

    public void stop() throws IllegalStateException {
        stayAwake(false);
        _stop();
    }

    public void pause() throws IllegalStateException {
        stayAwake(false);
        _pause();
    }

    public void setWakeMode(Context context, int mode) {
        boolean washeld = false;
        if (this.mWakeLock != null) {
            if (this.mWakeLock.isHeld()) {
                washeld = true;
                this.mWakeLock.release();
            }
            this.mWakeLock = null;
        }

        PowerManager pm = (PowerManager) context.getSystemService(Context.POWER_SERVICE);
        this.mWakeLock = pm.newWakeLock(mode | 0x20000000, NBAVPlayer.class.getName());
        this.mWakeLock.setReferenceCounted(false);
        if (washeld)
            this.mWakeLock.acquire();
    }

    public void setScreenOnWhilePlaying(boolean screenOn) {
        if (this.mScreenOnWhilePlaying != screenOn) {
            if ((screenOn) && (this.mSurfaceHolder == null)) {
                Log.w("MediaPlayer", "setScreenOnWhilePlaying(true) is ineffective without a SurfaceHolder");
            }
            this.mScreenOnWhilePlaying = screenOn;
            updateSurfaceScreenOn();
        }
    }

    private void stayAwake(boolean awake) {
        if (this.mWakeLock != null) {
            if ((awake) && (!this.mWakeLock.isHeld()))
                this.mWakeLock.acquire();
            else if ((!awake) && (this.mWakeLock.isHeld())) {
                this.mWakeLock.release();
            }
        }
        this.mStayAwake = awake;
        updateSurfaceScreenOn();
    }

    private void updateSurfaceScreenOn() {
        if (this.mSurfaceHolder != null)
            this.mSurfaceHolder.setKeepScreenOn((this.mScreenOnWhilePlaying) && (this.mStayAwake));
    }

    public void releaseAll() {
        stayAwake(false);
        updateSurfaceScreenOn();
        if (this.mAudioTrack != null) {
            this.mAudioTrack.release();
            this.mAudioTrack = null;
        }
        this.mOnPreparedListener = null;
        this.mOnBufferingUpdateListener = null;
        this.mOnPlaybackBufferingUpdateListener = null;
        this.mOnCompletionListener = null;
        this.mOnSeekCompleteListener = null;
        this.mOnErrorListener = null;
        this.mOnInfoListener = null;
        this.mOnVideoSizeChangedListener = null;
        _release();
    }

    public void reset() {
        stayAwake(false);
        _reset();

        this.mEventHandler.removeCallbacksAndMessages(null);
    }

    public void setAudioStreamType(int streamtype) {

    }

    protected void finalize() {
        native_finalize();
    }

    /**
     * callback event from event
     * @param mediaplayer_ref
     * @param what
     * @param arg1
     * @param arg2
     * @param obj
     */
    private static void postEventFromNative(Object mediaplayer_ref, int what, int arg1, int arg2, Object obj) {
        NBAVPlayer mp = (NBAVPlayer) ((WeakReference) mediaplayer_ref).get();
        if (mp == null) {
            return;
        }

        if ((what == 200) && (arg1 == 2)) {
            mp.start();
        }
        if (mp.mEventHandler != null) {
            Message m = mp.mEventHandler.obtainMessage(what, arg1, arg2, obj);
            mp.mEventHandler.sendMessage(m);
        }
    }

    public void setOnPreparedListener(OnPreparedListener listener) {
        this.mOnPreparedListener = listener;
    }

    public void setOnCompletionListener(OnCompletionListener listener) {
        this.mOnCompletionListener = listener;
    }

    public void setOnBufferingUpdateListener(OnBufferingUpdateListener listener) {
        this.mOnBufferingUpdateListener = listener;
    }

    public void setOnPlaybackBufferingUpdateListener(OnPlaybackBufferingUpdateListener listener) {
        this.mOnPlaybackBufferingUpdateListener = listener;
    }

    public void setOnSeekCompleteListener(OnSeekCompleteListener listener) {
        this.mOnSeekCompleteListener = listener;
    }

    public void setOnVideoSizeChangedListener(OnVideoSizeChangedListener listener) {
        this.mOnVideoSizeChangedListener = listener;
    }

    public void setOnErrorListener(OnErrorListener listener) {
        this.mOnErrorListener = listener;
    }

    public void setOnInfoListener(OnInfoListener listener) {
        this.mOnInfoListener = listener;
    }

    private boolean isVideoScalingModeSupported(int mode) {
        return (mode == 1) || (mode == 2);
    }

    private class EventHandler extends Handler {
        private NBAVPlayer mMediaPlayer;

        public EventHandler(NBAVPlayer mp, Looper looper) {
            super(looper);
            this.mMediaPlayer = mp;
        }

        public void handleMessage(Message msg) {
            if (this.mMediaPlayer.mNativeContext == 0) {
                Log.w("MediaPlayer", "mediaplayer went away with unhandled events");
                return;
            }

            Log.i("MediaPlayer", "Info (" + msg.what + "," + msg.arg1 + "," + msg.arg2 + ")");

            switch (msg.what) {
                case MEDIA_PREPARED:
                    if (NBAVPlayer.this.mOnPreparedListener != null)
                        NBAVPlayer.this.mOnPreparedListener.onPrepared(this.mMediaPlayer);
                    return;
                case MEDIA_PLAYBACK_COMPLETE:
                    if (NBAVPlayer.this.mOnCompletionListener != null)
                        NBAVPlayer.this.mOnCompletionListener.onCompletion(this.mMediaPlayer);
                    NBAVPlayer.this.stayAwake(false);
                    return;
                case MEDIA_BUFFERING_UPDATE:
                    if (NBAVPlayer.this.mOnBufferingUpdateListener != null)
                        NBAVPlayer.this.mOnBufferingUpdateListener.onBufferingUpdate(this.mMediaPlayer, msg.arg1);
                    return;
                case MEDIA_PLAYBACK_BUFFERING_UPDATE:
                    if (NBAVPlayer.this.mOnPlaybackBufferingUpdateListener != null)
                        NBAVPlayer.this.mOnPlaybackBufferingUpdateListener.onPlaybackBufferingUpdate(this.mMediaPlayer, msg.arg1);
                    return;
                case MEDIA_SEEK_COMPLETE:
                    if (NBAVPlayer.this.mOnSeekCompleteListener != null)
                        NBAVPlayer.this.mOnSeekCompleteListener.onSeekComplete(this.mMediaPlayer);
                    return;
                case MEDIA_SET_VIDEO_SIZE:
                    if (NBAVPlayer.this.mOnVideoSizeChangedListener != null)
                        NBAVPlayer.this.mOnVideoSizeChangedListener.onVideoSizeChanged(this.mMediaPlayer, msg.arg1, msg.arg2);
                    return;
                case MEDIA_SET_AUDIO_PARAMS:
                    if (NBAVPlayer.this.mAudioTrack != null) {
                        NBAVPlayer.this.mAudioTrack.stop();
                        NBAVPlayer.this.mAudioTrack.release();
                        NBAVPlayer.this.mAudioTrack = null;
                    }

                    int sampleRateInHz = msg.arg1;
                    int channelConfig = (msg.arg2 == 2) ? 12 : 4;
                    int bufferSizeInBytes = AudioTrack.getMinBufferSize(sampleRateInHz, channelConfig, 2) * 2;
                    try {
                        NBAVPlayer.this.mAudioTrack = new AudioTrack(3, sampleRateInHz, channelConfig, 2, bufferSizeInBytes, 1);
                    } catch (IllegalArgumentException e) {
                        Log.e("MediaPlayer", "initialize AudioTrack failed. err=" + e.toString());
                    }
                    if (NBAVPlayer.this.mAudioTrack != null) {
                        NBAVPlayer.this.mAudioTrack.play();
                        NBAVPlayer.this._setAudioTrack(NBAVPlayer.this.mAudioTrack);
                    } else {
                        NBAVPlayer.this.mEventHandler.removeCallbacksAndMessages(null);
                        dispatchMediaError(-1010, 0);
                    }
                    return;
                case MEDIA_ERROR:
                    dispatchMediaError(msg.arg1, msg.arg2);
                    return;
                case MEDIA_INFO:
                    if (msg.arg1 != 700) {
                        Log.i("MediaPlayer", "Info (" + msg.arg1 + "," + msg.arg2 + ")");
                    }
                    if (NBAVPlayer.this.mOnInfoListener != null) {
                        NBAVPlayer.this.mOnInfoListener.onInfo(this.mMediaPlayer, msg.arg1, msg.arg2);
                    }

                    return;
                case MEDIA_NOP:
                    break;
                default:
                    Log.e("MediaPlayer", "Unknown message type " + msg.what);
                    return;
            }
        }

        private void dispatchMediaError(int what, int extra) {
            Log.e("MediaPlayer", "Error (" + what + "," + extra + ")");
            boolean error_was_handled = false;
            if (NBAVPlayer.this.mOnErrorListener != null) {
                error_was_handled = NBAVPlayer.this.mOnErrorListener.onError(this.mMediaPlayer, what, extra);
            }
            if ((NBAVPlayer.this.mOnCompletionListener != null) && (!error_was_handled)) {
                NBAVPlayer.this.mOnCompletionListener.onCompletion(this.mMediaPlayer);
            }
            NBAVPlayer.this.stayAwake(false);
        }
    }

    public static abstract interface OnBufferingUpdateListener {
        public abstract void onBufferingUpdate(NBAVPlayer paramMediaPlayer, int paramInt);
    }

    public static abstract interface OnCompletionListener {
        public abstract void onCompletion(NBAVPlayer paramMediaPlayer);
    }

    public static abstract interface OnErrorListener {
        public abstract boolean onError(NBAVPlayer paramMediaPlayer, int what, int extra);
    }

    public interface OnInfoListener {
        boolean onInfo(NBAVPlayer paramMediaPlayer, int what, int extra);
    }

    public interface OnPlaybackBufferingUpdateListener {
        void onPlaybackBufferingUpdate(NBAVPlayer paramMediaPlayer, int paramInt);
    }

    public interface OnPreparedListener {
        void onPrepared(NBAVPlayer paramMediaPlayer);
    }

    public interface OnSeekCompleteListener {
        void onSeekComplete(NBAVPlayer paramMediaPlayer);
    }

    public interface OnTimedTextListener {
        void onTimedText(NBAVPlayer paramMediaPlayer, TimedText paramTimedText);
    }

    public interface OnVideoSizeChangedListener {
        void onVideoSizeChanged(NBAVPlayer paramMediaPlayer, int width, int height);
    }

    /**private native method*/
    private native void _setAudioTrack(AudioTrack paramAudioTrack);

    private native void _setVideoSurface(Surface paramSurface);

    private native void _start() throws IllegalStateException;

    private native void _stop() throws IllegalStateException;

    private native void _pause() throws IllegalStateException;

    private native void _release();

    private native void _reset();

    private static final native void native_init();

    private final native void native_setup(Object paramObject, Context paramContext);

    private final native void native_finalize();

    private native void _setDataSource(String paramString, String[] paramArrayOfString1, String[] paramArrayOfString2) throws IOException, IllegalArgumentException, SecurityException, IllegalStateException;

    /**public native method*/
    public native void setDataSource(FileDescriptor paramFileDescriptor, long paramLong1, long paramLong2) throws IOException, IllegalArgumentException, IllegalStateException;

    public native void prepare() throws IOException, IllegalStateException;

    public native void prepareAsync() throws IllegalStateException;

    public native int getVideoWidth();

    public native int getVideoHeight();

    public native boolean isPlaying();

    public native void setLooping(boolean looping);

    public native void seekTo(int paramInt) throws IllegalStateException;

    public native int getCurrentPosition();

    public native int getDuration();

    public native int getVideoFrameRate();

    public native Map<String, Object> getMetadata(int track);

    public native void setNextMediaPlayer(NBAVPlayer paramMediaPlayer);

    public native void setScreenMetrics(int screenWidth, int screenHeight);
}
