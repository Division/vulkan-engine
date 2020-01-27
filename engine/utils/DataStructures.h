#pragma once

#include <set>
#include <vector>
#include <array>

namespace core { namespace utils {

	class Graph
	{
	public:
		typedef std::set<uint32_t> Bucket;

		Graph(uint32_t vertex_count = 0, bool is_directed = true) : buckets(vertex_count), directed(is_directed) {}

		uint32_t VertexCount() const { return buckets.size(); }
		bool IsDirected() const { return directed; }

		void AddEdge(uint32_t vertexA, uint32_t vertexB)
		{
			assert(vertexA < buckets.size());
			assert(vertexB < buckets.size());
			buckets[vertexA].insert(vertexB);
			if (!directed)
				buckets[vertexB].insert(vertexA);
		}

		const Bucket& GetVertexBucket(uint32_t vertex) const { assert(vertex < buckets.size()); return buckets[vertex]; }

	private:
		std::vector<Bucket> buckets;
		bool directed;
	};

	// TODO: move to cpp
	template <typename T>
	class SmallVectorBase
	{
	public:
		SmallVectorBase(T* begin, T* end, size_t capacity)
			: _begin(begin)
			, _end(end)
			, _capacity(capacity)
			, allocated_heap(false)
		{}

		SmallVectorBase(const SmallVectorBase& other)
			: _begin(0)
			, _end(0)
			, _capacity(0)
			, allocated_heap(false)
		{
			*this = other;
		}

		// TODO: move and copy construct

		~SmallVectorBase()
		{
			clear();
			if (allocated_heap && _begin)
			{
				free(_begin);
			}
		}

		size_t size() const { return end() - begin(); }
		size_t capacity() const { return _capacity; }

		T* begin() { return _begin; }
		T* end() { return _end; }
		const T* begin() const { return _begin; }
		const T* end() const { return _end; }
		const T* data() const { return _begin; }
		T* data() { return _begin; }

		SmallVectorBase& operator=(const SmallVectorBase& other)
		{
			if (this != &other)
			{
				clear();
				reserve(other.size());
				_end = _begin + other.size();
				for (int i = 0; i < other.size(); i++)
				{
					new (_begin + i) T();
					*(_begin + i) = other[i];
				}
			}

			return *this;
		}


		T& operator[](size_t index) noexcept {
			assert(index < size());
			return *(begin() + index);
		}

		const T& operator[](size_t index) const noexcept {
			assert(index < size());
			return *(begin() + index);
		}

		void push_back(const T& value)
		{
			if (begin() + capacity() == end())
			{
				auto new_capacity = pow(2, (size_t)ceil(log2((double)(capacity() + 1))));
				reserve(new_capacity);
			}

			assert(end() < begin() + capacity());
			new (end()) T(value);
			_end += 1;
		}

		void pop_back()
		{
			assert(end() > begin());
			if (_end > _begin)
			{
				(_end - 1)->~T();
				_end -= 1;
			}
		}

		void reserve(size_t new_size)
		{
			if (new_size > capacity())
			{
				auto new_capacity = new_size;
				auto new_begin = (T*)malloc(new_capacity * sizeof(T));
				const auto count = size();
				for (size_t i = 0; i < count; i++)
				{
					auto& item = *(begin() + i);
					new (new_begin + i) T(std::move(item)); // move 
					item.~T();
				}

				if (allocated_heap)
					free(_begin);
				else
					allocated_heap = true;

				_begin = new_begin;
				_end = new_begin + count;
				_capacity = new_capacity;
			}
		}

		void resize(size_t new_size)
		{
			if (new_size > size())
			{
				if (new_size > capacity())
					reserve(new_size);

				for (size_t i = 0; i < new_size - size(); i++)
					new (_end + i) T();

				_end = begin() + new_size;
			}
			else if (new_size < size())
			{
				for (auto value = begin() + new_size; value < end(); value++)
				{
					(*value).~T();
				}

				_end = begin() + new_size;
			}
		}

		void clear()
		{
			for (auto& value : *this)
				value.~T();
			
			_end = _begin;
		}

	private:
		T* _begin;
		T* _end;
		size_t _capacity;
		bool allocated_heap = false;
	};

	template <typename T, size_t N>
	class SmallVector : public SmallVectorBase<T>
	{
	public:
		SmallVector() : SmallVectorBase((T*)buffer, (T*)buffer, N) {};
		
		SmallVector(const SmallVector<T, N>& other)
			: SmallVectorBase((T*)buffer, (T*)buffer, N)
		{
			*this = other;
		}

	private:
		char buffer[sizeof(T) * N];
	};

} }