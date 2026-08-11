//TEST:LANG_SERVER(filecheck=CHECK):

cbuffer Shader
{
    float cbufferValue;
};

float globalValue;

float scopeTest(float parameterValue)
{
    float localValue = cbufferValue + globalValue + parameterValue;
    return localValue;
}

//SEMANTIC_TOKENS
// CHECK: legend: global local field
// CHECK: 4,10 variable global field
// CHECK: 7,6 variable global
// CHECK: 9,22 parameter
// CHECK: 11,10 variable local
// CHECK: 11,23 variable global field
// CHECK: 11,38 variable global
// CHECK: 11,52 parameter
// CHECK: 12,11 variable local
