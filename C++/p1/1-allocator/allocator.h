#pragma once
#include <vector>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <cstring>
#include <map>


enum class AllocErrorType 
{
    InvalidFree,
    NoMemory,
};

class AllocError: std::runtime_error 
{
    AllocErrorType type;

public:
    AllocError(AllocErrorType _type, std::string message) : runtime_error(message), type(_type) {}

    AllocErrorType getType() const
    {
	    return type;
    }
};

class Allocator;

class Pointer 
{
	static size_t maxId;
	static std::map<size_t, std::pair<void*, size_t>> Alias;

	size_t id = 0;

public:
	Pointer()
	{
		id = ++maxId;
	}
	~Pointer() {}

	template<typename T>
	Pointer(T ptr, size_t size) : Pointer()
	{
		Alias[id] = std::make_pair(reinterpret_cast<void*>(ptr), size);
	}

    void *get() const
    {
	    return Alias[id].first;
    }

	void rebase(void *ptr)
	{
		Alias[id].first = ptr;
	}

	void resize(size_t newSize)
	{
		Alias[id].second = newSize;
	}

	unsigned int getId()
	{
		return id;
	}

	size_t size() const
	{
		return Alias[id].second;
	}
};

class Allocator 
{
	void *base = nullptr;
	size_t size = 0;
	std::vector<Pointer> Pointers;

public:
	Allocator() {}
    Allocator(void *base, size_t size) : base(base), size(size) {}
    
	Pointer alloc(size_t N);

	void realloc(Pointer &p, size_t N);

	void free(Pointer &p);

	std::string dump();

	void defrag();
};