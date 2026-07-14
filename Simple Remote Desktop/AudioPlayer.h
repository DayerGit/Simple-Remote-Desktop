#pragma once
#include "SocketReceiver.h"

#include <Audioclient.h>
#include <Audiopolicy.h>
#include <mmdeviceapi.h>

class AudioPlayer {
public:

	AudioPlayer() noexcept;

	AudioPlayer(AudioPlayer&) = delete;
	AudioPlayer(AudioPlayer&&) = delete;
	AudioPlayer operator=(AudioPlayer&) = delete;
	AudioPlayer operator=(AudioPlayer&&) = delete;

	HRESULT Init(int port, int sampleRate = 44100, int channels = 1, int bits = 16) noexcept;

	HRESULT Play() noexcept;

	void UnInit() noexcept;

	~AudioPlayer() noexcept;

private:
	IMMDeviceEnumerator* _pEnumerator;
	IMMDevice* _pDevice;
	IAudioClient* _pAudioClient;

	IAudioRenderClient* _pAudioRenderClient;

	UINT32 _bufferFrameCount;
	BYTE* _pData;
	HANDLE _hEventAudioBlock;

	WAVEFORMATEX _format = { 0 };

	SocketReceiver _receiver;
};