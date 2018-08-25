package de.niko.gtrwifi;

import android.os.Handler;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.concurrent.ConcurrentLinkedDeque;
import java.util.concurrent.ConcurrentLinkedQueue;

public class MainActivity extends AppCompatActivity {
    public static final int SERVERPORT = 8266;

    private ServerSocket serverSocket;
    Thread serverThread = null;

    private TextView history;
    private EditText input;
    private Button send;

    public ConcurrentLinkedQueue<String> msgQueue;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        history = findViewById(R.id.history);
        input = findViewById(R.id.input);
        send = findViewById(R.id.send);
        send.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                final String msg = input.getText().toString();

                history.post(new Runnable() {
                    @Override
                    public void run() {
                        history.setText(history.getText().toString() + "\n                            " + msg);
                    }
                });

                msgQueue.add(msg);

                input.setText("");
            }
        });

        msgQueue = new ConcurrentLinkedQueue<>();

        Log.d("main", "Checksum: " + calculateChecksum("Hallo".getBytes()));

        this.serverThread = new Thread(new ServerThread());
        this.serverThread.start();
    }

    @Override
    protected void onStop() {
        super.onStop();
        /*
        try {
            serverSocket.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
        */
    }

    class ServerThread implements Runnable {
        public void run() {
            Socket socket = null;
            try {
                serverSocket = new ServerSocket(SERVERPORT);
                Log.d("main", "ServerSocket: " + serverSocket.isClosed());
            } catch (IOException e) {
                e.printStackTrace();
            }

            while (!Thread.currentThread().isInterrupted()) {
                try {
                    socket = serverSocket.accept();

                    CommunicationThread commThread = new CommunicationThread(socket);
                    new Thread(commThread).start();

                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }
    }

    class CommunicationThread implements Runnable {
        private Socket clientSocket;
        private InputStream input;
        private OutputStream output;

        public CommunicationThread(Socket clientSocket) {
            this.clientSocket = clientSocket;

            try {
                this.input = this.clientSocket.getInputStream();
                this.output = this.clientSocket.getOutputStream();
            } catch (IOException e) {
                e.printStackTrace();
            }
        }

        public void run() {
            while (!Thread.currentThread().isInterrupted()) {
                try {
                    if (input.available() > 0 && input.read() == 221) {
                        String command = readUntil(input);
                        String length = readUntil(input);
                        String checksum = readUntil(input);
                        String appName = readUntil(input);
                        String data = "";

                        int len = Integer.parseInt(length);
                        int r;
                        for (int i = 0; i < len; ) {
                            if ((r = input.read()) != -1) {
                                data += (char) r;
                                i++;
                            }
                        }

                        final String finalData = data;
                        history.post(new Runnable() {
                            @Override
                            public void run() {
                                history.setText(history.getText().toString() + "\n" + finalData);
                            }
                        });

                        Log.d("main", "RECEIVED: " + command + " " + length + " " + checksum + " " + appName + " " + data);
                    }

                    String msg;
                    Log.d("main", "Queue Size: " + msgQueue.size());
                    if ((msg = msgQueue.poll()) != null) {
                        Log.d("main", "Proccessing message: " + msg);
                        sendNetData(output, msg.getBytes());
                    }


                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }
    }

    static String readUntil(InputStream is) {
        StringBuilder sb = new StringBuilder();

        try {
            int r;
            while ((r = is.read()) != -1) {
                if (r == 0)
                    break;

                sb.append((char) r);
            }
        } catch (IOException e) {
            e.printStackTrace();
        }

        return sb.toString();
    }

    static void sendNetData(OutputStream output, byte[] data) {
        try {
            output.write(221);
            output.write("NETDATA".getBytes());
            output.write(0);

            output.write(String.valueOf(data.length).getBytes());
            output.write(0);

            output.write((String.valueOf(calculateChecksum(data))).getBytes());
            output.write(0);

            output.write("@ANDRO".getBytes());
            output.write(0);

            output.write(data);
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    static long calculateChecksum(byte[] data) {
        long checksum = 0;

        for (byte b : data) {
            checksum += (char) (b & 0xFF);

            if (checksum > 10000) {
                checksum = checksum % 10000;
            }
        }

        return checksum;
    }
}
