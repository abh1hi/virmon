package com.virmon.android;

import android.app.Activity;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.hardware.usb.UsbAccessory;
import android.hardware.usb.UsbManager;
import android.os.Build;
import android.os.Bundle;

import android.os.Build;
import android.os.ParcelFileDescriptor;
import android.util.Log;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.WindowManager;
import android.widget.Toast;

import java.io.DataInputStream;
import java.io.FileDescriptor;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

public class MainActivity extends Activity
        implements SurfaceHolder.Callback, View.OnTouchListener, NetworkClient.DataListener {
    private static final String TAG = "VirMon";

    // USB Components (Disabled/Deprecated for WiFi)
    // ...

    private SurfaceView mSurfaceView;
    private VideoDecoder mDecoder;
    private NetworkClient mNetworkClient;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        mSurfaceView = new SurfaceView(this);
        setContentView(mSurfaceView);
        mSurfaceView.getHolder().addCallback(this);
        mSurfaceView.setOnTouchListener(this);

        mDecoder = new VideoDecoder();

        // Initialize Network Client
        mNetworkClient = new NetworkClient(this);
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (mNetworkClient != null) {
            mNetworkClient.start();
            Toast.makeText(this, "Searching for VirMon PC...", Toast.LENGTH_SHORT).show();
        }
    }

    @Override
    protected void onPause() {
        super.onPause();
        if (mNetworkClient != null) {
            mNetworkClient.stop();
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (mDecoder != null)
            mDecoder.stop();
    }

    // NetworkClient Callback
    @Override
    public void onVideoFrame(byte[] data, int length, long timestamp) {
        if (mDecoder != null) {
            mDecoder.queueFrame(data, length, timestamp);
        }
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        try {
            mDecoder.configure(holder.getSurface(), 1920, 1080); // Fixed resolution for MVP
        } catch (IOException e) {
            Log.e(TAG, "Decoder config failed", e);
        }
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        mDecoder.stop();
    }

    @Override
    public boolean onTouch(View v, MotionEvent event) {
        if (mNetworkClient == null)
            return true;

        // Simple Touch Protocol: [Type:1][Action:1][ID:1][X:4][Y:4][Pressure:4]
        try {
            ByteBuffer bb = ByteBuffer.allocate(15).order(ByteOrder.LITTLE_ENDIAN);
            bb.put((byte) 0x02); // Type = Input
            bb.put((byte) event.getActionMasked());
            bb.put((byte) event.getPointerId(event.getActionIndex()));
            bb.putFloat(event.getX() / v.getWidth()); // Normalized X
            bb.putFloat(event.getY() / v.getHeight()); // Normalized Y
            bb.putFloat(event.getPressure());

            mNetworkClient.sendInputEvent(bb.array());
        } catch (Exception e) {
            Log.e(TAG, "Write input failed", e);
        }
        return true;
    }
}
