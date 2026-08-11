// unit-test-messiah-property.cpp

#include "slang-com-ptr.h"
#include "slang.h"
#include "unit-test/slang-unit-test.h"

#include <stdio.h>
#include <string.h>

using namespace Slang;

SLANG_UNIT_TEST(messiahPropertyPrelude)
{
    ComPtr<slang::IGlobalSession> globalSession;
    SLANG_CHECK_ABORT(
        slang_createGlobalSession(SLANG_API_VERSION, globalSession.writeRef()) == SLANG_OK);

    slang::TargetDesc targetDesc = {};
    targetDesc.format = SLANG_HLSL;
    targetDesc.profile = globalSession->findProfile("sm_5_0");

    slang::SessionDesc sessionDesc = {};
    sessionDesc.targetCount = 1;
    sessionDesc.targets = &targetDesc;

    ComPtr<slang::ISession> session;
    SLANG_CHECK_ABORT(globalSession->createSession(sessionDesc, session.writeRef()) == SLANG_OK);

    const char* source = R"(
@_IsFirstPass
{
    Name = IsFirstPass;
    Visible = false;
    Default = true;
};

@_MipLevels : 3
{
    Name = MipLevels;
    Visible = false;
    Default = 6;
}

texture2D legacyTexture : layout(t0) < Visible = "false"; >;
texture2DArray legacyTextureArray;
texture1D legacyTexture1D;
texture3D legacyTexture3D;
textureCube legacyTextureCube;
textureCubeArray legacyTextureCubeArray;
sampler legacySampler : layout(s0);
sampler1D legacySampler1D;
sampler2D legacySampler2D;
sampler3D legacySampler3D;
samplerCUBE legacySamplerCube;
sampler_state legacySamplerState;
samplerstate legacySamplerStateAlias;
samplercomparisonstate legacyComparisonSampler;
[@_IsFirstPass] uint conditionalGlobal = 1;

texture2D effectTexture : EffectTexture : layout(t1) < Visible = "false"; > = "White";
sampler effectSampler : layout(s1)
{
    Texture = effectTexture;
    Filter = Linear;
    Address = Wrap;
};

void stateMain(uint stateValue)
{
}

[@_IsFirstPass] technique ConditionalTechnique
{
    pass MainPass
    {
        VertexShader = stateMain();
        PixelShader = stateMain();
    }
}

RWStructuredBuffer<uint> outputBuffer;

[numthreads(1, 1, 1)]
void computeMain(uint3 threadID : SV_DispatchThreadID)
{
    MSLProperty property = 7;
    if (@_IsFirstPass || @_MipLevels > 1 || @_EngineInjectedProperty)
        outputBuffer[threadID.x] = property;
    const uint staticMask = 1;
    if ((@_MipLevels & staticMask) != 0)
        outputBuffer[threadID.x] = property;
    if ((@_IsFirstPass))
        outputBuffer[threadID.x] = property;
    [@_IsFirstPass && @_EngineInjectedProperty]
    outputBuffer[threadID.x] = conditionalGlobal;
    if (@_EngineInjectedProperty)
        outputBuffer[threadID.x] = property;
}
)";

    ComPtr<slang::IBlob> diagnostics;
    auto module = session->loadModuleFromSourceString(
        "messiahProperty",
        "messiah-property.fx",
        source,
        diagnostics.writeRef());
    if (!module && diagnostics && diagnostics->getBufferPointer())
    {
        fprintf(
            stderr,
            "messiahPropertyPrelude module diagnostics:\n%.*s\n",
            (int)diagnostics->getBufferSize(),
            (const char*)diagnostics->getBufferPointer());
    }
    SLANG_CHECK_ABORT(module != nullptr);

    ComPtr<slang::IEntryPoint> entryPoint;
    module->findAndCheckEntryPoint(
        "computeMain",
        SLANG_STAGE_COMPUTE,
        entryPoint.writeRef(),
        diagnostics.writeRef());
    SLANG_CHECK_ABORT(entryPoint != nullptr);

    if (diagnostics && diagnostics->getBufferPointer())
    {
        const char* diagnosticText = (const char*)diagnostics->getBufferPointer();
        SLANG_CHECK_ABORT(strstr(diagnosticText, "30081") == nullptr);
    }

    const char* invalidSource = R"(
@_Property
{
}

uint runtimeValue;

uint propertyWithTernary()
{
    return @_Property ? 1 : 0;
}

uint propertyWithRuntimeValue()
{
    return @_Property + runtimeValue;
}
)";

    ComPtr<slang::IBlob> invalidDiagnostics;
    session->loadModuleFromSourceString(
        "invalidMessiahProperty",
        "invalid-messiah-property.fx",
        invalidSource,
        invalidDiagnostics.writeRef());
    SLANG_CHECK_ABORT(invalidDiagnostics && invalidDiagnostics->getBufferPointer());
    const char* invalidDiagnosticText = (const char*)invalidDiagnostics->getBufferPointer();
    SLANG_CHECK_ABORT(strstr(invalidDiagnosticText, "30088") != nullptr);
    SLANG_CHECK_ABORT(strstr(invalidDiagnosticText, "30089") != nullptr);
    SLANG_CHECK_ABORT(strstr(invalidDiagnosticText, "30081") == nullptr);

    const char* staticLogicalSource = R"(
@_Property
{
}

uint propertyWithStaticLogicalExpression()
{
    if (@_Property && (1 && 1))
        return 1;
    return 0;
}
)";

    ComPtr<slang::IBlob> staticLogicalDiagnostics;
    auto staticLogicalModule = session->loadModuleFromSourceString(
        "staticLogicalMessiahProperty",
        "static-logical-messiah-property.fx",
        staticLogicalSource,
        staticLogicalDiagnostics.writeRef());
    SLANG_CHECK_ABORT(staticLogicalModule != nullptr);
    if (staticLogicalDiagnostics && staticLogicalDiagnostics->getBufferPointer())
    {
        const char* staticLogicalDiagnosticText =
            (const char*)staticLogicalDiagnostics->getBufferPointer();
        SLANG_CHECK_ABORT(strstr(staticLogicalDiagnosticText, "30088") == nullptr);
    }

    const char* duplicateTechniqueSource = R"(
float4 clipPixelShader() : SV_Target
{
    return 0;
}

technique Clip
{
    PixelShader = clipPixelShader();
}

technique Clip
{
    PixelShader = clipPixelShader();
}
)";

    ComPtr<slang::IBlob> duplicateTechniqueDiagnostics;
    auto duplicateTechniqueModule = session->loadModuleFromSourceString(
        "duplicateMessiahTechnique",
        "duplicate-messiah-technique.fx",
        duplicateTechniqueSource,
        duplicateTechniqueDiagnostics.writeRef());
    SLANG_CHECK_ABORT(duplicateTechniqueModule != nullptr);
}
