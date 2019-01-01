#ifndef NET_H
#define NET_H

#include "stddef.h"
#include "status.h"
#include "syscalls.h"

// constants

#define PROTOCOL_TCP 1
#define PROTOCOL_UDP 2

// structs

typedef struct {
	unsigned int length;
	unsigned int checksum;
	unsigned char* buf;
} Data;

typedef struct {
	unsigned int length;
	unsigned int checksum;
	unsigned char appName[9];
	unsigned char* buf;
} NetData;

typedef struct {
	unsigned char* ssid;
	int rssi;
	int encType;
} Network;

typedef struct NetworkList {
	Network network;
	struct NetworkList* next;
} NetworkList;

// functions

void openSerial();
void closeSerial();

void sendCommand(unsigned char* command);
int receiveStatus();
int receiveStatusTimeout(int timeout);

int receiveString(unsigned char* buf, int maxLen);
int receiveStringTimeout(unsigned char* buf, int maxLen, int timeout);

void freeData(Data* data);
void freeNetData(NetData* data);
void freeNetList(NetworkList* netList);

unsigned int calculateChecksum(unsigned char* buf, int len);

Data* receiveData(int retryCount);
Data* receiveDataTimeout(int timeout, int retryCount);

void sendNetData(unsigned char* buf, unsigned int length);
void sendNetDataRetry(unsigned char* buf, unsigned int length, int retryCount);
void sendUdpData(unsigned char* buf, unsigned int length);
void sendNetDataProt(unsigned char* buf, unsigned int length, int protocol);
NetData* receiveNetData();
NetData* receiveNetDataTimeout(int timeout);

NetworkList* getAvailableNetworks();
int connect(int index, unsigned char* pass);
int disconnect();
Network getNetworkInfo();
int startAP(unsigned char* ssid, unsigned char* password);
int stopAP();

#endif