add_library(${PROJECT_NAME} UnrealVoxelSim/Forest/Generation/Generator.cpp)
target_include_directories(${PROJECT_NAME} PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/public>" "$<INSTALL_INTERFACE:include>")
target_link_libraries(${PROJECT_NAME} PUBLIC UnrealVoxelSim.Forest.Generation.Api UnrealVoxelSim.Trees.Api UnrealVoxelSim.Voxel.Solid.Api)
UnrealVoxelSimConfigureTarget(${PROJECT_NAME})
