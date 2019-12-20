#ifndef main_h
#define main_h

void createBinFile();
void recordBinFile();
void renameBinFile();
void startRecording();
void displayWebPageCode(void * parameter);
void recordDataCode(void * parameter);
void connectWifi();
void transferFileNames();
boolean waitForACK(uint32_t timeout);
void sendCmd(String cmd, boolean addEOL);

#endif