#include "netUI.h"
#include "stddef.h"

NetworkSelectionUI initNetworkSelectionUI(int x, int y, int w, int h, NetworkList* nl) {
	NetworkList* tmp = nl;
	NetworkSelectionUI nsui;
	
	nsui.x = x;
	nsui.y = y;
	nsui.xidth = w;
	nsui.height = h;
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
	
	if (nsui->scroll >= nsui->networkCount - nsui->height) {
		nsui->scroll = nsui->networkCount - nsui->height - 1;
	}
	
	if (nsui->scroll < 0) {
		nsui->scroll = 0;
	}
	
	// iterate over networks to skip scrolled items
	for (scrollCounter = 0; scrollCounter < scroll; scrollCounter += 1) {
		tmp = tmp->next;
	}
	
	// print (at most) [height] lines with network ssid's
	for (lineCounter = 0; lineCounter < nsui->height; lineCounter += 1) {
		if (tmp != NULL) {
			locate(nsui->x, nsui->y + lineCounter);
			Print(tmp->network.ssid);
			
			tmp = tmp->next;
		}
	}
}