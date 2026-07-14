#include "KeyboardRepeater.h"

int KeyboardRepeater::Init(int port) {
    return this->_receiver.Init(port);
}

int KeyboardRepeater::Repeat() {
    Message msg;

    if (!this->_receiver.Accept()) 
        return HRESULT_FROM_WIN32(SOCKET_ERROR);

    while (1) {
        int res = this->_receiver.Receive(msg);
        if (res <= 0) break;

        if (msg.header.msgType != MessageTypes::KEYBOARD_TYPE) continue;

        INPUT keyboardInput = {};
        keyboardInput.type = INPUT_KEYBOARD;

        keyboardInput.ki.wVk = msg.msgKeyboard.vKey;
        keyboardInput.ki.dwFlags = msg.msgKeyboard.isUp ? KEYEVENTF_KEYUP : 0;
        
        SendInput(1, &keyboardInput, sizeof(INPUT));
    }

    return S_OK;
}

void KeyboardRepeater::UnInit() {
    this->_receiver.UnInit();
}