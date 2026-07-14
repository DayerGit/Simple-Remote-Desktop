#pragma once
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <WinSock2.h>

#include "Message.h"
#include "SocketType.h"

class SocketReceiver {
public:
	SocketReceiver() noexcept;

	SocketReceiver(SocketReceiver&) = delete;
	SocketReceiver(SocketReceiver&&) = delete;
	SocketReceiver operator=(SocketReceiver&) = delete;
	SocketReceiver operator=(SocketReceiver&&) = delete;

	int Init(int port, SocketType type = SocketType::SOCKET_TCP) noexcept;

	bool Accept() noexcept;
	int Receive(Message& msg) const noexcept;
	int CloseClient() const noexcept;

	const char* GetClientIP() const noexcept;
	int GetClientPort() const noexcept;

	int UnInit();

	~SocketReceiver() noexcept;

private:
	SOCKET _socket, _clientSocket;
	SOCKADDR_IN _sa, _clientSA;
	SocketType _type;
	int _clientSALen;
};