#ifndef NET_H
#define NET_H

#include "stddef.h"
#include "status.h"
#include "syscalls.h"

typedef struct {
	unsigned int length;
	unsigned int checksum;
	unsigned char* buf;
} Data;

typedef struct {
	unsigned char* ssid;
	int rssi;
	int encType;
} Network;

typedef struct NetworkList {
	Network network;
	struct NetworkList* next;
} NetworkList;

void openSerial();
void closeSerial();

void sendCommand(unsigned char* command);
int receiveStatus();
int receiveStatusTimeout(int timeout);

int receiveString(unsigned char* buf, int maxLen);
int receiveStringTimeout(unsigned char* buf, int maxLen, int timeout);

void freeData(Data* data);

unsigned int calculateChecksum(unsigned char* buf, int len);

Data* receiveData(int retryCount);
Data* receiveDataTimeout(int timeout, int retryCount);

NetworkList* getAvailableNetworks();

#endif