//TEST:LANG_SERVER(filecheck=CHECK):

#include "cbuffer-macro-header.fxh"

cbuffer HeaderProbe
{
    DECLARE_HEADER_MEMBERS
    float3 Padding;
};

// DOCUMENT_SYMBOLS
// CHECK: HeaderProbe: 3 cbuffer @5
// CHECK:   HeaderFirst: 8 float @7
// CHECK:   HeaderSecond: 8 float @7
// CHECK:   Padding: 8 float3 @8
