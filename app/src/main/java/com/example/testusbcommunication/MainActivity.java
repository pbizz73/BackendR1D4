/**
 * Done by Aleksandar Damnjanovic aka Kind Spirit from Kind Spirit Technology YouTube Channel
 * February 25. 2024.
 */

package com.example.testusbcommunication;
import android.annotation.SuppressLint;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;

import androidx.appcompat.app.AppCompatActivity;

import com.hoho.android.usbserial.driver.UsbSerialDriver;
import com.hoho.android.usbserial.driver.UsbSerialPort;
import com.hoho.android.usbserial.driver.UsbSerialProber;

import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.nio.charset.StandardCharsets;
import java.util.List;
import java.util.Map;

/**
 This is base example of this project. It holds code for basic communication between esp microcontroller and
 android app.
 */
public class MainActivity extends AppCompatActivity {

    private TextView textView;
    private EditText sendText;
    private Button sendButton;
    UsbDeviceConnection connection;
    List<UsbSerialDriver> availableDrivers;
    UsbSerialDriver driver;
    UsbSerialPort port;
    UsbManager manager;
    Map<String, UsbDevice> devices;
    UsbDevice device = null;
    PendingIntent pi;
    private boolean running= false;
    private final Object porting =new Object();
    private static String ACTION_USB_PERMISSON = "com.android.example.USB_PERMISSION";
    private final BroadcastReceiver receiver = new BroadcastReceiver(){
        @Override
        public void onReceive(Context context, Intent intent){
            String action = intent.getAction();
            if(ACTION_USB_PERMISSON.equals(action))
                synchronized (this){
                if(manager.hasPermission(device))
                    connect();
                }
        }
    };
    private void connect(){
        availableDrivers = UsbSerialProber.getDefaultProber().findAllDrivers(manager);
        driver = availableDrivers.get(0);
        connection = manager.openDevice(driver.getDevice());
        port = driver.getPorts().get(0);
        try {
            port.open(connection);
            port.setParameters(115200,8,UsbSerialPort.STOPBITS_1,UsbSerialPort.PARITY_NONE);
        } catch (IOException e) {
            throw new RuntimeException(e);
        }

        new Thread(new Runnable() {
            @Override
            public void run() {
                running = true;
                while(running){
                    synchronized (porting){
                        if(!port.isOpen())
                            break;
                    }
                    byte[] data = new byte [500];
                    synchronized (porting){
                        try {
                            byte[] buffer = new byte[1024]; // Larger buffer size
                            StringBuilder incomingMessage = new StringBuilder();
                            //port.read(data,2000);
                            int numBytesRead;
                            while ((numBytesRead = port.read(buffer, 2000)) > 0) {
                                String receivedData = new String(buffer, 0, numBytesRead, StandardCharsets.UTF_8);
                                incomingMessage.append(receivedData);
                                // Check if message is complete (e.g., terminated by a newline)
                                if (receivedData.contains("\n")) { // Assume ESP sends "\n" as a terminator
                                    break;
                                }
                            }
                            runOnUiThread(new Runnable() {
                                @Override
                                public void run() {
                                    textView.setText("");
                                    textView.setText(incomingMessage.toString().trim());
                                }
                            });
                            Thread.sleep(200);
                            synchronized (porting){
                                running = port.isOpen();
                            }
                        } catch (IOException | InterruptedException e) {
                            throw new RuntimeException(e);
                        }
                    }
                }
            }
        }).start();
    }






    @Override
    protected void onCreate(Bundle savedInstanceState){
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        textView = findViewById(R.id.textView);
        sendButton = findViewById(R.id.sendButton);
        sendText = findViewById(R.id.sendText);

        manager = (UsbManager) getApplicationContext().getSystemService(Context.USB_SERVICE);
        devices = manager.getDeviceList();
        device = null;
        sendButton.setOnClickListener(new View.OnClickListener(){
            @Override
            public void onClick(View v){
                try {
                    synchronized (porting){
                        port.write(sendText.getText().toString().getBytes(StandardCharsets.UTF_8),2000);
                        sendText.setText("");
                    }
                } catch (IOException e) {
                    throw new RuntimeException(e);
                }
            }
        });

    }


    @SuppressLint("UnspecifiedRegisterReceiverFlag")
    @Override
    protected void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
        if(intent.getAction().equals(UsbManager.ACTION_USB_DEVICE_ATTACHED)){

            IntentFilter filter = new IntentFilter(ACTION_USB_PERMISSON);
            registerReceiver(receiver,filter);

            for(Map.Entry<String, UsbDevice>entry:devices.entrySet())
                device = entry.getValue();
            pi = PendingIntent.getBroadcast(getApplicationContext(),0,
                    new Intent(ACTION_USB_PERMISSON),PendingIntent.FLAG_IMMUTABLE);
            manager.requestPermission(device,pi);
        }else if(intent.getAction().equals(UsbManager.ACTION_USB_DEVICE_DETACHED)){
            synchronized (porting){
                try {
                    port.close();
                    connection.close();
                } catch (IOException e) {
                    throw new RuntimeException(e);
                }
            }
        }


    }
}

