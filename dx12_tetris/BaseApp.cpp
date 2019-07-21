#include "BaseApp.h"
#include <Windows.h>

void BaseApp::initTime()
{
    totalTime = 0;
    frameTime = 0;

    ::initTime();
    startTime = curTime = getCurrentTime();
}

void BaseApp::updateTime()
{
    LARGE_INTEGER prevTime = curTime;

    curTime = getCurrentTime();
    frameTime = getTimeDifference(prevTime, curTime);
    totalTime = getTimeDifference(startTime, curTime);
}

void BaseApp::closeWindow()
{
    exitAPI();
}

void BaseApp::setWindowTitle(const char *title) {
    SetWindowText(hwnd, title);
}