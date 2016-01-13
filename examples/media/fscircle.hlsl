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
  float r = distance(p/d, float2(0.50, 0.50));
  float rw = fwidth(r);
  smoothstep(input.arcs.y - anglew, input.arcs.y + anglew, angle)*smoothstep(input.arcs.x - anglew, input.arcs.x + anglew
  
  float delta = fwidth(dist);
  float alpha = 1.0-smoothstep(0.45-delta, 0.45, dist);
	return diffuse.Sample(samp, input.t)*input.color*float4(1,1,1,alpha);
}