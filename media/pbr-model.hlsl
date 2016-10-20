cbuffer PSobj : register(b0) {
  float4x4 matViewProj;
  float4x4 matWorld;
}

cbuffer PBRobj : register(b1) {
  float4 vecEye;
  float4 vecLight;
  float4 colorLight;
}

struct PS_INPUT
{
	float4 p : SV_POSITION; 
  float4 color : COLOR;
	float2 t : TEXCOORD;
};

const float PI = 3.14159265358979323846;

Texture2D<float4> diffusemap : register(t0); // RGB is albedo/diffuse, Alpha is material opacity
Texture2D<float4> normalmap : register(t1); // alpha channel indicated height
Texture2D<float4> specrough : register(t2); //Specular map in RGB, roughness map in alpha
SamplerState samp : register(s0);

// Schlick's Approximation for the fresnel term: https://en.wikipedia.org/wiki/Schlick%27s_approximation
// This function assumes dot(V, H) has already been computed
float3 SchlickFresnel(float3 R_0, float dotVH)
{
  // R_0 *= R_0; // Do specular maps already precompute this?
  return R_0 + (1 - R_0) * pow(1 - dotVH, 5.0);
}

// Disney's Fresnel term that compensates for roughness: https://disney-animation.s3.amazonaws.com/library/s2012_pbs_disney_brdf_notes_v2.pdf
float3 RoughFresnel(float3 color, float roughness, float dotLH, float dotNL, float dotNV)
{
  float FD90 = 0.5 + 2*dotLH*dotLH*roughness;
  return (color / PI) * (1 + (FD90 - 1)*pow((1.0 - dotNL), 5.0)) * (1 + (FD90 - 1)*pow((1.0 - dotNV), 5.0));
}

// Traditional Cook-Torrance geometric attenuation function
float GeometricAttenuation(float dotNH, float dotNV, float dotVH, float dotNL)
{
  return min(1, (2*dotNH*dotNV)/dotVH, (2*dotNH*dotNL)/dotVH);
}

// A modified, reciprical version of the GGX Geometric attenuation function (Walter, et al.): http://www.cs.cornell.edu/~srm/publications/EGSR07-btdf.pdf
float GGX_GA_recip(float dotNX, float a2) // a2 is roughness^4
{
  return dotNX + sqrt( (dotNX - dotNX * a2) * dotNX + a2 ); // Corresponds to (n∙X) + √(a² + (1 + a²)(n∙X)²)
}

// GGX Trowbridge-Reitz Normal Distribution Function: http://www.cs.cornell.edu/~srm/publications/EGSR07-btdf.pdf
float GGX_NDF(float dotNH, float a2)
{ 
  float term = dotNH*dotNH*(a2 - 1) + 1;
  return a2/(PI * term * term);
}

// Oren-Nayer reflectance model for calculating diffuse lighting.
float OrenNayarDiffuse(float3 albedo, )
{
  float C2 = ((dotEL - dotNE*dotNL) >= 0) ? min(1, dotNL/dotNE) : dotNL;
  return (albedo/PI)*E_0*(dotNL*(1 - rcp(2 + 0.65*shi)) + (rcp(2.222222 + 0.1*shi)*(dotEL - dotNE*dotNL)*C2));
}

float4 mainPS(PS_INPUT input) : SV_Target
{ 
  // TODO: Calculate this in the vector shader or outside the shader
  float4x4 m = mul(matViewProj, matWorld);
  float3 eyepos = mul(m, vecEye).xyz;
  float3 lightpos = mul(m, vecLight).xyz;
  
  float4 sr = specrough.Sample(samp, input.t);
  float4 albedo = diffusemap.Sample(samp, input.t);
  float4 nsamp = normalmap.Sample(samp, input.t);
  float opacity = albedo.a;
  albedo.a = 1.0;
  
  float3 specular = sr.rgb; // TODO: Translate this into taking a metalness map instead
  float roughness = sr.a;
  float alpha = roughness*roughness;
  float a2 = alpha*alpha;
  float ao = nsamp.b;
  
  float3 normal = float3(nsamp.x, nsamp.y, 1.0 - nsamp.x - nsamp.y);
  float3 pos = input.p.xyz;
  pos.z += nsamp.a;
  float3 total = float3(0,0,0);
  
  vec3 N = normalize(normal); // Normalized surface normal
  vec3 V = normalize(eyepos - pos); // vector between the eye position and the pixel position
  float dotNV = saturate(dot(N,V), 0.0, 1.0);
  
  // Loop through lights
  {
    vec3 L = normalize(lightpos - pos); // Vector between the light source position and the pixel position
    vec3 H = normalize(L + V); // Half-angle vector

    float dotNL = saturate(dot(N,L), 0.0, 1.0);
    float dotNH = saturate(dot(N,H), 0.0, 1.0); 
    float dotLH = saturate(dot(L,H), 0.0, 1.0); 
    float dotVH = saturate(dot(V,H), 0.0, 1.0);
    
    float3 F = SchlickFresnel(specular, dotVH);
    //float3 F = RoughFresnel(specular, roughness, dotLH, dotNL, dotNV)
    float D = GGX_NDF(dotNM, a2);
    
    // The complete Cook-Torrance equation would be this:
    //float G = GeometricAttenuation(dotNH, dotNV, dotVH, dotNL);
    // float3 frag = D*F*G/(4*dotNL*dotNV)
    
    // However, the GGX Geometric attenuation function cancels out the entire bottom half (4*dotNL*dotNV), so we use this optimized version instead 
    float G_V = GGX_GA_recip(dotNV, a2);
    float G_L = GGX_GA_recip(dotNL, a2);
    float3 spec = D*F*rcp(G_V * G_L); // Cook-torrance calculates the specular lighting value
    float3 diffuse = albedo / PI;
    float lightdist = length(pos - lightpos);
    float attenuation = PI/(lightdist * lightdist);
    
    total += colorLight * dotNL * attenuation * (diffuse * (1.0f - spec) + spec); // dotNL is used as a lambertanian diffuse term
  }
  
  // Postprocessing
  return float4(total, opacity);
}