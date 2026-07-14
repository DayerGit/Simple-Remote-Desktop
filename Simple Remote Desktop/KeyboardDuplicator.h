#pragma once
#include "SocketSender.h"

class KeyboardDuplicator {
public:

	KeyboardDuplicator() = default;

	KeyboardDuplicator(KeyboardDuplicator&) = delete;
	KeyboardDuplicator(KeyboardDuplicator&&) = delete;
	KeyboardDuplicator operator=(KeyboardDuplicator&) = delete;
	KeyboardDuplicator operator=(KeyboardDuplicator&&) = delete;

	int Init(const std::string& addr, int port);

	int Duplicate(unsigned char vKey, bool isUp);

	void UnInit();

	~KeyboardDuplicator() = default;

private:

	SocketSender _sender;
};