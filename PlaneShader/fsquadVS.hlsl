struct VS_Output
{
  float4 Pos : SV_POSITION;
  float2 Tex : TEXCOORD0;
};

VS_Output main(uint id : SV_VertexID)
{
  VS_Output Output;
  Output.Tex = float2((id << 1) & 2, id & 2);
  Output.Pos = float4(Output.Tex * float2(2, -2) + float2(-1, 1), 0, 1);
  return Output;
}