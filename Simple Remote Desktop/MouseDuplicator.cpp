#include "MouseDuplicator.h"

HRESULT MouseDuplicator::Init(const std::string& addr, int port, POINT windowSize) {
	this->_windowSize = windowSize;
	return this->_sender.Init(addr, port);
}

HRESULT MouseDuplicator::Duplicate(MessageMouseTypes eventType, POINT coordPoint) {
	Message msg = {};
	msg.header.msgType = MessageTypes::MOUSE_TYPE;
	msg.header.dataSize = sizeof(msg.msgMouse);

	msg.msgMouse.event = eventType;
	msg.msgMouse.coords.x = (float)coordPoint.x / this->_windowSize.x;
	msg.msgMouse.coords.y = (float)coordPoint.y / this->_windowSize.y;

	this->_sender.Send(msg);
	return 0;
}

int MouseDuplicator::UnInit() {
	return this->_sender.UnInit();
}