
#include <Windows.h>
#include "platform.h"

#if defined(_WIN32)


double invFreq;

void initTime() 
{
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    invFreq = 1.0 / double(freq.QuadPart);
}

LARGE_INTEGER getCurrentTime() 
{
    LARGE_INTEGER curr;
    QueryPerformanceCounter(&curr);
    return curr;
}

double getTimeDifference(const LARGE_INTEGER from, const LARGE_INTEGER to)
{
    UINT64 diff = to.QuadPart - from.QuadPart;
    return (static_cast<double>(diff) * invFreq); // return ms
}

void ErrorMsg(const char *string) {
    MessageBoxA(NULL, string, "Error", MB_OK | MB_ICONERROR);
}

void WarningMsg(const char *string) {
    MessageBoxA(NULL, string, "Warning", MB_OK | MB_ICONWARNING);
}

void InfoMsg(const char *string) {
    MessageBoxA(NULL, string, "Information", MB_OK | MB_ICONINFORMATION);
}

#endif