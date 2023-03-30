package com.ghkjgod.carcontroller;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.View;

import com.ghkjgod.carcontroller.databinding.ActivityMainBinding;
import com.ghkjgod.carcontroller.net.UdpBroadcast;

import org.java_websocket.client.WebSocketClient;
import org.java_websocket.handshake.ServerHandshake;
import org.json.JSONException;
import org.json.JSONObject;

import java.net.DatagramPacket;
import java.net.URI;
import java.nio.charset.StandardCharsets;
import java.util.Timer;
import java.util.TimerTask;

import io.github.controlwear.virtual.joystick.android.JoystickView;

public class MainActivity extends AppCompatActivity {
    private ActivityMainBinding binding;
    private UdpBroadcast udpBroadcast;

    private WebSocketClient webSocketClient;
    public static final String TAG = "MainActivity";
    public static final int OPEN_FLAG = 1;
    public static final int CLOSE_FLAG = 2;
    public static final int ERROR_FLAG = 3;

    private Timer timer ;
    private Handler handler = new Handler(new Handler.Callback() {
        @Override
        public boolean handleMessage(Message msg) {
            switch (msg.what){
                case OPEN_FLAG:
                    binding.statusText.setText("connected car");
                    binding.connectBtn.setText("已找到");
                    break;
                case CLOSE_FLAG:
                    binding.statusText.setText("disconnect car");
                    binding.connectBtn.setText("点击重连");
                    if (udpBroadcast != null) {
                        udpBroadcast.stopAll();
                        udpBroadcast = null;
                        binding.connectBtn.setText("已断开");
                    }
                    break;
                case ERROR_FLAG:
                    binding.statusText.setText("connect error");
                    binding.connectBtn.setText("点击重连");
                    if (udpBroadcast != null) {
                        udpBroadcast.stopAll();
                        udpBroadcast = null;
                        binding.connectBtn.setText("已断开");
                    }
                    break;
                default:
                    Log.d(TAG, "no find what!");
            }
            return true;
        }
    });
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());
        initView();
    }

    private void setTimerTask() {
        timer.schedule(new TimerTask() {
            @Override
            public void run() {
                if (webSocketClient != null && webSocketClient.isOpen()) {
                    byte[] heart = new byte[2];
                    heart[0] = (byte) 0xAA;
                    heart[1] = (byte) 0xAA;
                    webSocketClient.send(heart);
                    Log.d(TAG, "heart……");

                }
            }
        }, 1000, 1000/* 表示1000毫秒之後，每隔1000毫秒執行一次 */);
    }
    public void handlerMove(int angle, int strength) {
        int pwmMax = 255;
        int pwmLeft = 0;
        int pwmRight = 0;
        JSONObject jsonObject = new JSONObject();
        try {
            if (angle >= 0 && angle < 90) {
                //   右前 左轮功率大
                double a = Math.toRadians(angle);
                pwmLeft = (pwmMax * strength) / 100;
                pwmRight = (int) (Math.sin(a) * pwmLeft);
                jsonObject.put("LF", pwmLeft);
                jsonObject.put("LB", pwmLeft);
                jsonObject.put("RF", pwmRight);
                jsonObject.put("RB", pwmRight);
            }else if (angle >= 90 && angle < 180) {
                //   左前 右轮功率大
                int realAngle = 180 - angle;
                double a = Math.toRadians(realAngle);
                pwmRight = (pwmMax * strength) / 100;
                pwmLeft = (int) (Math.sin(a) * pwmRight);
                jsonObject.put("LF", pwmLeft);
                jsonObject.put("LB", pwmLeft);
                jsonObject.put("RF", pwmRight);
                jsonObject.put("RB", pwmRight);
            }else if (angle >= 180 && angle < 270) {
                //   左后推 右轮功率大
                int realAngle = angle - 180;
                double a = Math.toRadians(realAngle);
                pwmRight = (pwmMax * strength) / 100;
                pwmLeft = (int) (Math.sin(a) * pwmRight);
                jsonObject.put("LF", 0 - pwmLeft);
                jsonObject.put("LB", 0 - pwmLeft);
                jsonObject.put("RF", 0 - pwmRight);
                jsonObject.put("RB", 0 - pwmRight);
            }else if (angle >= 270 && angle < 360) {
                //   右后推 左轮功率大
                int realAngle = 360 - angle ;
                double a = Math.toRadians(realAngle);
                pwmLeft = (pwmMax * strength) / 100;
                pwmRight = (int) (Math.sin(a) * pwmLeft);
                jsonObject.put("LF", 0 - pwmLeft);
                jsonObject.put("LB", 0 - pwmLeft);
                jsonObject.put("RF", 0 - pwmRight);
                jsonObject.put("RB", 0 - pwmRight);
            }

        } catch (JSONException e) {
            throw new RuntimeException(e);
        }
        if (webSocketClient != null && webSocketClient.isOpen()) {
            Log.d(TAG,jsonObject.toString());
            webSocketClient.send(jsonObject.toString());
        }

    }
    public void handlerTank(int angle, int strength) {
        int pwmMax = 255;
        int pwm = 0;
        Log.d(TAG, "tank angle:"+ angle);
        Log.d(TAG, "tank strength:"+ strength);
        JSONObject jsonObject = new JSONObject();
        try {
            if(angle == 0){
                pwm = (pwmMax * strength) / 100;
                jsonObject.put("LF", pwm);
                jsonObject.put("LB", pwm);
                jsonObject.put("RF", 0-pwm);
                jsonObject.put("RB", 0-pwm);
            }
            if(angle == 180){
                pwm = (pwmMax * strength) / 100;
                jsonObject.put("LF", 0-pwm);
                jsonObject.put("LB", 0-pwm);
                jsonObject.put("RF", pwm);
                jsonObject.put("RB", pwm);
            }
        } catch (JSONException e) {
            throw new RuntimeException(e);
        }
        if (webSocketClient != null && webSocketClient.isOpen()) {
            Log.d(TAG,jsonObject.toString());
            webSocketClient.send(jsonObject.toString());
        }



    }
    private void initView(){

        binding.joystickView.setOnMoveListener(new JoystickView.OnMoveListener() {
            @Override
            public void onMove(int angle, int strength) {
                // do whatever you want
                binding.angle.setText(angle + "°");
                binding.strength.setText(strength + "%");
                handlerMove(angle, strength);

            }
        }, 17);

        binding.tankJoystick.setButtonDirection(-1);
        binding.tankJoystick.setOnMoveListener(new JoystickView.OnMoveListener() {
            @Override
            public void onMove(int angle, int strength) {
                handlerTank(angle, strength);
            }
        });

        binding.connectBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (udpBroadcast != null) {
                    udpBroadcast.stopAll();
                    udpBroadcast = null;
                    binding.connectBtn.setText("已终止");
                } else {
                    binding.connectBtn.setText("寻找中");
                    udpBroadcast = new UdpBroadcast();
                    udpBroadcast.startBroadcast("scanCar".getBytes(StandardCharsets.UTF_8), new UdpBroadcast.OnReceiveListener() {
                        @Override
                        public void onReceive(DatagramPacket packet, UdpBroadcast.Broadcast broadcast) {
                            String ip = packet.getAddress().getHostAddress();//发送者的ip
                            int port = packet.getPort();//发送者的端口
                            int dataLen = packet.getLength();//数据包长度
                            String data = new String(packet.getData(), 0, dataLen);//数据内容

                            udpBroadcast.stopAll();

                            URI serverURI = URI.create("ws://" + data + ":8088");
                            Log.d(TAG, serverURI.toString());

                            if (webSocketClient != null) {
                                webSocketClient.close();
                                webSocketClient = null;
                            }
                            webSocketClient = new WebSocketClient(serverURI) {
                                @Override
                                public void onOpen(ServerHandshake handshakedata) {

                                    Log.d(TAG, "connected car");
                                    Message msg = new Message();
                                    msg.what = OPEN_FLAG;
                                    handler.sendMessage(msg);
                                    if(timer !=null){
                                        timer.cancel();
                                        timer = null;
                                    }
                                    timer = new Timer();
                                    setTimerTask();
                                }

                                @Override
                                public void onMessage(String message) {
                                    Log.d(TAG, "message:" + message);


                                }
                                @Override
                                public void onClose(int code, String reason, boolean remote) {
                                    Log.d(TAG, "onClose");
                                    Message msg = new Message();
                                    msg.what = CLOSE_FLAG;
                                    handler.sendMessage(msg);
                                    if(timer !=null){
                                        timer.cancel();
                                        timer = null;
                                    }

                                }

                                @Override
                                public void onError(Exception ex) {
                                    Log.d(TAG, "onError:" + ex.getMessage());
                                    Message msg = new Message();
                                    msg.what = ERROR_FLAG;
                                    handler.sendMessage(msg);
                                    webSocketClient.close();
                                    if(timer !=null){
                                        timer.cancel();
                                        timer = null;
                                    }
                                }
                            };
                            webSocketClient.setConnectionLostTimeout(3);
                            webSocketClient.connect();

                        }
                    });
                }


            }
        });



    }
    @Override
    public void onDestroy() {
        super.onDestroy();
        binding = null;
    }


}