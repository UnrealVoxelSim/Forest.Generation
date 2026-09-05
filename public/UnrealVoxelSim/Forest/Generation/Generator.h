#pragma once

#include "UnrealVoxelSim/Forest/Generation/Api/IGenerator.h"
#include "UnrealVoxelSim/Trees/Api/IPlanter.h"
#include "UnrealVoxelSim/Voxel/Solid/Api/IReader.h"
#include "UnrealVoxelSim/Voxel/Solid/Api/MaterialId.h"

#include <span>
#include <thread>
#include <vector>

namespace UnrealVoxelSim::Forest::Generation
{
	class Generator final : public Api::IGenerator
	{
	public:
		Generator(const Voxel::Solid::Api::IReader& solids,
		          Trees::Api::IPlanter& trees,
		          std::span<const Voxel::Solid::Api::MaterialId> groundMaterials);
		[[nodiscard]] std::expected<Api::Result, Api::Error> Generate(Api::Request request) override;

	private:
		const Voxel::Solid::Api::IReader& m_Solids;
		Trees::Api::IPlanter& m_Trees;
		std::vector<Voxel::Solid::Api::MaterialId> m_GroundMaterials;
		std::thread::id m_OwnerThread{std::this_thread::get_id()};
	};
}
