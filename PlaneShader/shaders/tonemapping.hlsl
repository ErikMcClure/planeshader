struct PS_INPUT
{
	float4 p : SV_POSITION;
  float2 t : TEXCOORD0;
};

Texture2D<float4> diffuse : register(t0);
SamplerState samp : register(s0);

float4 mainPS(PS_INPUT input) : SV_Target
{ 
  float3 c = diffuse.Sample(samp, input.t);
  float luma = 0.2126*c.r + 0.7152*c.g + 0.0722*c.b;
  return c / (1 + luma/1.0);
}