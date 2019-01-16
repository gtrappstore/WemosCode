#if defined (ARDUINO_ESP8266_WEMOS_D1MINILITE)
  #define WEMOS 1
#else
  #define WEMOS 0
#endif

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>

#if WEMOS == 1
  #include <SoftwareSerial.h>
  SoftwareSerial softSer(3, 1);
#endif

const int MAX_CLIENTS = 8;
int connectedClients = 0;

WiFiClient* clients[MAX_CLIENTS];
WiFiServer netServer(8266);
WiFiUDP udpSocket;

void setup() {
  // put your setup code here, to run once:
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200, SERIAL_8N1);

  #if WEMOS == 1
    Serial.swap();
    softSer.begin(115200);
    pinMode(3, INPUT);
    pinMode(1, OUTPUT);
  #endif

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  udpSocket.begin(8267);

  digitalWrite(LED_BUILTIN, LOW); // turn the LED on

  randomSeed(analogRead(0));

  LOG("Started");
}

void loop() {
  // put your main code here, to run repeatedly:
  
  if (WiFi.getMode() == WIFI_STA) {
    if (WiFi.isConnected()) {
      if (clients[0] == NULL) {
        clients[0] = new WiFiClient();
      }

      if (!clients[0]->connected()) {
        clients[0]->connect(WiFi.gatewayIP(), 8266);
        connectedClients = 1;
      }
    }
  } else if (WiFi.getMode() == WIFI_AP) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
      if (NULL != clients[i] && !clients[i]->connected()) {
        clients[i]->stop();
        delete clients[i];
        clients[i] = NULL;
        connectedClients--;
        LOG("Client disconnected: " + String(connectedClients));
      }
    }

    WiFiClient newClient = netServer.available();

    if (newClient) {
      for (int i = 0; i < MAX_CLIENTS; i++) {
        if (NULL == clients[i]) {
          clients[i] = new WiFiClient(newClient);
          clients[i]->setNoDelay(true);
          connectedClients++;
          LOG("Client connected: " + String(connectedClients));
          break;
        }
      }
    }

    unsigned char number_client;
    number_client= wifi_softap_get_station_num();
    LOG("Connected Devices: " + String(number_client));
  }

  // handle Serial commands
  if (Serial.available() > 0) {
    byte data = Serial.read();

    if (data == 219) {
      String command = Serial.readStringUntil(0);

      LOG("Hey, 219");

      if (command.length() == 0) {
        sendCommandAck("TO");
      } else {
        LOG("Command Received: ");

        if (String("GETNETS").equals(command)) {
          LOG("GETNETS");
          sendCommandAck();
          getNetworks();
        } else if (String("CONNECT").equals(command)) {
          LOG("CONNECT");
          sendCommandAck();
          connect();
        } else if (String("DISCONNECT").equals(command)) {
          LOG("DISCONNECT");
          sendCommandAck();
          disconnect();
        } else if (String("GETNETINFO").equals(command)) {
          LOG("GETNETINFO");
          sendCommandAck();
          getNetInfo();
        } else if (String("STARTAP").equals(command)) {
          LOG("STARTAP");
          sendCommandAck();
          startAP();
        } else if (String("STOPAP").equals(command)) {
          LOG("STOPAP");
          sendCommandAck();
          stopAP();
        } else if (String("SEARCH").equals(command)) {
          LOG("SEARCH");
          sendCommandAck();
//          browseFiles();
        } else if (String("DOWNLOAD").equals(command)) {
          LOG("DOWNLOAD");
          sendCommandAck();
//          downloadFile();
        }else if (String("GETAPPINFO").equals(command)) {
          LOG("GETAPPINFO");
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
        if (spreadTcpData(&Serial)) {
          sendCommandAck();
        } else {
          sendCommandAck("DE");
        }
      } else if (String("NETDATAUDP").equals(command)) {
        spreadUdpData(&Serial);
      } else {
        sendCommandAck("UC");
      }
    }
  }

  // handle UDP packets
  int packetSize = udpSocket.parsePacket();
  if (packetSize) {
    byte data = udpSocket.read();
    if (data == 221) {
      String command = udpSocket.readStringUntil(0);
      
      if (String("NETDATA").equals(command)) {
        spreadUdpData(&udpSocket);
      }
    }
  }

  // handle TCP packets
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (NULL != clients[i] && clients[i]->connected()) {
      byte data = clients[i]->read();
      if (data == 221) {
        String command = clients[i]->readStringUntil(0);
        
        if (String("NETDATA").equals(command)) {
          spreadTcpData(clients[i]);
        }
      }
    }

    yield();
  }
}

bool spreadTcpData(Stream* input) { // spread tcp data comming from Serial or from clients
  unsigned int dataLength = 0;
  byte* data = readNetDataFromStream(input, &dataLength);

  if (data == NULL) {
    return false;
  }

  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (NULL != clients[i] && clients[i]->connected()) {
      if (input != clients[i]) { // send to all clients who aren't the sender
        clients[i]->write(data, dataLength);
      }
    }
  }

  if (input != &Serial) { // send to Serial if it isn't the sender
    Serial.write(data, dataLength);
  }

  delete[] data;
  return true;
}

bool spreadUdpData(Stream* input) { // spread data comming from Serial or from udpSocket
  unsigned int dataLength = 0;
  byte* data = readNetDataFromStream(input, &dataLength);

  if (data == NULL) {
    return false;
  }

  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (NULL != clients[i] && clients[i]->connected()) {
      if (input != &udpSocket || udpSocket.remoteIP() != clients[i]->remoteIP()) { // send to all clients (without the one who sent the message)
        udpSocket.beginPacket(clients[i]->remoteIP(), 8267);
        udpSocket.write(data, dataLength);
        udpSocket.endPacket();
      }
    }
  }

  if (input != &Serial) { // send to Serial if it does't come from there
    Serial.write(data, dataLength);
  }

  delete[] data;
  return true;
}

byte* readNetDataFromStream(Stream* input, unsigned int *length) {
  String dataLenStr = input->readStringUntil(0);
  unsigned long dataLength = dataLenStr.toInt();

  String checksum = input->readStringUntil(0);
  String appName = input->readStringUntil(0);

  unsigned int headerLength = 1 + 8; // Start-Byte + "NETDATA"
  headerLength += dataLenStr.length() + 1;
  headerLength += checksum.length() + 1;
  headerLength += appName.length() + 1;

  unsigned int packetLength = headerLength + dataLength;

  byte* data = new byte[packetLength];
  unsigned long counter = 0;

  data[counter] = 221;
  counter += 1;
  
  memcpy(&data[counter], "NETDATA", 8);
  counter += 8;

  memcpy(&data[counter], dataLenStr.c_str(),dataLenStr.length() + 1);
  counter += dataLenStr.length() + 1;

  memcpy(&data[counter], checksum.c_str(), checksum.length() + 1);
  counter += checksum.length() + 1;

  memcpy(&data[counter], appName.c_str(), appName.length() + 1);
  counter += appName.length() + 1;

  while (counter < packetLength) { // all remaining bytes are data
    int numShouldRead = (int) (packetLength - counter);
    int numRead = input->readBytes(&data[counter], numShouldRead);
    counter += numRead;
    yield();
  }

  unsigned long cs = checksum.toInt();
  if (cs == calculateChecksum(&data[headerLength], dataLength)) {
    *length = packetLength;
    return data;
  }

  *length = 0;
  delete[] data;
  return NULL;
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

    LOG(WiFi.SSID(i) + " " + String(WiFi.RSSI(i)) + " " + String(WiFi.encryptionType(i)));
  }

  buf = new unsigned char[len];

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
  delete[] buf;
}


void connect() {
  String ssid = Serial.readStringUntil(0);
  String password = Serial.readStringUntil(0);
  
  LOG(ssid + ": " + password);
  WiFi.begin(ssid.c_str(), password.c_str());
}

void disconnect() {
  WiFi.disconnect();
}

void getNetInfo() {
  unsigned char* buf;
  int len = 0;
  int counter = 0;

  if (WiFi.getMode() == WIFI_AP) {
    len += 2;
    len += WiFi.softAPSSID().length() + 1;
    len += 2;
    buf = (unsigned char*) malloc(len);

    int tmp;
    tmp = 2;
    memcpy(&buf[counter], String("1").c_str(), tmp);
    counter += tmp;
    
    tmp = WiFi.softAPSSID().length() + 1;
    memcpy(&buf[counter], WiFi.softAPSSID().c_str(), tmp);
    counter += tmp;

    tmp = 2;
    memcpy(&buf[counter], String("0").c_str(), tmp);
    counter += tmp;
  } else if (WiFi.isConnected()) {
    LOG("Connected to: " + WiFi.SSID() + " " + String(WiFi.RSSI()));

    len += 2;
    len += WiFi.SSID().length() + 1;
    len += String(WiFi.RSSI()).length() + 1;
    buf = (unsigned char*) malloc(len);

    int tmp;
    tmp = 2;
    memcpy(&buf[counter], String("0").c_str(), tmp);
    counter += tmp;
    
    tmp = WiFi.SSID().length() + 1;
    memcpy(&buf[counter], WiFi.SSID().c_str(), tmp);
    counter += tmp;

    tmp = String(WiFi.RSSI()).length() + 1;
    memcpy(&buf[counter], String(WiFi.RSSI()).c_str(), tmp);
    counter += tmp;
  } else {
    len = 3;
    buf = (unsigned char*) malloc(len);
    memset(buf, 0, len);
  }

  sendData(buf, len);
  free(buf);
}

void startAP() {
  String ssid = Serial.readStringUntil(0);
  String password = Serial.readStringUntil(0);

  LOG("Hotspot: " + ssid + " " + password);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid.c_str(), password.c_str());
  netServer.begin();

  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (NULL != clients[i]) {
      clients[i]->stop();
      delete clients[i];
      clients[i] = NULL;
    }
  }

  connectedClients = 0;
}

void stopAP() {
  WiFi.softAPdisconnect(false);
  WiFi.mode(WIFI_STA);
  netServer.stop();

  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (NULL != clients[i]) {
      clients[i]->stop();
      delete clients[i];
      clients[i] = NULL;
    }
  }

  connectedClients = 0;
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
  
//  LOG(url); //GET muss mit in der URL stehen
//  LOG("Port:"+portString);
  
  WiFiClient wfClient;

//  if (!wfClient.connect(host.c_str(), port)) {
//    LOG("connection failed");
//  return;
//  }

  wfClient.print(url);

//  LOG("request sent");

  int contentLength = 0;
  String contentLengthStr;

  String header="";
  
  while(wfClient.connected()) {
    String line = wfClient.readStringUntil('\n');
    header=header+line;
    
//    LOG(line);
    if (line.startsWith("Content-Length")) {
      contentLengthStr = line.substring(line.indexOf(":") + 2);
//      LOG("--- " + contentLengthStr + " ---");
      contentLength = contentLengthStr.toInt();
    } else if (line == "\r") {
//      LOG("headers received");
      break;
    }
  }
  
  if(mode==0 || mode==2){
    sendData((unsigned char*) header.c_str(), header.length());
  }

//  LOG(contentLengthStr);

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
//    LOG(String(len) + " Bytes written (" + String(counter) + ")");
    } 
    yield();
  }

}


boolean sendData(byte* data, int len) {
  String ack;
  
  do {    
    Serial.write((byte) 221); // 0xDD
  Serial.print("DATA");
  Serial.write((byte) 0);
    LOG("DATA command sent");
    
    Serial.print(len, DEC);
  Serial.write((byte) 0);
    LOG("Length sent: " + len);
  
  unsigned long cs = calculateChecksum(data, len);
  Serial.print(cs);
  Serial.write((byte) 0);
    LOG("Checksum sent: " + String(cs));
  
    int counter = 0;
    while(counter < len) {
      int transfered = Serial.write(&data[counter], len - counter);
      counter += transfered;
      yield();
    }

    LOG("Data sent");

    ack = receiveAck();
    LOG("Ack received: " + ack);
    
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


unsigned long calculateChecksum(byte* data, unsigned long length) {
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
//    LOG("connection failed");
    return;
  }

  wfClient.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "User-Agent: BuildFailureDetectorESP8266\r\n" +
               "Connection: close\r\n\r\n");

//  LOG("request sent");

  int contentLength = 0;
  String contentLengthStr;

  while(wfClient.connected()) {
    String line = wfClient.readStringUntil('\n');
//    LOG(line);
    if (line.startsWith("Content-Length")) {
      contentLengthStr = line.substring(line.indexOf(":") + 2);
//      LOG("--- " + contentLengthStr + " ---");
      contentLength = contentLengthStr.toInt();
    } else if (line == "\r") {
//      LOG("headers received");
      break;
    }
  }
  
//  LOG(contentLengthStr);

  unsigned char buf[contentLength];

  int counter = 0;
  while(wfClient.connected() && counter < contentLength) {
    if (wfClient.available()) {
      int len = wfClient.read(buf, sizeof buf);
      //Serial.write(buf, len);

      counter += len;
//      LOG(String(len) + " Bytes written (" + String(counter) + ")");
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

void LOG(String msg) {
  #if WEMOS == 1
    softSer.println(msg);
  #endif
}

