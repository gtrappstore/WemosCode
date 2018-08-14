#ifndef NET_H
#define NET_H

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
void receiveStatus();
void receiveStatusTimeout(int timeout);

void receiveString(unsigned char* buf);
void receiveStringTimeout(unsigned char* buf, int timeout);

void freeData(Data* data);

int calculateChecksum(unsigned char* buf, int len);

Data* receiveData();
Data* receiveStatusTimeout(int timeout);

NetworkList* getAvailableNetworks();

#endif