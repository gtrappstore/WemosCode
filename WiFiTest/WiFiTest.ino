#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>

#define LED D4

SoftwareSerial softSer(3, 1);
WiFiClient netClient;

void setup() {
  // put your setup code here, to run once:
  pinMode(LED, OUTPUT);
  Serial.begin(9600, SERIAL_8N1);
  Serial.swap();
  softSer.begin(9600);
  pinMode(3, INPUT);
  pinMode(1, OUTPUT);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  randomSeed(analogRead(0));

  softSer.println("Started");

/*
  softSer.println("CHECKSUM: " + String(checksum((unsigned char*) "Test String\0", 13)));
  sendData((unsigned char*) "Test String\0", 13);
  */
}

void loop() {
  // put your main code here, to run repeatedly:

  if (netClient.status() == 0 && WiFi.status() == WL_CONNECTED) {
    netClient.connect(WiFi.gatewayIP(), 8266);
  }

  if (Serial.available() > 0) {
    byte data = Serial.read();
    softSer.write(data);

    if (data == 219) {
      String command = Serial.readStringUntil(0);

      softSer.println("Hey, 219");

      if (command.length() == 0) {
        sendCommandAck("TO");
      } else {
        softSer.print("Command Received: ");

        if (String("GETNETS").equals(command)) {
          softSer.println("GETNETS");
          sendCommandAck();
          getNetworks();
        } else if (String("CONNECT").equals(command)) {
          softSer.println("CONNECT");
          sendCommandAck();
          connect();
        } else if (String("DISCONNECT").equals(command)) {
          softSer.println("DISCONNECT");
          sendCommandAck();
          disconnect();
        } else if (String("GETNETINFO").equals(command)) {
          softSer.println("GETNETINFO");
          sendCommandAck();
          getNetInfo();
        } else if (String("SEARCH").equals(command)) {
          softSer.println("SEARCH");
          sendCommandAck();
//          browseFiles();
        } else if (String("DOWNLOAD").equals(command)) {
          softSer.println("DOWNLOAD");
          sendCommandAck();
//          downloadFile();
        }else if (String("GETAPPINFO").equals(command)) {
          softSer.println("GETAPPINFO");
          sendCommandAck();
//          getAppInfo();
        } else if (String("GETBYID").equals(command)) {
          
        } else {
          sendCommandAck("UC");
        }
      }
    } else if (data == 221) {
      String command = Serial.readStringUntil(0);

      if (String("NETDATA").equals(command)) {
        String lenStr = Serial.readStringUntil(0);
        unsigned long len = lenStr.toInt();

        String checksum = Serial.readStringUntil(0);
        String appName = Serial.readStringUntil(0);

        netClient.write((byte) 221);
        netClient.print("NETDATA");
        netClient.write((byte) 0);
        netClient.print(lenStr);
        netClient.write((byte) 0);
        netClient.print(checksum);
        netClient.write((byte) 0);
        netClient.print(appName);
        netClient.write((byte) 0);

        unsigned long counter = 0;
        while (counter < len) {
          int b = Serial.read();
          if (b != -1) {
            netClient.write(b);
            counter++;
          }
        }

        softSer.println(String(counter) + String(" Bytes written"));
      }
    }
  }

  if (netClient.available() > 0) {
    byte data = netClient.read();
    if (data == 221) {
      String command = netClient.readStringUntil(0);

      if (String("NETDATA").equals(command)) {
        String lenStr = netClient.readStringUntil(0);
        unsigned long len = lenStr.toInt();

        String checksum = netClient.readStringUntil(0);
        String appName = netClient.readStringUntil(0);

        Serial.write((byte) 221);
        Serial.print("NETDATA");
        Serial.write((byte) 0);
        Serial.print(lenStr);
        Serial.write((byte) 0);
        Serial.print(checksum);
        Serial.write((byte) 0);
        Serial.print(appName);
        Serial.write((byte) 0);

        unsigned long counter = 0;
        while (counter < len) {
          int b = netClient.read();
          if (b != -1) {
            Serial.write(b);
            counter++;
          }
        }

        softSer.println(String(counter) + String(" Bytes written"));
      }
    }
  }
}

void getNetworks() {
  int n = WiFi.scanNetworks();
  int len = 0;
  int counter = 0;
  unsigned char* buf;

  for (int i = 0; i < n; i++) {
    len += WiFi.SSID(i).length() + 1;
    len += String(WiFi.RSSI(i)).length() + 1;
    len += String(WiFi.encryptionType(i)).length() + 1;

    softSer.println(WiFi.SSID(i) + " " + String(WiFi.RSSI(i)) + " " + String(WiFi.encryptionType(i)));
  }

  buf = (unsigned char*) malloc(len);

  for (int i = 0; i < n; i++) {
    int tmp;
    tmp = WiFi.SSID(i).length() + 1;
    memcpy(&buf[counter], WiFi.SSID(i).c_str(), tmp);
    counter += tmp;

    tmp = String(WiFi.RSSI(i)).length() + 1;
    memcpy(&buf[counter], String(WiFi.RSSI(i)).c_str(), tmp);
    counter += tmp;

    tmp = String(WiFi.encryptionType(i)).length() + 1;
    memcpy(&buf[counter], String(WiFi.encryptionType(i)).c_str(), tmp);
    counter += tmp;
  }

  sendData(buf, len);
  free(buf);
}


void connect() {
  String netIndex = Serial.readStringUntil(0);
  int selectedNetwork = netIndex.toInt();
  softSer.println("Selection: " + String(selectedNetwork));

  String password = Serial.readStringUntil(0);
  softSer.println(WiFi.SSID(selectedNetwork) + ": " + password);

  WiFi.begin(WiFi.SSID(selectedNetwork).c_str(), password.c_str());
}

void disconnect() {
  WiFi.disconnect();
}

void getNetInfo() {
  unsigned char* buf;
  int len = 0;
  int counter = 0;
  
  if (WiFi.isConnected()) {
    softSer.println("Connected to: " + WiFi.SSID() + " " + String(WiFi.RSSI()));
    
    len += WiFi.SSID().length() + 1;
    len += String(WiFi.RSSI()).length() + 1;
    buf = (unsigned char*) malloc(len);

    int tmp;
    tmp = WiFi.SSID().length() + 1;
    memcpy(&buf[counter], WiFi.SSID().c_str(), tmp);
    counter += tmp;

    tmp = String(WiFi.RSSI()).length() + 1;
    memcpy(&buf[counter], String(WiFi.RSSI()).c_str(), tmp);
    counter += tmp;
  } else {
    len = 2;
    buf = (unsigned char*) malloc(len);
    memset(buf, 0, len);
  }

  sendData(buf, len);
  free(buf);
}


void getWebContent() {
  
  String modeString=Serial.readStringUntil(0);
  int mode=modeString.toInt();
  /*modes: 
  0=all
  1=only content
  2=only headers
  */
  
  String url = Serial.readStringUntil(0);
  String portString= Serial.readStringUntil(0);
  int port=portString.toInt();
  
  softSer.println(url); //GET muss mit in der URL stehen
  softSer.println("Port:"+portString);
  
  WiFiClient wfClient;

//  if (!wfClient.connect(host.c_str(), port)) {
//    softSer.println("connection failed");
//  return;
//  }

  wfClient.print(url);

  softSer.println("request sent");

  int contentLength = 0;
  String contentLengthStr;

  String header="";
  
  while(wfClient.connected()) {
    String line = wfClient.readStringUntil('\n');
    header=header+line;
    
    softSer.println(line);
    if (line.startsWith("Content-Length")) {
      contentLengthStr = line.substring(line.indexOf(":") + 2);
      softSer.println("--- " + contentLengthStr + " ---");
      contentLength = contentLengthStr.toInt();
    } else if (line == "\r") {
      softSer.println("headers received");
      break;
    }
  }
  
  if(mode==0 || mode==2){
    sendData((unsigned char*) header.c_str(), header.length());
  }

  softSer.println(contentLengthStr);

  unsigned char buf[256];
 
  int counter = 0;
  
  if(mode==2){
    return;
  }
  
  while(wfClient.connected() && counter < contentLength) {
    if (wfClient.available()) {
      int len = wfClient.read(buf, sizeof buf);

    if (sendData(buf, len)) {
      return;
    }
      
    counter += len;
    softSer.println(String(len) + " Bytes written (" + String(counter) + ")");
    } 
    yield();
  }

}


boolean sendData(unsigned char* data, int len) {
  String ack;
  
  do {    
    Serial.write((byte) 221); // 0xDD
  Serial.print("DATA");
  Serial.write((byte) 0);
    softSer.println("DATA command sent");
    
    Serial.print(len, DEC);
  Serial.write((byte) 0);
    softSer.println("Length sent: " + len);
  
  unsigned long cs = checksum(data, len);  
  Serial.print(cs);
  Serial.write((byte) 0);
    softSer.println("Checksum sent: " + String(cs));
  
    int counter = 0;
    while(counter < len) {
      int transfered = Serial.write(&data[counter], len - counter);
      counter += transfered;
      yield();
    }

    softSer.println("Data sent");

    ack = receiveAck();
    softSer.println("Ack received: " + ack);
    
    yield();
  } while(!String("OK").equals(ack));

  return false;
}

String receiveAck() {
  long start = millis();
  
  while(!timeElapsed(start, 1000)) {
    if (Serial.available() > 0) {
      unsigned char c = Serial.read();
      if (c == 220) {
        return Serial.readStringUntil(0);
      }
    }

    yield();
  }

  return "CA1";
}

bool timeElapsed(long start, long ms) {
  return millis() - start > ms;
}


unsigned long checksum(unsigned char* data, int length) {
  unsigned long cs = 0;
  
  for (int i = 0; i < length; i++) {
    cs += (unsigned long) data[i];
  
    if (cs > 10000) {
      cs = cs % 10000;
    }
  }

  return cs;
}

String serialReadString() {
  char c;
  String resp = "";
  
  while (true) {
    if (Serial.available() > 0) {
      c = Serial.read();
      if (c == 0) {
        break;
      } else {
        resp += c;
      }
    }

    yield(); // to prevent watchdog timeout
  }

  return resp;
}

String serialReadString(long timeout) {
  String result = "";
  long start = millis();
  
  while(true) {
    if (Serial.available() > 0) {
      unsigned char c = Serial.read();

      if (c == 0) {
        return result;
      }
      
      result += c;
    }

    if (timeElapsed(start, timeout)) {
      return result;
    }

    yield();
  }
}

void downloadPic(String appNumber){
  delay(1000);
  
  WiFiClient wfClient;
  //int appNumber=1;
  
  String host = "gtr-app-store.herokuapp.com";
  String url = "/apps/" + (String)appNumber + "?dl=1";
  int port = 80; // prefer HTTP over HTTPS cause of cert problems

  if (!wfClient.connect(host.c_str(), port)) {
    softSer.println("connection failed");
    return;
  }

  wfClient.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "User-Agent: BuildFailureDetectorESP8266\r\n" +
               "Connection: close\r\n\r\n");

  softSer.println("request sent");

  int contentLength = 0;
  String contentLengthStr;

  while(wfClient.connected()) {
    String line = wfClient.readStringUntil('\n');
    softSer.println(line);
    if (line.startsWith("Content-Length")) {
      contentLengthStr = line.substring(line.indexOf(":") + 2);
      softSer.println("--- " + contentLengthStr + " ---");
      contentLength = contentLengthStr.toInt();
    } else if (line == "\r") {
      softSer.println("headers received");
      break;
    }
  }
  
  softSer.println(contentLengthStr);

  unsigned char buf[contentLength];

  int counter = 0;
  while(wfClient.connected() && counter < contentLength) {
    if (wfClient.available()) {
      int len = wfClient.read(buf, sizeof buf);
      //Serial.write(buf, len);

      counter += len;
      softSer.println(String(len) + " Bytes written (" + String(counter) + ")");
    }
    yield();
  }

  sendData(buf,contentLength);
  
  //for(int i=0;i<contentLength;i++){
  //    softSer.print(buf[i],HEX);
  //}
}


void sendCommand(String command) {
  Serial.write((byte) 219);
  Serial.print(command);
  Serial.write((byte) 0);
}

void sendCommandAck() {
  sendCommandAck("OK");
}

void sendCommandAck(String status) {
  Serial.write((byte) 220); // 0xDC
  Serial.print(status);
  Serial.write((byte) 0);
}



