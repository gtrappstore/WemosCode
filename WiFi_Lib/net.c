#include "net.h"
#include "stddef.h"
#include "stdio.h"
#include "stdlib.h"

void openSerial() {
    unsigned char mode[6];
    mode[0] = 0;
    mode[1] = 9; // 115200 baud
    mode[2] = 0;
    mode[3] = 0;
    mode[4] = 0;
    mode[5] = 0;

    Serial_Open(mode);
}

void closeSerial() {
	Serial_Close(1);
}

void sendCommand(unsigned char* command) {
	Serial_BufferedTransmitOneByte((unsigned char) 219);
	Serial_BufferedTransmitNBytes(command, strlen(command) + 1);
}

void sendStatus(unsigned char* status) {
	Serial_BufferedTransmitOneByte((unsigned char) 220);
	Serial_BufferedTransmitNBytes(status, strlen(status) + 1);
}

int receiveString(unsigned char* buf, int maxLen) {
	return receiveStringTimeout(buf, maxLen, 500);
}

int receiveStringTimeout(unsigned char* buf, int maxLen, int timeout) { // Bei true ist nicht getimeouted
	int startTicks = RTC_GetTicks();
	int numChars = 0;
	int ret = 0;
	
	while (!RTC_Elapsed_ms(startTicks, timeout)) {
		if (Serial_GetReceivedBytesAvailable() >= 1) {
			unsigned char c;
			Serial_ReadOneByte(&c);
			
			if (c == 0 || numChars >= maxLen - 1) {
				ret = 1;
				break;
			}
			
			buf[numChars++] = c;
		}
	}
	
	buf[numChars] = 0;
	return ret;
}

int receiveStatus() {
	return receiveStatusTimeout(500);
}

int receiveStatusTimeout(int timeout) {
	int startTicks = RTC_GetTicks();
	unsigned char statusBuf[11];
	
	while (!RTC_Elapsed_ms(startTicks, timeout)) {
		if (Serial_GetReceivedBytesAvailable() >= 1) {
			unsigned char c;
			Serial_ReadOneByte(&c);
			
			if (c == 220) {
				receiveStringTimeout(statusBuf, 11, timeout);
				return convertStatus(statusBuf);
			}
		}
	}
	
	return -1;
}

unsigned int calculateChecksum(unsigned char* buf, int len) {
	unsigned int checksum = 0;
	int counter;
	
	for (counter = 0; counter < len; counter++) {
		checksum += buf[counter];
		
		if (checksum > 10000) {
			checksum = checksum % 10000;
		}
	}
	
	return checksum;
}

Data* receiveData(int retryCount) {
	return receiveDataTimeout(500, retryCount);
}

Data* receiveDataTimeout(int timeout, int retryCount) {
	int startTicks = RTC_GetTicks();
	Data* data = NULL;
	unsigned char buf[11];
	int retry = 0;
	
	if (retryCount < 0) {
		return NULL;
	}
	
	while (!RTC_Elapsed_ms(startTicks, timeout)) {
		unsigned char c;
		
		if (Serial_ReadOneByte(&c) == 0 && c == 221) {
			receiveStringTimeout(buf, 11, timeout);
			if (strcmp(buf, "DATA", 4) == 0) {
				unsigned int counter = 0;
				short received;
				data = (Data*) malloc(sizeof(Data));
				data->buf = NULL;
				
				if (!receiveStringTimeout(buf, 11, timeout)) {
					retry = 1;
					break;
				}
				data->length = (unsigned int) atol(buf);
				
				if (!receiveStringTimeout(buf, 11, timeout)) {
					retry = 1;
					break;
				}
				data->checksum = (unsigned int) atol(buf);
				
				data->buf = (unsigned char*) malloc(data->length);
				while (1) {
					Serial_ReadNBytes(&data->buf[counter], data->length - counter, &received);
					counter += received;
					
					if (counter >= data->length) {
						if (data->checksum != calculateChecksum(data->buf, data->length)) {
							retry = 1;
						}
						
						break;
					}

					if (RTC_Elapsed_ms(startTicks, timeout)) {
						freeData(data);
						data = NULL;
						break;
					}
				}

				break;
			}
		}
	}
	
	if (retry == 1) {
		freeData(data);
		data = NULL;
		sendStatus("DE");
		data = receiveDataTimeout(timeout, retryCount - 1);
	} else {
		sendStatus("OK");
	}
	
	return data;
}

void sendNetData(unsigned char* buf, unsigned int length) {
	sendNetDataRetry(buf, length, 0);
}

void sendNetDataRetry(unsigned char* buf, unsigned int length, int retryCount) {
	sendNetDataProt(buf, length, PROTOCOL_TCP);
	
	if (receiveStatus() != STATUS_OK) {
		if (retryCount > 0) {
			sendNetDataRetry(buf, length, retryCount - 1);
		}
	}
}

void sendUdpData(unsigned char* buf, unsigned int length) {
	sendNetDataProt(buf, length, PROTOCOL_UDP);
}

void sendNetDataProt(unsigned char* buf, unsigned int length, int protocol) {
	unsigned char text[11];
	unsigned int counter = 0;
	
	if (protocol != PROTOCOL_TCP && protocol != PROTOCOL_UDP) {
		return; // TODO: return error code
	}
	
	Serial_BufferedTransmitOneByte((unsigned char) 221);
	
	if (protocol == 1) {
		Serial_BufferedTransmitNBytes("NETDATA", strlen("NETDATA") + 1);
	} else if (protocol == 2) {
		Serial_BufferedTransmitNBytes("NETDATAUDP", strlen("NETDATAUDP") + 1);
	}
	
	sprintf(text, "%u", length);
	Serial_BufferedTransmitNBytes(text, strlen(text) + 1);
	
	sprintf(text, "%u", calculateChecksum(buf, length));
	Serial_BufferedTransmitNBytes(text, strlen(text) + 1);
	
	GetAppName(text);
	Serial_BufferedTransmitNBytes(text, strlen(text) + 1);
	
	while (counter < length) {
		int bytes = length - counter;
		
		if (bytes > 256) {
			bytes = 256;
		}
		
		while (Serial_BufferedTransmitNBytes(&buf[counter], bytes) == 2) {
			bytes = bytes / 2;
		}
		
		counter += bytes;
	}
}

NetData* receiveNetData() {
	return receiveNetDataTimeout(500);
}

NetData* receiveNetDataTimeout(int timeout) {
	int startTicks = RTC_GetTicks();
	NetData* data = NULL;
	unsigned char buf[11];
	int retry = 0;
	
	while (!RTC_Elapsed_ms(startTicks, timeout)) {
		unsigned char c;
		
		if (Serial_ReadOneByte(&c) == 0 && c == 221) {
			receiveStringTimeout(buf, 11, 1000);
			if (strcmp(buf, "NETDATA", 7) == 0) {
				int counter = 0;
				short received;
				data = (NetData*) malloc(sizeof(NetData));
				data->buf = NULL;
				
				if (!receiveStringTimeout(buf, 11, 1000)) {
					retry = 1;
					break;
				}
				data->length = (unsigned int) atol(buf);
				
				if (!receiveStringTimeout(buf, 11, 1000)) {
					retry = 1;
					break;
				}
				data->checksum = (unsigned int) atol(buf);
				
				if (!receiveStringTimeout(buf, 11, 1000)) {
					retry = 1;
					break;
				}
				memcpy(data->appName, buf, 9);
				
				startTicks = RTC_GetTicks();
				data->buf = (unsigned char*) malloc(data->length);
				while (1) {
					Serial_ReadNBytes(&data->buf[counter], data->length - counter, &received);
					counter += received;
					
					if (counter >= data->length) {
						if (data->checksum != calculateChecksum(data->buf, data->length)) {
							retry = 1;
						}
						
						break;
					}

					if (RTC_Elapsed_ms(startTicks, 1000)) {
						freeNetData(data);
						data = NULL;
						break;
					}
				}

				break;
			}
		}
	}
	
	if (retry == 1) {
		freeNetData(data);
		data = NULL;
	}
	
	return data;
}

void freeData(Data* data) {
	if (data != NULL) {
		if (data->buf != NULL) {
			free(data->buf);
			data->buf = NULL;
		}
		
		free(data);
	}
}

void freeNetData(NetData* data) {
	if (data != NULL) {
		if (data->buf != NULL) {
			free(data->buf);
			data->buf = NULL;
		}
		
		free(data);
	}
}

void freeNetList(NetworkList* netList) {
	if (netList != NULL) {
		freeNetList(netList->next);
		netList->next = NULL;
		
		if (netList->network.ssid != NULL) {
			free(netList->network.ssid);
			netList->network.ssid = NULL;
		}
		
		free(netList);
	}
}

NetworkList* getAvailableNetworks() {
	int status;
	Data* data = NULL;
	int counter;
	NetworkList *head = NULL, *tail = NULL;
	
	sendCommand((unsigned char*) "GETNETS");
	status = receiveStatus();
	
	if (status != STATUS_OK) {
		return NULL;
	}
	
	data = receiveDataTimeout(20000, 5);
	if (data == NULL) {
		return NULL;
	}
	
	counter = 0;
	while (counter < data->length) {
		NetworkList* netElement;
		int ssidLength;
		
		if (memchr(&data->buf[counter], 0, data->length - counter) == NULL) {
			break;
		}
		
		netElement = (NetworkList*) malloc(sizeof(NetworkList));
		netElement->network.ssid = NULL;
		netElement->next = NULL;
		
		ssidLength = strlen(&data->buf[counter]);
		
		netElement->network.ssid = (unsigned char*) malloc(ssidLength + 1);
		strncpy(netElement->network.ssid, &data->buf[counter], ssidLength);
		counter += ssidLength + 1;
		
		if (memchr(&data->buf[counter], 0, data->length - counter) == NULL) {
			freeNetList(netElement);
			netElement = NULL;
			break;
		}
		
		netElement->network.rssi = atoi(&data->buf[counter]);
		counter += strlen(&data->buf[counter]) + 1;
		
		if (memchr(&data->buf[counter], 0, data->length - counter) == NULL) {
			freeNetList(netElement);
			netElement = NULL;
			break;
		}
		
		netElement->network.encType = atoi(&data->buf[counter]);
		counter += strlen(&data->buf[counter]) + 1;
		
		if (head == NULL) {
			head = netElement;
			tail = netElement;
		} else {
			tail->next = netElement;
			tail = netElement;
		}
	}
	
	freeData(data);
	data = NULL;
	
	return head;
}

int connect(int index, unsigned char* pass) {
	unsigned char* buf[11];
	int status;
	
	sendCommand((unsigned char*) "CONNECT");
	sprintf(buf, "%d", index);
	Serial_BufferedTransmitNBytes(buf, strlen(buf) + 1);
	Serial_BufferedTransmitNBytes(pass, strlen(pass) + 1);
	
	status = receiveStatus();
	
	if (status == STATUS_OK) {
		return 1;
	}
	
	return 0;
}

int disconnect() {
	int status;
	
	sendCommand((unsigned char*) "DISCONNECT");
	status = receiveStatus();
	
	if (status == STATUS_OK) {
		return 1;
	}
	
	return 0;
}

Network getNetworkInfo() {
	int status;
	Network net;
	Data* data;
	
	net.ssid = NULL;
	net.rssi = 0;
	net.encType = 0;
	
	sendCommand((unsigned char*) "GETNETINFO");
	status = receiveStatus();
	
	if (status != STATUS_OK) {
		return net;
	}
	
	data = receiveDataTimeout(500, 1);
	if (data == NULL) {
		return net;
	}
	
	{
		int counter = 0;
		int ssidLength;
		
		if (memchr(&data->buf[counter], 0, data->length - counter) == NULL) {
			freeData(data);
			data = NULL;
			return net;
		}
		
		ssidLength = strlen(&data->buf[counter]);
		
		net.ssid = (unsigned char*) malloc(ssidLength + 1);
		strncpy(net.ssid, &data->buf[counter], ssidLength);
		counter += ssidLength + 1;
		
		if (memchr(&data->buf[counter], 0, data->length - counter) == NULL) {
			free(net.ssid);
			net.ssid = NULL;
			freeData(data);
			data = NULL;
			return net;
		}
		
		net.rssi = atoi(&data->buf[counter]);
	}
	
	freeData(data);
	data = NULL;
	
	return net;
}

int startAP(unsigned char* ssid, unsigned char* password) {
	int status;
	
	sendCommand((unsigned char*) "STARTAP");
	Serial_BufferedTransmitNBytes(ssid, strlen(ssid) + 1);
	Serial_BufferedTransmitNBytes(password, strlen(password) + 1);
	status = receiveStatus();
	
	if (status == STATUS_OK) {
		return 1;
	}
	
	return 0;
}

int stopAP() {
	int status;
	
	sendCommand((unsigned char*) "STOPAP");
	status = receiveStatus();
	
	if (status == STATUS_OK) {
		return 1;
	}
	
	return 0;
}

Data* getWebContent(int mode, unsigned char* host, unsigned char* url, int port) {
	sendCommand("WEBCONTENT");
	sendStringSerial(mode);
	sendStringSerial(host);
	sendStringSerial(url);
	sendStringSerial(port);

	return receiveData(10);
}