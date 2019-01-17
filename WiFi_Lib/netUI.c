#include "netUI.h"
#include "stddef.h"
#include "dispbios.h"
#include "keybios.h"
#include "stdio.h"

int createNetworkScreen(unsigned char* title) {
	unsigned int key;
	unsigned char ssidBuf[33]; // WiFi ssids are at most 32 chars
	unsigned char passwordBuf[65]; // WiFi passwords are at most 64 chars
	
	memset(ssidBuf, 0, 33);
	Cursor_SetFlashOn(0);
	
	while (1){
		int x = 0;
		
		if (strlen(ssidBuf) > 20) {
			x = strlen(ssidBuf) - 20;
		}
		
		Bdisp_AllClr_VRAM();
		locate(1, 1);
		if (title == NULL) {
			Print("Network name:");
		} else {
			Print(title);
		}
		locate(1, 3);
		Print(&ssidBuf[x]);
		locate(strlen(ssidBuf) + 1, 3);
		
		GetKey(&key);
		
		if (key == KEY_CTRL_DEL && strlen(ssidBuf) >= 1) {
			ssidBuf[strlen(ssidBuf) - 1] = 0;
		} else if (key == KEY_CTRL_EXIT) {
			return 0;
		} else if (key == KEY_CTRL_EXE) {
			break;
		} else if (key < 128 && strlen(ssidBuf) < 32) {
			ssidBuf[strlen(ssidBuf)] = (unsigned char) key;
		}
	}
	
	memset(passwordBuf, 0, 65);
	Cursor_SetFlashOn(0);
	
	while (1){
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
		} else if (key == KEY_CTRL_EXIT) {
			return 0;
		} else if (key == KEY_CTRL_EXE) {
			break;
		} else if (key < 128 && strlen(passwordBuf) < 64) {
			passwordBuf[strlen(passwordBuf)] = (unsigned char) key;
		}
	}
	
	startAP(ssidBuf, passwordBuf);
	return 1;
}

int connectScreen(int appOnlyNetworks, unsigned char* title) {
	NetworkList* nets;
	NetworkSelectionUI nsui;
	int direction = 0;
	unsigned int key;
	unsigned char ssidBuf[33]; // WiFi ssids are at most 32 chars
	unsigned char passwordBuf[65]; // WiFi passwords are at most 64 chars
	int reload = 1;
	
	while(reload) {
		Bdisp_AllClr_VRAM();
		locate(1, 1);
		Print("Searching...");
		Bdisp_PutDisp_DD();
		
		nets = getAvailableNetworks();
		
		if (nets == NULL) { // list is NULL -> no networks found / error
			while (1) {
				Bdisp_AllClr_VRAM();
				locate(1, 1);
				Print("No networks found");
				
				GetKey(&key);
				if (key == KEY_CTRL_F5) { // reload
					reload = 1;
					break;
				} else if (key == KEY_CTRL_EXIT) {
					return 0;
				}
			}
		} else {
			nsui = initNetworkSelectionUI(1, 3, 21, 6, nets);
			nsui.appOnly = appOnlyNetworks;

			while(1){
				Bdisp_AllClr_VRAM();
				locate(1, 1);
				if (title == NULL) {
					Print("Choose a network:");
				} else {
					Print(title);
				}
				drawNetworkSelectionUI(&nsui, direction);
				GetKey(&key);

				if(key == KEY_CTRL_UP) {
					direction = -1;
				} else if (key == KEY_CTRL_DOWN) {
					direction = 1;
				} else if (key == KEY_CTRL_F5) {
					reload = 1;
					break;
				} else if (key == KEY_CTRL_EXIT) {
					freeNetList(nets);
					nets = NULL;
					return 0;
				} else if (key == KEY_CTRL_EXE) {
					reload = 0;
					break;
				}
			}
		}
	}
	
	// copy the ssid before networklist gets deleted
	memcpy(ssidBuf, getSelectedSSID(&nsui), strlen(getSelectedSSID(&nsui)) + 1);
	
	freeNetList(nets);
	nets = NULL;
	
	memset(passwordBuf, 0, 65);
	Cursor_SetFlashOn(0);
	
	while (1){
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
		} else if (key == KEY_CTRL_EXIT) {
			return 0;
		} else if (key == KEY_CTRL_EXE) {
			break;
		} else if (key < 128 && strlen(passwordBuf) < 64) {
			passwordBuf[strlen(passwordBuf)] = (unsigned char) key;
		}
	}
	
	connect(ssidBuf, passwordBuf);
	return 1;
}

NetworkSelectionUI initNetworkSelectionUI(int x, int y, int width, int height, NetworkList* nl) {
	NetworkList* tmp = nl;
	NetworkSelectionUI nsui;
	
	nsui.x = x;
	nsui.y = y;
	nsui.width = width;
	nsui.height = height;
	nsui.selection = 0;
	nsui.scroll = 0;
	nsui.networkList = nl;
	nsui.networkCount = 0;
	nsui.appOnly = 0;
	
	while(tmp != NULL) {
		nsui.networkCount += 1;
		tmp = tmp->next;
	}
	
	return nsui;
}

int drawNetworkSelectionUI(NetworkSelectionUI* nsui, int direction) {
	NetworkList* tmp = nsui->networkList;
	int scrollCounter = 0;
	int lineCounter = 0;
	unsigned char arrow[3] = {0xE6, 0x9B, 0x00};
	unsigned char arrowUp[3] = {0xE6, 0x92, 0x00};
	unsigned char arrowDown[3] = {0xE6, 0x93, 0x00};
	unsigned char line[22];
	int ssidLength;
	DISPBOX box;
	unsigned char appName[9];
	
	GetAppName(appName);
	
	if (nsui->appOnly) {
		int appOnlySelection = 0;
		int counter = 0;
		int appOnlyNetworkCount = 0;
		NetworkList* a;
		
		for (a = nsui->networkList; a != NULL; a = a->next) {
			if (strncmp(appName, a->network.ssid, strlen(appName)) == 0) {
				appOnlyNetworkCount += 1;
			}
		}
		
		for (counter = 0, a = nsui->networkList; counter < nsui->selection && a != NULL; counter += 1, a = a->next) {
			if (strncmp(appName, a->network.ssid, strlen(appName)) == 0) {
				appOnlySelection += 1;
			}
		}
		
		appOnlySelection += direction;
		
		if (appOnlySelection < 0) {
			appOnlySelection = 0;
		} else if (appOnlySelection >= appOnlyNetworkCount) {
			appOnlySelection = appOnlyNetworkCount - 1;
		}
		
		for (counter = 0, a = nsui->networkList; a != NULL; counter += 1, a = a->next) {
			if (strncmp(appName, a->network.ssid, strlen(appName)) == 0) {
				if (appOnlySelection <= 0) {
					nsui->selection = counter;
					break;
				}
				
				appOnlySelection -= 1;
			}
		}
	} else {
		nsui->selection += direction;
	}
	
	if (nsui->selection < 0) {
		nsui->selection = 0;
	} else if (nsui->selection >= nsui->networkCount) {
		nsui->selection = nsui->networkCount - 1;
	}
	
	if (numNetworksBetween(nsui->scroll, nsui->selection, nsui) >= nsui->height) {
		for (nsui->scroll = 0; numNetworksBetween(nsui->scroll, nsui->selection, nsui) >= nsui->height; nsui->scroll += 1);
	} else if (numNetworksBetween(nsui->scroll, nsui->selection, nsui) < 0) {
		for (nsui->scroll = nsui->selection; numNetworksBetween(nsui->scroll, nsui->selection, nsui) < 0; nsui->scroll -= 1);
	}
	
	if (nsui->scroll > nsui->networkCount - nsui->height) {
		nsui->scroll = nsui->networkCount - nsui->height;
	}
	
	if (nsui->scroll < 0) {
		nsui->scroll = 0;
	}
	
	// iterate over networks to skip scrolled items
	for (scrollCounter = 0; scrollCounter < nsui->scroll; scrollCounter += 1) {
		tmp = tmp->next;
	}
	
	tmp = getNextNetwork(tmp, nsui);
	
	// print (at most) [height] lines with network ssid's
	for (lineCounter = 0; lineCounter < nsui->height; lineCounter += 1) {
		if (tmp != NULL) {
			int prefixCount = 0;
			
			if (nsui->appOnly) {
				prefixCount += strlen(appName) + 1;
			}
			
			ssidLength = strlen(&tmp->network.ssid[prefixCount]);
			if (ssidLength > nsui->width - 1) {
				ssidLength = nsui->width - 1;
			}
			
			memcpy(line, &tmp->network.ssid[prefixCount], ssidLength);
			line[ssidLength] = 0;
			
			locate(nsui->x + 1, nsui->y + lineCounter);
			Print(line);
			
			box.left = (nsui->x + nsui->width - 1) * 6 - 20;
			box.top = (nsui->y + lineCounter) * 8 - 8;
			box.right = box.left + 20;
			box.bottom = box.top + 6;
				
			Bdisp_AreaClr_VRAM(&box);
			
			drawStrengthIndicator((nsui->x + nsui->width - 1) * 6 - 19, (nsui->y + lineCounter) * 8 - 6, (int) ((tmp->network.rssi + 100.0f) / 10.0f));
			if (tmp->network.encType != 7) {
				box.left = (nsui->x + nsui->width - 1) * 6 - 26;
				box.right = box.left + 6;
				
				Bdisp_AreaClr_VRAM(&box);
				
				drawLock((nsui->x + nsui->width - 1) * 6 - 25, (nsui->y + lineCounter) * 8 - 7, 1);
			}
			
//			tmp = tmp->next;
			tmp = getNextNetwork(tmp->next, nsui);
		}
	}
	
	{
		int y = nsui->y + numNetworksBetween(nsui->scroll, nsui->selection, nsui);
		locate(nsui->x, y);
		Print(arrow);
	}
	
	if (numNetworksBetween(0, nsui->scroll, nsui) >= 1) {
		locate(nsui->x + nsui->width - 1, nsui->y);
		Print(arrowUp);
	}
	
	if (numNetworksBetween(nsui->scroll, nsui->networkCount, nsui) > nsui->height) {
		locate(nsui->x + nsui->width - 1, nsui->y + nsui->height - 1);
		Print(arrowDown);
	}
	
	return nsui->selection;
}

unsigned char* getSelectedSSID(NetworkSelectionUI* nsui) {
	NetworkList* nl = nsui->networkList;
	int counter;
	
	for (counter = 0; counter < nsui->selection && nl != NULL; counter += 1, nl = nl->next);
	
	if (nl != NULL) {
		return nl->network.ssid;
	}
	
	return NULL;
}

NetworkList* getNextNetwork(NetworkList* current, NetworkSelectionUI* nsui) {
	NetworkList* next = current;
	unsigned char appName[9];
	GetAppName(appName);
	
	if (nsui->appOnly) {
		while(next != NULL && strncmp(appName, next->network.ssid, strlen(appName)) != 0) {
			next = next->next;
		}
	}
	
	return next;
}

int numNetworksBetween(int index1, int index2, NetworkSelectionUI* nsui) {
	int num = 0;
	unsigned char appName[9];
	GetAppName(appName);
	
	if (nsui->appOnly && index2 > index1) {
		int counter = 0;
		NetworkList* a;
		
		for (counter = 0, a = nsui->networkList; counter < index1; counter += 1, a = a->next);
		
		for (counter = 0; index1 + counter < index2; counter += 1, a = a->next) {
			if (strncmp(appName, a->network.ssid, strlen(appName)) == 0) {
				num += 1;
			}
		}
	} else {
		num += index2 - index1;
	}
	
	return num;
}

void drawLock(int x, int y, int closed) {
	if (closed != 0) {
		Bdisp_DrawLineVRAM(x + 1, y, x + 3, y);
		Bdisp_SetPoint_VRAM(x + 3, y + 1, 1);
	} else {
		Bdisp_DrawLineVRAM(x + 1, y, x + 2, y);
	}
	
	Bdisp_SetPoint_VRAM(x + 1, y + 1, 1);
	
	{
		DISPBOX box;
		box.left = x;
		box.top = y + 2;
		box.right = x + 4;
		box.bottom = y + 5;
		
		Bdisp_AreaClr_VRAM(&box);
		
		Bdisp_SetPoint_VRAM(x + 2, y + 4, 1);
		
		Bdisp_AreaReverseVRAM(x, y + 2, x + 4, y + 5);
	}
}

void drawStrengthIndicator(int x, int y, int strength) {
	int counter;
	
	for (counter = 0; counter < 4; counter += 1) {
		int xPos = x + counter * 5;
		drawBallIndicator(xPos, y, counter < strength);
	}
}

void drawBallIndicator(int x, int y, int filled) {
	Bdisp_DrawLineVRAM(x + 1, y, x + 2, y);
	
	if (filled != 0) {
		Bdisp_DrawLineVRAM(x, y + 1, x + 3, y + 1);
		Bdisp_DrawLineVRAM(x, y + 2, x + 3, y + 2);
	} else {
		Bdisp_DrawLineVRAM(x, y + 1, x, y + 2);
		Bdisp_DrawLineVRAM(x + 3, y + 1, x + 3, y + 2);
	}
	
	Bdisp_DrawLineVRAM(x + 1, y + 3, x + 2, y + 3);
}