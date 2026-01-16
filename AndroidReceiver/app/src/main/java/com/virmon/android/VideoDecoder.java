package com.virmon.android;

import android.media.MediaCodec;
import android.media.MediaFormat;
import android.os.Build;
import android.util.Log;
import android.view.Surface;

import java.io.IOException;
import java.nio.ByteBuffer;

public class VideoDecoder {
    private static final String TAG = "VideoDecoder";
    private static final String MIME_TYPE = "video/x-motion-jpeg"; // Motion JPEG (matches Windows JPEG encoder)

    private MediaCodec mCodec;
    private WorkerThread mWorker;
    private boolean mRunning = false;

    public void configure(Surface surface, int width, int height) throws IOException {
        mCodec = MediaCodec.createDecoderByType(MIME_TYPE);
        MediaFormat format = MediaFormat.createVideoFormat(MIME_TYPE, width, height);

        // Low latency settings
        if (Build.VERSION.SDK_INT >= 30) {
            format.setInteger(MediaFormat.KEY_LOW_LATENCY, 1);
        }
        // Set priority and operating rate for smoothness
        format.setInteger(MediaFormat.KEY_PRIORITY, 0); // Realtime priority
        format.setInteger(MediaFormat.KEY_OPERATING_RATE, Short.MAX_VALUE); // Max FPS hint

        // Configure codec with the Output Surface
        mCodec.configure(format, surface, null, 0);
        mCodec.start();

        mRunning = true;
        mWorker = new WorkerThread();
        mWorker.start();
    }

    public void stop() {
        mRunning = false;
        if (mWorker != null) {
            try {
                mWorker.join();
            } catch (InterruptedException e) {
            }
            mWorker = null;
        }
        if (mCodec != null) {
            mCodec.stop();
            mCodec.release();
            mCodec = null;
        }
    }

    // Pass data from USB to decoder
    // This example assumes 'data' contains one valid NAL unit or frame
    public void queueFrame(byte[] data, int length, long timestampUs) {
        if (!mRunning || mCodec == null)
            return;

        try {
            int inputBufferIndex = mCodec.dequeueInputBuffer(10000); // Wait 10ms
            if (inputBufferIndex >= 0) {
                ByteBuffer inputBuffer = mCodec.getInputBuffer(inputBufferIndex);
                inputBuffer.clear();
                inputBuffer.put(data, 0, length);
                mCodec.queueInputBuffer(inputBufferIndex, 0, length, timestampUs, 0);
            } else {
                Log.w(TAG, "No input buffer available, dropping frame");
            }
        } catch (Exception e) {
            Log.e(TAG, "Error queuing frame", e);
        }
    }

    private class WorkerThread extends Thread {
        @Override
        public void run() {
            MediaCodec.BufferInfo bufferInfo = new MediaCodec.BufferInfo();
            while (mRunning && mCodec != null) {
                try {
                    int outputBufferIndex = mCodec.dequeueOutputBuffer(bufferInfo, 10000); // 10ms
                    if (outputBufferIndex >= 0) {
                        // releaseOutputBuffer(index, true) renders it to the Surface
                        mCodec.releaseOutputBuffer(outputBufferIndex, true);
                    } else if (outputBufferIndex == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED) {
                        Log.d(TAG, "Output format changed");
                    } else if (outputBufferIndex == MediaCodec.INFO_TRY_AGAIN_LATER) {
                        // No output yet
                    }
                } catch (Exception e) {
                    Log.e(TAG, "Error processing output", e);
                }
            }
        }
    }
}
