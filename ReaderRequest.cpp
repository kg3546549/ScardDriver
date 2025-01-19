#include "ReaderRequest.hpp"

using namespace Protocol;



ReaderRequest::ReaderRequest(Command c, Sender s, int mc, Result r, int dl, std::vector<std::string> data)
: cmd(c), sender(s), msgCnt(mc), result(r), dataLength(dl), data{ data } {
	
}

ReaderRequest::ReaderRequest(json j) {
	if (!j.contains("cmd") || !j.contains("sender") || !j.contains("msgCnt") || 
		!j.contains("result") || !j.contains("dataLength") || !j.contains("data")) {
		throw std::invalid_argument("올바르지 않은 JSON 형식입니다.");
	}
	this->cmd = j["cmd"].get<Protocol::Command>();
	this->sender = j["sender"].get<Protocol::Sender>();
	this->msgCnt = j["msgCnt"].get<int>();
	this->result = j["result"].get<Protocol::Result>();
	this->dataLength = j["dataLength"].get<int>();
	this->data = j["data"].get<std::vector<std::string>>();
}

ReaderRequest::~ReaderRequest()
{
}

json ReaderRequest::toJson() {
	json j;
	j["cmd"] = this->cmd;
	j["sender"] = this->sender;
	j["msgCnt"] = this->msgCnt;
	j["result"] = this->result;
	j["dataLength"] = this->dataLength;
	j["data"] = this->data;

	return j;
}


