#include "netUI.h"
#include "stddef.h"
#include "dispbios.h"

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
	
	nsui->selection += direction;
	
	if (nsui->selection < 0) {
		nsui->selection = 0;
	} else if (nsui->selection >= nsui->networkCount) {
		nsui->selection = nsui->networkCount - 1;
	}
	
	if (nsui->selection >= nsui->scroll + nsui->height) {
		nsui->scroll = nsui->selection - nsui->height + 1;
	} else if (nsui->selection < nsui->scroll) {
		nsui->scroll = nsui->selection;
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
	
	// print (at most) [height] lines with network ssid's
	for (lineCounter = 0; lineCounter < nsui->height; lineCounter += 1) {
		if (tmp != NULL) {
			ssidLength = strlen(tmp->network.ssid);
			if (ssidLength > nsui->width - 1) {
				ssidLength = nsui->width - 1;
			}
			
			memcpy(line, tmp->network.ssid, ssidLength);
			line[ssidLength] = 0;
			
			locate(nsui->x + 1, nsui->y + lineCounter);
			Print(line);
			
			box.left = (nsui->x + nsui->width - 1) * 6 - 20;
			box.top = (nsui->y + lineCounter) * 8 - 8;
			box.right = box.left + 20;
			box.bottom = box.top + 6;
				
			Bdisp_AreaClr_VRAM(&box);
			
			drawStrengthIndicator((nsui->x + nsui->width - 1) * 6 - 19, (nsui->y + lineCounter) * 8 - 6, (int) ((tmp->network.rssi + 100.0f) / 10.0f));
			if (tmp->network.encType == 1) {
				box.left = (nsui->x + nsui->width - 1) * 6 - 26;
				box.right = box.left + 6;
				
				Bdisp_AreaClr_VRAM(&box);
				
				drawLock((nsui->x + nsui->width - 1) * 6 - 25, (nsui->y + lineCounter) * 8 - 7, 1);
			}
			
			tmp = tmp->next;
		}
	}

	locate(nsui->x, nsui->y + nsui->selection - nsui->scroll);
	Print(arrow);
	
	if (nsui->scroll >= 1) {
		locate(nsui->x + nsui->width - 1, nsui->y);
		Print(arrowUp);
	}
	
	if (nsui->scroll + nsui->height < nsui->networkCount) {
		locate(nsui->x + nsui->width - 1, nsui->y + nsui->height - 1);
		Print(arrowDown);
	}
	
	return nsui->selection;
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