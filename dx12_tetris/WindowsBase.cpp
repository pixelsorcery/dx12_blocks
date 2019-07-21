
#include "BaseApp.h"
#include <Windows.h>

extern BaseApp *app;
bool active = true;

LRESULT CALLBACK WinProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_CREATE:

    break;

    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT:
        ValidateRect(hwnd, NULL);
        return 0;

    case WM_CHAR:
        if (wparam == 27) // escape
            PostMessage(hwnd, WM_CLOSE, 0, 0);
        return 0;

    case WM_CLOSE:
        app->closeWindow();
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    case WM_MOUSEMOVE:
        break;
    case WM_KEYDOWN:
        app->onKey((unsigned int)wparam, true);
        break;
    case WM_KEYUP:
        app->onKey((unsigned int)wparam, false);
        break;
    case WM_SYSKEYDOWN:
        app->onKey((unsigned int)wparam, true);
        break;
    case WM_SYSKEYUP:
        app->onKey((unsigned int)wparam, false);
        break;
    }

    return DefWindowProc(hwnd, msg, wparam, lparam);
}


int WINAPI WinMain(HINSTANCE hThisInst, HINSTANCE hLastInst, LPSTR lpszCmdLine, int nCmdShow)
{
    WNDCLASSA wc = { 0 };
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.hCursor = LoadCursor(0, IDC_ARROW);
    wc.hInstance = hThisInst;
    wc.lpfnWndProc = WinProc;
    wc.lpszClassName = "lazyllama";
    RegisterClassA(&wc);

    app->setInstance(hThisInst);

    app->initTime();

    // init api
    if (!app->initAPI())
    {
        return 0;
    }

    // init time, etc
    if (!app->init())
    {
        return 0;
    }

    // loop
    MSG msg;

    do 
    {
        while (true) 
        {

            //InvalidateRect(app->getWindow(), NULL, FALSE);

            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                //TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            if (msg.message == WM_QUIT) break;

            //app->updateTime();
            //app->makeFrame();
            app->updateTime();
            app->updateGame();
            app->drawFrame();
        }

        //app->exit();
        }while (app->isDone() != true);

    delete app;

    return (int)msg.wParam;
}