#pragma once

#include <stdint.h>
#include <thread>
#include <atomic>
#include <functional>
#include <condition_variable>
#include <LockFreeQueue/concurrentqueue.h>
#include "memory/Containers.h"
#include "memory/PoolAllocator.h"

namespace Thread {

	constexpr size_t JOB_POOL_ITEM_SIZE = 256;
	constexpr size_t JOB_POOL_ITEM_COUNT = 1000;


	typedef Memory::PoolAllocator<Memory::Tag::JobSystem> JobSystemAllocator;

	/*
	class SpinlockMutex
	{
	private:
		std::atomic_flag flag;
	public:
		SpinlockMutex(): flag(ATOMIC_FLAG_INIT) {}
		
		void lock()
		{
			while(flag.test_and_set(std::memory_order_acquire));
		}

		void unlock()
		{
			flag.clear(std::memory_order_release);
		}
	};

	class ScopedSpinLock
	{
	private:
		SpinlockMutex& mutex;
	public:
		ScopedSpinLock(SpinlockMutex& mutex) : mutex(mutex)
		{
			mutex.lock();
		}

		~ScopedSpinLock()
		{
			mutex.unlock();
		}
	};*/

	class Job
	{
	public:
		enum class Priority : int
		{
			High = 0,
			Low,
			Count
		};

		Job() = default;
		virtual ~Job() = default;
		virtual void Execute() {}
	private:
		friend class WorkloadSet;
		Priority priority = Priority::High;
	};

	class FunctionJob : public Job
	{
	public:
		FunctionJob(std::function<void()> function) 
			: Job()
			, function(std::move(function))
		{}

		virtual void Execute() override;
	
	private:
		std::function<void()> function;
	};

	class Workload
	{
	public:

		Workload(Job::Priority priority);
		void Add(Job* job);
		bool HasJobs() const { return job_counter.load() > 0; };
		void JobCompleted(Job* job);

		Job::Priority GetPriority() const { return priority; }
		Job* GetNextJob();

	private:
		std::atomic_uint32_t job_counter = 0;
		moodycamel::ConcurrentQueue<Job*> queue;
		Job::Priority priority;
	};

	class WorkloadSet
	{
	public:
		WorkloadSet(JobSystemAllocator& allocator);
		void Wait();
		void Add(Job* job, Job::Priority priority);
		Job* GetNextJob();
		void JobCompleted(Job* job);
		void NotifyAll();
		bool HasJobs(Job::Priority priority);

	private:
		JobSystemAllocator& allocator;
		std::array<std::unique_ptr<Workload>, (size_t)Job::Priority::Count> workloads;
		std::condition_variable condition;
		std::mutex wait_mutex;
	};

	class WorkerThread
	{
	public:
		WorkerThread(int index, WorkloadSet& workload_set);
		void Join();
		void Stop();

	private:
		Job* GetNextJob();

	private:
		WorkloadSet& workload_set;
		std::atomic_bool enabled;
		std::thread thread;
		int index;
	};

	class Scheduler
	{
	public:
		static void Initialize();
		static void Shutdown();
		static Scheduler& Get();
		
		template <typename T, typename ...Args>
		T* SpawnJob(Job::Priority priority, Args&& ...args)
		{
			static_assert(sizeof(T) <= JOB_POOL_ITEM_SIZE);
			void* memory = reinterpret_cast<T*>(pool_allocator.Allocate());
			T* job = new (memory) T(std::forward<Args>(args)...);
			workload_set.Add(job, priority);
			return job;
		}

		void Wait(Job::Priority priority);

		Scheduler(uint32_t thread_number);
		~Scheduler();

	private:
		JobSystemAllocator pool_allocator; // must be first to be constructed before workload_set
		WorkloadSet workload_set;
		std::vector<std::unique_ptr<WorkerThread>> threads;
	};

}