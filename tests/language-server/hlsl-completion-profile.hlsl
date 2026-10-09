/*COMPLETE:global-before*/
#include "hlsl-completion-headers/common.hlsli"
/*COMPLETE:global-after*/

Texture2D<float4> image : register(t0);
RWTexture2D<float4> outputImage : register(u0);
StructuredBuffer<IncludedPayload> payloads : register(t1);
RWStructuredBuffer<IncludedPayload> outputPayloads : register(u1);
ByteAddressBuffer bytes : register(t2);
RWByteAddressBuffer outputBytes : register(u2);
SamplerState samplerState : register(s0);
RayQuery<RAY_FLAG_NONE> rayQuery;

struct VertexInput
{
    /*COMPLETE:struct-body*/
    float4 position : /*COMPLETE:semantic*/POSITION;
};

cbuffer Parameters : register(b1)
{
    /*COMPLETE:cbuffer-body*/
    float4 tint;
};

[/*COMPLETE:attribute*/numthreads(1, 1, 1)]
void shade(uint3 threadId : SV_DispatchThreadID)
{
    float4 vectorValue = 0;
    float scalarValue = 0;
    float3x3 matrixValue = (float3x3)0;
    IncludedPayload payload;
    /*COMPLETE:body*/
    vectorValue = /*COMPLETE:expression*/includedFunction(vectorValue);
    vectorValue = includedFunction(/*COMPLETE:argument*/vectorValue);
    scalarValue = vectorValue./*COMPLETE:vector*/x;
    scalarValue = scalarValue./*COMPLETE:scalar*/x;
    scalarValue = matrixValue./*COMPLETE:matrix*/_m00;
    vectorValue = payload./*COMPLETE:user-member*/add(vectorValue);
    vectorValue = image./*COMPLETE:texture*/Load(int3(0, 0, 0));
    outputImage./*COMPLETE:rwtexture*/GetDimensions(threadId.x, threadId.y);
    payload = payloads[0];
    outputPayloads./*COMPLETE:structured*/GetDimensions(threadId.x, threadId.y);
    scalarValue = bytes./*COMPLETE:byte-buffer*/Load(0);
    outputBytes./*COMPLETE:rwbyte-buffer*/Store(0, 0);
    scalarValue = rayQuery./*COMPLETE:ray-query*/CommittedStatus();
}
