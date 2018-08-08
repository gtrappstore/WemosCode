#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>

#define LED D4

SoftwareSerial softSer(3, 1);

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
}

void loop() {
  // put your main code here, to run repeatedly:

  /*
  unsigned char testData[] { 'A', '0', 'z', '6', '2', 'p', 'e', 'n', 'i', 'S'};
  softSer.println(checksum(testData, 10));
  return;
  */
    
  if (softSer.available() > 0) {
    byte data = softSer.read();
    //Serial.write(data);
    softSer.write(data);

    if (data == '0') {
      Serial.write((byte) 0);
      softSer.println("NULL Byte written");
    }

    if(data==48)
      downloadPic("1");
  }

  if (Serial.available() > 0) {
    byte data = Serial.read();
    softSer.write(data);

    if (data == 219) {
      String command = serialReadString(500, 10);

      softSer.println("Hey, 219");

      if (command.length() == 0) {
        sendCommandAck("TO");
      } else {
        softSer.print("Command Received: ");
        
        if (String("CONNECT").equals(command)) {
          softSer.println("CONNECT");
          sendCommandAck();
          connectToNetwork();
        } else if (String("DISCONNECT").equals(command)) {
          softSer.println("DISCONNECT");
          sendCommandAck();
          WiFi.disconnect();
          sendCommandAck("OF");
        } else if (String("GETNETINFO").equals(command)) {
          softSer.println("GETNETINFO");
          sendCommandAck();

          if (WiFi.isConnected()) {
            softSer.println("Connected to: " + WiFi.SSID());
            Serial.print(WiFi.SSID());
            int rssi = WiFi.RSSI();
            Serial.write('\t');
            Serial.print(rssi);
            softSer.print(rssi);
         }
      
          Serial.write((byte) 0);
        } else if (String("SEARCH").equals(command)) {
          softSer.println("SEARCH");
          sendCommandAck();
          browseFiles();
        } else if (String("DOWNLOAD").equals(command)) {
          softSer.println("DOWNLOAD");
          sendCommandAck();
          downloadFile();
        }else if (String("GETAPPINFO").equals(command)) {
          softSer.println("GETAPPINFO");
          sendCommandAck();
          getAppInfo();
        } else if (String("GETBYID").equals(command)) {
          
        } else {
          sendCommandAck("UC");
        }
      }
    }

    if (data == 110) {
      if (WiFi.isConnected()) {
        softSer.println("Connected to: " + WiFi.SSID());
        Serial.print(WiFi.SSID());
        int rssi = WiFi.RSSI();
        Serial.write('\t');
        Serial.print(rssi);
        softSer.print(rssi);
      }
      
      Serial.write((byte) 0);
    } else if (data == 111) {
      softSer.println("");
      softSer.println("Start Byte Received!");

      connectToNetwork();
//      downloadFile();
    } else if (data == 112) {
      browseFiles();
    } else if (data == 113) {
      downloadFile();
    } else if (data == 114) {
      softSer.println("Disconnecting...");
      WiFi.disconnect();
    }
  }
}

void sendCommandAck() {
  sendCommandAck("OK");
}

void sendCommandAck(String status) {
  Serial.write((byte) 220); // 0xDB
  Serial.print(status);
  Serial.write((byte) 0);
}

void sendCommand(String command) {
  Serial.write((byte) 219);
  Serial.print(command);
  Serial.write((byte) 0);
}

void connectToNetwork() {
  int n = WiFi.scanNetworks();
  for (int i = 0; i < n; i++) {
    // Print SSID and RSSI for each network found
    softSer.print(i);
    softSer.print(": ");
    softSer.print(WiFi.SSID(i));
    softSer.print(" (");
    softSer.print(WiFi.RSSI(i));
    softSer.print(")");
    softSer.println((WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*");
    
    Serial.print(WiFi.SSID(i));
    Serial.print("\n");
  }

  Serial.write((byte) 0);

  softSer.println("");
  
  String netIndex = serialReadString();
  int selectedNetwork = netIndex.toInt();
  softSer.println("Selection: " + String(selectedNetwork));

  String password = serialReadString();
  softSer.println(WiFi.SSID(selectedNetwork) + ": " + password);

  WiFi.begin(WiFi.SSID(selectedNetwork).c_str(), password.c_str());

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    softSer.print(".");
  }

  softSer.println("");
  softSer.println("WiFi connected");
  softSer.println("IP address: ");
  softSer.println(WiFi.localIP());

  sendCommandAck("OF");
}

void browseFiles() {
  String searchQuery = serialReadString();

  softSer.println(searchQuery);
  
  WiFiClient wfClient;

  String host = "gtr-app-store.herokuapp.com";
  String url = "/search?name=" + searchQuery;
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

  unsigned char buf[256];

  int counter = 0;
  while(wfClient.connected() && counter < contentLength) {
    if (wfClient.available()) {
      int len = wfClient.read(buf, sizeof buf);
      Serial.write(buf, len);
      
      counter += len;
      softSer.println(String(len) + " Bytes written (" + String(counter) + ")");
    }
    yield();
  }

  Serial.write((byte) 0);
}

void downloadFile() {
  String appNumber = serialReadString();

  softSer.println("Download App Number " + appNumber);

  delay(1000);
  
  WiFiClient wfClient;

  String host = "gtr-app-store.herokuapp.com";
  String url = "/apps/" + appNumber + "?dl=1";
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

  Serial.print(contentLength);
  softSer.println(contentLengthStr);
  Serial.write((byte) 0);

  unsigned char buf[256];

  int counter = 0;
  while(wfClient.connected() && counter < contentLength) {
    if (wfClient.available()) {
      int len = wfClient.read(buf, sizeof buf);
//      Serial.write(buf, len);

      if (sendData(buf, len)) {
        return;
      }
      
      counter += len;
      softSer.println(String(len) + " Bytes written (" + String(counter) + ")");
    }
    yield();
  }
}

void getAppInfo() {
  String appNumber = serialReadString();
  
  softSer.println("Get Info Number " + appNumber);

  downloadPic(appNumber); //downloaden und senden
  //senden der Textinfos als einen String mit Trennzeichen (FESTZULEGEN), Trennen auf dem GTR  
  
  
}

boolean sendData(unsigned char* data, int length) {
  String ack;
  
  do {    
    sendCommand("DATA");
    softSer.println("DATA command sent");
    
    char lenBuf[11];
    sprintf(lenBuf, "%010d", length);
    String len = String(lenBuf);
    Serial.print(len);
    softSer.println("Length sent: " + len);
    
    String cs = checksum(data, length);
    Serial.print(cs);
    softSer.println("Checksum sent: " + cs);

    long r = random(20);
    if (r == 1) {
      data[0] = (unsigned char) 8;
    }
    softSer.println("RANDOM: " + String(r));
  
    int counter = 0;
    while(counter < length) {
      int transfered = Serial.write(&data[counter], length - counter);
      counter += transfered;
      yield();
    }

    softSer.println("Data sent");

    ack = receiveAck();
    softSer.println("Ack received: " + ack);

    if (String("CA").equals(ack) || String("CA1").equals(ack)) {
      return true;
    }
    
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
        return serialReadString();
      }
    }

    yield();
  }

  return "CA1";
}

bool timeElapsed(long start, long ms) {
  return millis() - start > ms;
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

String checksum(unsigned char* data, int length) {
  int cs = 0;
  char checksumBuf[5];
  
  for (int i = 0; i < length; i++) {
    cs += (int)data[i];
  }

  cs = cs % 10000;
  sprintf(checksumBuf, "%04d", cs);
  return String(checksumBuf);
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

String serialReadString(int timeout, int maxLength) {
  char c;
  String resp = "";
  int startTime = millis();
  
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

    if (resp.length() >= maxLength) {
      break;
    }

    if (millis() - startTime > timeout) {
      return String();
    }
  }

  return resp;
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


