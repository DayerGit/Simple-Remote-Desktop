#include "SocketReceiver.h"

SocketReceiver::SocketReceiver() noexcept {
	this->_socket = this->_clientSocket = INVALID_SOCKET;
	this->_sa = this->_clientSA = { 0 };
	this->_clientSALen = sizeof(this->_clientSA);
	this->_type = SocketType::SOCKET_NONE;
}

int SocketReceiver::Init(int port, SocketType type) noexcept {
	this->_type = type;

	this->_socket = socket(AF_INET, type == SocketType::SOCKET_TCP ? SOCK_STREAM : SOCK_DGRAM, 0);
	if (INVALID_SOCKET == this->_socket) return this->_socket;

	this->_sa.sin_family = AF_INET;
	this->_sa.sin_port = htons(port);
	this->_sa.sin_addr.s_addr = INADDR_ANY;

	int Res = bind(this->_socket, (const sockaddr*) &this->_sa, sizeof(this->_sa));
	if (SOCKET_ERROR == Res) return Res;

	if (type == SocketType::SOCKET_TCP) {
		Res = listen(this->_socket, 100);
		if (SOCKET_ERROR == Res) return Res;
	}

	return Res;
}

bool SocketReceiver::Accept() noexcept {
	if (this->_type == SocketType::SOCKET_UDP) return 0;

	this->_clientSALen = sizeof(this->_clientSA);
	this->_clientSocket = accept(this->_socket, (sockaddr*)&this->_clientSA, &this->_clientSALen);
	
	return (this->_clientSocket != INVALID_SOCKET);
}

int SocketReceiver::Receive(Message& msg) const noexcept {
	int msgTypeSize = sizeof(msg.header.msgType), szSize = sizeof(size_t), receivedSize = 0;
	int received = 0;

	while (received < msgTypeSize) {
		int ret = 0;
		if (this->_type == SocketType::SOCKET_TCP) ret = recv(this->_clientSocket, (char*)&msg.header.msgType + received, msgTypeSize - received, 0);
		else ret = recvfrom(this->_socket, (char*)&msg.header.msgType + received, msgTypeSize - received, 0, 0, 0);

		if (ret <= 0) return -1;
		received += ret;
	}
	received = 0;

	while (received < szSize) {
		int ret = 0;
		if (this->_type == SocketType::SOCKET_TCP) ret = recv(this->_clientSocket, (char*)&msg.header.dataSize + received, szSize - received, 0);
		else ret = recvfrom(this->_socket, (char*)&msg.header.dataSize + received, szSize - received, 0, 0, 0);

		if (ret <= 0) return -1;
		received += ret;
	}
	received = 0;

	switch (msg.header.msgType) {
	case MessageTypes::DESKTOP_TYPE: {
		if (this->_type == SocketType::SOCKET_UDP) return -1;

		int rcSize = sizeof(msg.msgDesk.rc);

		if (msg.header.dataSize <= rcSize) return -1;

		while (received < rcSize) {
			int ret = recv(this->_clientSocket, (char*)&msg.msgDesk.rc + received, rcSize - received, 0);
			if (ret <= 0) return -1;
			received += ret;
		}
		received = 0;

		int width = msg.msgDesk.rc.right - msg.msgDesk.rc.left;
		int height = msg.msgDesk.rc.bottom - msg.msgDesk.rc.top;
		if (width <= 0 || height <= 0) {
			msg.msgDesk.pData = nullptr;
			return rcSize;
		}

		auto pixelDataSize = msg.header.dataSize - rcSize;
		msg.msgDesk.pData = new uint8_t[pixelDataSize];
		received = 0;
		while (received < pixelDataSize) {
			int ret = recv(this->_clientSocket, (char*)msg.msgDesk.pData + received, pixelDataSize - received, 0);
			if (ret <= 0) {
				delete[] msg.msgDesk.pData;
				msg.msgDesk.pData = nullptr;
				return -1;
			}
			received += ret;
		}
		receivedSize = rcSize + pixelDataSize;
		break;
	}

	case MessageTypes::MOUSE_TYPE: {
		if (this->_type == SocketType::SOCKET_UDP) return -1;

		while (received < msg.header.dataSize) {
			int ret = recv(this->_clientSocket, (char*)(&msg.msgMouse) + received, msg.header.dataSize - received, 0);
			if (ret <= 0) return -1;
			received += ret;
		}
		received = 0;

		receivedSize = msg.header.dataSize;
		break;
	}

	case MessageTypes::KEYBOARD_TYPE: {
		if (this->_type == SocketType::SOCKET_UDP) return -1;

		while (received < msg.header.dataSize) {
			int ret = recv(this->_clientSocket, (char*)(&msg.msgKeyboard) + received, msg.header.dataSize - received, 0);
			if (ret <= 0) return -1;
			received += ret;
		}
		received = 0;

		receivedSize = msg.header.dataSize;
		break;
	}

	case MessageTypes::AUDIO_TYPE: {
		msg.msgAudio.pData = new uint8_t[msg.header.dataSize];
		received = 0;
		while (received < msg.header.dataSize) {
			int ret = recvfrom(this->_socket, (char*)msg.msgAudio.pData + received, msg.header.dataSize - received, 0, 0, 0);
			if (ret <= 0) {
				delete[] msg.msgAudio.pData;
				msg.msgAudio.pData = nullptr;
				return -1;
			}
			received += ret;
		}
		break;
	}
	}

	return msgTypeSize + szSize + receivedSize;
}

int SocketReceiver::CloseClient() const noexcept {
	if (this->_type == SocketType::SOCKET_UDP) return -1;

	return closesocket(this->_clientSocket);
}

const char* SocketReceiver::GetClientIP() const noexcept {
	if (_clientSocket == INVALID_SOCKET) return {};
	return inet_ntoa(_clientSA.sin_addr);
}

int SocketReceiver::GetClientPort() const noexcept {
	if (_clientSocket == INVALID_SOCKET) return 0;
	return ntohs(_clientSA.sin_port);
}

int SocketReceiver::UnInit() {
	if (INVALID_SOCKET != this->_socket) 
		return closesocket(this->_socket);

	return 0;
}

SocketReceiver::~SocketReceiver() noexcept {
	this->UnInit();
}