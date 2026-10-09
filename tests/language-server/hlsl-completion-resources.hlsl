#include "hlsl-completion-headers/common.hlsli"

Texture1D<float4> image1D;
Texture1DArray<float4> image1DArray;
Texture2DArray<float4> image2DArray;
Texture2DMS<float4> image2DMS;
Texture2DMSArray<float4> image2DMSArray;
Texture3D<float4> image3D;
TextureCube<float4> imageCube;
TextureCubeArray<float4> imageCubeArray;
RWTexture1D<float4> output1D;
RWTexture1DArray<float4> output1DArray;
RWTexture2DArray<float4> output2DArray;
RWTexture3D<float4> output3D;
Buffer<float4> buffer;
RWBuffer<float4> outputBuffer;
StructuredBuffer<IncludedPayload> records;
AppendStructuredBuffer<IncludedPayload> appendRecords;
ConsumeStructuredBuffer<IncludedPayload> consumeRecords;
RasterizerOrderedTexture2D<float4> orderedImage;
RasterizerOrderedByteAddressBuffer orderedBytes;
FeedbackTexture2D<SAMPLER_FEEDBACK_MIN_MIP> feedback;
SamplerState samplerState;
SamplerComparisonState comparisonState;
InputPatch<IncludedPayload, 3> inputPatch;
OutputPatch<IncludedPayload, 3> outputPatch;
ConstantBuffer<IncludedPayload> parameters;

void resourceMembers()
{
    image1D./*COMPLETE:texture1D*/GetDimensions();
    image1DArray./*COMPLETE:texture1D-array*/GetDimensions();
    image2DArray./*COMPLETE:texture2D-array*/GetDimensions();
    image2DMS./*COMPLETE:texture2D-ms*/GetDimensions();
    image2DMSArray./*COMPLETE:texture2D-ms-array*/GetDimensions();
    image3D./*COMPLETE:texture3D*/GetDimensions();
    imageCube./*COMPLETE:texture-cube*/GetDimensions();
    imageCubeArray./*COMPLETE:texture-cube-array*/GetDimensions();
    output1D./*COMPLETE:rwtexture1D*/GetDimensions();
    output1DArray./*COMPLETE:rwtexture1D-array*/GetDimensions();
    output2DArray./*COMPLETE:rwtexture2D-array*/GetDimensions();
    output3D./*COMPLETE:rwtexture3D*/GetDimensions();
    buffer./*COMPLETE:buffer*/Load(0);
    outputBuffer./*COMPLETE:rwbuffer*/Load(0);
    records./*COMPLETE:readonly-structured*/Load(0);
    appendRecords./*COMPLETE:append*/Append((IncludedPayload)0);
    consumeRecords./*COMPLETE:consume*/Consume();
    orderedImage./*COMPLETE:ordered-texture*/Load(0);
    orderedBytes./*COMPLETE:ordered-bytes*/Store(0, 0);
    feedback./*COMPLETE:feedback*/WriteSamplerFeedback(image1D, samplerState, 0);
    samplerState./*COMPLETE:sampler*/;
    comparisonState./*COMPLETE:comparison-sampler*/;
    inputPatch./*COMPLETE:input-patch*/;
    outputPatch./*COMPLETE:output-patch*/;
    parameters./*COMPLETE:constant-buffer*/color;
    parameters./*COMPLETE:constant-buffer-method*/add(0);
    records[0]./*COMPLETE:buffer-element*/color;
    RayDesc ray;
    ray./*COMPLETE:ray-desc*/Origin;
}

void geometry(inout PointStream<IncludedPayload> pointStream,
              inout LineStream<IncludedPayload> lineStream,
              inout TriangleStream<IncludedPayload> triangleStream)
{
    pointStream./*COMPLETE:point-stream*/RestartStrip();
    lineStream./*COMPLETE:line-stream*/RestartStrip();
    triangleStream./*COMPLETE:triangle-stream*/RestartStrip();
}
