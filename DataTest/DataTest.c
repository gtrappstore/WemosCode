/*****************************************************************/
/*                                                               */
/*   CASIO fx-9860G SDK Library                                  */
/*                                                               */
/*   File name : [ProjectName].c                                 */
/*                                                               */
/*   Copyright (c) 2006 CASIO COMPUTER CO., LTD.                 */
/*                                                               */
/*****************************************************************/
#include "fxlib.h"
#include "stddef.h"
#include "net.h"

#include "stdio.h"

typedef int bool;
#define true 1
#define false 0

// structs
typedef struct {
	unsigned char* msg;
	int user;
} Message;

typedef struct MessageList {
	Message message;
	struct MessageList* next;
} MessageList;

// prototypes
void connectScreen();
void drawMessages(MessageList* messageList);
char getCharFromInput(int column, int row, bool alpha);


//****************************************************************************
//  AddIn_main (Sample program main function)
//
//  param   :   isAppli   : 1 = This application is launched by MAIN MENU.
//                        : 0 = This application is launched by a strip in eACT application.
//
//              OptionNum : Strip number (0~3)
//                         (This parameter is only used when isAppli parameter is 0.)
//
//  retval  :   1 = No error / 0 = Error
//
//****************************************************************************
int AddIn_main(int isAppli, unsigned short OptionNum)
{
	unsigned int key;
	NetworkList* netList = NULL;
	NetData* data = NULL;
	int startMillis;
	unsigned char text[22];
	unsigned char buf[512];
	MessageList* messageList = NULL;
	NetData* netData = NULL;
	int col, row;
	unsigned short keycode;
	bool alpha = false;
	int lastKeyTicks = RTC_GetTicks();
	bool needRedraw = true;
	
	while (true) {
		Bdisp_AllClr_DDVRAM();
	
		locate(1, 1);
		Print((unsigned char*) "F1 - Create Chatroom");
		locate(1, 2);
		Print((unsigned char*) "F2 - Join Chatroom");
	
		GetKey(&key);
	
		if (key == KEY_CTRL_F1) {
			openSerial();
			startAP("ESP Chat", "123456789");
			break;
		} else if (key == KEY_CTRL_F2) {
			openSerial();
			connectScreen();
			break;
		}
	}

//	openSerial();
//	connectScreen();
//	startAP("ESP Test", "123456789");
	
	memset(buf, 0, 512);

	Keyboard_ClrBuffer();
	
	while (1) {
		int index = 0;
		
		if (RTC_Elapsed_ms(lastKeyTicks, 200) && Keyboard_GetKeyWait(&col, &row, 1, 0, 0, &keycode) == 1) {
			if (col == 4 && row == 9) { // MENU
				Keyboard_GetKeyWait(&col, &row, 0, 0, 0, &keycode); // go to the menu
			} else if (col == 3 && row == 2) { // EXE
				MessageList* msgElement = (MessageList*) malloc(sizeof(MessageList));
				msgElement->message.msg = (unsigned char*) malloc(strlen(buf) + 1);
				memcpy(msgElement->message.msg, buf, strlen(buf) + 1);
				msgElement->message.user = 0;
				
				msgElement->next = messageList;
				messageList = msgElement;
				
				memset(buf, 0, 512);
				needRedraw = true;
				
				sendNetData(msgElement->message.msg, strlen(msgElement->message.msg));
			} else if (col == 4 && row == 5) { // DEL
				if (strlen(buf) > 0) {
					buf[strlen(buf) - 1] = 0;
					needRedraw = true;
				}
			} else if (col == 7 && row == 8) { // ALPHA
					alpha = !alpha;
			} else {
				unsigned char c = getCharFromInput(col, row, alpha);
				
				if (c != 0) {
					buf[strlen(buf)] = c;
					needRedraw = true;
				}
			}
			
			lastKeyTicks = RTC_GetTicks();
		}
		
		Keyboard_ClrBuffer();
		
		netData = receiveNetDataTimeout(10);
		if (netData != NULL) {
			MessageList* msgElement = (MessageList*) malloc(sizeof(MessageList));
			msgElement->message.msg = (unsigned char*) malloc(netData->length + 1);
			memcpy(msgElement->message.msg, netData->buf, netData->length);
			msgElement->message.msg[netData->length] = 0;
			msgElement->message.user = 1;
			
			msgElement->next = messageList;
			messageList = msgElement;
			
			needRedraw = true;
		}
		
		if (needRedraw) {
			Bdisp_AllClr_DDVRAM();
			index = strlen(buf);
			if (index > 0) {
				if (index > 32) {
					index -= 32;
				} else {
					index = 0;
				}
			
				PrintMini(0, 59, &buf[index], MINI_OVER);
			}
			Bdisp_DrawLineVRAM(0, 57, 127, 57);
			drawMessages(messageList);
			Bdisp_PutDisp_DD();
			
			needRedraw = false;
		}
	}
	
	
	
	netList = getAvailableNetworks();

	locate(1, 1);

	if (netList != NULL) {
		Print((unsigned char*) "RECEIVED");

		for (key = 0; netList != NULL; key++, netList = netList->next) {
			unsigned char text[51];

			locate(1, key + 2);
			sprintf(text, "%s %d %d", netList->network.ssid, netList->network.rssi, netList->network.encType);
			Print(text);
//            Print(netList->network.ssid);
		}

		GetKey(&key);
		connect(key - 49, (unsigned char*) "123456789");

		Sleep(10000);
		sendNetData("Hurensohn", 9);
		startMillis = RTC_GetTicks();
		data = receiveNetData();
		sprintf(text, "%d", RTC_GetTicks() - startMillis);
		Bdisp_AllClr_VRAM();
		locate(1, 1);
		if (data != NULL) {
			Print(data->buf);
		} else {
			Print((unsigned char*) "No Data");
		}
		locate(1, 3);
		Print(text);
		disconnect();
	} else {
		Print((unsigned char*) "ERROR");
	}

	while(1){
		GetKey(&key);
	}

	return 1;
}

void connectScreen() {
	unsigned int key;
	NetworkList* netList = NULL;
	unsigned char text[100];
	
	netList = getAvailableNetworks();
		
	Bdisp_AllClr_VRAM();

	if (netList != NULL) {
		int lineIndex = 0;
		NetworkList* netElement = netList;
		
		for (lineIndex = 0; netElement != NULL && lineIndex < 8; lineIndex++, netElement = netElement->next) {
			locate(1, lineIndex + 1);
			sprintf(text, "%s %d %d", netElement->network.ssid, netElement->network.rssi, netElement->network.encType);
			Print(text);
		}

		GetKey(&key);
		connect(key - 49, (unsigned char*) "123456789");
		
		Bdisp_AllClr_VRAM();
		locate(1, 1);
		Print((unsigned char*) "Connecting...");
		Bdisp_PutDisp_DD();

		Sleep(2000);
	} else {
		locate(1, 1);
		Print((unsigned char*) "No Networks");
		GetKey(&key);
	}
	
	Bdisp_AllClr_DDVRAM();
	
	freeNetList(netList);
}

void drawMessages(MessageList* messageList) {
	int lineIndex = 8;
	MessageList* msgElement = messageList;
	
	for (lineIndex = 8; lineIndex >= 0 && msgElement != NULL; lineIndex--, msgElement = msgElement->next) {
		int x = 0;
		if (msgElement->message.user == 0) {
			x = 63;
		}
		PrintMini(x, lineIndex * 6, msgElement->message.msg, MINI_OVER);
	}
}

char getCharFromInput(int column, int row, bool alpha) {
	switch (row) {
		case 2:
			switch (column) {
				case 4:
					return 0x87;
					break;
				case 5:
					if (alpha) {
						return 0x22;
					} else {
						return 0x0F;
					}
					break;
				case 6:
					if (alpha) {
						return ' ';
					} else {
						return 0x2E;
					}
					break;
				case 7:
					if (alpha) {
						return 'Z';
					} else {
						return '0';
					}
					break;
			}
			break;
		case 3:
			switch (column) {
				case 3:
					if (alpha) {
						return 'Y';
					} else {
						return '-';
					}
					break;
				case 4:
					if (alpha) {
						return 'X';
					} else {
						return '+';
					}
					break;
				case 5:
					if (alpha) {
						return 'W';
					} else {
						return '3';
					}
					break;
				case 6:
					if (alpha) {
						return 'V';
					} else {
						return '2';
					}
					break;
				case 7:
					if (alpha) {
						return 'U';
					} else {
						return '1';
					}
					break;
			}
			break;
		case 4:
			switch (column) {
				case 3:
					if (alpha) {
						return 'T';
					} else {
						return 0xB9;
					}
					break;
				case 4:
					if (alpha) {
						return 'S';
					} else {
						return 0xA9;
					}
					break;
				case 5:
					if (alpha) {
						return 'R';
					} else {
						return '6';
					}
					break;
				case 6:
					if (alpha) {
						return 'Q';
					} else {
						return '5';
					}
					break;
				case 7:
					if (alpha) {
						return 'P';
					} else {
						return '4';
					}
					break;
			}
			break;
		case 5:
			switch (column) {
				case 5:
					if (alpha) {
						return 'O';
					} else {
						return '9';
					}
					break;
				case 6:
					if (alpha) {
						return 'N';
					} else {
						return '8';
					}
					break;
				case 7:
					if (alpha) {
						return 'M';
					} else {
						return '7';
					}
					break;
			}
			break;
		case 6:
			switch (column) {
				case 2:
					return 'L';
					break;
				case 3:
					if (alpha) {
						return 'K';
					} else {
						return ',';
					}
					break;
				case 4:
					if (alpha) {
						return 'J';
					} else {
						return ')';
					}
					break;
				case 5:
					if (alpha) {
						return 'I';
					} else {
						return '(';
					}
					break;
				case 6:
					return 'H';
					break;
				case 7:
					return 'G';
					break;
			}
			break;
		case 7:
			switch (column) {
				case 2:
					return 'F';
					break;
				case 3:
					return 'E';
					break;
				case 4:
					return 'D';
					break;
				case 5:
					return 'C';
					break;
				case 6:
					return 'B';
					break;
				case 7:
					return 'A';
					break;
			}
			break;
	}
	
	return 0;
}




//****************************************************************************
//**************                                              ****************
//**************                 Notice!                      ****************
//**************                                              ****************
//**************  Please do not change the following source.  ****************
//**************                                              ****************
//****************************************************************************


#pragma section _BR_Size
unsigned long BR_Size;
#pragma section


#pragma section _TOP

//****************************************************************************
//  InitializeSystem
//
//  param   :   isAppli   : 1 = Application / 0 = eActivity
//              OptionNum : Option Number (only eActivity)
//
//  retval  :   1 = No error / 0 = Error
//
//****************************************************************************
int InitializeSystem(int isAppli, unsigned short OptionNum)
{
	return INIT_ADDIN_APPLICATION(isAppli, OptionNum);
}

#pragma section

