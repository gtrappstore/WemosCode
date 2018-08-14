#ifndef SYSCALLS_H
#define SYSCALLS_H

int Serial_Open(unsigned char *mode);
int Serial_Close(int mode);
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

#endif