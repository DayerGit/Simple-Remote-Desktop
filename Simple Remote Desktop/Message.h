#pragma once
#include <stdint.h>

#include <Windows.h>

enum class MessageTypes : uint8_t {
	DESKTOP_TYPE = 0,
	MOUSE_TYPE = 1,
	KEYBOARD_TYPE = 2,
	AUDIO_TYPE = 3
};

struct MessageHeader {
	MessageTypes msgType;
	size_t dataSize;
};

struct MessageDesktop {
	RECT rc;
	uint8_t* pData;
};

enum class MessageMouseTypes : uint8_t {
	MOUSE_MOVE = 0,
	MOUSE_LBUTTON_DOWN = 1,
	MOUSE_LBUTTON_UP = 2,
	MOUSE_RBUTTON_DOWN = 3,
	MOUSE_RBUTTON_UP = 4,
};

struct MessageMouse {
	POINTFLOAT coords;
	MessageMouseTypes event;
};

struct MessageKeyboard {
	unsigned char vKey;
	bool isUp;
};

struct MessageAudio {
	uint8_t* pData;
};

struct Message {
	MessageHeader header;
	union {
		MessageDesktop msgDesk;
		MessageMouse msgMouse;
		MessageKeyboard msgKeyboard;
		MessageAudio msgAudio;
	};
};