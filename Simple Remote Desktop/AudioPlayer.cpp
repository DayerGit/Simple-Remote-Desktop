#include "AudioPlayer.h"

#include <vector>

AudioPlayer::AudioPlayer() noexcept {
	this->_pEnumerator = nullptr;
	this->_pDevice = nullptr;
	this->_pAudioClient = nullptr;

	this->_pAudioRenderClient = nullptr;

	this->_pData = nullptr;

	this->_format = { 0 };
}

HRESULT AudioPlayer::Init(int port, int sampleRate, int channels, int bits) noexcept {
	if (SOCKET_ERROR == this->_receiver.Init(port, SocketType::SOCKET_UDP)) 
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

	hRes = this->_pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM | AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY, 0, 0, &this->_format, nullptr);
	if (FAILED(hRes)) {
		this->UnInit();
		return hRes;
	}

	hRes = this->_pAudioClient->GetService(__uuidof(IAudioRenderClient), reinterpret_cast<void**>(&this->_pAudioRenderClient));
	if (FAILED(hRes)) {
		this->UnInit();
		return hRes;
	}

	hRes = this->_pAudioClient->GetBufferSize(&this->_bufferFrameCount);
	if (FAILED(hRes)) {
		this->UnInit();
		return hRes;
	}

	this->_pAudioRenderClient->GetBuffer(this->_bufferFrameCount, &this->_pData);
	memset(reinterpret_cast<char*>(this->_pData), 0, this->_bufferFrameCount * this->_format.nBlockAlign);
	this->_pAudioRenderClient->ReleaseBuffer(this->_bufferFrameCount, 0);

	this->_hEventAudioBlock = CreateEventW(NULL, FALSE, FALSE, NULL);
	this->_pAudioClient->SetEventHandle(this->_hEventAudioBlock);

	return hRes;
}

HRESULT AudioPlayer::Play() noexcept {
	HRESULT hRes = this->_pAudioClient->Start();
	if (FAILED(hRes)) return hRes;

	UINT32 bytesToTransfer = 0;

	Message msg;

	while (1) {
		int res = this->_receiver.Receive(msg);
		if (res <= 0) break;

		WaitForSingleObject(this->_hEventAudioBlock, INFINITE);
		UINT32 padding;
		if (FAILED(this->_pAudioClient->GetCurrentPadding(&padding))) break;

		UINT32 availableFrames = this->_bufferFrameCount - padding;

		if (availableFrames > 0) {
			BYTE* pData;
			if (SUCCEEDED(this->_pAudioRenderClient->GetBuffer(availableFrames, &pData))) {
				UINT32 bytesToCopy = min(msg.header.dataSize, availableFrames * this->_format.nBlockAlign);
				memcpy(pData, msg.msgAudio.pData, bytesToCopy);
				this->_pAudioRenderClient->ReleaseBuffer(availableFrames, 0);
				delete[] msg.msgAudio.pData;
			}
		}
	}

	hRes = this->_pAudioClient->Stop();
	
	return hRes;
}

void AudioPlayer::UnInit() noexcept {
	
	if (this->_pAudioRenderClient != nullptr) {
		this->_pAudioRenderClient->Release();
		this->_pAudioRenderClient = nullptr;
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

	if(CloseHandle(this->_hEventAudioBlock))
		this->_hEventAudioBlock = INVALID_HANDLE_VALUE;
}

AudioPlayer::~AudioPlayer() noexcept {
	this->UnInit();
}