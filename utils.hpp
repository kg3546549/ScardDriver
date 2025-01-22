#pragma once

#include <vector>
#include <string>
#include <sstream>
#include <iomanip>


std::vector<std::string> WSVtoSV(const std::vector<std::wstring>& wvec);

std::string uint8toString(uint8_t* arr, size_t len);

std::vector<uint8_t> hexStringToByteArray(const std::string& hex);

std::string bytesToHexString(const uint8_t* data, size_t length);