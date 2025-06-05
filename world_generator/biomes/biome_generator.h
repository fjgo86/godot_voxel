
#ifndef BIOME_GENERATOR_H
#define BIOME_GENERATOR_H

#include <tuple>

#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/fast_noise_lite.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/variant/vector2i.hpp>


#include "biome_loader.h"

using namespace godot;
class Terrain3D;

struct BiomeSelectionResult {
	int texture_index_1 = 0;
	int texture_index_2 = 0;
	float blend = 0.0f;
    float height = 0.0f;
};

class BiomeGenerator : public Object {
    GDCLASS(BiomeGenerator, Object);

private:
    Ref<FastNoiseLite> temperature_noise;// Biome's temperature.
    Ref<FastNoiseLite> humidity_noise;  // Biome's humidity.
    Ref<FastNoiseLite> elevation_noise; // Biome's elevation.
    Ref<FastNoiseLite> height_noise;    // Terrain's height.
    int32_t _seed = 1337;

    BiomeLoader* _biome_loader;

protected:
    static void _bind_methods();

public:
    BiomeGenerator();
    ~BiomeGenerator();
    void set_seed(int32_t seed);
    void set_biome_loader(BiomeLoader* loader);

    int32_t get_biome_texture_index_at(int x, int y, int z) const;
    std::vector<std::pair<int, float>> get_scored_biomes(const Vector2 &world_pos) const;


    BiomeSelectionResult get_biome_selection(int x, int y, int z) const;
};

#endif // BIOME_GENERATOR_H
