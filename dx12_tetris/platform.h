#pragma once

#if defined(_WIN32)
#include <Windows.h>
#endif

#include <assert.h>

#if _DEBUG
#define ASSERT(x) assert(x);
#else
#define ASSERT(x)
#endif

typedef unsigned int uint;

// Utility functions
void ErrorMsg(const char *string);
void WarningMsg(const char *string);
void InfoMsg(const char *string);

void initTime();
LARGE_INTEGER getCurrentTime();
double getTimeDifference(const LARGE_INTEGER from, const LARGE_INTEGER to);