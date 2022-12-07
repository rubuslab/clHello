package com.example.ccsharehello;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.View;
import android.widget.TextView;
import android.app.AlertDialog;

import com.example.ccsharehello.databinding.ActivityMainBinding;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;

public class MainActivity extends AppCompatActivity {

    // Used to load the 'ccsharehello' library on application startup.
    static {
        System.loadLibrary("cdx_ocl_yuv2nv12");
    }

    private ActivityMainBinding binding;
    private TextView mTextCost;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        // Example of a call to a native method
        TextView tv = binding.sampleText;
        tv.setText(getDevicesNameFromJNI());

        mTextCost = (TextView) findViewById(R.id.text_cost);
    }

    private boolean isFileExist(String filename) {
        boolean exist = false;
        try {
            File f = new File(filename);
            exist = f.exists();
        } catch (Exception e) {
            e.printStackTrace();
        }
        return exist;
    }

    public void onButtonI420ToNV12Click(View view) {
        int width = 1280;
        int height = 720;

        // read data from file
        String path = getExternalFilesDir("").getAbsolutePath();
        String filename = path + "/" + "sakura_1280x720_i420.YUV";
        if (!isFileExist(filename)) {
            new AlertDialog.Builder(this)
                    .setTitle("File")
                    .setMessage("File not exist.\n" + filename)
                    .show();
            return;
        }

        try {
            FileInputStream in = new FileInputStream(filename);
            int len = in.available();
            byte[] i420_yuv = new byte[len];
            in.read(i420_yuv);
            in.close();

            long start = System.currentTimeMillis();
            boolean ok = ConvertI420ToNV12JNI(width, height, i420_yuv);
            // i420_yuv
            long end = System.currentTimeMillis();
            int cost = (int)(end - start);
            String s = String.format("%d milliseconds", cost);
            mTextCost.setText(s);

            // write to new file
            String out_filename = path + "/" + String.format("cl_out_nv12_%dx%d.YUV", width, height);
            FileOutputStream out = new FileOutputStream(out_filename);
            out.write(i420_yuv);
            out.close();

            i420_yuv = null;
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    private void TestYuvI420ConvertNv12_SmallDebug(int width, int height) {
        // 测试时候可以构造 u 长度为 32(8的倍数) + 7,  width = 64 + 14
        // 测试时候u高度大于1，可以为2,               height = 4
        byte[] img = new byte[(int)(width * height * 1.5)];
        // set y data
        Log.i("Test", String.format("luminance, width: %d, height: %d", width, height));
        Log.i("Test", "------------------------------------------------------------");
        String sz = "";
        int count = 0;
        for (int h = 0; h < height; ++h) {
            sz = "";
            for (int w = 0; w < width; ++w) {
                count = count % 128;
                byte val = (byte)count++;
                img[h * width + w] = val;
                sz = sz + String.format("%3d ", val);
            }
            Log.i("Test", sz);
        }
        Log.i("Test", "uuuuu");
        // set i420 uv data
        int u_width = width / 2;
        int u_height = height / 2;
        int u_start = width * height;
        int v_start = u_start + u_width * u_height;
        byte u_count = 10;
        byte v_count = 40;
        for (int h = 0; h < u_height; ++h) {
            sz = String.format("u-h%d ", h);
            for (int w = 0; w < u_width; ++w) {
                int offset = u_width * h + w;
                img[u_start + offset] = u_count;
                img[v_start + offset] = v_count++;
                sz = sz + String.format("%3d ", u_count++);
            }
            Log.i("Test", sz);
        }
        Log.i("Test", "vvvv");
        // show v data
        for (int h = 0; h < u_height; ++h) {
            sz = String.format("v-h%d ", h);
            for (int w = 0; w < u_width; ++w) {
                int offset = u_width * h + w;
                byte val = img[v_start + offset];
                sz = sz + String.format("%3d ", val);
            }
            Log.i("Test", sz);
        }
        Log.i("Test", "---------");

        // start convert

        long start = System.currentTimeMillis();
        boolean ok = ConvertI420ToNV12JNI(width, height, img);
        long end = System.currentTimeMillis();
        int cost = (int)(end - start);
        String s = String.format("%d milliseconds", cost);
        mTextCost.setText(s);

        Log.i("Test", String.format("Convert result: %s", ok ? "Success" : "Failed"));

        int new_width = height;
        int new_height = width;
        Log.i("Test", "");
        Log.i("Test", String.format("Rotated image, new_width: %d, new_height: %d", new_width, new_height));
        Log.i("Test", "------------------------------");
        for (int h = 0; h < new_height; ++h) {
            sz = "";
            for (int w = 0; w < new_width; ++w) {
                int val = img[h * new_width + w];
                sz = sz + String.format("%3d ", val);
            }
            Log.i("Test", sz);
        }
        Log.i("Test", "uvuvuv");
        int uv_height = u_width;
        for (int h = 0; h < uv_height; ++h) {
            sz = String.format("uv-h%d ", h);
            for (int w = 0; w < new_width; ++w) {
                int val = img[(new_height + h) * new_width + w];
                sz = sz + String.format("%3d ", val);
            }
            Log.i("Test", sz);
        }
        Log.i("Test", "---------");
        img = null;
    }

    private void TestYuvI420ConvertNv12(int width, int height) {
        // 测试时候可以构造 u 长度为 32(8的倍数) + 7,  width = 64 + 14
        // 测试时候u高度大于1，可以为2,               height = 4
        byte[] img = new byte[(int)(width * height * 1.5)];
        // set y data
        Log.i("Test", String.format("luminance, width: %d, height: %d\n", width, height));
        Log.i("Test", "------------------------------------------------------------");
        int count = 0;
        for (int h = 0; h < height; ++h) {
            for (int w = 0; w < width; ++w) {
                count = count % 128;
                byte val = (byte)count++;
                img[h * width + w] = val;
            }
        }
        // set i420 uv data
        int u_width = width / 2;
        int u_height = height / 2;
        int u_start = width * height;
        int v_start = u_start + u_width * u_height;
        for (int h = 0; h <u_height; ++h) {
            for (int w = 0; w < u_width; ++w) {
                int offset = u_width * h + w;
                img[u_start + offset] = 1;
                img[v_start + offset] = 2;
            }
        }

        long start = System.currentTimeMillis();
        boolean ok = ConvertI420ToNV12JNI(width, height, img);
        long end = System.currentTimeMillis();
        int cost = (int)(end - start);
        String s = String.format("%d milliseconds", cost);
        mTextCost.setText(s);

        int new_width = height;
        int new_height = width;
        Log.i("Test", "\n");
        Log.i("Test", String.format("Rotated image, new_width: %d, new_height: %d\n", new_width, new_height));
        Log.i("Test", "------------------------------");
        img = null;
    }

    public void onButtonClickYuvI420ConvertNv12Small(View view) {
        // int img_width = 16;
        int img_width = 16;
        int img_height = 8;
        TestYuvI420ConvertNv12_SmallDebug(img_width, img_height);
    }

    public void onButtonClickYuvI420ConvertNv12InMem(View view) {
        TestYuvI420ConvertNv12(1080, 1920);
    }

    /**
     * A native method that is implemented by the 'ccsharehello' native library,
     * which is packaged with this application.
     */
    public native String getDevicesNameFromJNI();
    public native void SetImageDataJNI(int width, int height, int[] imgData);
    public native boolean ConvertI420ToNV12JNI(int width, int height, byte[] img_data);
}