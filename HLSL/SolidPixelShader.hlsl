cbuffer PSConstantData : register(b0) {
    float4 color;                    
    float4 backgroundColor;          
    float2 gazePointUV;                
    float aspectRatio;               
    float sizeSquared;                      
    float trail;                     
    float decay;                     
    float2 _16bytePadding;
}

SamplerState samp : register(s0);
Texture2D tex : register(t0);

struct PSInput {
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
};

float4 main(PSInput input) : SV_TARGET {
    return float4(tex.Sample(samp, input.uv).x * decay, 0.0f, 0.0f, 1.0f);
}
