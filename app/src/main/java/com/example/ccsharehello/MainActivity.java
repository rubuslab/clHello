package com.example.ccsharehello;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.TextView;

import com.example.ccsharehello.databinding.ActivityMainBinding;

public class MainActivity extends AppCompatActivity {

    // Used to load the 'ccsharehello' library on application startup.
    static {
        System.loadLibrary("ccsharehello");
    }

    private ActivityMainBinding binding;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        // Example of a call to a native method
        TextView tv = binding.sampleText;
        tv.setText(stringFromJNI());
    }

    public void onCallAddFunClick(View view) {
        Log.i("TEST", "Hi, call add button clicked.");
        int count = AddJNI(10, 20);
    }

    public void onSetImageDataClick(View view) {
        int width = 60;
        int height = 40;
        int[] img = new int[width * height];
        SetImageDataJNI(width, height, img);
        String sz = img.toString();
        Log.i("TEST", sz);
    }

    public void onButtonI420ToNV12Click(View view) {
        //int width = 32;
        //int height = 24;
        int width = 1080;  // 1080
        int height = 1920;  // 1920
        byte[] img = new byte[(int)(width * height * 1.5)];
        // set i420 uv data
        int uv_width = width / 2;
        int uv_height = height / 2;
        int u_start = width * height;
        int v_start = u_start + (uv_width * uv_height);
        for (int h = 0; h <uv_height; ++h) {
            for (int w = 0; w < uv_width; ++w) {
                int offset = uv_width * h + w;
                img[u_start + offset] = 1;
                img[v_start + offset] = 2;
            }
        }
        boolean ok = ConvertI420ToNV12JNI(width, height, img);
        img = null;
        // String sz = img.toString();
        // Log.i("TEST", sz);
    }

    /**
     * A native method that is implemented by the 'ccsharehello' native library,
     * which is packaged with this application.
     */
    public native String stringFromJNI();
    public native int AddJNI(int a, int b);
    public native void SetImageDataJNI(int width, int height, int[] imgData);
    public native boolean ConvertI420ToNV12JNI(int width, int height, byte[] img_data);
}