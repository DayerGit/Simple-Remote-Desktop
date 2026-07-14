#pragma once
#include "SocketSender.h"

#include <d3d11_4.h>
#include <compressapi.h>

class DesktopDuplicator {
public:

	DesktopDuplicator() noexcept;

	DesktopDuplicator(DesktopDuplicator&) = delete;
	DesktopDuplicator(DesktopDuplicator&&) = delete;
	DesktopDuplicator operator=(DesktopDuplicator&) = delete;
	DesktopDuplicator operator=(DesktopDuplicator&&) = delete;

	HRESULT Init(const std::string& addr, int port) noexcept;

	HRESULT Duplicate() noexcept;
	
	void UnInit() noexcept;

	~DesktopDuplicator() noexcept;

private:
	ID3D11Device* _pD3D11Device;
	ID3D11DeviceContext* _pD3D11DeviceContext;

	ID3D11Texture2D* _pStagingTexture;
	ID3D11Texture2D* _pReduceTexture;

	D3D11_BOX _D3D11_CopyBox = {0};

	ID3D11ShaderResourceView* _pD3D11ShaderResourceView;

	IDXGIDevice* _pDXGIDevice;
	IDXGIAdapter* _pDXGIAdapter;

	IDXGIOutput* _pDXGIOutput;
	IDXGIOutput1* _pDXGIOutput1;
	IDXGIOutputDuplication* _pDXGIOutputDuplication;

	IDXGIResource* _pDXGIResource;

	COMPRESSOR_HANDLE _hCompressor;

	SocketSender _sender;
};