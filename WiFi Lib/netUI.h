#ifndef NETUI_H
#define NETUI_H

#include "stddef.h"
#include "net.h"

// structs

typedef struct {
	int x, y, width, height;
	int selection;
	int scroll;
	NetworkList* networkList;
	int networkCount;
} NetworkSelectionUI;

// functions

NetworkSelectionUI initNetworkSelectionUI(int x, int y, int width, int height, NetworkList* nl);
int drawNetworkSelectionUI(NetworkSelectionUI* nsui, int selection);

#endif