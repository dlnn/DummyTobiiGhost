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
Texture2D backgroundTex : register(t0);
Texture2D wdightTex : register(t1);

struct PSInput {
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
};

float4 main(PSInput input) : SV_TARGET {
    float4 outputColor = float4(0.0f, 0.0f, 0.0f, 1.0f);
    float alphaWdightIndex = backgroundTex.Sample(samp, input.uv).x;
    
    if (alphaWdightIndex > 0.001f) {
        float2 wdightUV = float2(saturate(alphaWdightIndex), 0.0f);
        float4 wdight = wdightTex.Sample(samp, wdightUV);
        wdight.a *= color.a;
        outputColor = wdight;
    } else {
        outputColor = backgroundColor;
    }
    outputColor = float4(outputColor.rgb * outputColor.a , outputColor.a);
    return outputColor;
}
