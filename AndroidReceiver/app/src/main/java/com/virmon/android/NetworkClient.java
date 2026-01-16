package com.virmon.android;

import android.util.Log;

import java.io.DataInputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.Socket;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

public class NetworkClient {
    private static final String TAG = "VirMonNet";
    private static final int DISCOVERY_PORT = 55555;
    private static final int CONTROL_PORT = 5051;
    private static final int VIDEO_PORT = 5052;

    private boolean mRunning = false;
    private DataListener mListener;
    private Socket mTcpSocket;
    private OutputStream mTcpOutputStream;
    private DatagramSocket mUdpSocket;

    public interface DataListener {
        void onVideoFrame(byte[] data, int length, long timestamp);
    }

    public NetworkClient(DataListener listener) {
        mListener = listener;
    }

    public void start() {
        mRunning = true;
        new Thread(this::discoveryLoop).start();
    }

    public void stop() {
        mRunning = false;
        try {
            if (mTcpSocket != null)
                mTcpSocket.close();
            if (mUdpSocket != null)
                mUdpSocket.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    private void discoveryLoop() {
        Log.i(TAG, "Starting Discovery...");
        DatagramSocket socket = null;
        try {
            socket = new DatagramSocket(DISCOVERY_PORT);
            socket.setBroadcast(true);

            byte[] buf = new byte[1024];
            DatagramPacket packet = new DatagramPacket(buf, buf.length);

            while (mRunning) {
                socket.receive(packet);
                String msg = new String(packet.getData(), 0, packet.getLength());
                if (msg.contains("VIRMON_SERVER_BEACON")) {
                    Log.i(TAG, "Server Found: " + packet.getAddress().getHostAddress());
                    connect(packet.getAddress().getHostAddress());
                    break; // Discovery done, connection attempted
                }
            }
        } catch (IOException e) {
            Log.e(TAG, "Discovery Failed", e);
        } finally {
            if (socket != null)
                socket.close();
        }
    }

    private void connect(String ip) {
        try {
            // 1. TCP Connection
            mTcpSocket = new Socket(ip, CONTROL_PORT);
            mTcpOutputStream = mTcpSocket.getOutputStream();
            Log.i(TAG, "Connected to TCP Server");

            // 2. Start UDP Video Listener
            new Thread(this::videoLoop).start();

            // 3. Keep TCP Alive / Read Loop (for future server->client control events)
            DataInputStream dis = new DataInputStream(mTcpSocket.getInputStream());
            while (mRunning && mTcpSocket.isConnected()) {
                if (dis.read() == -1)
                    break;
            }
            Log.i(TAG, "TCP Disconnected");

        } catch (IOException e) {
            Log.e(TAG, "Connection Failed", e);
            // Retry discovery?
            if (mRunning) {
                try {
                    Thread.sleep(2000);
                } catch (InterruptedException ex) {
                }
                new Thread(this::discoveryLoop).start();
            }
        }
    }

    private void videoLoop() {
        try {
            // Bind to UDP port 5052 to receive video
            mUdpSocket = new DatagramSocket(VIDEO_PORT);
            Log.i(TAG, "Listening for Video on UDP " + VIDEO_PORT);

            byte[] buf = new byte[65535]; // Max UDP size
            DatagramPacket packet = new DatagramPacket(buf, buf.length);

            while (mRunning && !mUdpSocket.isClosed()) {
                mUdpSocket.receive(packet);
                processVideoPacket(packet);
            }
        } catch (IOException e) {
            Log.e(TAG, "Video Loop Error", e);
        }
    }

    private void processVideoPacket(DatagramPacket packet) {
        // Packet: [Seq:4][TS:8][Flags:1][Payload]
        if (packet.getLength() < 13)
            return;

        ByteBuffer bb = ByteBuffer.wrap(packet.getData(), 0, packet.getLength());
        bb.order(ByteOrder.LITTLE_ENDIAN); // Assuming Sender is LE (x86 default)

        // Skip Seq (4) for now or use for ordering
        int seq = bb.getInt(); // Read 4 bytes

        // Read Timestamp (8)
        // Note: Sender sends Big Endian for TS?
        // Code in NetworkTransport.cpp used `_byteswap_uint64(timestamp)` ?
        // Wait, line 88 in NetworkTransport: `*ts = _byteswap_uint64(timestamp);` ->
        // Network Byte Order (BE).
        // Since we set `order(ByteOrder.LITTLE_ENDIAN)` above, getting Long will
        // interpret LE.
        // We should fix one.
        // If sender sends BE, we should read BE.
        // Actually Packet Header usually is BE (Network Order).
        // But the rest of payload (like lengths inside NALs?) might be system
        // dependent.
        // Let's assume Network Byte Order for header.

        // Re-wrap for header with BE
        ByteBuffer headerBB = ByteBuffer.wrap(packet.getData(), 0, 13);
        headerBB.order(ByteOrder.BIG_ENDIAN);

        int seqBE = headerBB.getInt();
        long timestamp = headerBB.getLong();
        byte flags = headerBB.get();
        // Offset 13

        int payloadLen = packet.getLength() - 13;
        byte[] payload = new byte[payloadLen];
        System.arraycopy(packet.getData(), 13, payload, 0, payloadLen);

        if (mListener != null) {
            mListener.onVideoFrame(payload, payloadLen, timestamp);
        }
    }

    // Send Touch Event
    public void sendInputEvent(byte[] eventData) {
        if (mTcpOutputStream == null)
            return;
        try {
            // Just send raw bytes for MVP
            mTcpOutputStream.write(eventData);
        } catch (IOException e) {
            e.printStackTrace();
        }
    }
}
