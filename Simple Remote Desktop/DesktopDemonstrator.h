#pragma once
#include "SocketReceiver.h"

#include <d3d11_4.h>
#include <compressapi.h>

class DesktopDemonstrator {
public:

	DesktopDemonstrator() noexcept;

	DesktopDemonstrator(DesktopDemonstrator&) = delete;
	DesktopDemonstrator(DesktopDemonstrator&&) = delete;
	DesktopDemonstrator operator=(DesktopDemonstrator&) = delete;
	DesktopDemonstrator operator=(DesktopDemonstrator&&) = delete;

	HRESULT Init(int port, WNDPROC wndProc = (WNDPROC)DesktopDemonstrator::RemoteDesktopProc) noexcept;

	HRESULT Show() noexcept;

	void UnInit() noexcept;

	~DesktopDemonstrator() noexcept;

private:
	ID3D11Device* _pD3D11Device;

	IDXGIDevice* _pDXGIDevice;
	IDXGIAdapter* _pDXGIAdapter;
	IDXGIFactory* _pDXGIFactory;

	DXGI_SWAP_CHAIN_DESC _scDesc;
	IDXGISwapChain* _swapChain;

	D3D11_TEXTURE2D_DESC _texDesc;

	ID3D11Texture2D* _pBackBuffer;
	ID3D11Texture2D* _pRenderTexture;

	ID3D11DeviceContext* _pDeviceContext;
	ID3D11RenderTargetView* _pRenderTargetView;

	HWND _Window;

	static LRESULT WINAPI RemoteDesktopProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

	SocketReceiver _receiver;

	DECOMPRESSOR_HANDLE _hDecompressor;
};