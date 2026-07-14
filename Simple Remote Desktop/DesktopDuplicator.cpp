#include "DesktopDuplicator.h"

#define TEXTURE_SCALE 2

DesktopDuplicator::DesktopDuplicator() noexcept {
	this->_pD3D11Device = nullptr;
	this->_pD3D11DeviceContext = nullptr;

	this->_pStagingTexture = nullptr;
	this->_pReduceTexture = nullptr;

	this->_D3D11_CopyBox = { 0 };

	this->_pD3D11ShaderResourceView = nullptr;

	this->_pDXGIDevice = nullptr;
	this->_pDXGIAdapter = nullptr;

	this->_pDXGIOutput = nullptr;
	this->_pDXGIOutput1 = nullptr;
	this->_pDXGIOutputDuplication = nullptr;

	this->_pDXGIResource = nullptr;

	this->_hCompressor = (COMPRESSOR_HANDLE)INVALID_HANDLE_VALUE;
}

HRESULT DesktopDuplicator::Init(const std::string& addr, int port) noexcept {
	if (SOCKET_ERROR == this->_sender.Init(addr, port)) 
		return HRESULT_FROM_WIN32(SOCKET_ERROR);

	HRESULT hRes = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_VIDEO_SUPPORT, nullptr, 0, D3D11_SDK_VERSION, &this->_pD3D11Device, nullptr, &this->_pD3D11DeviceContext);
	if (FAILED(hRes)) {
		this->UnInit();
		return hRes;
	}

	HDC hdc = GetDC(NULL);
	auto screenX = GetDeviceCaps(hdc, HORZRES);  
	auto screenY = GetDeviceCaps(hdc, VERTRES);
	ReleaseDC(NULL, hdc);

	D3D11_TEXTURE2D_DESC stagingTextureDesc = { 0 };

	stagingTextureDesc.Width = screenX / TEXTURE_SCALE;
	stagingTextureDesc.Height = screenY / TEXTURE_SCALE;

	stagingTextureDesc.MipLevels = 1;
	stagingTextureDesc.ArraySize = 1;
	stagingTextureDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	stagingTextureDesc.Usage = D3D11_USAGE_STAGING;
	stagingTextureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	stagingTextureDesc.SampleDesc.Count = 1;
	stagingTextureDesc.SampleDesc.Quality = 0;
	stagingTextureDesc.BindFlags = 0;

	this->_D3D11_CopyBox.right = screenX / TEXTURE_SCALE;
	this->_D3D11_CopyBox.bottom = screenY / TEXTURE_SCALE;
	this->_D3D11_CopyBox.back = 1;

	hRes = this->_pD3D11Device->CreateTexture2D(&stagingTextureDesc, nullptr, &this->_pStagingTexture);
	if (FAILED(hRes)) {
		this->UnInit();
		return hRes;
	}

	D3D11_TEXTURE2D_DESC reduceTextureDesc = { 0 };

	reduceTextureDesc.Width = screenX;
	reduceTextureDesc.Height = screenY;

	reduceTextureDesc.MipLevels = 0;
	reduceTextureDesc.ArraySize = 1;
	reduceTextureDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	reduceTextureDesc.Usage = D3D11_USAGE_DEFAULT;
	reduceTextureDesc.SampleDesc.Count = 1;
	reduceTextureDesc.SampleDesc.Quality = 0;
	reduceTextureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	reduceTextureDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

	hRes = this->_pD3D11Device->CreateTexture2D(&reduceTextureDesc, nullptr, &this->_pReduceTexture);
	if (FAILED(hRes)) {
		this->UnInit();
		return hRes;
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = -1;
	hRes = _pD3D11Device->CreateShaderResourceView(this->_pReduceTexture, &srvDesc, &this->_pD3D11ShaderResourceView);
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

	hRes = this->_pDXGIAdapter->EnumOutputs(0, &this->_pDXGIOutput);
	if (FAILED(hRes)) {
		this->UnInit();
		return hRes;
	}

	hRes = this->_pDXGIOutput->QueryInterface(__uuidof(IDXGIOutput1), (void**)&this->_pDXGIOutput1);
	if (FAILED(hRes)) {
		this->UnInit();
		return hRes;
	}

	hRes = this->_pDXGIOutput1->DuplicateOutput(this->_pD3D11Device, &this->_pDXGIOutputDuplication);
	if (FAILED(hRes)) {
		this->UnInit();
		return hRes;
	}

	if (!CreateCompressor(COMPRESS_ALGORITHM_XPRESS_HUFF, NULL, &this->_hCompressor)) {
		this->UnInit();
		return HRESULT_FROM_WIN32(GetLastError());
	}
	return hRes;
}

HRESULT DesktopDuplicator::Duplicate() noexcept {
	DXGI_OUTDUPL_FRAME_INFO frameInfo;
	HRESULT hRes;
	
	while (1) {
		hRes = this->_pDXGIOutputDuplication->AcquireNextFrame(100, &frameInfo, &this->_pDXGIResource);

		if (DXGI_ERROR_WAIT_TIMEOUT == hRes) continue;
		else if (FAILED(hRes)) break;

		if (frameInfo.LastPresentTime.QuadPart == 0) {
			if (this->_pDXGIResource) {
				this->_pDXGIResource->Release();
				this->_pDXGIResource = nullptr;
			}

			hRes = this->_pDXGIOutputDuplication->ReleaseFrame();
			if (FAILED(hRes)) break;

			continue;
		}

		UINT sizeOfDirtyBuf = frameInfo.TotalMetadataBufferSize;
		auto dirtyBuf = new RECT[sizeOfDirtyBuf / sizeof(RECT)];
		hRes = this->_pDXGIOutputDuplication->GetFrameDirtyRects(sizeOfDirtyBuf, dirtyBuf, &sizeOfDirtyBuf);
		bool helper = FAILED(hRes);

		if (!helper) {
			ID3D11Texture2D* pAcquiredTexture = nullptr;
			hRes = this->_pDXGIResource->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&pAcquiredTexture);

			if (SUCCEEDED(hRes)) {

				this->_pD3D11DeviceContext->CopySubresourceRegion(this->_pReduceTexture, 0, 0, 0, 0, pAcquiredTexture, 0, nullptr);
				this->_pD3D11DeviceContext->GenerateMips(this->_pD3D11ShaderResourceView);
				this->_pD3D11DeviceContext->CopySubresourceRegion(this->_pStagingTexture, 0, 0, 0, 0, this->_pReduceTexture, 1, &this->_D3D11_CopyBox);

				D3D11_MAPPED_SUBRESOURCE mappedResource;
				hRes = this->_pD3D11DeviceContext->Map(this->_pStagingTexture, 0, D3D11_MAP_READ, 0, &mappedResource);
				if (SUCCEEDED(hRes)) {
					for (auto i = 0; i < sizeOfDirtyBuf / sizeof(RECT); i++) {
						Message msg = { };
						msg.header.msgType = MessageTypes::DESKTOP_TYPE;

						msg.msgDesk.rc = { dirtyBuf[i].left / TEXTURE_SCALE, dirtyBuf[i].top / TEXTURE_SCALE , 
												 dirtyBuf[i].right / TEXTURE_SCALE , dirtyBuf[i].bottom / TEXTURE_SCALE };

						LONG dataSize = 4L * (msg.msgDesk.rc.right - msg.msgDesk.rc.left) 
										   * (msg.msgDesk.rc.bottom - msg.msgDesk.rc.top);
						
						msg.msgDesk.pData = new uint8_t[dataSize];

						BYTE* srcBase = static_cast<BYTE*>(mappedResource.pData);
						auto width = msg.msgDesk.rc.right - msg.msgDesk.rc.left;

						for (auto y = msg.msgDesk.rc.top; y < msg.msgDesk.rc.bottom; y++) {
							BYTE* srcRow = srcBase + y * mappedResource.RowPitch + msg.msgDesk.rc.left * 4;
							BYTE* dstRow = msg.msgDesk.pData + (y - msg.msgDesk.rc.top) * width * 4;
							memcpy(dstRow, srcRow, width * 4);
						}

						size_t compressDataSize = 0;
						Compress(this->_hCompressor, msg.msgDesk.pData, dataSize, NULL, NULL, &compressDataSize);

						auto temp = new uint8_t[compressDataSize];
						Compress(this->_hCompressor, msg.msgDesk.pData, dataSize, temp, compressDataSize, &compressDataSize);

						delete[] msg.msgDesk.pData;
						msg.msgDesk.pData = temp;
						if (compressDataSize > 0) {
							msg.header.dataSize = compressDataSize + sizeof(RECT);
							this->_sender.Send(msg);
						}

						delete[] msg.msgDesk.pData;
					}

					this->_pD3D11DeviceContext->Unmap(this->_pStagingTexture, 0);
				}
			}

			if (pAcquiredTexture) {
				pAcquiredTexture->Release();
				pAcquiredTexture = nullptr;
			}
		}

		if (this->_pDXGIResource) {
			this->_pDXGIResource->Release();
			this->_pDXGIResource = nullptr;
		}

		hRes = this->_pDXGIOutputDuplication->ReleaseFrame();
		if (FAILED(hRes)) break;

		delete[] dirtyBuf;
		if (helper) break;

		Sleep(33);
	}

	return hRes;
}

void DesktopDuplicator::UnInit() noexcept {

	this->_sender.UnInit();

	if (this->_hCompressor) {
		CloseCompressor(this->_hCompressor);
		this->_hCompressor = (COMPRESSOR_HANDLE)INVALID_HANDLE_VALUE;
	}
	if (this->_pDXGIOutputDuplication) {
		this->_pDXGIOutputDuplication->Release();
		this->_pDXGIOutputDuplication = nullptr;
	}
	if (this->_pDXGIOutput1) {
		this->_pDXGIOutput1->Release();
		this->_pDXGIOutput1 = nullptr;
	}
	if (this->_pDXGIOutput) {
		this->_pDXGIOutput->Release();
		this->_pDXGIOutput = nullptr;
	}
	if (this->_pDXGIAdapter) {
		this->_pDXGIAdapter->Release();
		this->_pDXGIAdapter = nullptr;
	}
	if (this->_pD3D11ShaderResourceView) {
		this->_pD3D11ShaderResourceView->Release();
		this->_pD3D11ShaderResourceView = nullptr;
	}
	if (this->_pReduceTexture) {
		this->_pReduceTexture->Release();
		this->_pReduceTexture = nullptr;
	}
	if (this->_pStagingTexture) {
		this->_pStagingTexture->Release();
		this->_pStagingTexture = nullptr;
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

DesktopDuplicator::~DesktopDuplicator() noexcept {
	this->UnInit();
}