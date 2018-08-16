#ifndef STATUS_H
#define STATUS_H

#define STATUS_OK 0
#define STATUS_UC 1
#define STATUS_TO 2
#define STATUS_OF 3
#define STATUS_DE 4

int convertStatus(unsigned char* statusBuf);

#endif