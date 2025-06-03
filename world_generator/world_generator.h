#ifndef WORLD_GENERATOR_H
#define WORLD_GENERATOR_H

#include "generators/voxel_generator_script.h"

#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/fast_noise_lite.hpp>
#include <godot_cpp/core/class_db.hpp>
#include "../meshers/voxel_mesher.h"


using namespace godot;

class WorldGenerator : public zylann::voxel::VoxelGeneratorScript {
    GDCLASS(WorldGenerator, zylann::voxel::VoxelGeneratorScript)

private:
    godot::Ref<godot::FastNoiseLite> noise;
    float iso_scale = 10.0;

protected:
    static void _bind_methods();

public:
    static void expose_classes();

    WorldGenerator();
    ~WorldGenerator() override = default;

    int get_used_channels_mask() const override;
    Result generate_block(VoxelQueryData input) override;

    void set_noise(const godot::Ref<godot::FastNoiseLite> &p_noise);
    godot::Ref<godot::FastNoiseLite> get_noise() const;
};

#endif // WORLD_GENERATOR_H
