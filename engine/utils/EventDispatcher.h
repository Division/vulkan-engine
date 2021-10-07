#pragma once

#include <functional>
#include <vector>
#include <memory>

namespace utils
{

	template<class ...Args>
	class EventDispatcher : std::enable_shared_from_this<EventDispatcher<Args...>>
	{
	public:
		typedef std::function<void(Args...)> Callback;
	
	private:
		class EventDispatcherImpl
		{
		public:

			uint64_t AddCallback(Callback callback)
			{
				callbacks.push_back({ ++id_counter, callback });
				return id_counter;
			}

			void Dispatch(Args... args)
			{
				for (auto& callback : callbacks)
					callback.second(args...);
			}

			void RemoveCallback(uint64_t id)
			{
				callbacks.erase(std::remove_if(callbacks.begin(), callbacks.end(), [id](const auto& cb) { return id == cb.first; }));
			}

		private:
			uint64_t id_counter = 0;
			std::vector<std::pair<uint64_t, Callback>> callbacks;
		};

	public:

		class Handle
		{
			friend EventDispatcher;
			friend EventDispatcherImpl;

			uint64_t id = 0;
			std::weak_ptr<EventDispatcherImpl> dispatcher;
			Handle(uint64_t id, std::weak_ptr<EventDispatcherImpl> dispatcher) : id(id), dispatcher(dispatcher) {}

		public:
			Handle() = default;
			Handle(const Handle&) = delete;
			Handle& operator=(const Handle&) = delete;
			Handle(Handle&&) = default;
			Handle& operator=(Handle&&) = default;

			~Handle()
			{
				if (id > 0; auto d = dispatcher.lock())
				{
					d->RemoveCallback(id);
				}
			}
		};

	public:
		EventDispatcher()
		{
			impl = std::make_shared<EventDispatcherImpl>();
		}

		EventDispatcher(const EventDispatcher&) = delete;
		EventDispatcher(EventDispatcher&&) = default;
		EventDispatcher& operator=(const EventDispatcher&) = delete;
		EventDispatcher& operator=(EventDispatcher&&) = default;

		Handle AddCallback(Callback callback)
		{
			const auto id = impl->AddCallback(callback);
			return Handle(id, impl);
		}

		void Dispatch(Args... args)
		{
			impl->Dispatch(args...);
		}

	private:
		std::shared_ptr<EventDispatcherImpl> impl;
	};

}