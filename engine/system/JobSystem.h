#pragma once

#include <stdint.h>
#include <thread>
#include <atomic>
#include <condition_variable>
#include "memory/Containers.h"
#include "memory/PoolAllocator.h"

namespace core { namespace JobSystem {

	typedef Memory::PoolAllocator<Memory::Tag::JobSystem> JobSystemAllocator;

	class SubTask
	{
	public:

	private:
		SubTask* next = nullptr;
	};

	class Job
	{
	public:
		Job(JobSystemAllocator& allocator);

		void Execute();

	private:
		JobSystemAllocator& allocator;
		SubTask* first_subtask;
	};

	class Workload
	{
	public:
		enum class Priority : int
		{
			High = 0,
			Low
		};

		Workload(Priority priority);

		const std::atomic_uint32_t& GetNumberOfJobs() const { return number_of_jobs; }
		Priority GetPriority() const { return priority; }
		bool Steal(Workload* other);

	private:
		std::atomic_uint32_t number_of_jobs;
		//Memory::Deque<Job*, Memory::Tag::JobSystem> job_queue;
		std::deque<Job*> job_queue;
		Priority priority;
	};

	class WorkerThread
	{
	public:
		WorkerThread(JobSystemAllocator& allocator);

	private:
		Job* GetNextJob();

	private:
		std::atomic_bool enabled;
		JobSystemAllocator& allocator;
		std::thread thread;
		std::condition_variable condition;
		std::mutex mutex;
	};

	class Scheduler
	{
	public:
		Scheduler(uint32_t thread_number);
		~Scheduler();

	private:
		//Memory::Vector<Memory::Pointer<WorkerThread, Memory::Tag::JobSystem>, Memory::Tag::JobSystem> threads;
		//std::vector<Memory::Pointer<WorkerThread, Memory::Tag::JobSystem>> threads;
		std::vector<std::unique_ptr<WorkerThread>> threads;
		JobSystemAllocator pool_allocator;
	};

} }