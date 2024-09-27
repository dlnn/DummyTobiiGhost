#pragma once

#include <d3d11.h>
#include <dxgidebug.h>
#include <deque>
#include <iostream>

#include "DeviceContextStore.hpp"
#include "Common.h"
#include "Reousrce.h"
#include "Utils.hpp"


struct TobiiRenderSettings
{
	OverlayColor Color           = {0.0f, 0.74f, 1.0f, 0.8f};
	OverlayColor BackgroundColor = {0.0f, 0.0f, 0.0f, 0.0f};
	ShapeTypes   ShapeType       = Bubble;
	float        Size            = 0.5f;
	float        Trail           = 0.5f;
	float        Decay           = 0.5f;
	float        Responsiveness  = 0.25f;
	bool         Enable          = true;
};

struct TobiiRenderData
{
	OverlayColor Color           = {1.0f, 1.0f, 1.0f, 1.0f};
	OverlayColor BackgroundColor = {0.0f, 0.0f, 0.0f, 0.0f};
	ShapeTypes   ShapeType       = Bubble;
	float        Size            = 0.07f;
	float        Trail           = 0.5f;
	float        Decay           = 0.95f;
	float        Responsiveness  = 0.25f;
	bool         Enable          = true;

	bool DataIsDirty            = false;
	bool BackgroundColorIsDirty = false;

	std::deque<Point> GazePoints;
};

class TobiiRender
{
private:
	ID3D11Device*        _pDevice        = nullptr;
	ID3D11DeviceContext* _pDeviceContext = nullptr;
	ID3D11Debug*         _pDebug         = nullptr;

	IDXGISwapChain* _pDXGISwapChain = nullptr;
	IDXGIDebug1*    _pDXGIDebug     = nullptr;


	ID3D11RenderTargetView* _pMainRtv                  = nullptr;
	RenderTargetResource    _backRenderTargetResource  = {};
	RenderTargetResource    _frontRenderTargetResource = {};

	UINT          _vertexCount   = 0;
	ID3D11Buffer* _pVertexBuffer = nullptr;

	ID3D11VertexShader* _pVertexShader = nullptr;
	ID3D11InputLayout*  _pInputLayout  = nullptr;

	ID3D11Buffer*       _pPSConstantBuffer        = nullptr;
	PSConstantData*     _pPSConstantData          = nullptr;
	ID3D11SamplerState* _pSamplerState            = nullptr;
	ID3D11PixelShader*  _pPixelShaderSolid        = nullptr;
	ID3D11PixelShader*  _pPixelShaderBubble       = nullptr;
	ID3D11PixelShader*  _pPixelShaderNormalBlend  = nullptr;
	ID3D11PixelShader*  _pPixelShaderHeatmap      = nullptr;
	ID3D11PixelShader*  _pPixelShaderHeatmapBlend = nullptr;

	ID3D11RasterizerState* _pRasterizerState = nullptr;

	ShapeResource _shapeResource = {};

	HWND       _hWindow = nullptr;
	UINT       _width   = 800;
	UINT       _height  = 600;
	D3D11_RECT _dxRect  = {0, 0, static_cast<long>(_width), static_cast<long>(_height)};

	TobiiRenderData _renderData = {};

	const UINT _downsampleFactor = 4;

	bool CreateDevice()
	{
		DXGI_SWAP_CHAIN_DESC swapChainDesc;

		ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));
		swapChainDesc.BufferCount        = 2;
		swapChainDesc.BufferDesc.Width   = _width;
		swapChainDesc.BufferDesc.Height  = _height;
		swapChainDesc.BufferDesc.Format  = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.BufferUsage        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.OutputWindow       = _hWindow;
		swapChainDesc.SampleDesc.Count   = 1;
		swapChainDesc.SampleDesc.Quality = 0;
		swapChainDesc.Windowed           = TRUE;
		swapChainDesc.Flags              = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

		UINT createDeviceFlags = 0;
		createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;

		D3D_FEATURE_LEVEL           featureLevel;
		constexpr D3D_FEATURE_LEVEL featureLevelArray[2] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0,};

		auto hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, ARRAYSIZE(featureLevelArray), D3D11_SDK_VERSION, &swapChainDesc, &_pDXGISwapChain, &_pDevice, &featureLevel, &_pDeviceContext);

		if (hr == DXGI_ERROR_UNSUPPORTED)
			hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, ARRAYSIZE(featureLevelArray), D3D11_SDK_VERSION, &swapChainDesc, &_pDXGISwapChain, &_pDevice, &featureLevel, &_pDeviceContext);

		if (FAILED(hr))
		{
			std::cerr << "D3D11CreateDeviceAndSwapChain Failed: " << Utils::HrToString(hr) << std::endl;
			return false;
		}

		if (createDeviceFlags & D3D11_CREATE_DEVICE_DEBUG)
		{
			auto hr = _pDevice->QueryInterface(IID_PPV_ARGS(&_pDebug));
			if (FAILED(hr))
			{
				std::cerr << "EnableDebugLayer Failed: " << Utils::HrToString(hr) << std::endl;
			}
			else
			{
				_pDebug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
				_pDebug->Release();
			}
		}

		return true;
	}


	void CleanupDevice()
	{
		CleanupMainRenderTarget();


		Utils::SafeRelease(_pDXGISwapChain);
		Utils::SafeRelease(_pDebug);
		Utils::SafeRelease(_pDeviceContext);
		Utils::SafeRelease(_pDevice);
	}

	bool CreateMainRenderTarget()
	{
		ID3D11Texture2D* pBackBuffer;
		auto             hr = _pDXGISwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
		if (FAILED(hr))
		{
			std::cerr << "Get Back Buffer Failed: " << Utils::HrToString(hr) << std::endl;

			return false;
		}

		D3D11_TEXTURE2D_DESC tex2dDesc;
		ZeroMemory(&tex2dDesc, sizeof(D3D11_TEXTURE2D_DESC));
		pBackBuffer->GetDesc(&tex2dDesc);

		D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
		ZeroMemory(&rtvDesc, sizeof(D3D11_RENDER_TARGET_VIEW_DESC));

		rtvDesc.Format        = tex2dDesc.Format;
		rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

		hr = _pDevice->CreateRenderTargetView(pBackBuffer, &rtvDesc, &_pMainRtv);

		Utils::SafeRelease(pBackBuffer);

		if (FAILED(hr))
		{
			std::cerr << "Create Main Render Target Failed: " << Utils::HrToString(hr) << std::endl;

			return false;
		}
		return true;
	}

	void CleanupMainRenderTarget()
	{
		Utils::SafeRelease(_pMainRtv);
	}

	bool CreateBufferRenderTargetResource(RenderTargetResource& pRenderTarget) const
	{
		D3D11_TEXTURE2D_DESC texDesc;
		ZeroMemory(&texDesc, sizeof(D3D11_TEXTURE2D_DESC));
		texDesc.Width            = _width ? _width / _downsampleFactor : 1;
		texDesc.Height           = _height ? _height / _downsampleFactor : 1;
		texDesc.MipLevels        = 1;
		texDesc.ArraySize        = 1;
		texDesc.Format           = DXGI_FORMAT_R32_FLOAT;
		texDesc.SampleDesc.Count = 1;
		texDesc.Usage            = D3D11_USAGE_DEFAULT;
		texDesc.BindFlags        = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

		D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
		ZeroMemory(&rtvDesc, sizeof(D3D11_RENDER_TARGET_VIEW_DESC));
		rtvDesc.Format        = DXGI_FORMAT_R32_FLOAT;
		rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

		// @formatter:off
		if (SUCCEEDED(_pDevice->CreateTexture2D(&texDesc, nullptr, &pRenderTarget.pTexture)) &&
			SUCCEEDED(_pDevice->CreateRenderTargetView(pRenderTarget.pTexture, &rtvDesc, &pRenderTarget.pRtv)) &&
			SUCCEEDED(_pDevice->CreateShaderResourceView(pRenderTarget.pTexture, nullptr, &pRenderTarget.pSrv)))
			return true;
		// @formatter:on


		Utils::SafeRelease(pRenderTarget.pTexture);
		Utils::SafeRelease(pRenderTarget.pRtv);

		return false;
	}

	bool CreateBufferRenderTargetResource()
	{
		if (CreateBufferRenderTargetResource(_backRenderTargetResource) && CreateBufferRenderTargetResource(_frontRenderTargetResource))
		{
			return true;
		}

		CleanupBufferRenderTargetResource();
		return false;
	}

	void CleanupBufferRenderTargetResource()
	{
		_backRenderTargetResource.Release();
		_frontRenderTargetResource.Release();
	}

	bool CreateShaderResource()
	{
#pragma region VertexShader
		D3D11_BUFFER_DESC      bufferDesc;
		D3D11_SUBRESOURCE_DATA subResourceData;


		// @formatter:off
		const Vertex s_vertexData[6] =
		{
			{-1.0f, -1.0f,  0.0f,  1.0f},
			{-1.0f,  1.0f,  0.0f,  0.0f},
			{ 1.0f, -1.0f,  1.0f,  1.0f},
			{-1.0f,  1.0f,  0.0f,  0.0f},
			{ 1.0f,  1.0f,  1.0f,  0.0f},
			{ 1.0f, -1.0f,  1.0f,  1.0f}
		};
		// @formatter:on

		_vertexCount = ARRAYSIZE(s_vertexData);
		ZeroMemory(&bufferDesc, sizeof(D3D11_BUFFER_DESC));
		bufferDesc.Usage     = D3D11_USAGE_IMMUTABLE;
		bufferDesc.ByteWidth = sizeof(s_vertexData);
		bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

		ZeroMemory(&subResourceData, sizeof(D3D11_SUBRESOURCE_DATA));
		subResourceData.pSysMem = s_vertexData;


		auto hr = _pDevice->CreateBuffer(&bufferDesc, &subResourceData, &_pVertexBuffer);
		if (FAILED(hr))
		{
			std::cerr << "Create Vertex Constant Buffer Failed: " << Utils::HrToString(hr) << std::endl;
			return false;
		}

		hr = _pDevice->CreateVertexShader(Resource::VertexShaderBytes, sizeof(Resource::VertexShaderBytes), nullptr, &_pVertexShader);
		if (FAILED(hr))
		{
			std::cerr << "Create Vertex Shader Failed: " << Utils::HrToString(hr) << std::endl;
			return false;
		}

		// @formatter:off
		D3D11_INPUT_ELEMENT_DESC localLayout[] =
		{
			{"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, static_cast<UINT>(offsetof(Vertex, pos)), D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, static_cast<UINT>(offsetof(Vertex, uv)), D3D11_INPUT_PER_VERTEX_DATA, 0},
		};
		// @formatter:on

		hr = _pDevice->CreateInputLayout(localLayout, ARRAYSIZE(localLayout), Resource::VertexShaderBytes, sizeof(Resource::VertexShaderBytes), &_pInputLayout);
		if (FAILED(hr))
		{
			std::cerr << "Create Input Layout Failed: " << Utils::HrToString(hr) << std::endl;
			return false;
		}

#pragma endregion

#pragma region PixelShader

		ZeroMemory(&bufferDesc, sizeof(D3D11_BUFFER_DESC));
		bufferDesc.Usage          = D3D11_USAGE_DYNAMIC;
		bufferDesc.ByteWidth      = sizeof(PSConstantData);
		bufferDesc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
		bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

		ZeroMemory(&subResourceData, sizeof(D3D11_SUBRESOURCE_DATA));
		subResourceData.pSysMem = malloc(sizeof(PSConstantData));

		hr = _pDevice->CreateBuffer(&bufferDesc, &subResourceData, &_pPSConstantBuffer);

		if (FAILED(hr))
		{
			std::cerr << "Create Pixel Constant Buffer Failed: " << Utils::HrToString(hr) << std::endl;
			return false;
		}

		// @formatter:off
		D3D11_SAMPLER_DESC samplerDesc =
		{
			D3D11_FILTER_MIN_MAG_MIP_LINEAR,
			D3D11_TEXTURE_ADDRESS_CLAMP,
			D3D11_TEXTURE_ADDRESS_CLAMP,
			D3D11_TEXTURE_ADDRESS_CLAMP
		};
		// @formatter:on

		hr = _pDevice->CreateSamplerState(&samplerDesc, &_pSamplerState);
		if (FAILED(hr))
		{
			std::cerr << "Create Sampler State Failed: " << Utils::HrToString(hr) << std::endl;
			return false;
		}

		hr = _pDevice->CreatePixelShader(Resource::SolidPixelShaderBytes, sizeof(Resource::SolidPixelShaderBytes), nullptr, &_pPixelShaderSolid);
		if (FAILED(hr))
		{
			std::cerr << "Create Solid Pixel Shader Failed: " << Utils::HrToString(hr) << std::endl;
			return false;
		}

		hr = _pDevice->CreatePixelShader(Resource::BubblePixelShaderBytes, sizeof(Resource::BubblePixelShaderBytes), nullptr, &_pPixelShaderBubble);
		if (FAILED(hr))
		{
			std::cerr << "Create Bubble Pixel Shader Failed: " << Utils::HrToString(hr) << std::endl;
			return false;
		}

		hr = _pDevice->CreatePixelShader(Resource::NormalBlendPixelShaderBytes, sizeof(Resource::NormalBlendPixelShaderBytes), nullptr, &_pPixelShaderNormalBlend);
		if (FAILED(hr))
		{
			std::cerr << "Create Normal Blend Pixel Shader Failed: " << Utils::HrToString(hr) << std::endl;
			return false;
		}

		hr = _pDevice->CreatePixelShader(Resource::HeatmapPixelShaderBytes, sizeof(Resource::HeatmapPixelShaderBytes), nullptr, &_pPixelShaderHeatmap);
		if (FAILED(hr))
		{
			std::cerr << "Create Heatmap Pixel Shader Failed: " << Utils::HrToString(hr) << std::endl;
			return false;
		}


		hr = _pDevice->CreatePixelShader(Resource::HeatmapBlendPixelShaderBytes, sizeof(Resource::HeatmapBlendPixelShaderBytes), nullptr, &_pPixelShaderHeatmapBlend);
		if (FAILED(hr))
		{
			std::cerr << "Create Heatmap Blend Pixel Shader Failed: " << Utils::HrToString(hr) << std::endl;
			return false;
		}

#pragma endregion

#pragma region RasterizerState

		D3D11_RASTERIZER_DESC rasterizerDesc;
		ZeroMemory(&rasterizerDesc, sizeof(D3D11_RASTERIZER_DESC));

		rasterizerDesc.FillMode        = D3D11_FILL_SOLID;
		rasterizerDesc.CullMode        = D3D11_CULL_NONE;
		rasterizerDesc.DepthClipEnable = false;
		rasterizerDesc.ScissorEnable   = true;

		hr = _pDevice->CreateRasterizerState(&rasterizerDesc, &_pRasterizerState);
		if (FAILED(hr))
		{
			std::cerr << "Create Rasterizer State Failed: " << Utils::HrToString(hr) << std::endl;
			return false;
		}
#pragma endregion

		return true;
	}

	void CleanupShaderSource()
	{
		_vertexCount = 0;
		Utils::SafeRelease(_pVertexBuffer);
		Utils::SafeRelease(_pVertexShader);
		Utils::SafeRelease(_pInputLayout);

		Utils::SafeRelease(_pPSConstantBuffer);
		Utils::SafeRelease(_pSamplerState);
		Utils::SafeRelease(_pPixelShaderSolid);
		Utils::SafeRelease(_pPixelShaderBubble);
		Utils::SafeRelease(_pPixelShaderNormalBlend);
		Utils::SafeRelease(_pPixelShaderHeatmap);
		Utils::SafeRelease(_pPixelShaderHeatmapBlend);

		Utils::SafeRelease(_pRasterizerState);
	}

	bool CreateShapeResource()
	{
		UINT                pixelBytesCount = 6144;
		const unsigned int* pShapePixelData = nullptr;

		switch (_renderData.ShapeType)
		{
			case Solid:
				pShapePixelData = Resource::SolidPixelsBytes;
				break;

			case Bubble:
				pShapePixelData = Resource::BubblePixelsBytes;
				break;

			case Heatmap:
				pShapePixelData = Resource::HeatmapPixelsBytes;
				break;
		}

		if (pShapePixelData == nullptr)
			return false;


		D3D11_TEXTURE2D_DESC texDesc;
		ZeroMemory(&texDesc, sizeof(D3D11_TEXTURE2D_DESC));

		texDesc.Width            = pixelBytesCount / 4;// 4字节一个像素
		texDesc.Height           = 1;
		texDesc.MipLevels        = 1;
		texDesc.ArraySize        = 1;
		texDesc.SampleDesc.Count = 1;
		texDesc.Usage            = D3D11_USAGE_IMMUTABLE;
		texDesc.Format           = DXGI_FORMAT_R8G8B8A8_UNORM;
		texDesc.BindFlags        = D3D11_BIND_SHADER_RESOURCE;

		D3D11_SUBRESOURCE_DATA subResource;
		ZeroMemory(&subResource, sizeof(D3D11_SUBRESOURCE_DATA));

		subResource.pSysMem     = pShapePixelData;
		subResource.SysMemPitch = pixelBytesCount;

		auto hr = _pDevice->CreateTexture2D(&texDesc, &subResource, &_shapeResource.pTexture);
		if (FAILED(hr))
		{
			std::cerr << "Create Shape Texture Failed: " << Utils::HrToString(hr) << std::endl;
			return false;
		}

		hr = _pDevice->CreateShaderResourceView(_shapeResource.pTexture, nullptr, &_shapeResource.pSrv);
		if (FAILED(hr))
		{
			std::cerr << "Create Shape Srv Failed: " << Utils::HrToString(hr) << std::endl;
			_shapeResource.Release();
			return false;
		}

		return true;
	}

	void CleanupShapeResource()
	{
		_shapeResource.Release();
	}


	void RenderAndSwapBuffer()
	{
		auto pContextStore = CreateDeviceContextStore();
		{
			pContextStore->OMSetRenderTargets(1, &_backRenderTargetResource.pRtv, nullptr);

			const D3D11_RECT clipRect = {_dxRect.left / static_cast<long>(_downsampleFactor), _dxRect.top / static_cast<long>(_downsampleFactor), _dxRect.right / static_cast<long>(_downsampleFactor), _dxRect.bottom / static_cast<long>(_downsampleFactor)};
			pContextStore->RSSetScissorRects(1, &clipRect);

			const D3D11_VIEWPORT viewport = {0.0f, 0.0f, static_cast<float>(_width / _downsampleFactor), static_cast<float>(_height / _downsampleFactor), 0.0f, 1.0f};
			pContextStore->RSSetViewports(1, &viewport);

			auto pPixelShader = _pPixelShaderSolid;
			if (!_renderData.GazePoints.empty())
			{
				if (_renderData.ShapeType == Heatmap)
				{
					pPixelShader = _pPixelShaderHeatmap;
				}
				else
				{
					pPixelShader = _pPixelShaderBubble;
				}
			}
			pContextStore->PSSetShader(pPixelShader, nullptr, 0);
			pContextStore->PSSetShaderResources(0, 1, &_frontRenderTargetResource.pSrv);
			_pDeviceContext->Draw(_vertexCount, 0);

			pContextStore->Restore();
		}


		std::swap(_frontRenderTargetResource, _backRenderTargetResource);
	}

	DeviceContextStore* CreateDeviceContextStore() const
	{
		return new DeviceContextStore(_pDeviceContext);
	}

public:
	TobiiRender(HWND hwnd, UINT width, UINT height) : _hWindow(hwnd),
	                                                  _width(width),
	                                                  _height(height)
	{
	}

	~TobiiRender() { Release(); }

	bool Init()
	{
		// @formatter:off
		if (CreateDevice() &&
			CreateMainRenderTarget() &&
			CreateBufferRenderTargetResource() &&
			CreateShaderResource() &&
			CreateShapeResource())
		{
			_pPSConstantData = static_cast<PSConstantData*>(_aligned_malloc(sizeof(PSConstantData), 16));
			ZeroMemory(_pPSConstantData, sizeof(PSConstantData));

			return true;
		}
		// @formatter:on

		CleanupShapeResource();
		CleanupShaderSource();
		CleanupMainRenderTarget();
		CleanupDevice();

		return false;
	}

	void Render()
	{
		auto pContextStore = CreateDeviceContextStore();
		{
			unsigned int stride = sizeof(Vertex);
			unsigned int offset = 0;

			pContextStore->IASetInputLayout(_pInputLayout);
			pContextStore->IASetVertexBuffers(0, 1, &_pVertexBuffer, &stride, &offset);
			pContextStore->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			pContextStore->VSSetShader(_pVertexShader, nullptr, 0);

			pContextStore->RSSetState(_pRasterizerState);

			pContextStore->PSSetConstantBuffers(0, 1, &_pPSConstantBuffer);
			pContextStore->PSSetSamplers(0, 1, &_pSamplerState);

			if (_renderData.BackgroundColorIsDirty)
			{
				float clearColorWithAlpha[4] = {.0f, .0f, .0f, .0f};

				if (_renderData.Enable)
				{
					clearColorWithAlpha[0] = _renderData.BackgroundColor.R * _renderData.BackgroundColor.A;
					clearColorWithAlpha[1] = _renderData.BackgroundColor.G * _renderData.BackgroundColor.A;
					clearColorWithAlpha[2] = _renderData.BackgroundColor.B * _renderData.BackgroundColor.A;
					clearColorWithAlpha[3] = _renderData.BackgroundColor.A;
				}

				_pDeviceContext->ClearRenderTargetView(_pMainRtv, clearColorWithAlpha);

				_renderData.BackgroundColorIsDirty = false;
			}

			if (_renderData.Enable)
			{
				auto fWidth  = static_cast<float>(_width);
				auto fHeight = static_cast<float>(_height);

				if (_renderData.DataIsDirty || !_renderData.GazePoints.empty())
				{
					ZeroMemory(_pPSConstantData, sizeof(PSConstantData));


					if (!_renderData.GazePoints.empty())
					{
						auto gazePoint = _renderData.GazePoints.front();

						_pPSConstantData->GazePoint.X = gazePoint.X / fWidth;
						_pPSConstantData->GazePoint.Y = gazePoint.Y / fHeight;
					}

					_pPSConstantData->AspectRatio = fWidth / fHeight;

					static auto lastAspectRatio = 0.0f;

					if (lastAspectRatio != _pPSConstantData->AspectRatio)
					{
						lastAspectRatio = _pPSConstantData->AspectRatio;

						std::cout << "Aspect Ratio: " << lastAspectRatio << std::endl;
					}


					_pPSConstantData->Decay           = _renderData.ShapeType == Heatmap ? _renderData.Decay : 0.95f;
					_pPSConstantData->SizeSquared     = _renderData.Size * _renderData.Size;
					_pPSConstantData->Trail           = _renderData.Trail;
					_pPSConstantData->Color           = _renderData.ShapeType == Heatmap ? OverlayColor{1.0f, 1.0f, 1.0f, 0.6f} : _renderData.Color;
					_pPSConstantData->BackgroundColor = _renderData.ShapeType == Heatmap ? OverlayColor{0.0f, 0.0f, 0.0f, 0.0f} : _renderData.BackgroundColor;

					D3D11_MAPPED_SUBRESOURCE mappedResource;
					ZeroMemory(&mappedResource, sizeof(D3D11_MAPPED_SUBRESOURCE));

					auto hr = _pDeviceContext->Map(_pPSConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
					if (FAILED(hr))
					{
						std::cerr << "Map Pixel Shader Constant Buffer Failed: " << Utils::HrToString(hr) << std::endl;
						return;
					}

					memcpy(mappedResource.pData, _pPSConstantData, sizeof(PSConstantData));

					_pDeviceContext->Unmap(_pPSConstantBuffer, 0);

					_renderData.DataIsDirty = false;
				}

				RenderAndSwapBuffer();

				pContextStore->OMSetRenderTargets(1, &_pMainRtv, nullptr);
				pContextStore->RSSetScissorRects(1, &_dxRect);

				const D3D11_VIEWPORT viewport = {0.0f, 0.0f, fWidth, fHeight, 0.0f, 1.0f};
				pContextStore->RSSetViewports(1, &viewport);

				auto pPixelShader = _pPixelShaderNormalBlend;

				if (_renderData.ShapeType == Heatmap)
				{
					pPixelShader = _pPixelShaderHeatmapBlend;
				}

				pContextStore->PSSetShader(pPixelShader, nullptr, 0);
				pContextStore->PSSetShaderResources(0, 1, &_frontRenderTargetResource.pSrv);

				if (_shapeResource.pSrv)
					pContextStore->PSSetShaderResources(1, 1, &_shapeResource.pSrv);

				_pDeviceContext->Draw(_vertexCount, 0);
			}
		}
		pContextStore->Restore();
	}

	HRESULT Present(UINT SyncInterval = 1, UINT Flags = 0) const
	{
		if (_pDXGISwapChain == nullptr)
			return E_FAIL;

		return _pDXGISwapChain->Present(SyncInterval, Flags);
	}

	bool Resize(UINT width, UINT height)
	{
		if (_width == width && _height == height)
			return true;

		ZeroMemory(&_dxRect, sizeof(D3D11_RECT));
		_dxRect.right  = _width  = width;
		_dxRect.bottom = _height = height;

		_renderData.GazePoints.clear();

		CleanupBufferRenderTargetResource();
		CleanupMainRenderTarget();

		auto hr = _pDXGISwapChain->ResizeBuffers(2, _width, _height, DXGI_FORMAT_UNKNOWN, 0);
		if (FAILED(hr))
		{
			std::cerr << "Resize Buffers Failed: " << Utils::HrToString(hr) << std::endl;
			__debugbreak();
			return false;
		}

		if (CreateMainRenderTarget() && CreateBufferRenderTargetResource())
			return true;

		CleanupBufferRenderTargetResource();
		CleanupMainRenderTarget();

		return false;
	}

	void Release()
	{
		CleanupShapeResource();
		CleanupShaderSource();
		CleanupMainRenderTarget();
		CleanupDevice();
	}

	void UpdateSettings(const TobiiRenderSettings& settings)
	{
		_renderData.DataIsDirty = true;

		auto backgroundColorChanged = memcmp(&_renderData.BackgroundColor, &settings.BackgroundColor, sizeof(OverlayColor)) != 0;

		auto shapeChanged = _renderData.ShapeType != settings.ShapeType;

		if (backgroundColorChanged || _renderData.Enable != settings.Enable || shapeChanged)
		{
			_renderData.BackgroundColorIsDirty = true;
		}


		// 更新 RenderContext 的其他属性
		_renderData.ShapeType       = settings.ShapeType;
		_renderData.Enable          = settings.Enable;
		_renderData.BackgroundColor = settings.BackgroundColor;
		_renderData.Size            = std::fmax(settings.Size, 0.0f) * 0.15f;
		_renderData.Responsiveness  = settings.Responsiveness;

		if (_renderData.ShapeType == Heatmap)
		{
			//0.9975f
			_renderData.Decay = 0.9975f - settings.Decay * 0.0025f;
		}
		else
		{
			_renderData.Trail = settings.Trail;
			_renderData.Color = settings.Color;
		}

		if (shapeChanged)
		{
			_shapeResource.Release();

			// 重新创建 ShapeTypes 的 Texture 和 SRV（Shader Resource View）
			CreateShapeResource();

			FLOAT clearColor[4] = {0, 0, 0, 0};
			_pDeviceContext->ClearRenderTargetView(_frontRenderTargetResource.pRtv, clearColor);
			_pDeviceContext->ClearRenderTargetView(_backRenderTargetResource.pRtv, clearColor);
		}
	}

	void PushGazePoint(bool isActive, Point gazePoint)
	{
		if (isActive && _renderData.Enable)
		{
			auto Responsiveness = _renderData.Responsiveness * 0.9f + 0.1f;
			auto X              = gazePoint.X;
			auto Y              = gazePoint.Y;

			if (!_renderData.GazePoints.empty())
			{
				X = _renderData.GazePoints.back().X;
				Y = _renderData.GazePoints.back().Y;
			}

			for (auto i = 0; i < 3; ++i)
			{
				auto t    = (i + 1) * 0.33333334f * Responsiveness;
				auto newX = (gazePoint.X - X) * t + X;
				auto newY = (gazePoint.Y - Y) * t + Y;

				_renderData.GazePoints.push_back({newX, newY});
			}

			if (_renderData.GazePoints.size() > 3)
			{
				while (_renderData.GazePoints.size() > 3)
					_renderData.GazePoints.pop_front();
			}
		}

		if (!_renderData.GazePoints.empty())
			_renderData.GazePoints.pop_front();
	}


	ID3D11Device*        GetDevice() const { return _pDevice; }
	ID3D11DeviceContext* GetDeviceContext() const { return _pDeviceContext; }
	IDXGISwapChain*      GetSwapChain() const { return _pDXGISwapChain; }
};
