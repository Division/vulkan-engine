#pragma pack_matrix(column_major)

struct VIn {
    float4 position : POSITION;
    float4 normal : NORMAL;
    
#if defined(ATTRIB_TANGENT)
    float4 tangent : TANGENT;
#endif
#if defined(ATTRIB_BITANGENT)
    float4 binormal : BINORMAL;
#endif
#if defined(ATTRIB_TEXCOORD0)
    float2 texCoord0 : TEXCOORD;
#endif
#if defined (ATTRIB_JOINT_INDEX)
    float3 jointIndex : BLENDINDICES;
#endif
#if defined (ATTRIB_JOINT_WEIGHT)
    float3 jointWeight : BLENDWEIGHT;
#endif
};

struct VOut
{
    float4 position : SV_POSITION;
    float4 position_worldspace : POSITIONT;
    float4 normal_worldspace : NORMAL0;
#if defined (ATTRIB_TEXCOORD0)
    float2 texCoord0 : TEXCOORD;
#endif
};

cbuffer VS_CONSTANT_BUFFER : register(b0) {
    float4x4 objectModelMatrix;
    float4x4 objectNormalMatrix;
    float2 uvScale;
    float2 uvOffset;
    uint layer;
};

cbuffer VS_CONSTANT_BUFFER : register(b1) {
    float3 cameraPosition;
    uint2 cameraScreenSize;
    float4x4 cameraViewMatrix;
    float4x4 cameraProjectionMatrix;
};

#if defined(CONSTANT_BUFFER_SKINNING_MATRICES)
cbuffer VS_CONSTANT_BUFFER : register(b2) {
    float4x4 skinningMatrices[70];
};
#endif

VOut VShader(VIn input) {
    VOut output;

    float4x4 modelMatrix;
#if defined(CAP_SKINNING)
    modelMatrix = float4x4(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    modelMatrix += skinningMatrices[int(input.jointIndex.x)] * input.jointWeight.x;
    modelMatrix += skinningMatrices[int(input.jointIndex.y)] * input.jointWeight.y;
    modelMatrix += skinningMatrices[int(input.jointIndex.z)] * input.jointWeight.z;
#else
    modelMatrix = objectModelMatrix;
#endif

    float4 position_worldspace = mul(modelMatrix, input.position);
    float4 position_cameraspace = mul(cameraViewMatrix, position_worldspace);
    output.position = mul(cameraProjectionMatrix, position_cameraspace);
    output.position_worldspace = position_worldspace;
    output.normal_worldspace = normalize(mul(modelMatrix, float4(input.normal.xyz, 0)));
#if defined(ATTRIB_TEXCOORD0)
    output.texCoord0 = input.texCoord0 * uvScale + uvOffset;
#endif
    
    return output;
}

#if defined (CAP_LIGHTING)

Texture2D shadowMap : register(t7);
SamplerState shadowMapSampler : register(s7);
Texture2D projectorTexture : register(t8);
SamplerState projectorSampler : register(s8);

struct LightGrid {
    uint offset;
    uint lightsCount;
    uint projectorsCount;
};

struct Light {
  float3 position;
  float attenuation;
  float3 color;
  float radius;
  float3 direction;
  float coneAngle;
  float4x4 projectionMatrix;
  float2 shadowmapScale;
  float2 shadowmapOffset;
  uint mask;
};

struct Projector {
  float3 position;
  float attenuation;
  float4 color;
  float2 scale;
  float2 offset;
  float2 shadowmapScale;
  float2 shadowmapOffset;
  float4x4 projectionMatrix;
  float radius;
  uint mask;
};

cbuffer VS_CONSTANT_BUFFER : register(b3) {
    Light lights[100];
};

cbuffer VS_CONSTANT_BUFFER : register(b4) {
    Projector projectors[100];
};

StructuredBuffer<LightGrid> lightGridBuffer : register(t3);
StructuredBuffer<uint> lightIndicesBuffer : register(t4);

float3 calculateFragmentDiffuse(float normalizedDistanceToLight, float attenuation, float3 normal, float3 lightDir, float3 eyeDir, float3 lightColor, float materialSpecular) {
  float lightValue = clamp(dot(-lightDir, normal), 0.0, 1.0);
  float attenuationValue = pow(max(1.0 - normalizedDistanceToLight, 0.0), attenuation);
  float3 diffuse = lightColor * lightValue;

  // TODO: conditionnaly skip specular
  float3 reflected = reflect(lightDir, normal);
  float cosAlpha = clamp(dot(eyeDir, reflected), 0.0, 1.0);
  //float3 specular = pow(cosAlpha, 32.0) * lightColor * materialSpecular;
  float3 specular = float3(0,0,0);

  return attenuationValue * (diffuse + specular);
}

float calculateFragmentShadow(float2 uv, float fragmentDepth) {
  float shadow = 0.0;
  float bias = 0.001;
  uint w;
  uint h;
  shadowMap.GetDimensions(w, h);
  float2 texelSize = 1.0 / float2(w, h);

  [unroll]
  for(int x = -1; x <= 1; x++) {
    for(int y = -1; y <= 1; y++) {
      float closestDepth = shadowMap.SampleLevel(shadowMapSampler, uv + float2(x, y) * texelSize, 0.0).r;
      shadow += fragmentDepth - bias > closestDepth ? 1.0 : 0.0;
    }
  }

  shadow /= 9.0;
  return shadow;
}

#endif

//Texture2D texture0 : register(t0);
Texture2D shaderTexture : register(t0);
SamplerState SampleType : register(s0);

float4 PShader(VOut input) : SV_TARGET
{
    float4 result = float4(1, 1, 1, 1);

#if defined(RESOURCE_TEXTURE0)
    float4 textureColor = shaderTexture.Sample(SampleType, input.texCoord0);
    result = textureColor;
#endif

#if defined(RESOURCE_TEXTURE0)
    float2 uvDDX = ddx(input.texCoord0.xy);
    float2 uvDDY = ddy(input.texCoord0.xy);
#else
    float2 uvDDX = 0;
    float2 uvDDY = 0;
#endif

#if defined (CAP_LIGHTING)
    float TILE_SIZE = 32.0;
    float2 screenSize = float2(cameraScreenSize);
    int2 tilesCount = int2(ceil(screenSize / TILE_SIZE));
    float2 pixelCoord = float2(input.position.x, screenSize.y - input.position.y);
    int tileX = int(floor(pixelCoord.x / TILE_SIZE));
    int tileY = int(floor(pixelCoord.y / TILE_SIZE));

    int tileIndex = tileX + tilesCount.x * tileY;
    LightGrid gridItem = lightGridBuffer[tileIndex];

    uint lightOffset = gridItem.offset;
    uint pointLightCount = gridItem.lightsCount & 0x000fffu;
    uint spotLightCount = gridItem.lightsCount >> 16;

    uint projectorCount = gridItem.projectorsCount & 0x000fffu;
    uint decalCount = gridItem.projectorsCount >> 16;
    float3 eyeDir_worldspace = normalize(cameraPosition - input.position_worldspace.xyz); // vector to camera
    float4 lightsColor = float4(0,0,0,0);
    uint i;

    [loop]
    for (i = 0; i < pointLightCount; i++) {
        uint currentOffset = lightOffset + i;
        uint lightIndex = lightIndicesBuffer[currentOffset];

        [branch]
        if ((lights[lightIndex].mask & layer) > 0u) {
            float3 lightPosition = lights[lightIndex].position;
            float3 lightDir = input.position_worldspace.xyz - lightPosition;
            float distanceToLight = length(lightDir);
            lightDir /= distanceToLight; // normalize
            float normalizedDistanceToLight = distanceToLight / lights[lightIndex].radius;
            float materialSpecular = 0;
            float3 lightValue = calculateFragmentDiffuse(normalizedDistanceToLight, lights[lightIndex].attenuation, input.normal_worldspace.xyz, lightDir, eyeDir_worldspace, lights[lightIndex].color, materialSpecular);
            
            float3 coneDirection = lights[lightIndex].direction;
            float coneAngle = lights[lightIndex].coneAngle;
            float lightToSurfaceAngle = dot(lightDir, coneDirection);
            float innerLightToSurfaceAngle = lightToSurfaceAngle * 1.03;
            float epsilon = innerLightToSurfaceAngle - lightToSurfaceAngle;

            [branch]
            if (lightToSurfaceAngle > coneAngle && lights[lightIndex].shadowmapScale.x > 0) {
                float4 position_lightspace = mul(lights[lightIndex].projectionMatrix, float4(input.position_worldspace.xyz, 1));
                float4 position_lightspace_normalized = position_lightspace / position_lightspace.w;
                position_lightspace_normalized.xy = (position_lightspace_normalized.xy + 1.0) / 2.0;
                position_lightspace_normalized.y = 1 - position_lightspace_normalized.y;
                float2 shadowmapUV = position_lightspace_normalized.xy * lights[lightIndex].shadowmapScale + lights[lightIndex].shadowmapOffset;
                float shadow = calculateFragmentShadow(shadowmapUV, position_lightspace_normalized.z);
                lightValue *= 1.0 - shadow;
            }

            lightsColor += float4(lightValue, 0.0);
        }
    }

    [loop] 
    for (i = 0; i < int(projectorCount); i++) {
        //float4 projectedTexture = shaderTexture.Sample(SampleType, float2(0,0));
        uint currentOffset = lightOffset + i + pointLightCount + spotLightCount;
        uint projectorIndex = lightIndicesBuffer[currentOffset];

        float4 projectedTextureUV = mul(projectors[projectorIndex].projectionMatrix, float4(input.position_worldspace.xyz, 1));
        projectedTextureUV /= projectedTextureUV.w;
        projectedTextureUV.xy = (projectedTextureUV.xy + 1.0) / 2.0;
        projectedTextureUV.y = 1 - projectedTextureUV.y;
        //projectedTextureUV.z = projectedTextureUV.z / 2.0 + 0.5;
        float projectorBias = 0.001;

        [branch]
        if ((projectors[projectorIndex].mask & layer) > 0u
            && projectedTextureUV.x > projectorBias && projectedTextureUV.x < 1.0 - projectorBias
            && projectedTextureUV.y > projectorBias && projectedTextureUV.y < 1.0 - projectorBias
            && projectedTextureUV.z > projectorBias && projectedTextureUV.z < 1.0 - projectorBias) 
        {
            
            float2 spritesheetUV = projectedTextureUV.xy * projectors[projectorIndex].scale + projectors[projectorIndex].offset;
            float4 projectedTexture = projectorTexture.SampleGrad(projectorSampler, spritesheetUV, uvDDX, uvDDY);
            //lightsColor += projectedTexture;
            //float4 projectedTexture = shaderTexture.Sample(SampleType, float2(0,0));
            float3 lightPosition = projectors[projectorIndex].position;

            float3 lightDir = input.position_worldspace.xyz - lightPosition;
            float distanceToLight = length(lightDir);
            lightDir /= distanceToLight; // normalize
            float3 lightColor = projectedTexture.rgb * projectedTexture.a;
            float normalizedDistanceToLight = distanceToLight / projectors[projectorIndex].radius;
            float materialSpecular = 0;
            float3 lightValue = calculateFragmentDiffuse(normalizedDistanceToLight, projectors[projectorIndex].attenuation, input.normal_worldspace.xyz, lightDir, eyeDir_worldspace, lightColor, materialSpecular);
            lightValue *= 18.0;
            [branch]
            if (projectors[projectorIndex].shadowmapScale.x > 0) {
                float2 shadowmapUV = projectedTextureUV.xy * projectors[projectorIndex].shadowmapScale + projectors[projectorIndex].shadowmapOffset;
                float shadow = calculateFragmentShadow(shadowmapUV, projectedTextureUV.z);
                lightValue *= 1.0 - shadow;
            }

            lightsColor += float4(lightValue, 0);
        }
    }

    result *= lightsColor;
#endif

    // conversion to sRGB is done on d3d side
    //float gamma = 2.2;
    //result.rgb = pow(result.rgb, 1.0/gamma);
    return result;
}
