//TEST:LANG_SERVER(filecheck=CHECK):

@_UseFresnel
{
    Name = UseFresnel;
    Description = "Enables Fresnel masking";
    Visible = false;
    Default = true;
}

float FresnelMask;

float applyMask(float Soft)
{
//COMPLETE:16,20
    float result = ;
    if (@_UseFresnel)
        result = FresnelMask;
//COMPLETE:20,14
    if (@_Use)
        result = FresnelMask;
    return result;
}

void lazyCompletionBody(float input)
{
    float localValue = input;
//COMPLETE:29,18
    localValue = ;
}

void lazyCompletionCaller()
{
    lazyCompletionBody(0);
}

//HOVER:17,11
// CHECK: Soft
// CHECK: localValue
// CHECK: @_UseFresnel
// CHECK: FresnelMask
// CHECK: @_UseFresnel
// CHECK: MSL metadata
// CHECK: Name: UseFresnel
// CHECK: Description: "Enables Fresnel masking"
// CHECK: Visible: false
// CHECK: Default: true
