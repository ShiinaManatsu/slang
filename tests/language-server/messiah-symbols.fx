//TEST:LANG_SERVER(filecheck=CHECK):

@_UseFresnel
{
    Default = true;
}

texture2D SourceTexture;
sampler SourceSampler
{
    Texture = SourceTexture;
}

float shadeMain()
{
    return 0;
}

//HOVER:22,5
//HOVER:11,15
//HOVER:26,23
[!@_UseFresnel] technique Preview
{
    pass Main
    {
        PixelShader = shadeMain();
    }
}

// CHECK: @_UseFresnel
// CHECK: SourceTexture
// CHECK: shadeMain
