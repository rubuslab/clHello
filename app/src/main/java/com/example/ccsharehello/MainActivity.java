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
        int width = 32;
        int height = 32;
        byte[] img = new byte[width * height];
        boolean ok = ConvertI420ToNV12JNI(width, height, img);
        String sz = img.toString();
        Log.i("TEST", sz);
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