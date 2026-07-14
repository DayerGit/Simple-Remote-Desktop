#pragma once
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <string>
#include <WinSock2.h>

#include "Message.h"
#include "SocketType.h"

class SocketSender {
public:
	SocketSender() noexcept;

	SocketSender(const SocketSender&) = delete;
	SocketSender(SocketSender&&) = delete;
	SocketSender& operator=(const SocketSender&) = delete;
	SocketSender operator=(SocketSender&&) = delete;

	int Init(const std::string& addr, int port, SocketType type = SocketType::SOCKET_TCP) noexcept;

	int Send(Message& msg) const noexcept;

	int UnInit();

	~SocketSender() noexcept;

private:
	SOCKET _socket;
	SOCKADDR_IN _sa;
	SocketType _type;
};