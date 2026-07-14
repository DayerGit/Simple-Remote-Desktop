#include "DesktopDemonstrator.h"

LRESULT WINAPI DesktopDemonstrator::RemoteDesktopProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	switch (msg) {
	case WM_DESTROY: PostQuitMessage(0);
	default: return DefWindowProcW(hwnd, msg, wparam, lparam);
	}
	return 0;
}
DesktopDemonstrator::DesktopDemonstrator() noexcept {
	this->_pD3D11Device = nullptr;

	this->_pDXGIDevice = nullptr;
	this->_pDXGIAdapter = nullptr;
	this->_pDXGIFactory = nullptr;

	this->_scDesc = {};
	this->_swapChain = nullptr;

	this->_texDesc = {};

	this->_pBackBuffer = nullptr;
	this->_pRenderTexture = nullptr;

	this->_pDeviceContext = nullptr;
	this->_pRenderTargetView = nullptr;

	this->_Window = 0;

	this->_hDecompressor = (DECOMPRESSOR_HANDLE)INVALID_HANDLE_VALUE;
}

HRESULT DesktopDemonstrator::Init(int port, WNDPROC wndProc) noexcept {
	if (SOCKET_ERROR == this->_receiver.Init(port)) 
		return HRESULT_FROM_WIN32(SOCKET_ERROR);

	HRESULT hRes = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_VIDEO_SUPPORT, nullptr, 0, D3D11_SDK_VERSION, &this->_pD3D11Device, nullptr, nullptr);
	if (FAILED(hRes)) {
		this->UnInit();
		return hRes;
	}

	hRes = this->_pD3D11Device->QueryInterface(__uuidof(IDXGIDevice), (void**)&this->_pDXGIDevice);
	if (FAILED(hRes)) {
		this->UnInit();
		return hRes;
	}

	hRes = this->_pDXGIDevice->GetAdapter(&this->_pDXGIAdapter);
	if (FAILED(hRes)) {
		this->UnInit();
		return hRes;
	}

	hRes = this->_pDXGIAdapter->GetParent(__uuidof(IDXGIFactory), (void**)&this->_pDXGIFactory);
	if (FAILED(hRes)) {
		this->UnInit();
		return hRes;
	}

	WNDCLASSEXW wcl = { 0 };
	wcl.cbSize = sizeof(wcl);
	wcl.lpszClassName = L"SimpleRemoteDesktopClass";
	wcl.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wcl.hCursor = LoadCursorW(NULL, IDC_ARROW);
	wcl.lpfnWndProc = wndProc;

	RegisterClassExW(&wcl);

	this->_Window = CreateWindowExW(NULL, L"SimpleRemoteDesktopClass", L"Simple Remote Desktop", WS_POPUP, 0, 0, 1280, 720, 0, 0, 0, 0);

	this->_scDesc.BufferCount = 1;
	this->_scDesc.BufferDesc.Width = 1280;
	this->_scDesc.BufferDesc.Height = 720;
	this->_scDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	this->_scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	this->_scDesc.OutputWindow = this->_Window;
	this->_scDesc.SampleDesc.Count = 1;
	this->_scDesc.Windowed = TRUE;

	hRes = this->_pDXGIFactory->CreateSwapChain(this->_pD3D11Device, &this->_scDesc, &this->_swapChain);
	if (FAILED(hRes)) {
		this->UnInit();
		return hRes;
	}

	hRes = this->_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&this->_pBackBuffer);
	if (FAILED(hRes)) {
		this->UnInit();
		return hRes;
	}

	this->_pD3D11Device->GetImmediateContext(&this->_pDeviceContext);

	hRes = this->_pD3D11Device->CreateRenderTargetView(this->_pBackBuffer, nullptr, &this->_pRenderTargetView);
	if (FAILED(hRes)) {
		this->UnInit();
		return hRes;
	}

	this->_texDesc.Width = 1280;
	this->_texDesc.Height = 720;
	this->_texDesc.MipLevels = 1;
	this->_texDesc.ArraySize = 1;
	this->_texDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	this->_texDesc.SampleDesc.Count = 1;
	this->_texDesc.SampleDesc.Quality = 0;
	this->_texDesc.Usage = D3D11_USAGE_DEFAULT;
	this->_texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	this->_texDesc.CPUAccessFlags = 0;
	this->_texDesc.MiscFlags = 0;

	hRes = this->_pD3D11Device->CreateTexture2D(&this->_texDesc, nullptr, &this->_pRenderTexture);
	if (FAILED(hRes)) { 
		this->UnInit(); 
		return hRes; 
	}

	if (!CreateDecompressor(COMPRESS_ALGORITHM_XPRESS_HUFF, NULL, &this->_hDecompressor)) {
		this->UnInit();
		return HRESULT_FROM_WIN32(GetLastError());
	}

	return hRes;
}

HRESULT DesktopDemonstrator::Show() noexcept {
	if (!this->_receiver.Accept()) 
		return HRESULT_FROM_WIN32(SOCKET_ERROR);
	ShowWindow(this->_Window, SW_SHOWNORMAL);

	MSG message;
	Message msg;
	while (true) {
		if (PeekMessageW(&message, 0, 0, 0, 1)) {
			TranslateMessage(&message);
			DispatchMessageW(&message);
		}
		else {
			int res = this->_receiver.Receive(msg);
			if (res <= 0) break;

			if (msg.header.msgType != MessageTypes::DESKTOP_TYPE) continue;

			size_t decompressDataSize = 0;
			Decompress(this->_hDecompressor, msg.msgDesk.pData, msg.header.dataSize - sizeof(msg.msgDesk.rc), NULL, NULL, &decompressDataSize);

			auto temp = new uint8_t[decompressDataSize];
			Decompress(this->_hDecompressor, msg.msgDesk.pData, msg.header.dataSize - sizeof(msg.msgDesk.rc), temp, decompressDataSize, &decompressDataSize);

			delete[] msg.msgDesk.pData;
			msg.msgDesk.pData = temp;
			{
				D3D11_BOX box = { msg.msgDesk.rc.left, msg.msgDesk.rc.top, 0, 
								  msg.msgDesk.rc.right, msg.msgDesk.rc.bottom, 1 };

				this->_pDeviceContext->UpdateSubresource(this->_pRenderTexture, 0, &box, msg.msgDesk.pData, 
														 4 * (msg.msgDesk.rc.right - msg.msgDesk.rc.left), 0);

				this->_pDeviceContext->CopyResource(this->_pBackBuffer, this->_pRenderTexture);

				this->_swapChain->Present(0, 0);

			}
			
			delete[] msg.msgDesk.pData;
		}
	}
}

void DesktopDemonstrator::UnInit() noexcept {

	this->_receiver.UnInit();

	if (this->_pRenderTargetView) {
		this->_pRenderTargetView->Release();
		this->_pRenderTargetView = nullptr;
	}
	if (this->_pDeviceContext) {
		this->_pDeviceContext->Release();
		this->_pDeviceContext = nullptr;
	}
	if (this->_pRenderTexture) {
		this->_pRenderTexture->Release();
		this->_pRenderTexture = nullptr;
	}
	if (this->_pBackBuffer) {
		this->_pBackBuffer->Release();
		this->_pBackBuffer = nullptr;
	}
	if (this->_swapChain) {
		this->_swapChain->Release();
		this->_swapChain = nullptr;
	}
	if (this->_pDXGIFactory) {
		this->_pDXGIFactory->Release();
		this->_pDXGIFactory = nullptr;
	}
	if (this->_pDXGIAdapter) {
		this->_pDXGIAdapter->Release();
		this->_pDXGIAdapter = nullptr;
	}
	if (this->_pDXGIDevice) {
		this->_pDXGIDevice->Release();
		this->_pDXGIDevice = nullptr;
	}
	if (this->_pD3D11Device) {
		this->_pD3D11Device->Release();
		this->_pD3D11Device = nullptr;
	}
}

DesktopDemonstrator::~DesktopDemonstrator() {
	this->UnInit();
}