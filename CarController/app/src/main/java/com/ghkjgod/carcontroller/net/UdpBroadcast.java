package com.ghkjgod.carcontroller.net;

import android.util.Log;

import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.SocketException;
import java.nio.charset.StandardCharsets;

public class UdpBroadcast {
    private DatagramSocket ds;
    private Broadcast broadcast;
    private Listener listener;
    public static final String TAG = "UdpBroadcast";
    private final int listenPort = 9568;
    private final int remotePort = 9567;

    public interface OnReceiveListener {

        void onReceive(DatagramPacket packet, Broadcast broadcast);

    }

    public UdpBroadcast() {
        try {
            ds = new DatagramSocket(listenPort);
            this.broadcast = new Broadcast();
        } catch (SocketException e) {
            throw new RuntimeException(e);
        }
    }

    public void startBroadcast(byte[] broadcastBody, OnReceiveListener onReceiveListener) {
        listener = new Listener();
        listener.setOnReceiveListener(onReceiveListener);
        listener.start();
        this.broadcast.start();

    }

    public void stopAll() {

        if (listener != null) {
            listener.stopListener();
            listener.interrupt();
        }
        if (broadcast != null) {
            broadcast.stopBroadcast();
            broadcast.interrupt();
        }

    }

    public class Broadcast extends Thread {
        public void sendBroadcast(byte[] data) throws IOException {
            //发送一份请求数据,暴露监听端口
            byte[] requestDataBytes = data;
            //创建一个DatagramPacket
            DatagramPacket requestPacket = new DatagramPacket(requestDataBytes,
                    requestDataBytes.length);
            //指定接收方的ip地址
            requestPacket.setAddress(InetAddress.getByName("255.255.255.255"));
            //指定接收方的端口号
            requestPacket.setPort(remotePort);
            //开始发送广播
            ds.send(requestPacket);

        }

        public void stopBroadcast() {
            this.stop = true;
        }

        private boolean stop = false;

        @Override
        public void run() {
            super.run();
            while (!stop) {
                try {
                    sendBroadcast("scanCar".getBytes(StandardCharsets.UTF_8));
                    Log.d(TAG, "Broadcast …………");
                    Thread.sleep(1000);
                } catch (InterruptedException e) {
                    Log.d(TAG, "Broadcast isInterrupted……");
                    break;
                } catch (IOException e) {
                    throw new RuntimeException(e);
                }
            }
        }

    }

    private class Listener extends Thread {
        private OnReceiveListener mCallback;
        private boolean done = false;

        private void stopListener() {
            done = true;
            if (ds != null) {
                ds.close();
                ds = null;
            }
        }

        @Override
        public void run() {
            super.run();

            while (!done) {
                //构建接收实体
                final byte[] buf = new byte[10000];
                DatagramPacket receivePack = new DatagramPacket(buf, buf.length);
                Log.d(TAG, "Listener run");
                //开始接收
                try {
                    ds.receive(receivePack);
                } catch (IOException e) {
                    Log.d(TAG, "Listener socket is closed");
                    break;
                }
                if (mCallback != null) {
                    mCallback.onReceive(receivePack, broadcast);
                }
                try {
                    Thread.sleep(1);
                } catch (InterruptedException e) {
                    Log.d(TAG, "Listener isInterrupted……");
                    break;
                }
            }
        }

        public void setOnReceiveListener(OnReceiveListener listener) {
            this.mCallback = listener;
        }
    }

}
