
#include "globals.hlsli"

struct VS_INPUT
{
    float2 pos : POSITION;
};

struct PS_INPUT
{
    float4 pos : SV_POSITION;
};

float4 main(PS_INPUT input) : SV_Target
{
    return float4(1,0,0,1); 
}
