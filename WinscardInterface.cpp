#include "WinscardInterface.hpp"

extern WinscardDriver & WD;


#define __DEBUG__ 1


LONG ProcessWinscard(Protocol::ReaderRequest * data) {
	//응답 JSON 생성
	data->setSender(Protocol::Response);
	LONG result;


	switch (data->getCmd()) {
		case Protocol::Cmd_SCard_Establish_Context : {
			result = WD.SCard_EstablishContext(SCARD_SCOPE_SYSTEM);

			if (result != SCARD_S_SUCCESS) {
				std::cout << "!ERR-SCard Establish : " << WinscardDriver::errToString(result) << std::endl;
				data->setResult(Protocol::Default_Fail);
				return -1;
			}

			data->setResult(Protocol::Success);
			return 1;
		}

		break;

		case Protocol::Cmd_SCard_Reader_List : {
			result = WD.SCard_ListReaders();
			if (result != SCARD_S_SUCCESS) {
				std::cout << "!ERR-SCard List : " << WinscardDriver::errToString(result) << std::endl;
				data->setResult(Protocol::Default_Fail);
				return -1;
			}
			std::vector<std::string> RList = WD.getReaderList();

			data->setResult(Protocol::Success);
			data->setDataLength(RList.size());
			data->setData(RList);

			return 1;
		}

		break;

		case Protocol::Cmd_SCard_Connect_Card : {

			/*int idx = data->getData()[0].c_str();*/
			int idx = 0;
			//TODO : Reader Number Select 
			result = WD.SCard_Connect(idx);
			if (result != SCARD_S_SUCCESS) {
				auto err = WinscardDriver::errToString(result);
				std::cout << "!ERR-SCard Connect : " << err << std::endl;
				data->setDataLength(1);
				data->getPtrData()->push_back(err);
				data->setResult(Protocol::Default_Fail);
				return -1;
			}

			data->setResult(Protocol::Success);

			return 1;
			
		}

		break;

		case Protocol::Cmd_SCard_Disconnect_Card: {

			//TODO : Reader Number Select 
			result = WD.SCard_Disconnect();
			if (result != SCARD_S_SUCCESS) {
				std::cout << "!ERR-SCard Disconnect : " << WinscardDriver::errToString(result) << std::endl;
				data->setResult(Protocol::Default_Fail);
				return -1;
			}

			data->setResult(Protocol::Success);

			return 1;
		}
		break;

		case Protocol::Cmd_SCard_Transmit : {
			data->setResult(Protocol::Default_Fail);
			return -1;
		}
		break;

		//MIFARE INTERFACE
		case Protocol::Cmd_MI_Get_UID: {
			uint8_t UID[4] = { 0x00, };
			result = WD.getSCardUID(UID);
			if (result != SCARD_S_SUCCESS) {
				std::cout << "!ERR-MI Get UID: " << WinscardDriver::errToString(result) << std::endl;
				data->setResult(Protocol::Default_Fail);
				return -1;
			}
			
			std::string sUID = bytesToHexString(UID, sizeof(UID));

#ifdef __DEBUG__
			std::cout << "sUID : " << sUID << std::endl;	
#endif
			
			data->setResult(Protocol::Success);
			data->setDataLength(1);
			data->getPtrData()->push_back(sUID);
			return 1;
		}
		break;

		case Protocol::Cmd_MI_Load_Key : {
			std::vector<std::string> vstr = data->getData();

			if (vstr[0].length() != 12) {
				std::cout << "!ERR-MI Key Load Invalid Key Length "
					<< " : " << vstr[0].size()
					<< std::endl;
				data->setResult(Protocol::Default_Fail);
				return -1;
			}

			std::vector<uint8_t> key = hexStringToByteArray(vstr[0]);

			//std::vector<uint8_t> vRes = hexStringToByteArray(vstr[1]);
			uint8_t resBuf[256];
			DWORD resLen = sizeof(resBuf);

			result = WD.loadKey(key.data(), 0, resBuf, &resLen);
			if (result == -1) {
				std::cout << "!ERR-MI Key Load : Error "
					<< WinscardDriver::errToString(result)
					<< std::endl;
				data->setResult(Protocol::Default_Fail);
				return -1;
			}

			data->setResult(Protocol::Success);
			data->setDataLength(1);
			data->getPtrData()->push_back( bytesToHexString(resBuf, resLen) );
			return 1;
		}
		break;

		case Protocol::Cmd_MI_Authentication : {
			std::vector<std::string> vstr = data->getData();

			//Key Type Check
			if (vstr[1] != "A" && vstr[1] != "B") {
				std::cout << "!ERR-MI Key Load Invalid Key Type "
					<< " : " << vstr[1]
					<< std::endl;
				data->setResult(Protocol::Default_Fail);
				return -1;
			}

			//int i = vstr[0][0]-0x30;
			int i = std::stoi( vstr[0] );

			uint8_t resBuf[256];
			DWORD resLen = sizeof(resBuf);

			//TODO : Authentication Block
			result = WD.authentication(i, vstr[1].c_str()[0], 0, resBuf, &resLen);
			if (result == -1) {
				printf("[Authentication Block Err %d] \n", i);
				data->setResult(Protocol::Default_Fail);
				data->setDataLength(1);
				data->getPtrData()->push_back(bytesToHexString(resBuf, resLen));
				return -1;
			}

			data->setResult(Protocol::Success);
			data->setDataLength(1);
			data->getPtrData()->push_back(bytesToHexString(resBuf, resLen));
			return 1;
		}
		break;

		case Protocol::Cmd_MI_Read_Block : {
			std::vector<std::string> vstr = data->getData();
			uint8_t resBuf[256];
			DWORD resLen = sizeof(resBuf);

			int i = std::stoi(vstr[0]);

			result = WD.readBinaryBlock(i, resBuf, &resLen);
			if (result == -1) {
				printf("[Read Block Err %d]\n", i);
				data->setResult(Protocol::Default_Fail);
				data->setDataLength(1);
				data->getPtrData()->push_back(bytesToHexString(resBuf, resLen));
				return -1;
			}

			data->setResult(Protocol::Success);
			data->setDataLength(1);
			data->getPtrData()->push_back(bytesToHexString(resBuf, resLen));
			return 1;
		}
		break;

		case Protocol::Cmd_MI_Write_Block:



		break;

		default :
			data->setResult(Protocol::Default_Fail);
			data->setData({ "Undefined Command!" });
			return -1;
		break;

	}
	
	return 0;
}