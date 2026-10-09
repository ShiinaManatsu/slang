#include "hlsl-completion-headers/common.hlsli"

@_UseColor
{
    Default = true;
}

float4 shade(float4 value)
{
    if (@_/*COMPLETE:property*/UseColor)
        value = includedFunction(value);
    /*COMPLETE:body*/
    return value;
}
