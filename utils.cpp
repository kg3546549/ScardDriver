#include <utils.hpp>

//ONLY ASCII
//TODO : UTF-8~~
std::vector<std::string> WSVtoSV(const std::vector<std::wstring>& wvec) {
    std::vector<std::string> result;
    result.reserve(wvec.size());

    for (const auto& wstr : wvec) {
        result.push_back( std::string(wstr.begin(), wstr.end() ) );
    }

    return result;
}

std::string uint8toString(uint8_t * arr, size_t len) {
    std::string result(reinterpret_cast<const char*>(arr), len);
    return result;
}

std::vector<uint8_t> hexStringToByteArray(const std::string& hex) {
    std::vector<uint8_t> bytes;
    if (hex.length() % 2 != 0) {
        throw std::invalid_argument("Hex string length must be even.");
    }

    for (size_t i = 0; i < hex.length(); i += 2) {
        uint8_t byte = static_cast<uint8_t>(std::stoul(hex.substr(i, 2), nullptr, 16));
        bytes.push_back(byte);
    }

    return bytes;
}

std::string bytesToHexString(const uint8_t* data, size_t length) {
    char buffer[3];  // 2자리 HEX + 널 문자
    std::string result;

    for (size_t i = 0; i < length; i++) {
        snprintf(buffer, sizeof(buffer), "%02X", data[i]);  // 2자리 16진수 변환
        result += buffer;
    }

    return result;
}