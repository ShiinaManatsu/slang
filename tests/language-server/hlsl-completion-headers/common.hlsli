#define INCLUDED_MACRO 1

typedef float4 IncludedVector;

struct IncludedPayload
{
    float4 color;
    float weight;
    float4 add(float4 other) { return color + other; }
};

cbuffer IncludedConstants : register(b0)
{
    float4 includedColor;
};

float4 includedFunction(float4 value)
{
    return saturate(value + includedColor);
}
