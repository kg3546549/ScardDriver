﻿#include <iostream>
#include <vector>

#include <boost/asio.hpp>
#include <winscard.h>
#include <windows.h>


#include "ReaderRequest.pb.h"
#include "WinscardDriver.hpp"
#include "SocketListener.hpp"

#if 0
int main()
{
    SCARDCONTEXT hContext;
    
    SCARDHANDLE hCard;
    DWORD dwActiveProtocol;
    LONG result;

    LONG lRet = SCardEstablishContext(SCARD_SCOPE_SYSTEM, NULL, NULL, &hContext);
    if (lRet != SCARD_S_SUCCESS) {
        printf("Failed to establish context: %lX\n", lRet);
        return -1;
    }

    wchar_t readers[256];
    std::vector<std::wstring> readerVec;
    LPTSTR pReader;
    DWORD readersLen = sizeof(readers);
    lRet = SCardListReaders(hContext, NULL, readers, &readersLen);
    if (lRet != SCARD_S_SUCCESS) {
        printf("Failed to list readers: %lX\n", lRet);
        SCardReleaseContext(hContext);
        return -1;
    }
    printf("Available readers: %ls \n", readers);
    
    pReader = readers;
    while ('\0' != *pReader)
    {
        //Reader 리스트 wstring으로 변환하여 vector에 삽입
        std::wstring reader(pReader);
        readerVec.push_back(reader);

        // Advance to the next value.
        pReader = pReader + wcslen(pReader) + 1;
    }


    for (auto str : readerVec) {
        std::wcout << "Reader : " << str << std::endl;
    }
    

    // 3. 첫 번째 리더기에 카드 연결
    printf("Connect to %S\n", readerVec[0].c_str());
    result = SCardConnect(
        hContext, 
        readerVec[0].c_str(), 
        SCARD_SHARE_SHARED, 
        SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1, 
        &hCard, 
        &dwActiveProtocol
    );
    if (result != SCARD_S_SUCCESS) {
        
        std::cerr << "Failed to connect to the card: ";
        printf("%X\n", result);
        SCardReleaseContext(hContext);
        return 1;
    }

    // 4. 카드에서 UID 얻기
    //std::vector<BYTE> uid(256);  // UID는 최대 256바이트 크기
    //DWORD dwLength = (DWORD)uid.size();
    //result = SCardGetAttrib(hCard, SCARD_ATTR_ATR_STRING, uid.data(), &dwLength);
    //if (result != SCARD_S_SUCCESS) {
    //    std::cerr << "Failed to get UID: " << result << std::endl;
    //    SCardDisconnect(hCard, SCARD_LEAVE_CARD);
    //    SCardReleaseContext(hContext);
    //    return 1;
    //}


    //// 5. UID 출력
    const SCARD_IO_REQUEST* pciProtocol = (dwActiveProtocol == SCARD_PROTOCOL_T0) ? SCARD_PCI_T0 : SCARD_PCI_T1;

    unsigned char apduCommand[] = { 0xFF, 0xCA, 0x00, 0x00, 0x00 };

    unsigned char response[256];
    DWORD dwResponseLength = sizeof(response);

    result = SCardTransmit(hCard, pciProtocol, apduCommand, sizeof(apduCommand), nullptr, response, &dwResponseLength);
    if (result == SCARD_S_SUCCESS) {
        std::cout << "UID: ";
        for (DWORD i = 0; i < dwResponseLength; ++i) {
            printf("%02X ", response[i]);
        }
        std::cout << std::endl;
    }
    else {

        std::cerr << "Failed to transmit APDU: " << result << std::endl;
    }

    //loadAuthenticationKey
    unsigned char key[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    unsigned char loadAuthenticationKeysCmd[] = { 0xff,0x82, 0x00,0x00,0x06,0xff,0xff,0xff,0xff,0xff,0xff};
    memcpy(&(loadAuthenticationKeysCmd[5]), key, 6);

    std::cout << "Scard Transmit Command : ";
    for (int i = 0; i < 11; i++) {
        printf("%02x", loadAuthenticationKeysCmd[i]);
    }
    printf("\n");
        

    result = SCardTransmit(hCard, pciProtocol, loadAuthenticationKeysCmd, sizeof(loadAuthenticationKeysCmd), nullptr, response, &dwResponseLength);
    if (result == SCARD_S_SUCCESS) {
        std::cout << "Load Key Success" << std::endl;
        printf("    └ Response Data : ");
        for (int cnt = 0; cnt < dwResponseLength; cnt++) {
            printf("%02x ", response[cnt]);
        }
        printf("\n");
    }
    else {
        std::cerr << "Failed to Load Key : " << result << std::endl;
    }

    for (int i = 0; i < 64; i++) {

        printf("[Read %02d Block Start]\n", i);
        unsigned char authenticationCmd[] = { 0xff,0x88, 0x00, i,0x60,0x00 };
        result = SCardTransmit(hCard, pciProtocol, authenticationCmd, sizeof(authenticationCmd), nullptr, response, &dwResponseLength);
        if (result == SCARD_S_SUCCESS) {
            std::cout << "  └ Authentication Block " << i << std::endl;
            printf("    └ Response Data : ");
            for (int cnt = 0; cnt < dwResponseLength; cnt++) {
                printf("%02x ", response[cnt]);
            }
            printf("\n");
            if (response[0] == 0x63 && response[1] == 0x00) {
                printf("    └ Autentication Failed\n");
                continue;
            }
        }
        else {
            std::cerr << "  └ Failed to Autentication : " << result << std::endl;
        }

        printf("\n");
        /*Sleep(1);*/
        
        dwResponseLength = sizeof(response);
        unsigned char readBinaryBlocks[] = {0xff, 0xb0, 0x00, i, 0x10};
        result = SCardTransmit(hCard, pciProtocol, readBinaryBlocks, sizeof(readBinaryBlocks), nullptr, response, &dwResponseLength);
        if (result == SCARD_S_SUCCESS) {
            std::cout << "  └ Read Block " << i << " Transmit Result" << std::endl;
            printf("  └ Read Data (%d) : ", dwResponseLength);
            for (int cnt = 0; cnt < dwResponseLength; cnt++) {
                printf("%02x", response[cnt]);
            }
            printf("\n");

            if (response[0] == 0x63 && response[1] == 0x00) {
                printf("    └ Autentication Failed\n");
                continue;
            }
        }
        else {
            std::cerr << "  └ Failed to Read Block : ";
            printf("%x\n", result);
        }
        printf("\n");
        Sleep(1);
    }
    

    
    

    // 6. 카드 연결 해제 및 컨텍스트 해제
    SCardDisconnect(hCard, SCARD_LEAVE_CARD);
    SCardReleaseContext(hContext);

}
#endif

#if 0
int main2(void) {

	WinscardDriver& WD = WinscardDriver::getIncetance();
	
	LONG result;

	result = WD.SCard_EstablishContext(SCARD_SCOPE_SYSTEM);
	if (result != SCARD_S_SUCCESS) {
		std::cout << "!ERR : " << WinscardDriver::errToString(result) << std::endl;
		return 0;
	}

	result = WD.SCard_ListReaders();
	if (result != SCARD_S_SUCCESS) {
		std::cout << "!ERR : " << WinscardDriver::errToString(result) << std::endl;
		return 0;
	}
	
	std::vector<std::wstring> readerList = WD.getReaderList();

	for (auto reader : readerList) {
		std::wcout << "[Reader] : " << reader << std::endl;
	}

	result = WD.SCard_Connect(1);
	if (result != SCARD_S_SUCCESS) {
		std::cout << "!ERR : " << WinscardDriver::errToString(result) << std::endl;
		return 0;
	}

	uint8_t UID[4] = { 0x00, };
	WD.getSCardUID( UID );

	printf("UID : ");
	for (int i = 0; i < 4; i++) {
		printf("%02X ", UID[i]);
	}
	printf("\n");

	uint8_t key[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	for (int i = 0; i < 64; i++) {
		uint8_t response[256];
		DWORD resLen = sizeof(response);
		int ret;

		printf("\n\n\n");

		ret = WD.loadKey(key, 0, response, &resLen);
		if (ret == -1) {
			printf("[Load Key Err]\n");
			continue;
		}

		resLen = sizeof(response);
		ret = WD.authentication(i, 'A', 0, response, &resLen);
		if (ret == -1) {
			printf("[Authentication Block Err %d] \n", i);
			continue;
		}

		resLen = sizeof(response);
		ret = WD.readBinaryBlock(i, response, &resLen);
		if (ret == -1) {
			printf("[Read Block Err %d]\n", i);
			continue;
		}

		printf("[Read Block : %d]\n", i);
		for (int j = 0; j < resLen; j++) {
			printf("%02X ", response[j]);
		}
		
	}

	WD.SCard_Disconnect();
	WD.SCard_RealeaseContext();
}
#endif

int main() {
	ReaderRequest::ReaderRequest readerPB;

	readerPB.set_cmd(ReaderRequest::Cmd_SCard_Establish_Context);
	readerPB.set_msgcnt(1);
	readerPB.add_result(std::string("Test"));

	std::cout << "Create PB Instance Complete" << std::endl;

	size_t size = readerPB.ByteSizeLong();  // 직렬화된 데이터의 크기를 계산
	unsigned char* buffer = new unsigned char[size];
	
	std::cout << "size : " << size << std::endl;
	


	if (!readerPB.SerializeToArray(buffer, size)) {
		std::cerr << "Serialization to array failed!" << std::endl;
		return 0;
	}


	std::cout << "Serialized data: ";
	for (size_t i = 0; i < size; ++i) {
		std::cout << std::hex << (0xFF & buffer[i]) << " ";
	}
	std::cout << std::endl;



	ReaderRequest::ReaderRequest requestPB;
	uint8_t *reqArr = new uint8_t[size];
	if (!requestPB.ParseFromArray(buffer, size)) {
		std::cerr << "Failed to serialize the message!" << std::endl;
		return -1;
	}

	//std::cout << "requestPB.cmd() : " << std::hex << requestPB.cmd() << std::endl;
	//std::cout << "requestPB.msgcnt() : " << requestPB.msgcnt() << std::endl;

	//std::cout << "requsetPB.result_size() : " << requestPB.result_size() << std::endl;
	//std::string result = "";
	/*
	std::cout << "requsetPB.result(0) : " << result << std::endl;*/

	//for (int i = 0; i < requestPB.result_size();i++) {
	//	std::cout << requestPB.result(i) << std::endl;
	//}


	//std::cout << "Serialize Data : " << ser_data << std::endl;

	//SocketListener SL = SocketListener();

	//SL.StartListener();

	return 1;
}