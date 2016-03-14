#include "allocator.h"

Pointer Allocator::alloc(size_t N)
{
	Pointer ans;
	for (size_t i = 0; i + 1 < Pointers.size(); ++i)
		if (reinterpret_cast<uintptr_t>(Pointers[i + 1].get()) - reinterpret_cast<uintptr_t>(Pointers[i].get()) - Pointers[i].size() >= N)
		{
			ans = Pointer(Pointers[i].get() + Pointers[i].size(), N);
			Pointers.insert(Pointers.begin() + i, ans);
			return ans;
		}

	if (Pointers.empty())
	{
		ans = Pointer(base, N);
		Pointers.push_back(ans);
		return ans;
	}
	if (reinterpret_cast<uintptr_t>(base) + size - reinterpret_cast<uintptr_t>(Pointers.back().get()) - Pointers.back().size() < N)
	{
		throw AllocError(AllocErrorType::NoMemory, "ENOMEM");
	}
	ans = Pointer(Pointers.back().get() + Pointers.back().size(), N);
	Pointers.push_back(ans);
	return ans;
}

void Allocator::realloc(Pointer &p, size_t N)
{
	if (p.get() == nullptr)
	{
		p = alloc(N);
		return;
	}
	if (p.size() >= N)
	{
		p.resize(N);
		return;
	}
	size_t index = 0;
	while (Pointers[index].get() != p.get())
		++index;
	if (index + 1 == Pointers.size())
		if (reinterpret_cast<uintptr_t>(base) + size - reinterpret_cast<uintptr_t>(p.get()) >= N)
		{
			p.resize(N);
			return;
		}
	auto newPtr = alloc(N);
	index = 0;
	while (Pointers[index].get() != p.get())
		++index;
	Pointers.erase(Pointers.begin() + index);
	p = newPtr;
}

void Allocator::free(Pointer &p)
{
	auto was = false;
	auto st = Pointers.begin();
	auto end = Pointers.end();
	while (st != end)
	{
		if (p.get() == st->get())
		{
			was = true;
			Pointers.erase(st);
			break;
		}
		st++;
	}
	p = Pointer();
	if (!was)
		throw AllocError(AllocErrorType::InvalidFree, "EIFREE");
}

std::string Allocator::dump()
{
	std::stringstream s;
	for (auto x : Pointers)
		s << x.get() << ' ' << x.size() << '\n';
	return s.str();
}

void Allocator::defrag()
{
	if (Pointers.empty())
		return;
	if (Pointers[0].get() != base)
	{
		std::memmove(base, Pointers[0].get(), Pointers[0].size());
		Pointers[0].rebase(base);
	}
	for (size_t i = 1; i < Pointers.size(); ++i)
		if (reinterpret_cast<uintptr_t>(Pointers[i - 1].get()) + Pointers[i - 1].size() != reinterpret_cast<uintptr_t>(Pointers[i].get()))
		{
			auto to = Pointers[i - 1].get() + Pointers[i - 1].size();
			std::memmove(to, Pointers[i].get(), Pointers[i].size());
			Pointers[i].rebase(to);
		}
}

size_t Pointer::maxId = 0;
std::map<size_t, std::pair<void*, size_t>> Pointer::Alias = std::map<size_t, std::pair<void*, size_t>>();