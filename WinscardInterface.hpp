#pragma once

#include <windows.h>
#include <winscard.h>

#include "ReaderRequest.hpp"
#include "WinscardDriver.hpp"

LONG ProcessWinscard(Protocol::ReaderRequest* data);