float3 fresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

float3 fresnelSchlickRoughness(float cosTheta, float3 F0, float roughness)
{
    return F0 + (max(1.0 - roughness, F0) - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}   

float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
	
    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return num / denom;
}
float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
}

float3 CalculateLighting(float3 albedo, float3 radiance, float3 N, float3 V, float3 L, float roughness, float metallic)
{

    float3 F0 = 0.04;
    F0 = lerp(F0, albedo, metallic);

    float3 H = normalize(V + L);
    // cook-torrance brdf
    float NDF = DistributionGGX(N, H, roughness);        
    float G   = GeometrySmith(N, V, L, roughness);      
    float3 F  = fresnelSchlick(max(dot(H, V), 0.0), F0);       
    
    float3 kS = F;
    float3 kD = float3(1.0, 1.0, 1.0) - kS;
    kD *= 1.0 - metallic;	  
    
    float3 numerator    = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
    float3 specular     = numerator / max(denominator, 0.001);  
        
    // add to outgoing radiance Lo
    float NdotL = max(dot(N, L), 0.0);
    return (kD * albedo / PI + specular) * radiance * NdotL; 
}

float3 CalculateAmbient(float3 albedo, float3 N, float3 V, float roughness, float metallic, float ao)
{
    float3 F0 = 0.04;
    F0 = lerp(F0, albedo, metallic);

    float3 R = reflect(-V, N);
    const float MAX_REFLECTION_LOD = 3.0;
    float3 prefilteredColor = radiance_cubemap.SampleLevel(SamplerLinearWrap, R, roughness * MAX_REFLECTION_LOD).rgb;
    float3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness); 

    float2 envBRDF  = brdf_lut.Sample(SamplerLinearClamp, float2(max(dot(N, V), 0.0), 1.0f - roughness)).rg;
    float3 specular = prefilteredColor * (F * envBRDF.r + envBRDF.g);

    float3 kS = F;
    float3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;	  
    float3 irradiance = irradiance_cubemap.Sample(SamplerLinearWrap, N).rgb;
    float3 diffuse = irradiance * albedo;
    
    float3 ambient = (kD * diffuse + specular) * ao;
    return ambient;
}