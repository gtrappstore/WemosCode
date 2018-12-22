#include "status.h"

int convertStatus(unsigned char* statusBuf) {
	int status = -1;
	
	if (memcmp(statusBuf, "OK", 2) == 0) {
		status = STATUS_OK;
	} else if (memcmp(statusBuf, "UC", 2) == 0) {
		status = STATUS_UC;
	} else if (memcmp(statusBuf, "TO", 2) == 0) {
		status = STATUS_TO;
	} else if (memcmp(statusBuf, "OF", 2) == 0) {
		status = STATUS_OF;
	} else if (memcmp(statusBuf, "DE", 2) == 0) {
		status = STATUS_DE;
	}
	
	return status;
}