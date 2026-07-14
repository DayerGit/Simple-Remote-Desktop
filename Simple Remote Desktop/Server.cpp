#include <iostream>
#include <thread>

#include <WinSock2.h>

#include "AudioDuplicator.h"
#include "KeyboardRepeater.h"
#include "MouseRepeater.h"
#include "DesktopDuplicator.h"

#include "SocketReceiver.h" 

#include "Server.h"

int Server() {
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

    while (true) {
        SocketReceiver receiver;

        int initRes = receiver.Init(5555);
        if (initRes != 0) {
            std::cerr << "SocketReceiver::Init failed: " << initRes << std::endl;
            WSACleanup();
            CoUninitialize();
            return -1;
        }

        if (!receiver.Accept()) {
            std::cerr << "Accept failed" << std::endl;
            WSACleanup();
            CoUninitialize();
            return -1;
        }

        std::cout << "Client connected." << std::endl;

        KeyboardRepeater kbdRepeat;
        MouseRepeater msRepeat;
        AudioDuplicator audDup;
        DesktopDuplicator deskDup;

        auto IP = std::string(receiver.GetClientIP());

        kbdRepeat.Init(5556);
        msRepeat.Init(5557);
        audDup.Init(IP, 5558);
        deskDup.Init(IP, 5559);

        std::thread kbdThread(&KeyboardRepeater::Repeat, &kbdRepeat);
        std::thread msThread(&MouseRepeater::Repeat, &msRepeat);
        std::thread audThread(&AudioDuplicator::Duplicate, &audDup);

        deskDup.Duplicate();

        kbdThread.join();
        msThread.join();
        audThread.join();
    }

    WSACleanup();
    CoUninitialize();
}
