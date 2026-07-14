#include <iostream>
#include <thread>

#include <WinSock2.h>

#include "AudioPlayer.h"
#include "KeyboardDuplicator.h"
#include "MouseDuplicator.h"
#include "DesktopDemonstrator.h"

#include "SocketSender.h" 

KeyboardDuplicator kbdDup;
MouseDuplicator msDup;

static LRESULT WINAPI DesktopDupProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN: {
        msDup.Duplicate(msg == WM_LBUTTONDOWN ? MessageMouseTypes::MOUSE_LBUTTON_DOWN : MessageMouseTypes::MOUSE_RBUTTON_DOWN,
            { LOWORD(lparam), HIWORD(lparam) });
        break;
    }
    case WM_LBUTTONUP:
    case WM_RBUTTONUP: {
        msDup.Duplicate(msg == WM_LBUTTONUP ? MessageMouseTypes::MOUSE_LBUTTON_UP : MessageMouseTypes::MOUSE_RBUTTON_UP,
            { LOWORD(lparam), HIWORD(lparam) });
        break;
    }
    case WM_MOUSEMOVE: {
        msDup.Duplicate(MessageMouseTypes::MOUSE_MOVE, { LOWORD(lparam), HIWORD(lparam) });
        break;
    }
    case WM_KEYDOWN:
    case WM_KEYUP: {
        kbdDup.Duplicate(wparam, msg == WM_KEYUP);
        break;
    }
    default: return DefWindowProcW(hwnd, msg, wparam, lparam);
    }
    return 0;
}

int Client() {
    HRESULT hRes = CoInitializeEx(NULL, COINIT::COINIT_MULTITHREADED);
    if (FAILED(hRes)) {
        std::cerr << "0x" << std::hex << hRes << std::endl;
        return -1;
    }

    WSADATA wsa;
    int res = WSAStartup(MAKEWORD(2, 2), &wsa);
    if (res < 0) {
        std::cerr << "0x" << WSAGetLastError() << std::endl;
        return -1;
    }

    std::string ipServer;
    std::cout << "Server IP: ";
    std::cin >> ipServer;

    SocketSender sender;

    int initRes = sender.Init(ipServer, 5555);
    if (initRes != 0) {
        std::cerr << "SocketSender::Init failed: " << initRes << std::endl;
        WSACleanup();
        CoUninitialize();
        return -1;
    }

    std::cout << "Connected." << std::endl;

    AudioPlayer audPlay;
    DesktopDemonstrator deskDemon;

    kbdDup.Init(ipServer, 5556);
    msDup.Init(ipServer, 5557, { 1280, 720 });
    audPlay.Init(5558);
    deskDemon.Init(5559, DesktopDupProc);

    std::thread audThread(&AudioPlayer::Play, &audPlay);

    deskDemon.Show();

    audThread.join();

    WSACleanup();
    CoUninitialize();

    return 0;
}
