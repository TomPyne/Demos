#pragma once

#include <vector>

template<typename ID, typename DataType>
struct IDArray
{
	explicit IDArray()
	{
		Data.push_back({});
		RefCounts.push_back(0);
	}

	ID Create()
	{
		ID id = MakeID();
		Data[(uint32_t)id] = {};
		return id;
	}

	ID Create(DataType&& data)
	{
		ID id = MakeID();
		Data[(uint32_t)id] = std::move(data);
		return id;
	}

	ID Create(DataType** outData)
	{
		ID id = MakeID();
		*outData = &Data[(uint32_t)id];
		return id;
	}

	void Update(ID id, DataType& data)
	{
		Data[(uint32_t)id] = data;
	}

	void AddRef(ID id)
	{
		RefCounts[(uint32_t)id]++;
	}

	DataType* Release(ID id)
	{
		if (id == ID::INVALID || RefCounts[(uint32_t)id] == 0)
			return nullptr;

		if (--RefCounts[(uint32_t)id] == 0)
		{
			FreeIDs.push_back(id);
			return &Data[(uint32_t)id];
		}

		return nullptr;
	}

	DataType* GetUnchecked(ID id) noexcept
	{
		return (uint32_t)id <= Data.size() ? &Data[(uint32_t)id] : nullptr;
	}

	DataType* Get(ID id)
	{
		return RefCounts[(uint32_t)id] ? &Data[(uint32_t)id] : nullptr;
	}

	inline std::vector<DataType>& GetArray() noexcept { return Data; }
	inline uint32_t RefCount(ID id) const noexcept { return RefCounts[(uint32_t)id] > 0; }
	inline size_t Size() const noexcept { return Data.size(); }

private:
	std::vector<ID>		FreeIDs;
	std::vector<DataType> Data;
	std::vector<uint32_t> RefCounts;

	ID MakeID()
	{
		if (!FreeIDs.empty())
		{
			ID id = FreeIDs.back();
			FreeIDs.pop_back();
			RefCounts[(uint32_t)id]++;
			return id;
		}
		else
		{
			ID id = (ID)Data.size();
			Data.push_back({});
			RefCounts.push_back(1);

			return id;
		}
	}
};
