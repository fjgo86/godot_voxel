
#include <algorithm>


#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "biome_generator.h"


using namespace godot;


BiomeGenerator::BiomeGenerator() : _biome_loader(nullptr) {
}

BiomeGenerator::~BiomeGenerator() {
    // Cleanup if necessary
}

void BiomeGenerator::_bind_methods() {
}

void BiomeGenerator::set_seed(int32_t seed) {
    _seed = seed;

    temperature_noise.instantiate();
    temperature_noise->set_seed(_seed + 1000);
    temperature_noise->set_frequency(0.001f);

    humidity_noise.instantiate();
    humidity_noise->set_seed(_seed + 2000);
    humidity_noise->set_frequency(0.001f);

    elevation_noise.instantiate();
    elevation_noise->set_seed(_seed + 3000);
    elevation_noise->set_frequency(0.001f);
    elevation_noise->set_fractal_type(FastNoiseLite::FRACTAL_FBM);
    elevation_noise->set_fractal_octaves(4);
    elevation_noise->set_fractal_lacunarity(2.0f);
    elevation_noise->set_fractal_gain(0.5f);
    
    height_noise.instantiate();
    height_noise->set_seed(seed + 4000);
	height_noise->set_noise_type(godot::FastNoiseLite::NoiseType::TYPE_PERLIN);
    height_noise->set_fractal_type(FastNoiseLite::FRACTAL_FBM);
    height_noise->set_fractal_octaves(4);
    height_noise->set_fractal_gain(0.5f);
    height_noise->set_fractal_lacunarity(2.0f);
    height_noise->set_frequency(0.002f);
}

void BiomeGenerator::set_biome_loader(BiomeLoader* loader) {
    if (!loader) {
        UtilityFunctions::printerr("BiomeLoader cannot be null.");
        return;
    }
    _biome_loader = loader;
}

int32_t BiomeGenerator::get_biome_texture_index_at(int x, int y, int z) const {
    // Parámetros climáticos deterministas (por ahora fijos)
    float height = float(y) / 256.0f;
    float temperature = 0.5f; // TODO: usar ruido o datos climáticos
    float humidity = 0.5f;

    // Obtener lista ordenada de biomas por score
    std::vector<int32_t> sorted = _biome_loader->get_sorted_biome_ids(height, temperature, humidity);
    if (sorted.empty()) {
        return 0; // fallback
    }

    const BiomeTemplate& biome = _biome_loader->get_biome_by_index(sorted[0]);
    if (biome.textures.empty()) {
        return 0;
    }

    const String& texture_name = biome.textures[0]; // Primera textura
    int32_t texture_id = _biome_loader->get_texture_id_by_name(texture_name);
    return texture_id >= 0 ? texture_id : 0;
}

BiomeSelectionResult BiomeGenerator::get_biome_selection(int x, int y, int z) const {
    BiomeSelectionResult result;

    if (!_biome_loader) {
        return result; // fallback: todo a cero
    }

    Vector2 pos2d(x, z);

    // Ruido climático normalizado
    float temperature = temperature_noise->get_noise_2d(pos2d.x, pos2d.y) * 0.5f + 0.5f;
    float humidity    = humidity_noise->get_noise_2d(pos2d.x, pos2d.y) * 0.5f + 0.5f;
    float elevation   = elevation_noise->get_noise_2d(pos2d.x, pos2d.y) * 0.5f + 0.5f;

    // Obtener los dos mejores biomas
    std::vector<std::pair<int, float>> scores;
    const auto &biomes = _biome_loader->get_biomes();

    for (const auto &pair : biomes) {
        int32_t id = pair.first;
        const BiomeTemplate &biome = pair.second;
        float score = biome.match_score(elevation, temperature, humidity);
        scores.emplace_back(id, score);
    }

    std::sort(scores.begin(), scores.end(), [](const auto &a, const auto &b) {
        return a.second > b.second;
    });

    if (scores.empty()) {
        return result;
    }

    const int32_t id1 = scores[0].first;
    const float score1 = scores[0].second;

    const int32_t id2 = (scores.size() > 1) ? scores[1].first : id1;
    const float score2 = (scores.size() > 1) ? scores[1].second : 0.0f;

    const float total_score = score1 + score2 + 0.001f;
    const float blend = score2 / total_score;

    const BiomeTemplate &biome1 = _biome_loader->get_biome_by_index(id1);
    const BiomeTemplate &biome2 = _biome_loader->get_biome_by_index(id2);

    // Altura física real interpolada entre los dos biomas
    float raw_noise1 = biome1.get_height_noise(pos2d);
    float raw_noise2 = biome2.get_height_noise(pos2d);

    float height = raw_noise1 * (1.0f - blend) + raw_noise2 * blend;

    // Asignación de texturas
    int32_t tex1 = !biome1.texture_indexes.empty() ? biome1.texture_indexes[0] : 0;
    int32_t tex2 = !biome2.texture_indexes.empty() ? biome2.texture_indexes[0] : tex1;

    result.texture_index_1 = tex1;
    result.texture_index_2 = tex2;
    result.blend = blend;
    result.height = height;

    return result;
}
