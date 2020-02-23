#include "JobSystem.h"

namespace core { namespace JobSystem {

	const size_t JOB_POOL_ITEM_SIZE = 256;
	const size_t JOB_POOL_ITEM_COUNT = 1000;

	Job::Job(JobSystemAllocator& allocator)
		: allocator(allocator)
	{}

	void Job::Execute()
	{

	}

	Workload::Workload(Priority priority)
		: priority(priority)
	{

	}

	bool Workload::Steal(Workload* other)
	{
		return false;
	}

	Job* WorkerThread::GetNextJob()
	{
		return nullptr;
	}

	WorkerThread::WorkerThread(JobSystemAllocator& allocator)
		: allocator(allocator)
		, enabled(true)
	{
		std::unique_lock<std::mutex> lock(mutex);
		thread = std::thread([&]() 
		{
			while (enabled)
			{
				auto* job = GetNextJob();
				if (job)
					job->Execute();
				else
					condition.wait(lock);
			}
		});
	}

	Scheduler::Scheduler(uint32_t thread_number)
		: pool_allocator(JOB_POOL_ITEM_SIZE, JOB_POOL_ITEM_COUNT)
	{
		//for (size_t i = 0; i < thread_number; i++)
			//threads.push_back(Memory::Pointer<WorkerThread, Memory::Tag::JobSystem>::Create(pool_allocator));
	}

	Scheduler::~Scheduler()
	{
		// TODO: Wait for jobs to complete
	}

} }