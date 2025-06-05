#ifndef WORLD_GENERATOR_H
#define WORLD_GENERATOR_H

#include "generators/voxel_generator_script.h"

#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/fast_noise_lite.hpp>
#include <godot_cpp/core/class_db.hpp>
#include "../meshers/voxel_mesher.h"

#include "biomes/biome_generator.h"
#include "biomes/biome_loader.h"


using namespace godot;

namespace zylann::voxel {
    class VoxelLodTerrain;
}

class WorldGenerator : public zylann::voxel::VoxelGeneratorScript {
    GDCLASS(WorldGenerator, zylann::voxel::VoxelGeneratorScript)

private:
    float iso_scale = 10.0;
    BiomeGenerator* _biome_generator;
    BiomeLoader* _biome_loader;
    Node* _parent;

    zylann::voxel::VoxelLodTerrain* _terrain;
protected:
    static void _bind_methods();

public:
    static void expose_classes();

    WorldGenerator();
    ~WorldGenerator();

    int get_used_channels_mask() const override;
    Result generate_block(VoxelQueryData input) override;

    void on_biome_textures_loaded();

    void set_parent(Node* node);
    void set_terrain(zylann::voxel::VoxelLodTerrain* terrain);
    BiomeLoader* get_biome_loader() { return _biome_loader; }
    
};

#endif // WORLD_GENERATOR_H
