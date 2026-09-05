#include "UnrealVoxelSim/Forest/Generation/Generator.h"

#include "UnrealVoxelSim/Trees/Api/CreateError.h"

#include <cassert>
#include <algorithm>
#include <cstdint>
#include <limits>
#include <stdexcept>

namespace UnrealVoxelSim::Forest::Generation
{
	namespace
	{
		[[nodiscard]] std::uint64_t Next(std::uint64_t& state) noexcept
		{
			state += 0x9E3779B97F4A7C15ULL;
			auto value = state;
			value = (value ^ (value >> 30U)) * 0xBF58476D1CE4E5B9ULL;
			value = (value ^ (value >> 27U)) * 0x94D049BB133111EBULL;
			return value ^ (value >> 31U);
		}
	}

	Generator::Generator(const Voxel::Solid::Api::IReader& solids,
	                     Trees::Api::IPlanter& trees,
	                     const std::span<const Voxel::Solid::Api::MaterialId> groundMaterials) :
		m_Solids(solids), m_Trees(trees), m_GroundMaterials(groundMaterials.begin(), groundMaterials.end())
	{
		if (m_GroundMaterials.empty() ||
			std::ranges::any_of(m_GroundMaterials, [](const auto material) { return !material.IsValid(); }))
			throw std::invalid_argument{"Forest generation requires at least one valid ground material."};
		std::ranges::sort(m_GroundMaterials);
		if (std::ranges::adjacent_find(m_GroundMaterials) != m_GroundMaterials.end())
			throw std::invalid_argument{"Forest ground materials must be unique."};
	}

	std::expected<Api::Result, Api::Error> Generator::Generate(const Api::Request request)
	{
		assert(std::this_thread::get_id() == m_OwnerThread);
		if (!request.SearchArea.IsValid() || request.SearchArea.IsEmpty())
			return std::unexpected{Api::Error::InvalidSearchArea};
		const auto width = static_cast<std::uint64_t>(static_cast<std::int64_t>(request.SearchArea.Max.X) - request.SearchArea.Min.X);
		const auto length = static_cast<std::uint64_t>(static_cast<std::int64_t>(request.SearchArea.Max.Y) - request.SearchArea.Min.Y);
		if (width == 0 || length == 0)
			return std::unexpected{Api::Error::InvalidSearchArea};

		std::uint64_t random = request.RandomSeed.Value();
		Api::Result result{request.CandidateCount, 0};
		for (std::size_t candidate = 0; candidate < request.CandidateCount; ++candidate)
		{
			const auto x = static_cast<std::int32_t>(static_cast<std::int64_t>(request.SearchArea.Min.X) +
				static_cast<std::int64_t>(Next(random) % width));
			const auto y = static_cast<std::int32_t>(static_cast<std::int64_t>(request.SearchArea.Min.Y) +
				static_cast<std::int64_t>(Next(random) % length));
			bool foundGround = false;
			Voxel::Api::Position root{};
			for (auto z = static_cast<std::int64_t>(request.SearchArea.Max.Z) - 1;
			     z >= request.SearchArea.Min.Z; --z)
			{
				const auto cell = m_Solids.Read({x, y, static_cast<std::int32_t>(z)});
				if (!cell)
					return std::unexpected{Api::Error::TerrainReadFailure};
				if (cell->IsEmpty())
					continue;
				if (!std::ranges::binary_search(m_GroundMaterials, cell->Material()))
					continue;
				if (z == std::numeric_limits<std::int32_t>::max())
					break;
				root = {x, y, static_cast<std::int32_t>(z + 1)};
				foundGround = true;
				break;
			}
			if (!foundGround)
				continue;
			const auto planted = m_Trees.Plant({root, request.Species, Trees::Api::ShapeSeed{Next(random)}});
			if (planted)
			{
				++result.Planted;
				continue;
			}
			switch (planted.error())
			{
			case Trees::Api::CreateError::UnknownSpecies:
				return std::unexpected{Api::Error::UnknownSpecies};
			case Trees::Api::CreateError::StorageFailure:
				return std::unexpected{Api::Error::TreeStorageFailure};
			case Trees::Api::CreateError::InvalidShape:
			case Trees::Api::CreateError::VolumeUnavailable:
				break;
			}
		}
		return result;
	}
}
