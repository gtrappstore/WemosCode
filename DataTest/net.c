#include "net.h"
#include "stdio.h"

void openSerial() {
    unsigned char mode[6];
    mode[0] = 0;
    mode[1] = 5;
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
				int counter = 0;
				short received;
				data = (Data*) malloc(sizeof(Data));
				
				if (!receiveStringTimeout(buf, 11, timeout)) {
					retry = 1;
					break;
				}
				data->length = atoi(buf);
				
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

void freeData(Data* data) {
	if (data != NULL) {
		if (data->buf != NULL) {
			free(data->buf);
			data->buf = NULL;
		}
		
		free(data);
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
		ssidLength = strlen(&data->buf[counter]);
		
		netElement->network.ssid = (unsigned char*) malloc(ssidLength + 1);
		strncpy(netElement->network.ssid, &data->buf[counter], ssidLength);
		counter += ssidLength + 1;
		
		if (memchr(&data->buf[counter], 0, data->length - counter) == NULL) {
			free(netElement->network.ssid);
			free(netElement);
			break;
		}
		
		netElement->network.rssi = atoi(&data->buf[counter]);
		counter += strlen(&data->buf[counter]) + 1;
		
		if (memchr(&data->buf[counter], 0, data->length - counter) == NULL) {
			free(netElement->network.ssid);
			free(netElement);
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
	
	return head;
}