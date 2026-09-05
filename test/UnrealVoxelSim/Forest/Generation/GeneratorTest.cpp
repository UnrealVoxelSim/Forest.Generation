#include "UnrealVoxelSim/Forest/Generation/Generator.h"

#include "UnrealVoxelSim/Trees/Api/CreateError.h"
#include "UnrealVoxelSim/Trees/Api/StandardSpecies.h"
#include "UnrealVoxelSim/Voxel/Solid/Api/Cell.h"
#include "UnrealVoxelSim/Voxel/Solid/Api/StandardMaterials.h"

#include <gtest/gtest.h>

#include <array>
#include <expected>
#include <optional>
#include <vector>

namespace UnrealVoxelSim::Forest::Generation
{
	namespace
	{
		class Terrain final : public Voxel::Solid::Api::IReader
		{
		public:
			[[nodiscard]] std::expected<Voxel::Solid::Api::Cell, Voxel::Api::ReadError> Read(
				const Voxel::Api::Position position) const noexcept override
			{
				return position.Z == 0
					? Voxel::Solid::Api::Cell{Voxel::Solid::Api::StandardMaterials::Dirt}
					: Voxel::Solid::Api::Cell{};
			}
		};

		class Planter final : public Trees::Api::IPlanter
		{
		public:
			[[nodiscard]] std::expected<Ecs::Api::EntityId, Trees::Api::CreateError> Plant(
				const Trees::Api::CreateRequest request) override
			{
				Requests.push_back(request);
				if (Failure)
					return std::unexpected{*Failure};
				return Ecs::Api::EntityId{};
			}

			std::vector<Trees::Api::CreateRequest> Requests;
			std::optional<Trees::Api::CreateError> Failure;
		};

		TEST(GeneratorTest, ProducesDeterministicGroundedCandidates)
		{
			Terrain terrain;
			const std::array ground{Voxel::Solid::Api::StandardMaterials::Dirt};
			Planter firstPlanter;
			Planter secondPlanter;
			Generator first{terrain, firstPlanter, ground};
			Generator second{terrain, secondPlanter, ground};
			const Api::Request request{{{-10, -10, 0}, {10, 10, 2}}, 4, Trees::Api::StandardSpecies::Oak, Api::Seed{42}};

			const auto firstResult = first.Generate(request);
			const auto secondResult = second.Generate(request);

			ASSERT_TRUE(firstResult);
			ASSERT_TRUE(secondResult);
			EXPECT_EQ(firstResult->Planted, 4U);
			ASSERT_EQ(firstPlanter.Requests.size(), secondPlanter.Requests.size());
			for (std::size_t index = 0; index < firstPlanter.Requests.size(); ++index)
			{
				EXPECT_EQ(firstPlanter.Requests[index].Root, secondPlanter.Requests[index].Root);
				EXPECT_EQ(firstPlanter.Requests[index].Seed, secondPlanter.Requests[index].Seed);
				EXPECT_EQ(firstPlanter.Requests[index].Root.Z, 1);
			}
		}

		TEST(GeneratorTest, SkipsUnavailableTreeVolumes)
		{
			Terrain terrain;
			const std::array ground{Voxel::Solid::Api::StandardMaterials::Dirt};
			Planter planter;
			planter.Failure = Trees::Api::CreateError::VolumeUnavailable;
			Generator generator{terrain, planter, ground};

			const auto result = generator.Generate(
				{{{-2, -2, 0}, {2, 2, 2}}, 3, Trees::Api::StandardSpecies::Oak, Api::Seed{1}});

			ASSERT_TRUE(result);
			EXPECT_EQ(result->Candidates, 3U);
			EXPECT_EQ(result->Planted, 0U);
		}
	}
}
