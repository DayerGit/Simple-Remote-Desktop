#include "SocketSender.h"

SocketSender::SocketSender() noexcept {
    this->_socket = INVALID_SOCKET;
    this->_sa = { 0 };
    this->_type = SocketType::SOCKET_NONE;
}

int SocketSender::Init(const std::string& addr, int port, SocketType type) noexcept {
    this->_type = type;
	this->_socket = socket(AF_INET, type == SocketType::SOCKET_TCP ? SOCK_STREAM : SOCK_DGRAM, 0);
	if (INVALID_SOCKET == this->_socket) return this->_socket;

	this->_sa.sin_family = AF_INET;
	this->_sa.sin_port = htons(port);
	this->_sa.sin_addr.S_un.S_addr = inet_addr(addr.c_str());

	return connect(this->_socket, (const sockaddr*) &this->_sa, sizeof(this->_sa));
}

int SocketSender::Send(Message& msg) const noexcept {
    if (!msg.header.dataSize) return 0;

    int res = send(this->_socket, (const char*)&msg.header.msgType, sizeof(msg.header.msgType), 0);
    if (res <= 0) return res;

    res = send(this->_socket, (const char*)&msg.header.dataSize, sizeof(msg.header.dataSize), 0);
    if (res <= 0) return res;

    LONG sent = 0;
    while (sent < msg.header.dataSize) {
        switch (msg.header.msgType) {
        case MessageTypes::DESKTOP_TYPE: {
            if (this->_type == SocketType::SOCKET_UDP) return -1;

            const size_t rectSize = sizeof(msg.msgDesk.rc);

            if (sent < rectSize) {
                size_t offsetInRect = sent;
                size_t remainingRect = rectSize - offsetInRect;
                res = send(this->_socket, (const char*)&msg.msgDesk.rc + offsetInRect, remainingRect, 0);
            }
            else {
                size_t pixelOffset = sent - rectSize;
                size_t totalPixelSize = msg.header.dataSize - rectSize;
                size_t remainingPixels = totalPixelSize - pixelOffset;
                res = send(this->_socket, (const char*)msg.msgDesk.pData + pixelOffset, remainingPixels, 0);
            }
            break;
        }

        case MessageTypes::MOUSE_TYPE: {
            if (this->_type == SocketType::SOCKET_UDP) return -1;

            res = send(this->_socket, (const char*)(&msg.msgMouse) + sent, msg.header.dataSize - sent, 0);
            break;
        }

        case MessageTypes::KEYBOARD_TYPE: {
            if (this->_type == SocketType::SOCKET_UDP) return -1;

            res = send(this->_socket, (const char*)(&msg.msgKeyboard) + sent, msg.header.dataSize - sent, 0);
            break;
        }

        case MessageTypes::AUDIO_TYPE: {
            res = send(this->_socket, (const char*)msg.msgAudio.pData, msg.header.dataSize - sent, 0);
            break;
        }
        }
        if (res <= 0) return -1;
        sent += res;
    }
    return sizeof(msg.header.msgType) + sizeof(msg.header.dataSize) + msg.header.dataSize;
}

int SocketSender::UnInit() {
    if (INVALID_SOCKET != this->_socket) 
        return closesocket(this->_socket);

    return 0;
}

SocketSender::~SocketSender() noexcept {
    this->UnInit();
}