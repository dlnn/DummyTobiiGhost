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
    float2 offset = (gazePointUV - input.uv) * float2(aspectRatio, 1.0f);
    
    float distSquared = dot(offset, offset);
    
    float isInsideCircle = (distSquared < (aspectRatio * aspectRatio)) ? 1.0f : 0.0f;
    
    isInsideCircle *= 1.0f - trail;
    
    isInsideCircle *= saturate((distSquared - sizeSquared) * 4.0);
    
    isInsideCircle = saturate(isInsideCircle * -5.0f + decay);
    
    float normalizedDist = saturate(distSquared / sizeSquared);
    
    normalizedDist = 1.0f - normalizedDist;
    
    normalizedDist *= 1.7f;
    
    return float4(tex.Sample(samp, input.uv).x * isInsideCircle + normalizedDist, 0.0f, 0.0f, 1.0f);
}
