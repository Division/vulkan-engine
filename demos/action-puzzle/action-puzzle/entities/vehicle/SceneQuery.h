#pragma once

//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of NVIDIA CORPORATION nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Copyright (c) 2008-2019 NVIDIA Corporation. All rights reserved.
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  


#include <physx/PxPhysicsAPI.h>

namespace Vehicle::Utils
{

	enum
	{
		DRIVABLE_SURFACE = 0xffff0000,
		UNDRIVABLE_SURFACE = 0x0000ffff
	};

	void setupDrivableSurface(physx::PxFilterData& filterData);

	void setupNonDrivableSurface(physx::PxFilterData& filterData);


	physx::PxQueryHitType::Enum WheelSceneQueryPreFilterBlocking
	(physx::PxFilterData filterData0, physx::PxFilterData filterData1,
		const void* constantBlock, physx::PxU32 constantBlockSize,
		physx::PxHitFlags& queryFlags);

	physx::PxQueryHitType::Enum WheelSceneQueryPostFilterBlocking
	(physx::PxFilterData queryFilterData, physx::PxFilterData objectFilterData,
		const void* constantBlock, physx::PxU32 constantBlockSize,
		const physx::PxQueryHit& hit);

	physx::PxQueryHitType::Enum WheelSceneQueryPreFilterNonBlocking
	(physx::PxFilterData filterData0, physx::PxFilterData filterData1,
		const void* constantBlock, physx::PxU32 constantBlockSize,
		physx::PxHitFlags& queryFlags);

	physx::PxQueryHitType::Enum WheelSceneQueryPostFilterNonBlocking
	(physx::PxFilterData queryFilterData, physx::PxFilterData objectFilterData,
		const void* constantBlock, physx::PxU32 constantBlockSize,
		const physx::PxQueryHit& hit);


	//Data structure for quick setup of scene queries for suspension queries.
	class VehicleSceneQueryData
	{
	public:
		VehicleSceneQueryData();
		~VehicleSceneQueryData();

		//Allocate scene query data for up to maxNumVehicles and up to maxNumWheelsPerVehicle with numVehiclesInBatch per batch query.
		static VehicleSceneQueryData* allocate
		(const physx::PxU32 maxNumVehicles, const physx::PxU32 maxNumWheelsPerVehicle, const physx::PxU32 maxNumHitPointsPerWheel, const physx::PxU32 numVehiclesInBatch,
			physx::PxBatchQueryPreFilterShader preFilterShader, physx::PxBatchQueryPostFilterShader postFilterShader, 
			physx::PxAllocatorCallback& allocator);

		//Free allocated buffers.
		void free(physx::PxAllocatorCallback& allocator);

		//Create a PxBatchQuery instance that will be used for a single specified batch.
		static physx::PxBatchQuery* setUpBatchedSceneQuery(const physx::PxU32 batchId, const VehicleSceneQueryData& vehicleSceneQueryData, physx::PxScene* scene);

		//Return an array of scene query results for a single specified batch.
		physx::PxRaycastQueryResult* getRaycastQueryResultBuffer(const physx::PxU32 batchId); 

		//Return an array of scene query results for a single specified batch.
		physx::PxSweepQueryResult* getSweepQueryResultBuffer(const physx::PxU32 batchId); 

		//Get the number of scene query results that have been allocated for a single batch.
		physx::PxU32 getQueryResultBufferSize() const; 

	private:

		//Number of queries per batch
		physx::PxU32 mNumQueriesPerBatch;

		//Number of hit results per query
		physx::PxU32 mNumHitResultsPerQuery;

		//One result for each wheel.
		physx::PxRaycastQueryResult* mRaycastResults;
		physx::PxSweepQueryResult* mSweepResults;

		//One hit for each wheel.
		physx::PxRaycastHit* mRaycastHitBuffer;
		physx::PxSweepHit* mSweepHitBuffer;

		//Filter shader used to filter drivable and non-drivable surfaces
		physx::PxBatchQueryPreFilterShader mPreFilterShader;

		//Filter shader used to reject hit shapes that initially overlap sweeps.
		physx::PxBatchQueryPostFilterShader mPostFilterShader;

	};

}
