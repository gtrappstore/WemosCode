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
	int appOnly;
} NetworkSelectionUI;

// functions

NetworkSelectionUI initNetworkSelectionUI(int x, int y, int width, int height, NetworkList* nl);
int drawNetworkSelectionUI(NetworkSelectionUI* nsui, int direction);
unsigned char* getSelectedSSID(NetworkSelectionUI* nsui);
NetworkList* getNextNetwork(NetworkList* current, NetworkSelectionUI* nsui);
int numNetworksBetween(int index1, int index2, NetworkSelectionUI* nsui);
void drawLock(int x, int y, int closed);
void drawStrengthIndicator(int x, int y, int strength);
void drawBallIndicator(int x, int y, int filled);

#endif