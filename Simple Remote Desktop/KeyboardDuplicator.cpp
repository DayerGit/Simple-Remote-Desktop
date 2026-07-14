#include "KeyboardDuplicator.h"

int KeyboardDuplicator::Init(const std::string& addr, int port) {
	return this->_sender.Init(addr, port);
}

int KeyboardDuplicator::Duplicate(unsigned char vKey, bool isUp) {
	Message msg = {};
	msg.header.msgType = MessageTypes::KEYBOARD_TYPE;
	msg.header.dataSize = sizeof(msg.msgKeyboard);

	msg.msgKeyboard.vKey = vKey;
	msg.msgKeyboard.isUp = isUp;

	return this->_sender.Send(msg);
}

void KeyboardDuplicator::UnInit() {
	this->_sender.UnInit();
}