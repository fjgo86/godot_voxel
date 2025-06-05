#ifndef BIOME_TEMPLATE_H
#define BIOME_TEMPLATE_H

#include <vector>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/classes/fast_noise_lite.hpp>

using namespace godot;

struct BiomeTemplate {
	// Textures for this biome
	std::vector<String> textures; // Filled from JSON
	std::vector<uint32_t> texture_indexes; // Filled in load_biomes
	String name;

	// Match range block
	struct MatchRange {
		float min_height = 0.0f;
		float max_height = 1.0f;
		float min_temperature = 0.0f;
		float max_temperature = 1.0f;
		float min_humidity = 0.0f;
		float max_humidity = 1.0f;
	} match_range;

	// Noise block
	struct Noise {
		int noise_type = 0;
		float frequency = 0.01f;
		float amplitude = 1.0f;
		float lacunarity = 2.0f;
		float gain = 0.5f;
		int octaves = 4;
	} noise;

	// Biome-specific data block
	struct BiomeData {
		float fertility = 0.5f;
		float vegetation_density = 0.5f;
	} biome_data;

	// Debug and shader-related block
	struct Debug {
		float uv_scale = 16.0f;
		float height_blend = 0.0f;
		float ao_strength = 1.0f;
	} debug;

	BiomeTemplate() {
        generator.instantiate();
    }

	BiomeTemplate(const BiomeTemplate& other) = default;

	void from_json(const Dictionary& data) {
		name = data.has("name") ? String(data["name"]) : "Unnamed";

		if (data.has("textures")) {
			Variant raw = data["textures"];
			if (raw.get_type() == Variant::ARRAY) {
				Array arr = raw;
				for (int i = 0; i < arr.size(); ++i) {
					Variant v = arr[i];
					if (v.get_type() == Variant::STRING) {
						textures.push_back(v);
					}
				}
			}
		}

		if (data.has("match_range")) {
			Dictionary r = data["match_range"];
			match_range.min_height = r.get("min_height", 0.0f);
			match_range.max_height = r.get("max_height", 1.0f);
			match_range.min_temperature = r.get("min_temperature", 0.0f);
			match_range.max_temperature = r.get("max_temperature", 1.0f);
			match_range.min_humidity = r.get("min_humidity", 0.0f);
			match_range.max_humidity = r.get("max_humidity", 1.0f);
		}

		if (data.has("noise")) {
			Dictionary n = data["noise"];
			noise.noise_type = n.get("type", 0);
			noise.frequency = n.get("frequency", 0.01f);
			noise.amplitude = n.get("amplitude", 1.0f);
			noise.lacunarity = n.get("lacunarity", 2.0f);
			noise.gain = n.get("gain", 0.5f);
			noise.octaves = n.get("octaves", 4);
		}

		if (data.has("biome_data")) {
			Dictionary b = data["biome_data"];
			biome_data.fertility = b.get("fertility", 0.5f);
			biome_data.vegetation_density = b.get("vegetation_density", 0.5f);
		}

		if (data.has("debug")) {
			Dictionary d = data["debug"];
			debug.uv_scale = d.get("uv_scale", 16.0f);
			debug.height_blend = d.get("height_blend", 0.0f);
			debug.ao_strength = d.get("ao_strength", 1.0f);
		}
	}

	inline float match_score(float height, float temperature, float humidity) const {
		float h_center = (match_range.min_height + match_range.max_height) * 0.5f;
		float t_center = (match_range.min_temperature + match_range.max_temperature) * 0.5f;
		float hu_center = (match_range.min_humidity + match_range.max_humidity) * 0.5f;

		float h_range = match_range.max_height - match_range.min_height + 0.001f;
		float t_range = match_range.max_temperature - match_range.min_temperature + 0.001f;
		float hu_range = match_range.max_humidity - match_range.min_humidity + 0.001f;

		float h_score = 1.0f - Math::abs(h_center - height) / h_range;
		float t_score = 1.0f - Math::abs(t_center - temperature) / t_range;
		float hu_score = 1.0f - Math::abs(hu_center - humidity) / hu_range;

		float h_penalty = 1.0f / h_range;
		float t_penalty = 1.0f / t_range;
		float hu_penalty = 1.0f / hu_range;

		const float h_weight = 1.0f;
		const float t_weight = 1.0f;
		const float hu_weight = 1.0f;

		float score = (
			h_score * h_penalty * h_weight +
			t_score * t_penalty * t_weight +
			hu_score * hu_penalty * hu_weight
		) / (h_weight + t_weight + hu_weight);

		return Math::clamp(score, 0.0f, 1.0f);
	}

    float BiomeTemplate::get_height_noise(Vector2 pos) const {
        generator->set_noise_type(static_cast<godot::FastNoiseLite::NoiseType>(noise.noise_type));
        generator->set_frequency(noise.frequency);
        generator->set_fractal_octaves(noise.octaves);
        generator->set_fractal_gain(noise.gain);
        generator->set_fractal_lacunarity(noise.lacunarity);
        float n = generator->get_noise_2d(pos.x, pos.y);
        return (n * 0.5f + 0.5f) * noise.amplitude;
    }

    private:
    Ref<FastNoiseLite> generator;

};

#endif // BIOME_TEMPLATE_H
