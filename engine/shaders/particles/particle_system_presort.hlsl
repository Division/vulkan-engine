#include "shaders/includes/global.hlsl"
#include "shaders/bitonic_sort/BitonicSortCommon.hlsli"
#include "shaders/particles/particle_common.hlsl"

RWStructuredBuffer<int> counters : register(u2, space0);
RWStructuredBuffer<Particle> particles : register(u4, space0);
RWStructuredBuffer<uint> draw_indices : register(u7, space0);
RWStructuredBuffer<uint2> sort_indices : register(u8, space0);

[numthreads(1, 1, 1)]
void PreSortDispatchArgs(uint GI : SV_GroupIndex)
{
	uint InstanceCount = counters[COUNTER_INDEX_ALIVE_AFTER_SIMULATION];
	uint ThreadGroupCount = (InstanceCount + 2047) / 2048;

	counters[COUNTER_INDEX_PRESORT_DISPATCH_GROUPS] = ThreadGroupCount;
	counters[COUNTER_INDEX_UPDATE_DISPATCH_GROUPS] = (InstanceCount + 1023) / 1024;
}

groupshared uint gs_SortIndices[2048];
groupshared uint gs_SortKeys[2048];

void FillSortKey(uint Element, uint ListCount)
{
	// Unused elements must sort to the end
	if (Element < ListCount)
	{
		uint particle_index = draw_indices[Element];
		Particle particle = particles[particle_index];

		float camera_zmax = camera.zMax * camera.zMax;
		float3 delta = camera.cameraPosition - particle.position.xyz;
		uint camera_distance = dot(delta, delta) / camera_zmax * uint(-1);

		gs_SortKeys[Element & 2047] = camera_distance;
		gs_SortIndices[Element & 2047] = particle_index;
	}
	else
	{
		gs_SortKeys[Element & 2047] = NullItem;
		gs_SortIndices[Element & 2047] = 0;
	}
}

void StoreKeyIndexPair(uint Element, uint ListCount)
{
	if (Element < ListCount)
		sort_indices[Element] = uint2(gs_SortIndices[Element & 2047], gs_SortKeys[Element & 2047]);
}

[numthreads(1024, 1, 1)]
void PreSortParticles(uint3 Gid : SV_GroupID, uint GI : SV_GroupIndex)
{
	uint VisibleParticles = counters[COUNTER_INDEX_ALIVE_AFTER_SIMULATION];

	uint GroupStart = Gid.x * 2048;

	FillSortKey(GroupStart + GI, VisibleParticles);
	FillSortKey(GroupStart + GI + 1024, VisibleParticles);

	GroupMemoryBarrierWithGroupSync();

	uint k;

	[unroll]
	for (k = 2; k <= 2048; k <<= 1)
	{
		//[unroll]
		for (uint j = k / 2; j > 0; j /= 2)
		{
			uint Index1 = InsertZeroBit(GI, j);
			uint Index2 = Index1 ^ (k == j * 2 ? k - 1 : j);

			uint A = gs_SortKeys[Index1];
			uint B = gs_SortKeys[Index2];

			if (ShouldSwap(A, B))
			{
				// Swap the keys
				gs_SortKeys[Index1] = B;
				gs_SortKeys[Index2] = A;

				// Then swap the indices (for 64-bit sorts)
				A = gs_SortIndices[Index1];
				B = gs_SortIndices[Index2];
				gs_SortIndices[Index1] = B;
				gs_SortIndices[Index2] = A;
			}

			GroupMemoryBarrierWithGroupSync();
		}
	}

	StoreKeyIndexPair(GroupStart + GI, VisibleParticles);
	StoreKeyIndexPair(GroupStart + GI + 1024, VisibleParticles);
}

[numthreads(1024, 1, 1)]
void OutputSortedIndices(uint3 id: SV_DispatchThreadID)
{
	if (id.x < counters[COUNTER_INDEX_ALIVE_AFTER_SIMULATION])
	{
		int particle_index = sort_indices[id.x].x;
		draw_indices[id.x] = particle_index;
	}
}