#include "MouseRepeater.h"

int MouseRepeater::Init(int port) {
	return this->_receiver.Init(port);
}

HRESULT MouseRepeater::Repeat() {
    Message msg;

    if (!this->_receiver.Accept()) 
        return HRESULT_FROM_WIN32(SOCKET_ERROR);

    while (1) {
        int res = this->_receiver.Receive(msg);
        if (res <= 0) break;

        if (msg.header.msgType != MessageTypes::MOUSE_TYPE) continue;

        INPUT mouseInput = {}; 
        mouseInput.type = INPUT_MOUSE;

        switch (msg.msgMouse.event) {
        case MessageMouseTypes::MOUSE_MOVE:
            mouseInput.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
            break;
        case MessageMouseTypes::MOUSE_LBUTTON_DOWN:
            mouseInput.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
            break;
        case MessageMouseTypes::MOUSE_LBUTTON_UP:
            mouseInput.mi.dwFlags = MOUSEEVENTF_LEFTUP;
            break;
        case MessageMouseTypes::MOUSE_RBUTTON_DOWN:
            mouseInput.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
            break;
        case MessageMouseTypes::MOUSE_RBUTTON_UP:
            mouseInput.mi.dwFlags = MOUSEEVENTF_RIGHTUP;
            break;
        default:
            continue;
        }

        mouseInput.mi.dx = msg.msgMouse.coords.x * 65535;
        mouseInput.mi.dy = msg.msgMouse.coords.y * 65535;

        SendInput(1, &mouseInput, sizeof(INPUT));
    }

    return S_OK;
}

int MouseRepeater::UnInit() {
    return this->_receiver.UnInit();
}