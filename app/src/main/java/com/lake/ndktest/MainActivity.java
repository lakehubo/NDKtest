package com.lake.ndktest;

import android.os.Environment;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;

public class MainActivity extends AppCompatActivity {
    private  SurfaceView surfaceView;
    private EditText urlEdittext_input;
    private SurfaceHolder surfaceHolder;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        Button startButton = (Button) this.findViewById(R.id.button_start);
        urlEdittext_input= (EditText) this.findViewById(R.id.input_url);
        surfaceView = (SurfaceView)findViewById(R.id.surfaceView);
        surfaceHolder = surfaceView.getHolder();
        startButton.setOnClickListener(new View.OnClickListener() {
            public void onClick(View arg0){
                String folderurl= Environment.getExternalStorageDirectory().getPath();
                String urltext_input=urlEdittext_input.getText().toString();
                final String inputurl=folderurl+"/MyLocalPlayer/"+urltext_input;
                new Thread(new Runnable() {
                    @Override
                    public void run() {
                        FFmpeg.play(inputurl,surfaceHolder.getSurface());
                    }
                }).start();
            }
        });

    }
}
