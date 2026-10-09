//TEST:LANG_SERVER(filecheck=CHECK):

struct Parameters
{
    float3 BaseColor;
    float Alpha;
};

void initialize(out Parameters parameters)
{
    parameters.BaseColor = 0;
    parameters.Alpha = 1;
}

void shade(Parameters input)
{
    Parameters localParameters;
    initialize(localParameters);
//COMPLETE:20,11
    input.;
//COMPLETE:22,21
    localParameters.;
}

#include "member-completion-before-include.slangh"

//CHECK: --------
//CHECK-NEXT: BaseColor:
//CHECK-NEXT: Alpha:
//CHECK-NEXT: --------
//CHECK-NEXT: BaseColor:
//CHECK-NEXT: Alpha:
