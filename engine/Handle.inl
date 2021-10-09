template <typename T>
class Handle
{
public:
	void AddToReleaser(std::shared_ptr<T>& res)
	{
		if (res && res.use_count() == 1)
			GetReleaser().Add(std::static_pointer_cast<Resource>(res));
	}

	Handle() = default;

	Handle(std::nullptr_t) {}

	Handle(std::unique_ptr<T> resource_src)
	{
		resource = std::move(resource_src);
	}

	Handle(const Handle& other)
	{
		resource = other.resource;
	}

	Handle(Handle&& other)
	{
		resource = std::move(other.resource);
	}

	Handle& operator=(const Handle& other)
	{
		AddToReleaser(resource);
		resource = other.resource;
		return *this;
	}

	Handle& operator=(std::nullptr_t)
	{
		AddToReleaser(resource);
		resource = nullptr;
		return *this;
	}

	Handle& operator=(Handle&& other)
	{
		AddToReleaser(resource);
		resource = std::move(other.resource);
		return *this;
	}

	bool operator==(const Handle& other)
	{
		return resource == other.resource;
	}

	bool operator!=(const Handle& other)
	{
		return !(*this == other);
	}

	~Handle()
	{
		AddToReleaser(resource);
	}

	void Reset()
	{
		resource.reset();
	}

	operator bool() const { return (bool)resource; }

	T* get() const { return resource.get(); }
	T& operator*() { return *resource; }
	T* operator->() { return resource.get(); }
	const T& operator*() const { return *resource; }
	const T* operator->() const { return resource.get(); }


private:
	mutable std::shared_ptr<T> resource;
};