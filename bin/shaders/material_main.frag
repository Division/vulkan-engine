#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

#define PI 3.1415926535

layout(std140, set = 0, binding = 0) uniform Camera {
    vec3 cameraPosition;
    ivec2 cameraScreenSize;
    mat4 cameraViewMatrix;
    mat4 cameraProjectionMatrix;
} camera;

layout(std140, set = 1, binding = 1) uniform ObjectParams {
    mat4 objectModelMatrix;
    mat4 objectNormalMatrix;
    vec2 uvScale;
    vec2 uvOffset;
    uint layer;
    float roughness;
} object_params;

#if defined(TEXTURE0)
layout(set = 1, binding = 2) uniform sampler2D texture0;
layout(location = 1) in vec2 fragTexCoord;
#endif

layout(location = 0) in vec3 fragColor;
layout(location = 2) in vec4 position_worldspace;
#if defined (LIGHTING)
layout(location = 4) in vec3 normal_worldspace;
#endif

layout(location = 0) out vec4 out_color;

#if defined (LIGHTING)
layout(set = 0, binding = 3) uniform sampler2D shadow_map;
layout(set = 0, binding = 10) uniform samplerCube environment_cubemap;
layout(set = 0, binding = 11) uniform samplerCube radiance_cubemap;
layout(set = 0, binding = 12) uniform samplerCube irradiance_cubemap;
/*Texture2D shadowMap : register(t7);

SamplerState shadowMapSampler : register(s7);
Texture2D projectorTexture : register(t8);
SamplerState projectorSampler : register(s8);
*/

struct LightGridItem
{
    uint offset;
    uint lightsCount;
    uint projectorsCount;
};

struct LightItem
{
	vec3 position;
	float attenuation;
	vec3 color;
	float radius;
	vec3 direction;
	float coneAngle;
	mat4 projectionMatrix;
	vec2 shadowmapScale;
	vec2 shadowmapOffset;
	uint mask;
};

struct ProjectorItem
{
	vec3 position;
	float attenuation;
	vec4 color;
	vec2 scale;
	vec2 offset;
	vec2 shadowmapScale;
	vec2 shadowmapOffset;
	mat4 projectionMatrix;
	float radius;
	uint mask;
};

layout(std140, set = 0, binding = 5) uniform Lights {
    LightItem lights[100];
};

layout(std140, set = 0, binding = 6) uniform Projectors {
    ProjectorItem projectors[100];
};

readonly layout(std430, set = 0, binding = 7) buffer LightGrid
{
    LightGridItem light_grid[];
};

readonly layout(std430, set = 0, binding = 8) buffer LightIndices
{
    uint light_indices[];
};

float GetAttenuation(float distance, float radius)
{
    float lightInnerR = radius * 0.1;
    float invLightOuterR = 1.0f / radius;
    float d = max(distance, lightInnerR);
    return clamp((1.0 - pow(d * invLightOuterR, 4.0)) / (d * d + 1.0), 0.0, 1.0);
}

vec3 calculateFragmentDiffuse(float normalizedDistanceToLight, float attenuation, vec3 normal, vec3 lightDir, vec3 eyeDir, vec3 lightColor, float materialSpecular) {
  float lightValue = clamp(dot(-lightDir, normal), 0.0, 1.0);
  float attenuationValue = pow(max(1.0 - normalizedDistanceToLight, 0.0), attenuation);
  vec3 diffuse = lightColor * lightValue;

  vec3 reflected = reflect(lightDir, normal);
  float cosAlpha = clamp(dot(eyeDir, reflected), 0.0, 1.0);
  //vec3 specular = pow(cosAlpha, 32.0) * lightColor * materialSpecular;
  vec3 specular = vec3(0,0,0);
  return attenuationValue * (diffuse + specular);
}


float calculateFragmentShadow(vec2 uv, float fragmentDepth) {
  float shadow = 0.0;
  float bias = 0.001;
  ivec2 size = textureSize(shadow_map, 0);
  vec2 texelSize = 1.0 / vec2(size.x, size.y);

  //[unroll]
  for(int x = -1; x <= 1; x++) {
    for(int y = -1; y <= 1; y++) {
      float closestDepth = texture(shadow_map, uv + vec2(x, y) * texelSize).r;
      shadow += fragmentDepth - bias > closestDepth ? 1.0 : 0.0;
    }
  }

  shadow /= 9.0;
  return shadow;
}

#include "includes/pbr_t.inc"

#endif

void main() {
	out_color = vec4(1,1,1,1);
	
#if defined(TEXTURE0)
    vec4 texture0_color = texture(texture0, fragTexCoord);
	out_color *= texture0_color;
#endif

#if defined(LIGHTING)
	vec4 light_color = vec4(0);
    float TILE_SIZE = 32.0;
    vec2 screenSize = vec2(camera.cameraScreenSize);
    ivec2 tilesCount = ivec2(ceil(screenSize / TILE_SIZE));
    vec2 pixelCoord = vec2(gl_FragCoord.x, screenSize.y - gl_FragCoord.y);
    int tileX = int(floor(pixelCoord.x / TILE_SIZE));
    int tileY = int(floor(pixelCoord.y / TILE_SIZE));

    int tileIndex = tileX + tilesCount.x * tileY;
    LightGridItem gridItem = light_grid[tileIndex];

    uint lightOffset = gridItem.offset;
    uint pointLightCount = gridItem.lightsCount & 0x000fffu;
    uint spotLightCount = gridItem.lightsCount >> 16;

    uint projectorCount = gridItem.projectorsCount & 0x000fffu;
    uint decalCount = gridItem.projectorsCount >> 16;
    vec3 eyeDir_worldspace = normalize(camera.cameraPosition - position_worldspace.xyz); // vector to camera
    
    //out_color = vec4((normalize(normal_worldspace.xyz) + 1.0f) / 2.0f, 1); return;

    /*pointLightCount = 0;
    vec3 lightPosition = vec3(-10, 0, -10);
    vec3 lightDir = position_worldspace.xyz - lightPosition;
    float distanceToLight = length(lightDir);
    lightDir /= distanceToLight; // normalize
    float materialSpecular = 0;
    //vec3 lightValue = calculateFragmentDiffuse(normalizedDistanceToLight, lights[lightIndex].attenuation, normal_worldspace.xyz, lightDir, eyeDir_worldspace, lights[lightIndex].color, materialSpecular);
    vec3 lightValue = CalculateLighting(normalize(normal_worldspace.xyz), -lightDir, eyeDir_worldspace, object_params.roughness);
    light_color += vec4(lightValue, 0.0);*/

	uint i;
	//[[loop]]
    for (i = 0; i < pointLightCount; i++) {
        uint currentOffset = lightOffset + i;
        uint lightIndex = light_indices[currentOffset];

        //[[branch]]
        if (true || (lights[lightIndex].mask/* & layer*/) > 0u) {
            vec3 lightPosition = lights[lightIndex].position;
            vec3 lightDir = position_worldspace.xyz - lightPosition;
            float distanceToLight = length(lightDir);
            lightDir /= distanceToLight; // normalize
            float attenuation = GetAttenuation(distanceToLight, lights[lightIndex].radius);
            //vec3 lightValue = calculateFragmentDiffuse(normalizedDistanceToLight, lights[lightIndex].attenuation, normal_worldspace.xyz, lightDir, eyeDir_worldspace, lights[lightIndex].color, materialSpecular);
            vec3 radiance = lights[lightIndex].color.rgb * attenuation;
            vec3 lightValue = CalculateLighting(out_color.xyz, radiance, normalize(normal_worldspace.xyz), eyeDir_worldspace, -lightDir, object_params.roughness, 0.0f);
            
            vec3 coneDirection = lights[lightIndex].direction;
            float coneAngle = lights[lightIndex].coneAngle;
            float lightToSurfaceAngle = dot(lightDir, coneDirection);
            float innerLightToSurfaceAngle = lightToSurfaceAngle * 1.03;
            float epsilon = innerLightToSurfaceAngle - lightToSurfaceAngle;

            //[branch]
            if (lightToSurfaceAngle > coneAngle && lights[lightIndex].shadowmapScale.x > 0.0) {
                vec4 position_lightspace = lights[lightIndex].projectionMatrix * vec4(position_worldspace.xyz, 1.0);

                vec4 position_lightspace_normalized = position_lightspace / position_lightspace.w;
                position_lightspace_normalized.xy = (position_lightspace_normalized.xy + 1.0) / 2.0;
                position_lightspace_normalized.y = 1.0 - position_lightspace_normalized.y;
                vec2 shadowmapUV = position_lightspace_normalized.xy * lights[lightIndex].shadowmapScale + lights[lightIndex].shadowmapOffset;
                float shadow = calculateFragmentShadow(shadowmapUV, position_lightspace_normalized.z);
                lightValue *= 1.0 - shadow;
            }

            light_color += vec4(lightValue, 0.0);
        }
    }

#if 0
    vec3 I = -eyeDir_worldspace;
    vec3 R = reflect(I, normalize(normal_worldspace));
    light_color = mix(vec4(texture(environment_cubemap, R.xyz).rgb, 1.0), light_color, 0.8);
#endif

	//light_color = vec4((normal_worldspace + vec3(1,1,1)) / 2.0, 1);

	//out_color.r += (pointLightCount + spotLightCount) * 0.2;
#else
	vec4 light_color = vec4(1.0, 1.0, 1.0, 1.0);
#endif

	out_color *= light_color;
    //if (out_color.r > 1.000001) out_color.gb = vec2(0, 0);
}