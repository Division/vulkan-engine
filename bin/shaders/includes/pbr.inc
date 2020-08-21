float ComputeLOD(float AParam, float pdf, float3 l) {
    float du = 2.0*1.2*(abs(l.z)+1.0);
    return max(0.0, AParam-0.5*log2(pdf*du*du)+1.0);
}

float GGX_PartialGeometry(float cosThetaN, float alpha) {
    float cosTheta_sqr = clamp(cosThetaN*cosThetaN, 0.0f, 1.0f);
    float tan2 = ( 1 - cosTheta_sqr ) / cosTheta_sqr;
    float GP = 2 / ( 1 + sqrt( 1 + alpha * alpha * tan2 ) );

    return GP;
}

float GGX_Distribution(float cosThetaNH, float alpha) {
    float alpha2 = alpha * alpha;
    float NH_sqr = clamp(cosThetaNH * cosThetaNH, 0.0f, 1.0f);
    float den = NH_sqr * alpha2 + (1.0 - NH_sqr);
    return alpha2 / ( PI * den * den );
}

float3 GGX_Sample(float2 E, float alpha) {
    float Phi = 2.0*PI*E.x;
    float cosThetha = saturate(sqrt( (1.0 - E.y) / (1.0 + alpha*alpha * E.y - E.y) ));
    float sinThetha = sqrt( 1.0 - cosThetha*cosThetha);
    return float3(sinThetha*cos(Phi), sinThetha*sin(Phi), cosThetha);
}

float3x3 GetSampleTransform(float3 Normal) {
  float3x3 w;
  float3 up = abs(Normal.y) < 0.999 ? float3(0,1,0) : float3(1,0,0);
  w[0] = normalize ( cross( up, Normal ) );
  w[1] = cross( Normal, w[0] );
  w[2] = Normal;
  return w;
}

float3 FresnelSchlick(float3 F0, float cosTheta) {
    return F0 + (float3(1.0, 1.0, 1.0) - F0) * pow(1.0 - clamp(cosTheta, 0.0f, 1.0f), 5.0);
}

float3 CookTorrance_GGX(float3 albedo, float3 n, float3 l, float3 v, float roughness, float3 f0, out float3 pdf) 
{
    pdf = 0.0;
    n = normalize(n);
    v = normalize(v);
    l = normalize(l);
    float3 h = normalize(v+l);
    //precompute dots
    float NL = dot(n, l);
    if (NL <= 0.000001) return float3(0.0, 0.0, 0.0);
    float NV = dot(n, v);
    if (NV <= 0.000001) return float3(0.0, 0.0, 0.0);
    float NH = dot(n, h);
    float HV = dot(h, v);
    
    //precompute roughness square
    float roug_sqr = roughness * roughness;
    
    //calc coefficients
    float G = GGX_PartialGeometry(NV, roug_sqr) * GGX_PartialGeometry(NL, roug_sqr);
    float D = GGX_Distribution(NH, roug_sqr);
    float3 F = FresnelSchlick(f0, HV);

    //mix
    float3 specK = G*D*F*0.25/(NV+0.001);    
    float3 diffK = clamp(1.0-F, 0.0f, 1.0f);
    return max(float3(0.0, 0.0, 0.0), albedo*diffK*NL/PI + specK);
}

float3 CalculateLighting(float3 albedo, float3 radiance, float3 N, float3 V, float3 L, float roughness, float metallic)
{
    //return float3(roughness, roughness, roughness);

    //float3 pdf;
    //return CookTorrance_GGX(float3(0.86, 0.176, 0), N, L, V, roughness, float3(0.24, 0.24, 0.24));

    float3 H = normalize(V + L);
    float roug_sqr = roughness * roughness;
    float G = GGX_PartialGeometry(dot(N,V), roug_sqr) * GGX_PartialGeometry(dot(N,L), roug_sqr);
    float D = GGX_Distribution(dot(N,H), roughness*roughness);

    float NL = dot(N, L);
    if (NL <= 0.0000001) return float3(0.0, 0.0, 0.0);
    float NV = dot(N, V);
    if (NV <= 0.0000001) return float3(0.0, 0.0, 0.0);
    // return float3(D, D, D);
    return float3(G, G, G);
}
