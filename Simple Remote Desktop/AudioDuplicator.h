#pragma once
#include "SocketSender.h"

#include <Audioclient.h>
#include <Audiopolicy.h>
#include <mmdeviceapi.h>

class AudioDuplicator {
public:
	AudioDuplicator() noexcept;

	AudioDuplicator(AudioDuplicator&) = delete;
	AudioDuplicator(AudioDuplicator&&) = delete;
	AudioDuplicator operator=(AudioDuplicator&) = delete;
	AudioDuplicator operator=(AudioDuplicator&&) = delete;

	HRESULT Init(const std::string& addr, int port, int sampleRate = 44100, int channels = 1, int bits = 16) noexcept;

	HRESULT Duplicate() noexcept;

	void UnInit() noexcept;

	~AudioDuplicator() noexcept;
private:
	IMMDeviceEnumerator* _pEnumerator;
	IMMDevice* _pDevice;
	IAudioClient* _pAudioClient;

	IAudioCaptureClient* _pAudioCaptureClient;

	WAVEFORMATEX _format;

	UINT32 _bufferFrameCount;
	BYTE* _pData;

	SocketSender _sender;
};