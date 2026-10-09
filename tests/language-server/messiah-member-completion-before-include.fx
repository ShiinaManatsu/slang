//TEST:LANG_SERVER(filecheck=CHECK):

struct Parameters
{
    half3 BaseColor;
    half Alpha;
};

void initialize(out Parameters parameters)
{
    parameters = (Parameters)0;
}

void shade(out Parameters MP)
{
    initialize(MP);
//COMPLETE:18,8
    MP.;
}

#include "member-completion-before-include.slangh"

//CHECK: --------
//CHECK-NEXT: BaseColor:
//CHECK-NEXT: Alpha:
