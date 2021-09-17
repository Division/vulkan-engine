// Physically Based Rendering
// Copyright (c) 2017-2018 Michał Siejak

// Physically Based shading model: Lambetrtian diffuse BRDF + Cook-Torrance microfacet specular BRDF + IBL for ambient.

// This implementation is based on "Real Shading in Unreal Engine 4" SIGGRAPH 2013 course notes by Epic Games.
// See: http://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf

// GGX/Towbridge-Reitz normal distribution function.
// Uses Disney's reparametrization of alpha = roughness^2.
float ndfGGX(float cosLh, float roughness)
{
	float alpha = roughness * roughness;
	float alphaSq = alpha * alpha;

	float denom = (cosLh * cosLh) * (alphaSq - 1.0) + 1.0;
	return alphaSq / max(PI * denom * denom, 0.00000);
}

// Single term for separable Schlick-GGX below.
float gaSchlickG1(float cosTheta, float k)
{
	return cosTheta / (cosTheta * (1.0 - k) + k);
}

// Schlick-GGX approximation of geometric attenuation function using Smith's method.
float gaSchlickGGX(float cosLi, float cosLo, float roughness)
{
	float r = roughness + 1.0;
	float k = (r * r) / 8.0; // Epic suggests using this roughness remapping for analytic lights.
	return gaSchlickG1(cosLi, k) * gaSchlickG1(cosLo, k);
}

// Shlick's approximation of the Fresnel factor.
float3 fresnelSchlick(float3 F0, float cosTheta)
{
	return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

// Returns number of mipmap levels for specular IBL environment map.
uint querySpecularTextureLevels()
{
	return 5;
}

float3 CalculateLighting(float3 albedo, float3 radiance, float3 N, float3 V, float3 L, float roughness, float metalness)
{
	float3 Li = L;
	float3 Lradiance = radiance;
	float3 Lo = V;
	// Angle between surface normal and outgoing light direction.
	float cosLo = max(0.0, dot(N, Lo));
	// Half-vector between Li and Lo.
	float3 Lh = normalize(Li + Lo);
	// Specular reflection vector.
	float3 Lr = 2.0 * cosLo * N - Lo;

	float3 Fdielectric = 0.04;
	// Fresnel reflectance at normal incidence (for metals use albedo color).
	float3 F0 = lerp(Fdielectric, albedo, metalness);

	// Calculate angles between surface normal and various light vectors.
	float cosLi = max(0.0, dot(N, Li));
	float cosLh = max(0.0, dot(N, Lh));

	// Calculate Fresnel term for direct lighting. 
	float3 F = fresnelSchlick(F0, max(0.0, dot(Lh, Lo)));
	// Calculate normal distribution for specular BRDF.
	float D = clamp(ndfGGX(cosLh, roughness), -10000, 10000);
	// Calculate geometric attenuation for specular BRDF.
	float G = gaSchlickGGX(cosLi, cosLo, roughness);

	// Diffuse scattering happens due to light being refracted multiple times by a dielectric medium.
	// Metals on the other hand either reflect or absorb energy, so diffuse contribution is always zero.
	// To be energy conserving we must scale diffuse BRDF contribution based on Fresnel factor & metalness.
	float3 kd = lerp(float3(1, 1, 1) - F, float3(0, 0, 0), metalness);

	// Lambert diffuse BRDF.
	// We don't scale by 1/PI for lighting & material units to be more convenient.
	// See: https://seblagarde.wordpress.com/2012/01/08/pi-or-not-to-pi-in-game-lighting-equation/
	float3 diffuseBRDF = kd * albedo;

	float Epsilon = 0.00001;
	// Cook-Torrance specular microfacet BRDF.
	float3 specularBRDF = (F * D * G) / max(Epsilon, 4.0 * cosLi * cosLo);
	//float3 specularBRDF = (D);

	// Total contribution for this light.
	return (diffuseBRDF + specularBRDF) * Lradiance * cosLi;
}

float3 CalculateAmbient(float3 albedo, float3 N, float3 V, float roughness, float metalness)
{
	float3 Fdielectric = 0.04;
	// Fresnel reflectance at normal incidence (for metals use albedo color).
	float3 F0 = lerp(Fdielectric, albedo, metalness);
	
	float3 Lo = V;
	// Angle between surface normal and outgoing light direction.
	float cosLo = max(0.0, dot(N, Lo));

	// Specular reflection vector.
	float3 Lr = 2.0 * cosLo * N - Lo;

	// Sample diffuse irradiance at normal direction.
	float3 irradiance = irradiance_cubemap.Sample(SamplerLinearWrap, N).rgb;

	// Calculate Fresnel term for ambient lighting.
	// Since we use pre-filtered cubemap(s) and irradiance is coming from many directions
	// use cosLo instead of angle with light's half-vector (cosLh above).
	// See: https://seblagarde.wordpress.com/2011/08/17/hello-world/
	float3 F = fresnelSchlick(F0, cosLo);

	// Get diffuse contribution factor (as with direct lighting).
	float3 kd = lerp(1.0 - F, 0.0, metalness);

	// Irradiance map contains exitant radiance assuming Lambertian BRDF, no need to scale by 1/PI here either.
	float3 diffuseIBL = kd * albedo * irradiance;

	// Sample pre-filtered specular reflection environment at correct mipmap level.
	uint specularTextureLevels = querySpecularTextureLevels();
	float3 specularIrradiance = radiance_cubemap.SampleLevel(SamplerLinearWrap, Lr, roughness * specularTextureLevels).rgb;

	float max_irradiance = 50;
	specularIrradiance = min(max_irradiance, specularIrradiance);

	// Split-sum approximation factors for Cook-Torrance specular BRDF.
	float2 specularBRDF = brdf_lut.Sample(SamplerLinearClamp, float2(cosLo, roughness)).rg;

	// Total specular IBL contribution.
	float3 specularIBL = (F0 * specularBRDF.x + specularBRDF.y) * specularIrradiance;

	// Total ambient lighting contribution.
	return diffuseIBL + specularIBL;
}