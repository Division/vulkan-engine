#include <set>
#include <vector>

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

} }