#pragma once
#include "SocketReceiver.h"

class KeyboardRepeater {
public:

	KeyboardRepeater() = default;

	KeyboardRepeater(KeyboardRepeater&) = delete;
	KeyboardRepeater(KeyboardRepeater&&) = delete;
	KeyboardRepeater operator=(KeyboardRepeater&) = delete;
	KeyboardRepeater operator=(KeyboardRepeater&&) = delete;

	int Init(int port);

	int Repeat();

	void UnInit();

	~KeyboardRepeater() = default;

private:
	SocketReceiver _receiver;
};