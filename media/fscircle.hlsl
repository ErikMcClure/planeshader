struct PS_INPUT
{
  float4 p : SV_POSITION;
  float4 xywh : TEXCOORD0;
  float4 arcs : TEXCOORD1;
  float outline : TEXCOORD2;
  float4 color : COLOR0;
  float4 outlinecolor : COLOR1;
};

float4 mainPS(PS_INPUT input) : SV_Target
{
  float2 p = input.xywh.xy;
  float2 d = input.xywh.zw;
  float angle = atan2(p.y, p.x);
  float anglew = fwidth(angle);
  float r = distance(p/d, float2(0.50, 0.50))*2;
  float w = fwidth(r);
  float outline = (input.outline / d.x)*2;
  
  float s = 1 - smoothstep(1 - outline - w, 1 - outline + w, r);
  float alpha = smoothstep(1+w, 1-w, r);
  float4 fill = float4(input.color.rgb, 1);
  float4 edge = float4(input.outlinecolor.rgb, 1);
  return (fill*input.color.a*s) + (edge*input.outlinecolor.a*saturate(alpha-s));
}