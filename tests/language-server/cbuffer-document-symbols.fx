//TEST:LANG_SERVER(filecheck=CHECK):

#define DECL_LAYOUT_MEMBER(NAME) float3 NAME;

cbuffer LayoutProbe
{
    float scalar;
    DECL_LAYOUT_MEMBER(vectorMember)
    float4 packed;
};

// DOCUMENT_SYMBOLS
// CHECK: LayoutProbe: 3 cbuffer
// CHECK:   scalar: 8 float
// CHECK:   vectorMember: 8 float3
// CHECK:   packed: 8 float4
