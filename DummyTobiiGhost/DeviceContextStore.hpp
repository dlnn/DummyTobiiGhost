#pragma once
#include <d3d11.h>
#include <cstring>
#if _DEBUG
#endif

#include "Utils.hpp"

#define SAFE_RELEASE(p) if (p) { p->Release(); p = nullptr; }

#define MARK_DIRTY(flag, getter, ...) \
    if (!_isDirty_##flag) \
    { \
        _pDeviceContext->getter(__VA_ARGS__); \
        _isDirty_##flag = 1; \
    }

#define SAVE_SHADER_STATE(shaderStage, shaderName) \
    void shaderStage##SetShader(ID3D11##shaderName* shader, ID3D11ClassInstance* const* ppClassInstances, UINT numClassInstances) \
    { \
        MARK_DIRTY(shaderStage##SetShader, shaderStage##GetShader, &_originalState.shaderName, _originalState.shaderName##Instances, &_originalState.shaderName##InstancesCount); \
        _pDeviceContext->shaderStage##SetShader(shader, ppClassInstances, numClassInstances); \
    } \
    void shaderStage##SetConstantBuffers(UINT startSlot, UINT numBuffers, ID3D11Buffer* const* ppConstantBuffers) \
    { \
        MARK_DIRTY(shaderStage##SetConstantBuffers, shaderStage##GetConstantBuffers, 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, _originalState.shaderName##ConstantBuffers); \
        _pDeviceContext->shaderStage##SetConstantBuffers(startSlot, numBuffers, ppConstantBuffers); \
    } \
    void shaderStage##SetShaderResources(UINT startSlot, UINT numViews, ID3D11ShaderResourceView* const* ppShaderResourceViews) \
    { \
        MARK_DIRTY(shaderStage##SetShaderResources, shaderStage##GetShaderResources, 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, _originalState.shaderName##ShaderResourceViews); \
        _pDeviceContext->shaderStage##SetShaderResources(startSlot, numViews, ppShaderResourceViews); \
    } \
    void shaderStage##SetSamplers(UINT startSlot, UINT numSamplers, ID3D11SamplerState* const* ppSamplers) \
    { \
        MARK_DIRTY(shaderStage##SetSamplers, shaderStage##GetSamplers, 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT, _originalState.shaderName##Samplers); \
        _pDeviceContext->shaderStage##SetSamplers(startSlot, numSamplers, ppSamplers); \
    }

#define RESTORE_SHADER_STATE(shaderStage, shaderName) \
    if (_isDirty_##shaderStage##SetShader) \
    { \
        _pDeviceContext->shaderStage##SetShader(_originalState.shaderName, _originalState.shaderName##Instances, _originalState.shaderName##InstancesCount); \
    } \
    if (_isDirty_##shaderStage##SetConstantBuffers) \
    { \
        _pDeviceContext->shaderStage##SetConstantBuffers(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, _originalState.shaderName##ConstantBuffers); \
    } \
    if (_isDirty_##shaderStage##SetShaderResources) \
    { \
        _pDeviceContext->shaderStage##SetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, _originalState.shaderName##ShaderResourceViews); \
    } \
    if (_isDirty_##shaderStage##SetSamplers) \
    { \
        _pDeviceContext->shaderStage##SetSamplers(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT, _originalState.shaderName##Samplers); \
    }

#define RESTORE_NORMAL_STATE(setterFunc, ...) \
    if (_isDirty_##setterFunc) \
    { \
        _pDeviceContext->setterFunc(__VA_ARGS__); \
        Utils::SafeReleaseArgs(__VA_ARGS__); \
    }


struct StateBackup
{
#pragma region InputAssembler
    D3D11_PRIMITIVE_TOPOLOGY PrimitiveTopology;
    ID3D11Buffer*            IndexBuffer;
    DXGI_FORMAT              IndexBufferFormat;
    UINT                     IndexBufferOffset;
    ID3D11Buffer*            VertexBuffer[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
    UINT                     VertexBufferStride;
    UINT                     VertexBufferOffset;
    ID3D11InputLayout*       InputLayout;
#pragma endregion

#pragma region VertexShader
    ID3D11VertexShader*       VertexShader;
    ID3D11ClassInstance*      VertexShaderInstances[D3D11_SHADER_MAX_INTERFACES];
    UINT                      VertexShaderInstancesCount;
    ID3D11Buffer*             VertexShaderConstantBuffers[D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];
    ID3D11ShaderResourceView* VertexShaderShaderResourceViews[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
    ID3D11SamplerState*       VertexShaderSamplers[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
#pragma endregion

#pragma region HullShader
    ID3D11HullShader*         HullShader;
    ID3D11ClassInstance*      HullShaderInstances[D3D11_SHADER_MAX_INTERFACES];
    UINT                      HullShaderInstancesCount;
    ID3D11Buffer*             HullShaderConstantBuffers[D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];
    ID3D11ShaderResourceView* HullShaderShaderResourceViews[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
    ID3D11SamplerState*       HullShaderSamplers[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
#pragma endregion

#pragma region DomainShader
    ID3D11DomainShader*       DomainShader;
    ID3D11ClassInstance*      DomainShaderInstances[D3D11_SHADER_MAX_INTERFACES];
    UINT                      DomainShaderInstancesCount;
    ID3D11Buffer*             DomainShaderConstantBuffers[D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];
    ID3D11ShaderResourceView* DomainShaderShaderResourceViews[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
    ID3D11SamplerState*       DomainShaderSamplers[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
#pragma endregion

#pragma region GeometryShader
    ID3D11GeometryShader*     GeometryShader;
    ID3D11ClassInstance*      GeometryShaderInstances[D3D11_SHADER_MAX_INTERFACES];
    UINT                      GeometryShaderInstancesCount;
    ID3D11Buffer*             GeometryShaderConstantBuffers[D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];
    ID3D11ShaderResourceView* GeometryShaderShaderResourceViews[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
    ID3D11SamplerState*       GeometryShaderSamplers[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
#pragma endregion

#pragma region StreamOutput
    ID3D11Buffer* StreamOutputTargets[D3D11_SO_BUFFER_SLOT_COUNT];
    UINT          StreamOutputOffsets[D3D11_SO_BUFFER_SLOT_COUNT];
#pragma endregion

#pragma region Rasterizer
    ID3D11RasterizerState* RasterizerState;
    D3D11_VIEWPORT         Viewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
    UINT                   NumViewports;
    D3D11_RECT             ScissorRects[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
    UINT                   NumScissorRects;
#pragma endregion

#pragma region PixelShader
    ID3D11PixelShader*        PixelShader;
    ID3D11ClassInstance*      PixelShaderInstances[D3D11_SHADER_MAX_INTERFACES];
    UINT                      PixelShaderInstancesCount;
    ID3D11Buffer*             PixelShaderConstantBuffers[D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];
    ID3D11ShaderResourceView* PixelShaderShaderResourceViews[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
    ID3D11SamplerState*       PixelShaderSamplers[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
#pragma endregion

#pragma region OutputMerger
    ID3D11RenderTargetView*    RenderTargetViews[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT];
    ID3D11DepthStencilView*    DepthStencilView;
    ID3D11UnorderedAccessView* UnorderedAccessViews[D3D11_1_UAV_SLOT_COUNT];

    ID3D11BlendState*        BlendState;
    FLOAT                    BlendFactor[4];
    UINT                     SampleMask;
    ID3D11DepthStencilState* DepthStencilState;
    UINT                     StencilRef;
#pragma endregion

#pragma region ComputeShader
    ID3D11ComputeShader*       ComputeShader;
    ID3D11ClassInstance*       ComputeShaderInstances[D3D11_SHADER_MAX_INTERFACES];
    UINT                       ComputeShaderInstancesCount;
    ID3D11Buffer*              ComputeShaderConstantBuffers[D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];
    ID3D11ShaderResourceView*  ComputeShaderShaderResourceViews[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
    ID3D11SamplerState*        ComputeShaderSamplers[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
    ID3D11UnorderedAccessView* ComputeShaderUnorderedAccessViews[D3D11_1_UAV_SLOT_COUNT];
#pragma endregion

#pragma region Other
    ID3D11Predicate* Predicate;
    BOOL             PredicateValue;
#pragma endregion

    StateBackup()
    {
        memset(this, 0, sizeof(StateBackup));
    }
};


class DeviceContextStore
{
private:
    ID3D11DeviceContext* _pDeviceContext = nullptr;
    StateBackup          _originalState;

    union
    {
        ULONG _dirtyFlags;

        struct
        {
#pragma region InputAssembler
            ULONG _isDirty_IASetPrimitiveTopology : 1;
            ULONG _isDirty_IASetIndexBuffer       : 1;
            ULONG _isDirty_IASetVertexBuffers     : 1;
            ULONG _isDirty_IASetInputLayout       : 1;
#pragma endregion

#pragma region VertexShader
            ULONG _isDirty_VSSetShader          : 1;
            ULONG _isDirty_VSSetConstantBuffers : 1;
            ULONG _isDirty_VSSetShaderResources : 1;
            ULONG _isDirty_VSSetSamplers        : 1;
#pragma endregion

#pragma region HullShader
            ULONG _isDirty_HSSetShader          : 1;
            ULONG _isDirty_HSSetConstantBuffers : 1;
            ULONG _isDirty_HSSetShaderResources : 1;
            ULONG _isDirty_HSSetSamplers        : 1;
#pragma endregion

#pragma region DomainShader
            ULONG _isDirty_DSSetShader          : 1;
            ULONG _isDirty_DSSetConstantBuffers : 1;
            ULONG _isDirty_DSSetShaderResources : 1;
            ULONG _isDirty_DSSetSamplers        : 1;
#pragma endregion

#pragma region GeometryShader
            ULONG _isDirty_GSSetShader          : 1;
            ULONG _isDirty_GSSetConstantBuffers : 1;
            ULONG _isDirty_GSSetShaderResources : 1;
            ULONG _isDirty_GSSetSamplers        : 1;
#pragma endregion

#pragma region Rasterizer
            ULONG _isDirty_SOSetTargets      : 1;
            ULONG _isDirty_RSSetState        : 1;
            ULONG _isDirty_RSSetViewports    : 1;
            ULONG _isDirty_RSSetScissorRects : 1;
#pragma endregion

#pragma region PixelShader
            ULONG _isDirty_PSSetShader          : 1;
            ULONG _isDirty_PSSetConstantBuffers : 1;
            ULONG _isDirty_PSSetShaderResources : 1;
            ULONG _isDirty_PSSetSamplers        : 1;
#pragma endregion

#pragma region OutputMerger
            ULONG _isDirty_OMSetBlendState                           : 1;
            ULONG _isDirty_OMSetDepthStencilState                    : 1;
            ULONG _isDirty_OMSetRenderTargets                        : 1;
            ULONG _isDirty_OMSetRenderTargetsAndUnorderedAccessViews : 1;
#pragma endregion

#pragma region ComputeShader
            ULONG _isDirty_CSSetShader               : 1;
            ULONG _isDirty_CSSetConstantBuffers      : 1;
            ULONG _isDirty_CSSetShaderResources      : 1;
            ULONG _isDirty_CSSetSamplers             : 1;
            ULONG _isDirty_CSSetUnorderedAccessViews : 1;
#pragma endregion

#pragma region Other
            ULONG _isDirty_SetPredication : 1;
#pragma endregion
        };
    };

public:
    DeviceContextStore(ID3D11DeviceContext* pDeviceContext)
        : _pDeviceContext(pDeviceContext)
    {
        _pDeviceContext->AddRef();
        _dirtyFlags = 0;
    }

    ~DeviceContextStore()
    {
        Restore();
        _pDeviceContext->Release();
    }

    ID3D11DeviceContext* GetRawContext() const
    {
        return _pDeviceContext;
    }

#pragma region InputAssembler
    void IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY topology)
    {
        MARK_DIRTY(IASetPrimitiveTopology, IAGetPrimitiveTopology, &_originalState.PrimitiveTopology);
        _pDeviceContext->IASetPrimitiveTopology(topology);
    }

    void IASetIndexBuffer(ID3D11Buffer* indexBuffer, DXGI_FORMAT format, UINT offset)
    {
        MARK_DIRTY(IASetIndexBuffer, IAGetIndexBuffer, &_originalState.IndexBuffer, &_originalState.IndexBufferFormat, &_originalState.IndexBufferOffset);
        _pDeviceContext->IASetIndexBuffer(indexBuffer, format, offset);
    }

    void IASetVertexBuffers(UINT startSlot, UINT numBuffers, ID3D11Buffer* const* vertexBuffers, const UINT* strides, const UINT* offsets)
    {
        MARK_DIRTY(IASetVertexBuffers, IAGetVertexBuffers, 0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT, _originalState.VertexBuffer, &_originalState.VertexBufferStride, &_originalState.VertexBufferOffset);
        _pDeviceContext->IASetVertexBuffers(startSlot, numBuffers, vertexBuffers, strides, offsets);
    }

    void IASetInputLayout(ID3D11InputLayout* inputLayout)
    {
        MARK_DIRTY(IASetInputLayout, IAGetInputLayout, &_originalState.InputLayout);
        _pDeviceContext->IASetInputLayout(inputLayout);
    }
#pragma endregion

    SAVE_SHADER_STATE(VS, VertexShader)
    SAVE_SHADER_STATE(HS, HullShader)
    SAVE_SHADER_STATE(DS, DomainShader)
    SAVE_SHADER_STATE(GS, GeometryShader)
    SAVE_SHADER_STATE(PS, PixelShader)
    SAVE_SHADER_STATE(CS, ComputeShader)

    void CSSetUnorderedAccessViews(UINT startSlot, UINT numUAVs, ID3D11UnorderedAccessView* const* ppUnorderedAccessViews, const UINT* pUAVInitialCounts)
    {
        MARK_DIRTY(CSSetUnorderedAccessViews, CSGetUnorderedAccessViews, 0, D3D11_1_UAV_SLOT_COUNT, _originalState.ComputeShaderUnorderedAccessViews);
        _pDeviceContext->CSSetUnorderedAccessViews(startSlot, numUAVs, ppUnorderedAccessViews, pUAVInitialCounts);
    }


#pragma region Rasterizer
    void SOSetTargets(UINT numBuffers, ID3D11Buffer* const* ppSOTargets, const UINT* pOffsets)
    {
        MARK_DIRTY(SOSetTargets, SOGetTargets, D3D11_SO_BUFFER_SLOT_COUNT, _originalState.StreamOutputTargets);

        for (UINT i = 0; i < D3D11_SO_BUFFER_SLOT_COUNT; ++i)
        {
            if (_originalState.StreamOutputTargets[i])
            {
                D3D11_BUFFER_DESC desc;
                _originalState.StreamOutputTargets[i]->GetDesc(&desc);
                _originalState.StreamOutputOffsets[i] = desc.StructureByteStride;
            }
            else
            {
                _originalState.StreamOutputOffsets[i] = 0;
            }
        }
        _pDeviceContext->SOSetTargets(numBuffers, ppSOTargets, pOffsets);
    }

    void RSSetState(ID3D11RasterizerState* pRasterizerState)
    {
        MARK_DIRTY(RSSetState, RSGetState, &_originalState.RasterizerState);
        _pDeviceContext->RSSetState(pRasterizerState);
    }

    void RSSetViewports(UINT numViewports, const D3D11_VIEWPORT* pViewports)
    {
        MARK_DIRTY(RSSetViewports, RSGetViewports, &_originalState.NumViewports, _originalState.Viewports);
        _pDeviceContext->RSSetViewports(numViewports, pViewports);
    }

    void RSSetScissorRects(UINT numRects, const D3D11_RECT* pRects)
    {
        MARK_DIRTY(RSSetScissorRects, RSGetScissorRects, &_originalState.NumScissorRects, _originalState.ScissorRects);
        _pDeviceContext->RSSetScissorRects(numRects, pRects);
    }
#pragma endregion

#pragma region OutputMerger
    void OMSetRenderTargets(UINT numViews, ID3D11RenderTargetView* const* ppRenderTargetViews, ID3D11DepthStencilView* pDepthStencilView)
    {
        MARK_DIRTY(OMSetRenderTargets, OMGetRenderTargets, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, _originalState.RenderTargetViews, &_originalState.DepthStencilView);
        _pDeviceContext->OMSetRenderTargets(numViews, ppRenderTargetViews, pDepthStencilView);
    }

    void OMSetRenderTargetsAndUnorderedAccessViews(UINT numRTVs, ID3D11RenderTargetView* const* ppRenderTargetViews, ID3D11DepthStencilView* pDepthStencilView, UINT uavStartSlot, UINT numUAVs, ID3D11UnorderedAccessView* const* ppUnorderedAccessViews, const UINT* pUAVInitialCounts)
    {
        MARK_DIRTY(OMSetRenderTargetsAndUnorderedAccessViews, OMGetRenderTargetsAndUnorderedAccessViews, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, _originalState.RenderTargetViews, &_originalState.DepthStencilView, 0, D3D11_1_UAV_SLOT_COUNT, _originalState.UnorderedAccessViews);
        _pDeviceContext->OMSetRenderTargetsAndUnorderedAccessViews(numRTVs, ppRenderTargetViews, pDepthStencilView, uavStartSlot, numUAVs, ppUnorderedAccessViews, pUAVInitialCounts);
    }

    void OMSetBlendState(ID3D11BlendState* pBlendState, const FLOAT blendFactor[4], UINT sampleMask)
    {
        MARK_DIRTY(OMSetBlendState, OMGetBlendState, &_originalState.BlendState, _originalState.BlendFactor, &_originalState.SampleMask);
        _pDeviceContext->OMSetBlendState(pBlendState, blendFactor, sampleMask);
    }

    void OMSetDepthStencilState(ID3D11DepthStencilState* pDepthStencilState, UINT stencilRef)
    {
        MARK_DIRTY(OMSetDepthStencilState, OMGetDepthStencilState, &_originalState.DepthStencilState, &_originalState.StencilRef);
        _pDeviceContext->OMSetDepthStencilState(pDepthStencilState, stencilRef);
    }
#pragma endregion

#pragma region Other
    void SetPredication(ID3D11Predicate* pPredicate, BOOL predicateValue)
    {
        MARK_DIRTY(SetPredication, GetPredication, &_originalState.Predicate, &_originalState.PredicateValue);
        _pDeviceContext->SetPredication(pPredicate, predicateValue);
    }
#pragma endregion


    void Restore()
    {
        if (_dirtyFlags == 0)
            return;

        // IA
        RESTORE_NORMAL_STATE(IASetPrimitiveTopology, _originalState.PrimitiveTopology)
        RESTORE_NORMAL_STATE(IASetIndexBuffer, _originalState.IndexBuffer, _originalState.IndexBufferFormat, _originalState.IndexBufferOffset)
        RESTORE_NORMAL_STATE(IASetVertexBuffers, 0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT, _originalState.VertexBuffer, &_originalState.VertexBufferStride, &_originalState.VertexBufferOffset)
        RESTORE_NORMAL_STATE(IASetInputLayout, _originalState.InputLayout)

        // VS -> HS -> DS -> GS
        RESTORE_SHADER_STATE(VS, VertexShader)
        RESTORE_SHADER_STATE(HS, HullShader)
        RESTORE_SHADER_STATE(DS, DomainShader)
        RESTORE_SHADER_STATE(GS, GeometryShader)

        // RS
        RESTORE_NORMAL_STATE(SOSetTargets, D3D11_SO_BUFFER_SLOT_COUNT, _originalState.StreamOutputTargets, _originalState.StreamOutputOffsets)
        RESTORE_NORMAL_STATE(RSSetState, _originalState.RasterizerState)
        RESTORE_NORMAL_STATE(RSSetViewports, _originalState.NumViewports, _originalState.Viewports)
        RESTORE_NORMAL_STATE(RSSetScissorRects, _originalState.NumScissorRects, _originalState.ScissorRects)

        // PS
        RESTORE_SHADER_STATE(PS, PixelShader)

        // OM
        RESTORE_NORMAL_STATE(OMSetBlendState, _originalState.BlendState, _originalState.BlendFactor, _originalState.SampleMask)
        RESTORE_NORMAL_STATE(OMSetDepthStencilState, _originalState.DepthStencilState, _originalState.StencilRef)
        RESTORE_NORMAL_STATE(OMSetRenderTargets, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, _originalState.RenderTargetViews, _originalState.DepthStencilView)
        RESTORE_NORMAL_STATE(OMSetRenderTargetsAndUnorderedAccessViews, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, _originalState.RenderTargetViews, _originalState.DepthStencilView, 0, D3D11_1_UAV_SLOT_COUNT, _originalState.UnorderedAccessViews, nullptr)

        // 独立CS
        RESTORE_SHADER_STATE(CS, ComputeShader)
        RESTORE_NORMAL_STATE(CSSetUnorderedAccessViews, 0, D3D11_1_UAV_SLOT_COUNT, _originalState.ComputeShaderUnorderedAccessViews, nullptr);

        RESTORE_NORMAL_STATE(SetPredication, _originalState.Predicate, _originalState.PredicateValue)

        _dirtyFlags = 0;
    }
};

#undef RESTORE_NORMAL_STATE
#undef RESTORE_SHADER_STATE
#undef SAVE_SHADER_STATE
#undef MARK_DIRTY
