#pragma once
#include "SocketSender.h"

class MouseDuplicator {
public:

	MouseDuplicator() = default;

	MouseDuplicator(MouseDuplicator&) = delete;
	MouseDuplicator(MouseDuplicator&&) = delete;
	MouseDuplicator operator=(MouseDuplicator&) = delete;
	MouseDuplicator operator=(MouseDuplicator&&) = delete;

	HRESULT Init(const std::string& addr, int port, POINT windowSize);

	HRESULT Duplicate(MessageMouseTypes eventType, POINT coordPoint);

	int UnInit();

	~MouseDuplicator() = default;

private:
	POINT _windowSize;
	SocketSender _sender;
};