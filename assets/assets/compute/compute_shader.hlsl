RWStructuredBuffer<float4> positions : register(u0, space0);

#define PI 3.14159265358979323846

float3 Wave(float u, float v, float t) 
{
	float3 p;
	p.x = u;
	p.y = sin(PI * (u + v + t));
	p.z = v;
	return p;
}

cbuffer GlobalConstants : register(b1, space0)
{
	float Step;
	float Time;
	uint Resolution;
};

float2 GetUV(uint3 id) 
{
	return (id.xy + 0.5) * Step - 1.0;
}

void SetPosition(uint3 id, float3 position) 
{
	if (id.x < Resolution && id.y < Resolution) 
	{
		positions[id.x + id.y * Resolution] = float4(position, 1);
	}
}

[numthreads(8, 8, 1)]
void FunctionKernel(uint3 id: SV_DispatchThreadID)
{
	float2 uv = GetUV(id);
	SetPosition(id, Wave(uv.x, uv.y, Time) * 15.0f);
}

