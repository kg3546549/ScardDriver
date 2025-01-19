#include <vector>
#include <string>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace Protocol {

	enum Command {
		Cmd_Err = 0,
		Cmd_SCard_Establish_Context = 101,
		Cmd_SCard_Reader_List = 102,
		Cmd_SCard_Connect_Card = 103,
		Cmd_SCard_Disconnect_Card = 104, //Return Default Data
		Cmd_SCard_Transmit = 105, //Return Cmd_SCard_Transmit_Data

		Cmd_MI_Get_UID = 201, //
		Cmd_MI_Load_Key = 202,
		Cmd_MI_Authentication = 203,
		Cmd_MI_Read_Block = 204,
		Cmd_MI_Write_Block = 205,
		Cmd_MI_Decrement = 206,
		Cmd_MI_Increment = 207,
		Cmd_MI_Restore = 208,
		Cmd_MI_HALT = 209,
	};

	enum Sender {
		Request = 10,
		Response = 20,
	};

	enum Result {
		Success = 0,
		Default_Fail = 99
	};
	
	class ReaderRequest
	{
	private:
		Command cmd = Command::Cmd_Err;
		Sender sender = Sender::Response;
		int msgCnt = 0;
		Result result = Result::Default_Fail;
		int dataLength = 0;
		std::vector<std::string> data;
	

	public:
		ReaderRequest(Command cmd, Sender sender, int msgCnt, Result result, int dataLength, std::vector<std::string> data);

		ReaderRequest(json j);

		~ReaderRequest();

		json toJson();
		
		friend std::ostream& operator<<(std::ostream& os, const ReaderRequest& obj) {
			os << "ReaderRequest [cmd=" << obj.cmd
				<< ", sender=" << obj.sender
				<< ", msgCnt=" << obj.msgCnt
				<< ", result=" << obj.result
				<< ", dataLength=" << obj.dataLength
				<< ", data=[";
			for (size_t i = 0; i < obj.data.size(); ++i) {
				os << obj.data[i];
				if (i < obj.data.size() - 1) {
					os << ", ";
				}
			}
			os << "]]";
			return os;
		}
	};
}