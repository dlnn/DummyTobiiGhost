#pragma once


enum ShapeTypes
{
	Bubble  = 0x0,
	Solid   = 0x1,
	Heatmap = 0x2,
};


struct OverlayColor
{
	float R;
	float G;
	float B;
	float A;

	bool operator==(const OverlayColor& other) const
	{
		return R == other.R && G == other.G && B == other.B && A == other.A;
	}

	bool operator!=(const OverlayColor& other) const
	{
		return !(*this == other);
	}

	OverlayColor operator*(float scalar) const
	{
		return {R * scalar, G * scalar, B * scalar, A};
	}
};


struct Point
{
	float X;
	float Y;
};


struct ShapeResource
{
	ID3D11Texture2D*          pTexture;
	ID3D11ShaderResourceView* pSrv;

	void Release()
	{
		Utils::SafeRelease(pTexture);
		Utils::SafeRelease(pSrv);
	}
};

struct RenderTargetResource
{
	ID3D11Texture2D*          pTexture;
	ID3D11RenderTargetView*   pRtv;
	ID3D11ShaderResourceView* pSrv;

	void Release()
	{
		Utils::SafeRelease(pTexture);
		Utils::SafeRelease(pRtv);
		Utils::SafeRelease(pSrv);
	}
};

struct PSConstantData
{
	OverlayColor Color;
	OverlayColor BackgroundColor;
	Point        GazePoint;
	float        AspectRatio;
	float        SizeSquared;
	float        Trail;
	float        Decay;
	float        Padding[2];
};

struct Vertex
{
	float pos[2];
	float uv[2];
};
