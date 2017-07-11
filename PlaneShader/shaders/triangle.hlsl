struct PS_INPUT
{
  float4 p : SV_POSITION;
  float4 xywh : TEXCOORD0;
  float4 corners : TEXCOORD1;
  float outline : TEXCOORD2;
  float4 color : COLOR0;
  float4 outlinecolor : COLOR1;
};

float linetopoint(float2 p1, float2 p2, float2 p)
{
  float2 n = p2 - p1;
  n = float2(n.y, -n.x);
  return dot(normalize(n), p1 - p);
}
float4 mainPS(PS_INPUT input) : SV_Target
{
  float2 p = input.xywh.xy;
  float2 d = input.xywh.zw;
  float4 c = input.corners;
  float2 dist;
  float outline = input.outline;
  float2 p2 = float2(c.w*d.x,0);
  float r1 = linetopoint(p2, float2(0, d.y), p);
  float r2 = -linetopoint(p2, d, p);
  float r = max(r1, r2);
  r = max(r, p.y - d.y);

  float w = fwidth((p.x + p.y / 2));
  float s = 1 - smoothstep(1 - outline - w, 1 - outline, r);
  float alpha = smoothstep(1, 1 - w, r);
  float4 fill = float4(input.color.rgb, 1);
  float4 edge = float4(input.outlinecolor.rgb, 1);
  return (fill*input.color.a*s) + (edge*input.outlinecolor.a*saturate(alpha - s));
}