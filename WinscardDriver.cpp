#include "WinscardDriver.hpp"

//#define __DEBUG__

LONG WinscardDriver::SCard_EstablishContext(DWORD dwScope) {
    LONG ret = SCardEstablishContext(dwScope, NULL, NULL, &(this->hContext));

    return ret;
}

LONG WinscardDriver::SCard_RealeaseContext() {
    return SCardReleaseContext(hContext);
}

LONG WinscardDriver::SCard_ListReaders() {
    wchar_t readers[256];
    DWORD readersLen = sizeof(readers);
    LPTSTR pReader; //Reader List String Ptr

    LONG ret = SCardListReaders(hContext, NULL, readers, &readersLen);
    if (ret != SCARD_S_SUCCESS) {
        /*printf("Failed to list readers: %lX\n", ret);*/
        SCard_RealeaseContext();
        return ret;
    }

    pReader = readers;
    while ('\0' != *pReader)
    {
        //Reader 리스트 wstring으로 변환하여 vector에 삽입
        std::wstring reader(pReader);
        readerList.push_back(reader);

        // Advance to the next value.
        pReader = pReader + wcslen(pReader) + 1;
    }
    return SCARD_S_SUCCESS;
}

std::vector<std::wstring> WinscardDriver::getReaderList() {
    return this->readerList;
}

int WinscardDriver::getReaderNum() {
    return this->readerList.size();
}

LONG WinscardDriver::SCard_Connect(int readerNum) {
    if (readerNum > readerList.size()) return -1;

    LONG result = SCardConnect(
        hContext,
        readerList[readerNum].c_str(),
        SCARD_SHARE_SHARED,
        SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1,
        &hCard,
        &dwActiveProtocol
    );

    if (result != SCARD_S_SUCCESS) {
#ifdef __DEBUG__
        std::cerr << "Failed to connect to the card: ";
        printf("%X\n", result);
#endif
        SCard_RealeaseContext();
        return result;
    }

    connectStatus = ConnectStatus::Connected;
    return result;
}


LONG WinscardDriver::SCard_Disconnect() {
    return SCardDisconnect(hCard, SCARD_LEAVE_CARD);
}


//Applications

//Get Smart Card UID(CSN)
LONG WinscardDriver::getSCardUID(uint8_t * UID) {
    if (this->connectStatus == Disconnected) {
        std::cerr << "Card is Disconneted now" << std::endl;
        return -1;
    }

    const SCARD_IO_REQUEST* pciProtocol = (dwActiveProtocol == SCARD_PROTOCOL_T0) ? SCARD_PCI_T0 : SCARD_PCI_T1;

    unsigned char apduCommand[] = { 0xFF, 0xCA, 0x00, 0x00, 0x00 };

    unsigned char response[256];
    DWORD resLen = sizeof(response);

    LONG result = SCardTransmit(
        hCard, 
        pciProtocol, 
        apduCommand,
        sizeof(apduCommand), 
        nullptr, 
        response, 
        &resLen
    );

    if (result == SCARD_S_SUCCESS) {
        /*
        std::cout << "UID: ";
        for (DWORD i = 0; i < dwResponseLength; ++i) {
            printf("%02X ", response[i]);
        }
        printf("\n");
        */

        if (response[resLen - 2] == 0x90 && response[resLen - 1] == 0x00) {
            memcpy_s(UID, 4, response, 4);
            return result;
        }

        else if (response[resLen - 2] == 0x63 && response[resLen - 1] == 0x00) {
            return -1;
        }
    }

    //std::cerr << "Failed to transmit APDU: " << result << std::endl;
    return result;
}

LONG WinscardDriver::loadKey(uint8_t * key, uint8_t keyNum, uint8_t* resBuf, DWORD* bufLen) {
    unsigned char loadAuthenticationKeysCmd[] = { 0xff,0x82, 0x00,keyNum,0x06,0xff,0xff,0xff,0xff,0xff,0xff };
    memcpy(&(loadAuthenticationKeysCmd[5]), key, 6);

#ifdef __DEBUG__ 
    std::cout << "Scard Transmit Command : ";
    for (int i = 0; i < 11; i++) {
        printf("%02x", loadAuthenticationKeysCmd[i]);
    }
    printf("\n");
#endif

    const SCARD_IO_REQUEST* pciProtocol = (dwActiveProtocol == SCARD_PROTOCOL_T0) ? SCARD_PCI_T0 : SCARD_PCI_T1;

    LONG result = SCardTransmit(
        hCard, 
        pciProtocol, 
        loadAuthenticationKeysCmd, 
        sizeof(loadAuthenticationKeysCmd), 
        nullptr, 
        resBuf,
        bufLen
    );

    if (result == SCARD_S_SUCCESS) {
#ifdef __DEBUG__ 
        std::cout << "Load Key Success" << std::endl;
        printf("    └ Response Data : ");
        for (int cnt = 0; cnt < dwResponseLength; cnt++) {
            printf("%02x ", resBuf[cnt]);
        }
        printf("\n");
#endif

        if (resBuf[(*bufLen) - 2] == 0x90 && resBuf[(*bufLen) - 1] == 0x00) {
            return result;
        }

#ifdef __DEBUG__ 
        std::cerr << "Failed to Load Key : " << result << std::endl;
#endif

        else if (resBuf[(*bufLen) - 2] == 0x63 && resBuf[(*bufLen) - 1] == 0x00) {
            return -1;
        }
    }

    return result;
}

//TODO : keyType Typedef
LONG WinscardDriver::authentication(int blkNum, char keyType, uint8_t keyNum, uint8_t* resBuf, DWORD* bufLen) {
    if (keyType != 'A' && keyType != 'B') { 
#ifdef __DEBUG__ 
        printf("Auth Key Type Error %c\n", keyType);
#endif
        return -1; 
    }

    //printf("[Read %02d Block Start]\n", blkNum);
    const SCARD_IO_REQUEST* pciProtocol = (dwActiveProtocol == SCARD_PROTOCOL_T0) ? SCARD_PCI_T0 : SCARD_PCI_T1;

    unsigned char authenticationCmd[] = { 0xff,0x88, 0x00, blkNum,0x60,0x00 };
    authenticationCmd[4] = keyType == 'A' ? 0x60 : 0x61;

    LONG result = SCardTransmit(
        hCard, 
        pciProtocol, 
        authenticationCmd, 
        sizeof(authenticationCmd), 
        nullptr, 
        resBuf,
        bufLen
    );

    if (result == SCARD_S_SUCCESS) {
        if (resBuf[(*bufLen) - 2] == 0x63 && resBuf[(*bufLen) - 1] == 0x00)
            return -1;
        
        else if (resBuf[(*bufLen) - 2] == 0x90 && resBuf[(*bufLen)-1] == 0x00)
            return result;
    }
    
    return result;
}

LONG WinscardDriver::readBinaryBlock(int blkNum, uint8_t * resBuf, DWORD *bufLen) {
    const SCARD_IO_REQUEST* pciProtocol = (dwActiveProtocol == SCARD_PROTOCOL_T0) ? SCARD_PCI_T0 : SCARD_PCI_T1;
#ifdef __DEBUG__ 
    printf("[Read Block %d]\n", blkNum);
#endif

    unsigned char readBinaryBlocks[] = { 0xff, 0xb0, 0x00, blkNum, 0x10 };
    LONG result = SCardTransmit(
        hCard, 
        pciProtocol, 
        readBinaryBlocks, 
        sizeof(readBinaryBlocks),
        nullptr,
        resBuf,
        bufLen
    );

    if (result == SCARD_S_SUCCESS) {


        if (resBuf[0] == 0x63 && resBuf[1] == 0x00) {
            return -1;
        }
        else if (resBuf[(*bufLen) - 2] == 0x90 && resBuf[(*bufLen) - 1] == 0x00)
            return result;
    }

    return result;
}


std::string WinscardDriver::errToString(LONG errCode) {



    switch (errCode) {
    case 0x00000000: return "SCARD_S_SUCCESS";
    case 0x80100001: return "SCARD_F_INTERNAL_ERROR";
    case 0x80100002: return "SCARD_E_CANCELLED";
    case 0x80100003: return "SCARD_E_INVALID_HANDLE";
    case 0x80100004: return "SCARD_E_INVALID_PARAMETER";
    case 0x80100005: return "SCARD_E_INVALID_TARGET";
    case 0x80100006: return "SCARD_E_NO_MEMORY";
    case 0x80100007: return "SCARD_F_WAITED_TOO_LONG";
    case 0x80100008: return "SCARD_E_INSUFFICIENT_BUFFER";
    case 0x80100009: return "SCARD_E_UNKNOWN_READER";
    case 0x8010000A: return "SCARD_E_TIMEOUT";
    case 0x8010000B: return "SCARD_E_SHARING_VIOLATION";
    case 0x8010000C: return "SCARD_E_NO_SMARTCARD";
    case 0x8010000D: return "SCARD_E_UNKNOWN_CARD";
    case 0x8010000E: return "SCARD_E_CANT_DISPOSE";
    case 0x8010000F: return "SCARD_E_PROTO_MISMATCH";
    case 0x80100010: return "SCARD_E_NOT_READY";
    case 0x80100011: return "SCARD_E_INVALID_VALUE";
    case 0x80100012: return "SCARD_E_SYSTEM_CANCELLED";
    case 0x80100013: return "SCARD_F_COMM_ERROR";
    case 0x80100014: return "SCARD_F_UNKNOWN_ERROR";
    case 0x80100015: return "SCARD_E_INVALID_ATR";
    case 0x80100016: return "SCARD_E_NOT_TRANSACTED";
    case 0x80100017: return "SCARD_E_READER_UNAVAILABLE";
    case 0x80100018: return "SCARD_P_SHUTDOWN";
    case 0x80100019: return "SCARD_E_PCI_TOO_SMALL";
    case 0x8010001A: return "SCARD_E_READER_UNSUPPORTED";
    case 0x8010001B: return "SCARD_E_DUPLICATE_READER";
    case 0x8010001C: return "SCARD_E_CARD_UNSUPPORTED";
    case 0x8010001D: return "SCARD_E_NO_SERVICE";
    case 0x8010001E: return "SCARD_E_SERVICE_STOPPED";
    case 0x8010001F: return "SCARD_E_UNSUPPORTED_FEATURE";
    case 0x80100020: return "SCARD_E_ICC_INSTALLATION";
    case 0x80100021: return "SCARD_E_ICC_CREATEORDER";
    case 0x80100023: return "SCARD_E_DIR_NOT_FOUND";
    case 0x80100024: return "SCARD_E_FILE_NOT_FOUND";
    case 0x80100025: return "SCARD_E_NO_DIR";
    case 0x80100026: return "SCARD_E_NO_FILE";
    case 0x80100027: return "SCARD_E_NO_ACCESS";
    case 0x80100028: return "SCARD_E_WRITE_TOO_MANY";
    case 0x80100029: return "SCARD_E_BAD_SEEK";
    case 0x8010002A: return "SCARD_E_INVALID_CHV";
    case 0x8010002B: return "SCARD_E_UNKNOWN_RES_MSG";
    case 0x8010002C: return "SCARD_E_NO_SUCH_CERTIFICATE";
    case 0x8010002D: return "SCARD_E_CERTIFICATE_UNAVAILABLE";
    case 0x8010002E: return "SCARD_E_NO_READERS_AVAILABLE";
    case 0x8010002F: return "SCARD_E_COMM_DATA_LOST";
    case 0x80100030: return "SCARD_E_NO_KEY_CONTAINER";
    case 0x80100031: return "SCARD_E_SERVER_TOO_BUSY";
    case 0x80100065: return "SCARD_W_UNSUPPORTED_CARD";
    case 0x80100066: return "SCARD_W_UNRESPONSIVE_CARD";
    case 0x80100067: return "SCARD_W_UNPOWERED_CARD";
    case 0x80100068: return "SCARD_W_RESET_CARD";
    case 0x80100069: return "SCARD_W_REMOVED_CARD";
    case 0x8010006A: return "SCARD_W_SECURITY_VIOLATION";
    case 0x8010006B: return "SCARD_W_WRONG_CHV";
    case 0x8010006C: return "SCARD_W_CHV_BLOCKED";
    case 0x8010006D: return "SCARD_W_EOF";
    case 0x8010006E: return "SCARD_W_CANCELLED_BY_USER";
    case 0x8010006F: return "SCARD_W_CARD_NOT_AUTHENTICATED";

    case -1 : return "General Application Error (6300)";
    
    default : return "UNKNOWN_ERROR";
    }
}