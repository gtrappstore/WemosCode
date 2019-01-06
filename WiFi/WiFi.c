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
#include "stdio.h"
#include "..\WIFI_Lib\net.h"
#include "..\WIFI_Lib\status.h"
#include "..\WIFI_Lib\netUI.h"

// custom types

typedef int bool;
#define true 1
#define false 0

struct Network {
    bool isConnected;
    unsigned char* ssid;
    int rssi;
} network = {false, NULL, 0};
// prototypes

void drawInfoBar();
void drawBubble(int x, int y, int filled);
void connectToNetwork();
void browseFiles();
void downloadFile();
void getAppByID();
void sendCommand(unsigned char*);
void readString(int x, int y, bool search);
void serialSendString(unsigned char* buffer);
void serialReadString(unsigned char* buffer);
void serialReadStringTimeout(int timeout,unsigned char* buffer);
char keyToChar(int key, bool shift, bool alpha);
int getKey();
void drawProgressBar(int y, float value);
void sendData(unsigned char* buffer, int length);
void checksum(unsigned char* data,int length, unsigned char* csbuffer);
void itoa(int integer,int len,unsigned char* data);
void drawBrowseResults(unsigned char* results);
int showAppInfo();
void sendAcknowledgement(unsigned char* state);
void drawImage();

// syscalls

int Serial_ReadOneByte(unsigned char *result);
int Serial_ReadNBytes(unsigned char *result, int max_size, short *actually_transferred);
int Serial_GetReceivedBytesAvailable();
int Serial_ClearReceiveBuffer();
int Serial_BufferedTransmitOneByte(unsigned char byte_to_transmit);
int Serial_BufferedTransmitNBytes(unsigned char* bytes_to_transmit, int requested_count);
int Serial_ClearTransmitBuffer();

int Keyboard_GetPressedKey(short* matrixcode);
int Keyboard_GetKeyWait(int*column, int*row, int type_of_waiting, int timeout_period, int menu, unsigned short*keycode );
int Keyboard_PRGM_GetKey( unsigned char*result );
int Keyboard_KeyDown();
void Keyboard_ClrBuffer();

int App_RegisterAddins();

int RTC_GetTicks();
int RTC_Elapsed_ms( int start_value, int duration_in_ms );

int isAnyKeyPressed();

int PRGM_GetKey();

// structs
//Network network = {false, NULL, 0};

unsigned char arrowRight[3] = {0xE6, 0x9B, 0};
unsigned char loupe[3]={0xE6,0x50,0};
unsigned char buf[500];
unsigned char status[4];

unsigned char checkSumDebug[10]={'A','0','z','6','2','p','e','n','i','S'};
unsigned char checkSumDebugBuffer[11];

unsigned char testPicture[138]={0x42,0x4d,0x8a,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x3e,0x00,0x00,0x00,0x28,0x00,0x00,0x00,0x1e,0x00,0x00,0x00,0x13,0x00,0x00,0x00,0x01,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x4c,0x00,0x00,0x00,0xc3,0x0e,0x00,0x00,0xc3,0x0e,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x7f,0xff,0xff,0xfc,0x00,0x00,0x01,0xfc,0x3f,0x07,0xe1,0xfc,0x21,0x04,0x21,0xfc,0x21,0x04,0x21,0xfc,0x27,0xdf,0x21,0xfc,0x25,0x55,0x21,0xfc,0x3f,0x57,0xe0,0x04,0x04,0x51,0x00,0x04,0x07,0xdf,0x01,0x84,0x00,0x00,0x02,0x44,0x70,0xee,0x38,0x44,0x49,0x09,0x49,0xc4,0x45,0xe9,0x4a,0x44,0x45,0x2e,0x4a,0x44,0x44,0xc8,0x49,0xc4,0x48,0x08,0x00,0x04,0x70,0x08,0x00,0x04,0x00,0x00,0x00,0x00};

bool shouldRequestNetInfo = true;

const FONTCHARACTER filename[] = {'\\','\\','f','l','s','0','\\','T','E','S','T','.','g','1','a',0};
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
    int selection = 0, lastTicks, lastKeyTicks;
    int col, row;
    unsigned short keycode;
    bool menu = false;
    DISPBOX dispBox;

    while(Keyboard_KeyDown()) {
    }

    Bdisp_AllClr_DDVRAM();


    while(true) {
        //GetKey(&key);
        //sprintf(buf, "Key: %d", key);
        //locate(2, 2);
        //Bdisp_AllClr_DDVRAM();
        //Print(buf);
		drawImage();
		GetKey(&key);
    }


    openSerial();
    Serial_ClearReceiveBuffer();
    Serial_ClearTransmitBuffer();

    drawInfoBar();
    lastTicks = RTC_GetTicks();

    while(1) {
        int line;
		
        dispBox.left = 0;
        dispBox.top = 7;
        dispBox.right = 127;
        dispBox.bottom = 63;

        Bdisp_AreaClr_VRAM(&dispBox);
		
		memset(checkSumDebugBuffer,0,11);
		checksum(checkSumDebug,10,checkSumDebugBuffer);
		locate(1,7);
		Print(checkSumDebugBuffer);
       
        for (line = 0; line < 4; line = line + 1) {
            int x = 3;
            if (line == selection) {
                x = x + 1;
            }

            switch(line) {
                case 0:
                    locate(x, 3 + line);
                    Print((unsigned char*) "Connect");
                    break;
                case 1:
                    locate(x, 3 + line);
                    Print((unsigned char*) "Disconnect");
                    break;
                case 2:
                    locate(x, 3 + line);
                    Print((unsigned char*) "Browse");
                    break;
                 case 3:
                    locate(x, 3 + line);
                    Print((unsigned char*) "Get by ID");
                    break;
            }
        }

        locate(2, 3 + selection);
        Print(arrowRight);        

        receiveStatusTimeout(20);
        if(memcmp(status,"OF",2)==0){
            shouldRequestNetInfo=true;
        }
              
        if (RTC_Elapsed_ms(lastTicks, 2500) && shouldRequestNetInfo) {
            
            getNetworkInfo();
            drawInfoBar();
            lastTicks = RTC_GetTicks();
        }


        Bdisp_PutDisp_DD();
		
        if (RTC_Elapsed_ms(lastKeyTicks, 200) && Keyboard_GetKeyWait(&col, &row, 1, 0, 0, &keycode)) {
            if (col == 4 && row == 9) { // MENU key was pressed
                Serial_Close(1);
                Keyboard_GetKeyWait(&col, &row, 0, 0, 0, &keycode);
                openSerial();
            } else if (col == 2 && row == 9) {
                selection = selection - 1;
            } else if (col == 3 && row == 8) {
                selection = selection + 1;
            } else if (col == 3 && row == 2) {
                switch (selection) {
                    case 0:
						Bdisp_AllClr_DDVRAM();
						locate(1, 1);
						Print((unsigned char*) "Scanning...");
						Bdisp_PutDisp_DD();
						connectToNetwork();         
                        break;
                    case 1:
                        sendCommand("DISCONNECT");
                        receiveStatus();
						
                        if (memcmp(status,"OK",2)==0){
                            disconnect();
							shouldRequestNetInfo = false;
                        }
                        break;
                    case 2:
                        browseFiles();            
                        break;
                    case 3:
                        getAppByID();
                        break;
                }
            }

            if (selection < 0) {
                selection = 3;
            } else if (selection > 3) {
                selection = 0;
            }

            lastKeyTicks = RTC_GetTicks();
        }

        Keyboard_ClrBuffer(); // clear the buffer to prevent strange behaviour when hitting the menu key
    }

	closeSerial();

    return 1;
}

void drawInfoBar() { //überarbeitet
    int x, y, bubble, key;
    DISPBOX dispBox;

    dispBox.left = 0;
    dispBox.top = 0;
    dispBox.right = 127;
    dispBox.bottom = 6;

    Bdisp_AreaClr_VRAM(&dispBox);

    for (y = 0; y < 7; y = y + 1) {
        for (x = 2 - ((y + 2) % 3); x < 127; x = x + 3) {
            Bdisp_SetPoint_VRAM(x, y, 1);
        }
    }

    if (network.isConnected) {
        int bubbleCount = (int) (12.0 / 100.0 * (network.rssi + 100.0));

        dispBox.left = 6;
        dispBox.top = 0;
        dispBox.right = dispBox.left + 6 * strlen(network.ssid);
        dispBox.bottom = 6;
        Bdisp_AreaClr_VRAM(&dispBox);

        locate(2, 1);
        Print(network.ssid);

        dispBox.left = 90;
        dispBox.top = 0;
        dispBox.right = dispBox.left + 30;
        dispBox.bottom = 6;
        Bdisp_AreaClr_VRAM(&dispBox);

        for (bubble = 0; bubble < 5; bubble = bubble + 1) {
            int filled = (bubble < bubbleCount);
            drawBubble(bubble * 6 + dispBox.left + 1, 1, filled);
        }
    }

    dispBox.left = 0;
    dispBox.top = 0;
    dispBox.right = 127;
    dispBox.bottom = 6;

    Bdisp_PutDispArea_DD(&dispBox);
}

void drawBubble(int x, int y, int filled) {
    Bdisp_DrawLineVRAM(x + 1, y, x + 3, y);
    Bdisp_DrawLineVRAM(x, y + 1, x + 4, y + 1);
    Bdisp_DrawLineVRAM(x, y + 2, x + 4, y + 2);
    Bdisp_DrawLineVRAM(x, y + 3, x + 4, y + 3);
    Bdisp_DrawLineVRAM(x + 1, y + 4, x + 3, y + 4);

    if (filled == 0) {
        DISPBOX clearBox;
        clearBox.left = x + 1;
        clearBox.top = y + 1;
        clearBox.right = x + 3;
        clearBox.bottom = y + 3;

        Bdisp_AreaClr_VRAM(&clearBox);
    }
}

void connectToNetwork() {    

    unsigned int key;
    int counter = 0, lines = 0, lineBegin = 0, selection = 0, scroll = 0, lastTicks, a = 0, numChars = 0;
    unsigned char text[22];
	NetworkList* nets;
	NetworkSelectionUI nsui;
	int direction = 0;
	unsigned char passwordBuf[65]; // WiFi passwords are at most 64 chars

    Serial_ClearReceiveBuffer();

    
    lastTicks = RTC_GetTicks();
    counter = 0;
	
	nets = getAvailableNetworks();

	Bdisp_AllClr_DDVRAM();
   
	nsui = initNetworkSelectionUI(1, 2, 21, 7, nets);

	while (1) {
		Bdisp_AllClr_VRAM();
		locate(1, 1);
		Print("Select Network:");
		drawNetworkSelectionUI(&nsui, direction);
		GetKey(&key);

		if (key == KEY_CTRL_UP) {
			direction = -1;
		}
		else if (key == KEY_CTRL_DOWN) {
			direction = 1;
		}
		else if (key == KEY_CTRL_EXIT) {
			freeNetList(nets);
			nets = NULL;
			//return 0; DEBUG- wieder auskommentieren -> als Abbruch werten
		}
		else if (key == KEY_CTRL_EXE) {
			break;
		}
	}

	freeNetList(nets);
	nets = NULL;

	Bdisp_AllClr_DDVRAM();

	memset(passwordBuf, 0, 65);
	Cursor_SetFlashOn(0);

	while (1) {
		int x = 0;

		if (strlen(passwordBuf) > 20) {
			x = strlen(passwordBuf) - 20;
		}

		Bdisp_AllClr_VRAM();
		locate(1, 1);
		Print("Password:");
		locate(1, 3);
		Print(&passwordBuf[x]);
		locate(strlen(passwordBuf) + 1, 3);

		GetKey(&key);

		if (key == KEY_CTRL_DEL && strlen(passwordBuf) >= 1) {
			passwordBuf[strlen(passwordBuf) - 1] = 0;
		}
		else if (key == KEY_CTRL_EXIT) {
			//return 0; DEBUG- wieder auskommentieren -> als Abbruch werten
		}
		else if (key == KEY_CTRL_EXE) {
			break;
		}
		else if (key < 128 && strlen(passwordBuf) < 64) {
			passwordBuf[strlen(passwordBuf)] = (unsigned char)key;
		}
	}

	connect(nsui.selection, passwordBuf);

    shouldRequestNetInfo=false;
}

void browseFiles() {
    unsigned int key;
    int counter = 0, lines = 0, lineBegin = 0, selection = 0, scroll = 0, i;
    unsigned char text[22];

	unsigned char* result;
	
	Bdisp_AllClr_DDVRAM();
	locate(21,1);
	Print(loupe);
	locate(1,1);
	Bdisp_DrawLineVRAM(0,7,127,7);
	Bdisp_PutDisp_DD;
	readString(1,1,true);
	
	memcpy(text,buf,strlen(buf)+1);
	
	Serial_ClearReceiveBuffer();
    Serial_ClearTransmitBuffer();
	
	sendCommand("SEARCH");
	receiveStatus();					
	if (memcmp(status,"OK",2)!=0){
		return;
	}           
	Serial_BufferedTransmitNBytes(text,strlen(text)+1);
	
    //Bdisp_AllClr_DDVRAM();

    serialReadStringTimeout(30000,buf);
	result=(unsigned char*)malloc(strlen(buf)+1);
	memcpy(result,buf,strlen(buf)+1);
	
	if(strlen(buf)==0){
		Bdisp_AllClr_DDVRAM();
		locate(0,3);
		Print((unsigned char*)"KEIN ERGEBNIS GEFUNDEN");
		Bdisp_PutDisp_DD();
		GetKey(&key);
		return;
	}
	drawBrowseResults(result);
}

void drawBrowseResults(unsigned char* results) {
	int counter = 0, lines = 0, lineBegin = 0, selection = 0, scroll = 0, i;
	unsigned char text[22];
	unsigned int key;
	
	unsigned int dl=0;

	Bdisp_AllClr_DDVRAM();
	locate(21, 1);
	Print(loupe);
	locate(1, 1);
	Bdisp_DrawLineVRAM(0, 7, 127, 7);
	Bdisp_PutDisp_DD;
	
	do {
		DISPBOX listArea;
		listArea.left = 0;
		listArea.top = 8;
		listArea.right = 127;
		listArea.bottom = 63;

		Bdisp_AreaClr_VRAM(&listArea);

		for (counter = 0, lines = 0, lineBegin = 0; results[counter] != 0; counter = counter + 1) {
			if (results[counter] == '\n') {
				lines = lines + 1;

				if (lines > scroll && lines <= scroll + 7) {
					memset(text, 0, sizeof text);
					memcpy(text, &results[lineBegin], counter - lineBegin);

					for (i = 0; i < counter - lineBegin; i = i + 1) {
						if (text[i] == '\t') {
							text[i] = ' ';
						}
					}

					locate(1, 1 + lines - scroll);
					Print(text);
				}

				lineBegin = counter + 1;
			}
		}

		Bdisp_AreaReverseVRAM(0, 8 * (selection - scroll + 1), 127, 8 * (selection - scroll + 1) + 7);

		GetKey(&key);

		if (key == 30018) {
			selection = selection - 1;
		}
		else if (key == 30023) {
			selection = selection + 1;
		}
		else if (key == 30009) {
			browseFiles();
			return;
		}
		else if (key == 30002) {
			return;
		}

		if (selection < 0) {
			selection = 0;
		}
		else if (selection >= lines) {
			selection = lines - 1;
		}
		else {
			if (selection < scroll) {
				scroll = scroll - 1;
			}
			else if (selection >= 8 + scroll) {
				scroll = scroll + 1;
			}
		}
	} while (key != 30004);

	for (counter = 0, lines = 0, lineBegin = 0; buf[counter] != 0; counter = counter + 1) {
		if (buf[counter] == '\n') {
			lines = lines + 1;
			lineBegin = counter + 1;
		}

		if (lines >= selection && buf[counter] == '\t') {
			memset(text, 0, sizeof text);
			memcpy(text, &buf[lineBegin], counter - lineBegin);

			memset(buf, 0, sizeof buf);
			memcpy(buf, text, sizeof text);

			break;
		}
	}
	
	dl=showAppInfo();
	if(dl==0){
		drawBrowseResults(results);
	}
	return;
	//downloadFile();
}

int showAppInfo(){
	unsigned int key=0;
	
	unsigned char* picture;
	unsigned short transmitted;
	unsigned char* infoText;
	unsigned char* appName;
	unsigned char* author;
	unsigned char* downloadCount;
	
	sendCommand("GETAPPINFO");
	receiveStatus();
	
	receiveStatusTimeout(20);
	if(memcmp(status,"OK",2)!=0){
		return 0;
	}
	
	Bdisp_AllClr_DDVRAM();
	
	//picture=receiveData(5000,&transmitted); DEBUG- wieder auskommentieren
	serialReadString(infoText);
	
	drawImage(picture);
	
	
	GetKey(&key);

	if (key == 30002) {
		return 0;
	}
	else if (key == 30004) {
		downloadFile();
		return 1;
	}
}

void downloadFile() {
    int fileHandle;
    unsigned int counter = 0, contentLength = 0;
    unsigned char data[256];
    short dataSize;

	sendCommand("DOWNLOAD");
               
    receiveStatus();
						
    if (memcmp(status,"OK",2)!=0){
		return;
    }
    serialSendString(buf);

    Bdisp_AllClr_DDVRAM();

    memset(buf, 0, sizeof buf);

    while(1) {
        if (Serial_GetReceivedBytesAvailable() > 0) {
            unsigned char c;
            Serial_ReadOneByte(&c);

            if (c == 0) {
                break;
            } else {
                buf[counter] = c;
                counter = counter + 1;
            }
        }
    }

    contentLength = atoi(buf);

    counter = 0;

    Bfile_CreateFile(filename, contentLength);
    fileHandle = Bfile_OpenFile(filename, _OPENMODE_READWRITE);

    while(counter < contentLength) {
		unsigned char* d;
		short received;
		int writeResult;
		
		unsigned int key=0;
		
        //d=receiveData(5000,&received); DEBUG- wieder auskommentieren
		if(d==0){
			Bfile_CloseFile(fileHandle);
			//File löschen
			
			/*Bdisp_AllClr_DDVRAM();
			locate(1,8);
			memset(buf,0,100);
			memcpy(buf,counter,4);
			Print(buf);
			locate(1,6);
			memset(buf,0,100);
			memcpy(buf,checksumRec,4);
			Print(buf);*/
			GetKey(&key);
			
			return;
		}
		counter = counter + received;

		Bdisp_AllClr_DDVRAM();

		locate(4, 2);
		Print((unsigned char*) "Downloading...");

		drawProgressBar(30, counter * 1.0 / contentLength);

		sprintf(buf, "%d/%d", counter, contentLength);
		locate(22 - strlen(buf), 8);
		Print(buf);

		writeResult = Bfile_WriteFile(fileHandle, d, received);
		free(d);
		
		Bdisp_PutDisp_DD();
		
    }

    Bfile_CloseFile(fileHandle);

    App_RegisterAddins();
}

void getAppByID(){

}

void sendCommand(unsigned char* command) {  //überarbeitet
    Serial_BufferedTransmitOneByte((unsigned char) 219);
    Serial_BufferedTransmitNBytes(command, strlen(command)+1);
}

void sendAcknowledgement(unsigned char* state){	
	Serial_BufferedTransmitOneByte((unsigned char)220);
	serialSendString(state);
}

void readString(int x, int y, bool search) {
    unsigned int key;
    int numChars = 0;
    DISPBOX clearBox;
    memset(buf, 0, sizeof buf);

    Keyboard_ClrBuffer();

    while(1) {
        GetKey(&key);

        clearBox.left = (x - 1) * 6;
        clearBox.top = (y - 1) * 8;
        clearBox.right = (x + numChars - 1) * 6;
        clearBox.bottom = y * 8;

        if (key == 30004 || key == 30002) { // EXE Key
            break;
        } else if (key == 30025) {
            numChars = numChars - 1;
            if (numChars < 0) {
                numChars = 0;
            }

            buf[numChars] = 0;
        } else if (key == 30006 || key == 30007) { // shift alpha

        } else {
            unsigned char c = (unsigned char) key;
            buf[numChars] = c;
            numChars = numChars + 1;
        }
		
		
		
        Bdisp_AreaClr_DDVRAM(&clearBox);
        locate(x, y);
        Print(buf);
        if(search==true){
             Bdisp_DrawLineVRAM(0,7,127,7);
        }
    }
}

void serialSendString(unsigned char* buffer) {
    unsigned int counter = 0;
	
	Serial_BufferedTransmitNBytes(buffer,strlen(buffer)+1);
}

void serialReadString(unsigned char* buffer){ //überarbeitet
	serialReadStringTimeout(100, buffer);
}

void serialReadStringTimeout(int timeout, unsigned char* buffer) { //überarbeitet
    unsigned int counter = 0;
    int lastTicks = RTC_GetTicks();
    memset(buffer, 0, sizeof buffer);

    while(true) {
        
        if (Serial_GetReceivedBytesAvailable() > 0) {
            unsigned char c;
            Serial_ReadOneByte(&c);

            buffer[counter] = c;
            counter = counter + 1;

            if (c == 0) {
                return;
            }
        }

        if (RTC_Elapsed_ms(lastTicks, timeout)) { // 100 ms timeout Standard
            buffer[counter]=0;
            return;
        }
    }
}

char keyToChar(int key, bool shift, bool alpha) {
    unsigned char c = 1;

    if (shift) {

    } else if (alpha) {
        switch (key) {
            case 30001:
                c = 'A';
                break;
            case 149:
                c = 'B';
                break;
            case 133:
                c = 'C';
                break;
            case 129:
                c = 'D';
                break;
            case 130:
                c = 'E';
                break;
            case 131:
                c = 'F';
                break;
            case 187:
                c = 'G';
                break;
            case 30046:
                c = 'H';
                break;
            case 40:
                c = 'I';
                break;
            case 41:
                c = 'J';
                break;
            case 44:
                c = 'K';
                break;
            case 14:
                c = 'L';
                break;
            case 55:
                c = 'M';
                break;
            case 56:
                c = 'N';
                break;
            case 57:
                c = 'O';
                break;
            case 52:
                c = 'P';
                break;
            case 53:
                c = 'Q';
                break;
            case 54:
                c = 'R';
                break;
            case 169:
                c = 'S';
                break;
            case 185:
                c = 'T';
                break;
            case 49:
                c = 'U';
                break;
            case 50:
                c = 'V';
                break;
            case 51:
                c = 'W';
                break;
            case 137:
                c = 'X';
                break;
            case 153:
                c = 'Y';
                break;
            case 48:
                c = 'Z';
                break;
            case 46:
                c = ' ';
                break;
            case 15:
                c = '\"';
                break;
        }
    } else {
        c = (unsigned char) key;
    }

    return c;
}

int getKey() {
    int col, row;
    int lastKeyTicks, lastClrTicks;
    unsigned short keycode;

    Bdisp_PutDisp_DD();

    Keyboard_ClrBuffer(); // clear the buffer to prevent strange behaviour when hitting the menu key

    while(true) {
        lastKeyTicks = RTC_GetTicks();
        while(!RTC_Elapsed_ms(lastKeyTicks, 100)) {
        }

        if (Keyboard_GetKeyWait(&col, &row, 1, 0, 0, &keycode)) {
            if (col == 4 && row == 9) { // MENU key was pressed
                Serial_Close(1);
                Keyboard_GetKeyWait(&col, &row, 0, 0, 0, &keycode);
                openSerial();
            } else {
                break;
            }
        }

        Keyboard_ClrBuffer();
    }

    Keyboard_ClrBuffer();

    return (row - 1) * 7 + col - 1;
}

void drawProgressBar(int y, float value) {
    int length = (int) (121.0  * value);
    DISPBOX dispBox;

    dispBox.left = 1;
    dispBox.top = y;
    dispBox.right = 126;
    dispBox.bottom = y + 5;

     Bdisp_AreaClr_DD(&dispBox);

    Bdisp_DrawLineVRAM(1, y, 126, y);
    Bdisp_DrawLineVRAM(1, y + 5, 126, y + 5);
    Bdisp_DrawLineVRAM(1, y, 1, y + 5);
    Bdisp_DrawLineVRAM(126, y, 126, y + 5);

    Bdisp_DrawLineVRAM(3, y + 2, 3 + length, y + 2);
    Bdisp_DrawLineVRAM(3, y + 3, 3 + length, y + 3);

    Bdisp_PutDispArea_DD(&dispBox);
}

void sendData(unsigned char* buffer, int length){
	do{
		unsigned char cs[4];
		unsigned char* len[10];
		int counter=0;
		int transmitCount;
		
		sendCommand("DATA");
		
		itoa(length,10,len);
	
		Serial_BufferedTransmitNBytes(len,10);
		checksum(buffer,length,cs);
		Serial_BufferedTransmitNBytes(cs,4);
		
		while(counter<length){
			transmitCount=length-counter;
			while(Serial_BufferedTransmitNBytes(&buffer[counter],transmitCount)!=0){
				transmitCount=(int)transmitCount/2;
				if(transmitCount==0){
					transmitCount=1;
				}
			}
			counter+=transmitCount;
		}
		
		receiveStatus();
	}while(memcmp(status,"OK",2)==0);
}

void checksum(unsigned char* data,int length, unsigned char* csbuffer){
	int i;
	int checksum=0;
	unsigned char* checksumBuf[5];
	
	for(i=0;i<length;i=i+1){
		checksum+=(int)data[i];
	}
	
	checksum=checksum%10000;
	
	sprintf(checksumBuf,"%04d",checksum);
	memcpy(csbuffer,checksumBuf,4);
	//itoa(checksum,4,csbuffer);
}

void itoa(int integer,int len,unsigned char* data){
	int i=0;
	int z=0;
	
	for(i=len-1;i>=0;i=i-1){
		z=(int)(integer/pow(10,i));
		data[len-1-i]=z+48;
		integer=integer-(z*pow(10,i));
	}
}

void drawImage(unsigned char* d){
	unsigned int startTicks = RTC_GetTicks();
	unsigned int width;
	unsigned int lineLengthBit;
	unsigned int heigth;
	unsigned int line;
	unsigned int y;
	unsigned int bitCounter;
	
	unsigned int xOffset=5;
	unsigned int yOffset=5;
	
	unsigned int i;
	unsigned int counter;
	
	unsigned int startOffset;
	
	unsigned short  actuallyReceived;
	//unsigned char* d;
	unsigned char currentByteToRead;
	unsigned char modifiedByte;
	
	//d=receiveData(60000,&actuallyReceived);
	//d=testPicture;
	
	Bdisp_AllClr_DDVRAM;
	
	startOffset=(int)d[10];
	width=(int)d[18];
	heigth=(int)d[22];
	
	counter=startOffset;
	lineLengthBit=width+(32-(width%32));
	
	for(i=heigth;i>0;i=i-1){
		for(y=0;y<(lineLengthBit/8);y++){
			currentByteToRead=d[counter];
			for(bitCounter=0;bitCounter<8;bitCounter=bitCounter+1){
				modifiedByte=currentByteToRead<<bitCounter;
				modifiedByte=modifiedByte>>7;
				if(modifiedByte==1){
					Bdisp_AreaReverseVRAM(y*8+bitCounter+yOffset,i+xOffset,y*8+bitCounter+yOffset,i+xOffset);
					Bdisp_PutDisp_DD();
				}
			}
			counter=counter+1;
		}
	}
	Bdisp_PutDisp_DD();
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

