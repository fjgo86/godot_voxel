#include "world_generator.h"


WorldGenerator::WorldGenerator() {
    noise.instantiate();
    noise->set_seed(1337);
    noise->set_frequency(0.01f);
}

void WorldGenerator::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_noise", "noise"), &WorldGenerator::set_noise);
    ClassDB::bind_method(D_METHOD("get_noise"), &WorldGenerator::get_noise);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "noise", PROPERTY_HINT_RESOURCE_TYPE, "FastNoiseLite"), "set_noise", "get_noise");
}

void WorldGenerator::set_noise(const Ref<FastNoiseLite> &p_noise) {
    noise = p_noise;
}

Ref<FastNoiseLite> WorldGenerator::get_noise() const {
    return noise;
}

int WorldGenerator::get_used_channels_mask() const {
    return (1 << zylann::voxel::VoxelBuffer::CHANNEL_SDF) | (1 << zylann::voxel::VoxelBuffer::CHANNEL_COLOR);
}

zylann::voxel::VoxelGenerator::Result WorldGenerator::generate_block(VoxelQueryData input)
{
    VoxelGenerator::Result result;
	result.max_lod_hint = false;


    zylann::voxel::VoxelBuffer &buffer = input.voxel_buffer;
	Vector3i origin = input.origin_in_voxels;

	noise->set_noise_type(godot::FastNoiseLite::NoiseType::TYPE_PERLIN);
	noise->set_frequency(0.01f);

	const int height_start = -10;
	const int height_range = 50;

	// Recorre cada voxel y genera altura
	for (int z = 0; z < buffer.get_size().z; ++z) {
		for (int x = 0; x < buffer.get_size().x; ++x) {
			int world_x = origin.x + x;
			int world_z = origin.z + z;

			// Altura en mundo real
			float n = noise->get_noise_2d(world_x, world_z);
			float height = height_start + (n * 0.5f + 0.5f) * height_range;

			for (int y = 0; y < buffer.get_size().y; ++y) {
                int world_y = origin.y + y;

                uint8_t biome_id = 0;

                if (world_y < 10) {
                    // Tierra / roca
                    biome_id = 3;
                } else if (world_y < 20) {
                    // Vegetación
                    biome_id = 2;
                } else if (world_y < 30) {
                    // Praderas
                    biome_id = 1;
                }
                Color color(float(biome_id) / 255.0f, 0.0f, 0.0f, 1.0f);
                buffer.set_voxel_f(color.to_rgba32(), x, y, z, zylann::voxel::VoxelBuffer::CHANNEL_COLOR);
            }
		}
	}
    return result;
}

void WorldGenerator::expose_classes() {
    ClassDB::register_class<WorldGenerator>();
}
