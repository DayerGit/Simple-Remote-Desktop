#pragma once

#include "SocketReceiver.h"

class MouseRepeater {
public:

	MouseRepeater() = default;

	MouseRepeater(MouseRepeater&) = delete;
	MouseRepeater(MouseRepeater&&) = delete;
	MouseRepeater operator=(MouseRepeater&) = delete;
	MouseRepeater operator=(MouseRepeater&&) = delete;

	int Init(int port);

	HRESULT Repeat();

	int UnInit();

	~MouseRepeater() = default;

private:
	SocketReceiver _receiver;
};