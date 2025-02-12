#pragma once

#include <windows.h>
#include <winscard.h>

#include <string>

#include "ReaderRequest.hpp"
#include "WinscardDriver.hpp"

LONG ProcessWinscard(Protocol::ReaderRequest* data);