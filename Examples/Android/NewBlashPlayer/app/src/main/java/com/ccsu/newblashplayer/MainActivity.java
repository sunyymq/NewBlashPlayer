package com.ccsu.newblashplayer;

import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

import com.ccsu.nbmediaplayer.NBAVPlayer;

import java.io.IOException;

public class MainActivity extends AppCompatActivity implements SurfaceHolder.Callback, NBAVPlayer.OnPreparedListener {

    private static final String TAG = MainActivity.class.getSimpleName();

    private NBAVPlayer mMediaPlayer = null;
    private SurfaceView mPlayerSuraceView = null;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        mPlayerSuraceView = this.findViewById(R.id.player_surface_view);
        mPlayerSuraceView.getHolder().addCallback(this);
    }

    @Override
    public void surfaceCreated(SurfaceHolder surfaceHolder) {
        mMediaPlayer = new NBAVPlayer(this);
        mMediaPlayer.setOnPreparedListener(this);
        try {
            mMediaPlayer.setDataSource("/mnt/sdcard/Movies/test_movie.mp4");
        } catch (IOException e) {
            e.printStackTrace();
        }
        mMediaPlayer.setSurface(surfaceHolder.getSurface());
        mMediaPlayer.prepareAsync();
    }

    @Override
    public void surfaceChanged(SurfaceHolder surfaceHolder, int i, int i1, int i2) {

    }

    @Override
    public void surfaceDestroyed(SurfaceHolder surfaceHolder) {

    }

    @Override
    public void onPrepared(NBAVPlayer paramMediaPlayer) {
        Log.i(TAG, "media player on prepared : " + mMediaPlayer);
        if (mMediaPlayer != null) {
            mMediaPlayer.start();
        }
    }
}
