package com.lake.ndktest;

import android.content.Context;
import android.content.Intent;
import android.content.res.Resources;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.SeekBar;
import android.widget.TextView;

public class VideoActivity extends AppCompatActivity implements View.OnClickListener {
    private SurfaceView surfaceView;
    private SurfaceHolder surfaceHolder;
    private FrameLayout mTitle, mVideoController, mAudioController;
    private ImageView btnBack;
    private ImageView btnSetting;
    private TextView tvTime, tvToatal;
    private TextView tvFilename;
    private ImageView btnPause;
    private SeekBar seekProgress;
    private boolean isShowController = false;
    private boolean isShowAudioController = false;
    private boolean pause = false;
    private int mProgress = 0;
    private String name;
    private LinearLayout audioList;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        requestWindowFeature(Window.FEATURE_NO_TITLE);// 隐藏标题
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN);// 设置全屏
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);//常亮
        setContentView(R.layout.activity_main);
        initView();
    }

    private void initView() {
        Intent intent = getIntent();
        final String path = intent.getStringExtra("path");
        name = intent.getStringExtra("name");

        mTitle = (FrameLayout) findViewById(R.id.video_title);
        mVideoController = (FrameLayout) findViewById(R.id.video_controller);
        mAudioController = (FrameLayout) findViewById(R.id.audioIndex_controller);
        btnBack = (ImageView) findViewById(R.id.btn_back);
        btnSetting = (ImageView) findViewById(R.id.btn_videoSetting);
        tvTime = (TextView) findViewById(R.id.tv_timenow);
        tvToatal = (TextView) findViewById(R.id.tv_timetotal);
        btnPause = (ImageView) findViewById(R.id.btn_pause);
        seekProgress = (SeekBar) findViewById(R.id.seek_video);
        tvFilename = (TextView) findViewById(R.id.tv_filename);
        surfaceView = (SurfaceView) findViewById(R.id.surfaceView);
        audioList = (LinearLayout) findViewById(R.id.audioList);
        btnBack.setOnClickListener(this);
        btnPause.setOnClickListener(this);
        surfaceView.setOnClickListener(this);
        btnSetting.setOnClickListener(this);
        tvFilename.setText(name);

        seekProgress.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                //Log.e("lake", "onProgressChanged: " + progress);
                mProgress = progress;
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {
                //pause(true);
            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
                seek(mProgress);
            }
        });

        surfaceHolder = surfaceView.getHolder();
        surfaceHolder.addCallback(new SurfaceHolder.Callback() {
            @Override
            public void surfaceCreated(final SurfaceHolder holder) {
                new Thread(new Runnable() {
                    @Override
                    public void run() {
                        play(path, holder.getSurface());
                    }
                }).start();
            }

            @Override
            public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {

            }

            @Override
            public void surfaceDestroyed(SurfaceHolder holder) {

            }
        });

    }

    @Override
    public void onClick(View v) {
        switch (v.getId()) {
            case R.id.btn_back:
                stop();
                VideoActivity.this.finish();
                break;
            case R.id.btn_videoSetting:
                mTitle.setVisibility(View.GONE);
                mVideoController.setVisibility(View.GONE);
                isShowController = false;
                mAudioController.setVisibility(View.VISIBLE);
                isShowAudioController = true;
                break;
            case R.id.btn_pause:
                pause(!pause);
                pause = !pause;
                btnPause.setImageResource(pause ? R.mipmap.play : R.mipmap.pause);
                break;
            case R.id.surfaceView:
                if (isShowAudioController) {
                    mAudioController.setVisibility(View.GONE);
                    isShowAudioController = false;
                    break;
                }
                mTitle.setVisibility(isShowController ? View.GONE : View.VISIBLE);
                mVideoController.setVisibility(isShowController ? View.GONE : View.VISIBLE);
                isShowController = !isShowController;
                break;
            default:
                break;
        }
    }

    public native int play(String inputurl, Object surface);

    public native int pause(boolean pause);

    public native int seek(int progress);

    public native int stop();

    public native int switchAudioChannel(int i);

    @Override
    protected void onPause() {
        super.onPause();
        if (!pause) {
            pause(true);
            pause = true;
            btnPause.setImageResource(R.mipmap.play);
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        Log.e("lake", "onDestroy: stop");
        stop();
    }

    /**
     * 时间进度信息回调
     *
     * @param currt_secd
     */
    public void setProgressInfo(final int currt_secd) {
        final String t = resetTimeInt(currt_secd / 3600) + ":" + resetTimeInt(currt_secd % 3600 / 60) + ":" + resetTimeInt(currt_secd % 60);
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                tvTime.setText(t);
                seekProgress.setProgress(currt_secd);

            }
        });
    }

    /**
     * 总时间回调
     *
     * @param total_secd
     */
    public void setTotalTime(int total_secd) {
        final String tt = resetTimeInt(total_secd / 3600) + ":" + resetTimeInt(total_secd % 3600 / 60) + ":" + resetTimeInt(total_secd % 60);
        seekProgress.setMax(total_secd);
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                tvToatal.setText(tt);
            }
        });
    }

    /**
     * 初始化音轨
     *
     * @param channel
     * @param language
     */
    public void initAudioChannel(final int[] channel, final String[] language, final int curIndex) {
        final TextView[] tvList = new TextView[channel.length];
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                for (int i = 0; i < channel.length; i++) {
                    final TextView tv = new TextView(VideoActivity.this);
                    final int index = channel[i];//通道号
                    if (curIndex == index) {
                        tv.setBackgroundColor(getResources().getColor(R.color.colorhalf));
                    }
                    tv.setText("音轨" + (i + 1) + getLanuage(language[i]));
                    tv.setTextSize(16);
                    tv.setTextColor(getResources().getColor(R.color.white));
                    LinearLayout.LayoutParams layoutParams = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, dip2px(VideoActivity.this, 40));
                    tv.setGravity(Gravity.CENTER_VERTICAL);
                    tv.setLayoutParams(layoutParams);
                    tv.setPadding(dip2px(VideoActivity.this, 20), 0, 0, 0);
                    audioList.addView(tv);
                    tvList[i] = tv;
                    tv.setOnClickListener(new View.OnClickListener() {
                        @Override
                        public void onClick(View v) {
                            for(int i = 0; i < channel.length; i++){
                                tvList[i].setBackgroundColor(getResources().getColor(R.color.colorWhite));
                            }
                            tv.setBackgroundColor(getResources().getColor(R.color.colorhalf));
                            switchAudioChannel(index);
                        }
                    });
                }
            }
        });
    }

    public String resetTimeInt(int time) {
        return time < 10 ? "0" + time : time + "";
    }

    static {
        System.loadLibrary("HbPlayer");
    }

    /**
     * dp转为px
     *
     * @param context  上下文
     * @param dipValue dp值
     * @return
     */
    private int dip2px(Context context, float dipValue) {
        Resources r = context.getResources();
        return (int) TypedValue.applyDimension(
                TypedValue.COMPLEX_UNIT_DIP, dipValue, r.getDisplayMetrics());
    }

    public String getLanuage(String l) {
        switch (l) {
            case "eng":
                return "(英文)";
            case "chi":
                return "(中文)";
            default:
                return "(未知)";
        }
    }

}
