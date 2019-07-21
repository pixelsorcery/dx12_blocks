#pragma once

#include <memory>
#include "Renderer.h"
#include "platform.h"

class BaseApp
{
public:
    //BaseApp();
    //virtual ~BaseApp();

    virtual char *getTitle() const = 0;

    //void loadConfig();
    //void updateConfig();
    virtual bool init() { return true; };
    virtual void exit() {};

    //virtual bool initCaps() = 0;
    virtual bool initAPI() = 0;
    virtual void exitAPI() = 0;

    virtual void initTime();
    virtual void updateTime();

    virtual void updateGame() = 0;

    virtual void onKey(const uint key, const bool pressed) = 0;

    virtual void drawFrame() = 0;

    bool isDone() const { return done; }

    void setInstance(const HINSTANCE hInst) { hInstance = hInst; }
    HWND getWindow() const { return hwnd; }

    void closeWindow();

protected:
    void setWindowTitle(const char *title);
    std::unique_ptr<Renderer> renderer;

    HINSTANCE hInstance;
    HWND hwnd;

    LARGE_INTEGER startTime, curTime, prevTime;
    double totalTime, frameTime;

    static const uint m_windowHeight = 768;
    static const uint m_windowWidth = 768;

    bool done;
};