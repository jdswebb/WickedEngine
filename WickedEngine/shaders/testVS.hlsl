
#include "globals.hlsli"

struct VS_INPUT
{
    float2 pos : POSITION;
};

struct PS_INPUT
{
    float4 pos : SV_POSITION;
};
 
PS_INPUT main(VS_INPUT input)
{
    PS_INPUT output;
    output.pos = float4(input.pos.x, input.pos.y, 0, 1);
    return output;
}
