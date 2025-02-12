#pragma once

#include <winscard.h>
#include <iostream>
#include <vector>

#include "utils.hpp"

enum ConnectStatus {
    Connected = 1,
    Disconnected = 0
};

class WinscardDriver {

private:
    SCARDCONTEXT hContext;
    SCARDHANDLE hCard;
    std::vector<std::wstring> readerList;
    DWORD dwActiveProtocol;

    ConnectStatus connectStatus = Disconnected;

    //SCARD_IO_REQUEST* pciProtocol;


    WinscardDriver() {

    }

    WinscardDriver(const WinscardDriver& ref) {

    }

    WinscardDriver& operator=(const WinscardDriver& ref) {

    }
    ~WinscardDriver() {

    }

public:
    /*static Functions*/
    static WinscardDriver& getIncetance() {
        static WinscardDriver s;
        return s;
    }
    static std::string errToString(LONG errCode);

    /*SCard Functions*/
    LONG SCard_EstablishContext(DWORD dwScope);
    LONG SCard_RealeaseContext();
    LONG SCard_ListReaders();
    std::vector<std::wstring> getWReaderList();
    std::vector<std::string> getReaderList();
    int getReaderNum();
    LONG SCard_Connect(int readerNum);
    LONG SCard_Disconnect();

    LONG SCard_getATR(uint8_t readerNum, BYTE* pATR, DWORD* atrLen);

    LONG SCard_Transmit(uint8_t * resBuf, DWORD* bufLen, uint8_t * sendData, DWORD* sendLen);

    /*Application Functions*/

    //Get Smart Card UID(CSN)
    LONG getSCardUID(uint8_t* UID);



    /*
    * 
    * MIFARE Classic 1K Methods
    * 
    */
    /*MIFARE: Load Authentication Keys*/
    LONG loadKey(uint8_t* key, uint8_t keyNum, uint8_t* resBuf, DWORD* bufLen);

    //TODO : keyType Typedef
    /*MIFARE: Block Authentication*/
    LONG authentication(int blkNum, char keyType, uint8_t keyNum, uint8_t* resBuf, DWORD* bufLen);

    /*MIFARE: Binary Block*/
    LONG readBinaryBlock(int blkNum, uint8_t* resBuf, DWORD *bufLen);

    /*MIFARE : Write Block*/

};