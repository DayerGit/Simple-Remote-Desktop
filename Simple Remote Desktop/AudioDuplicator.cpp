#include "AudioDuplicator.h"

AudioDuplicator::AudioDuplicator() noexcept {
	this->_pEnumerator = nullptr;
	this->_pDevice = nullptr;
	this->_pAudioClient = nullptr;

	this->_pAudioCaptureClient = nullptr;

	this->_format = { 0 };

	this->_bufferFrameCount = 0;
	this->_pData = nullptr;
}

HRESULT AudioDuplicator::Init(const std::string& addr, int port, int sampleRate, int channels, int bits) noexcept {
	if (SOCKET_ERROR == this->_sender.Init(addr, port, SocketType::SOCKET_UDP)) 
		return HRESULT_FROM_WIN32(SOCKET_ERROR);

	HRESULT hRes = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), reinterpret_cast<LPVOID*>(&this->_pEnumerator));
	if (FAILED(hRes)) {
		this->UnInit();
		return hRes;
	}

	hRes = this->_pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &this->_pDevice);
	if (FAILED(hRes)) {
		this->UnInit();
		return hRes;
	}

	hRes = this->_pDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, reinterpret_cast<void**>(&this->_pAudioClient));
	if (FAILED(hRes)) {
		this->UnInit();
		return hRes;
	}

	this->_format.wFormatTag = WAVE_FORMAT_PCM;
	this->_format.nChannels = channels;
	this->_format.nSamplesPerSec = sampleRate;
	this->_format.wBitsPerSample = bits;
	this->_format.nBlockAlign = (this->_format.nChannels * this->_format.wBitsPerSample) / 8;
	this->_format.nAvgBytesPerSec = this->_format.nSamplesPerSec * this->_format.nBlockAlign;
	this->_format.cbSize = 0;

	hRes = this->_pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK | AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM | AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY, 0, 0, &this->_format, nullptr);
	if (FAILED(hRes)) {
		this->UnInit();
		return hRes;
	}

	hRes = this->_pAudioClient->GetService(__uuidof(IAudioCaptureClient), reinterpret_cast<void**>(&this->_pAudioCaptureClient));
	if (FAILED(hRes)) {
		this->UnInit();
		return hRes;
	}

	return hRes;
}

HRESULT AudioDuplicator::Duplicate() noexcept {
	HRESULT hRes = this->_pAudioClient->Start();
	if (FAILED(hRes)) return hRes;

	DWORD flags = 0;

	bool bExit = false;

	while (!bExit) {
		hRes = this->_pAudioCaptureClient->GetNextPacketSize(&this->_bufferFrameCount);

		if (!this->_bufferFrameCount) {
			Sleep(10);
			continue;
		}

		hRes = this->_pAudioCaptureClient->GetBuffer(&this->_pData, &this->_bufferFrameCount, &flags, NULL, NULL);

		if (SUCCEEDED(hRes)) {
			size_t bytesToRead = this->_bufferFrameCount * this->_format.nBlockAlign;

			Message msg;
			msg.header.msgType = MessageTypes::AUDIO_TYPE;
			msg.header.dataSize = bytesToRead;
			msg.msgAudio.pData = this->_pData;

			if (!this->_sender.Send(msg)) bExit = true;

			this->_pAudioCaptureClient->ReleaseBuffer(this->_bufferFrameCount);
		}
	}

	hRes = this->_pAudioClient->Stop();

	return hRes;
}

void AudioDuplicator::UnInit() noexcept {
	if (this->_pAudioCaptureClient != nullptr) {
		this->_pAudioCaptureClient->Release();
		this->_pAudioCaptureClient = nullptr;
	}
	if (this->_pAudioClient != nullptr) {
		this->_pAudioClient->Release();
		this->_pAudioClient = nullptr;
	}
	if (this->_pDevice != nullptr) {
		this->_pDevice->Release();
		this->_pDevice = nullptr;
	}
	if (this->_pEnumerator != nullptr) {
		this->_pEnumerator->Release();
		this->_pEnumerator = nullptr;
	}
}

AudioDuplicator::~AudioDuplicator() noexcept {
	this->UnInit();
}