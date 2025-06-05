#ifndef BIOME_MANAGER_H
#define BIOME_MANAGER_H

#include <unordered_map>
#include <map>
#include <vector>
#include <tuple>
#include <algorithm>

#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/classes/texture2d_array.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/string.hpp>

#include "biome_template.h"

using namespace godot;

class BiomeLoader : public Node {
	GDCLASS(BiomeLoader, Node);

private:
	// Storage
	std::unordered_map<uint32_t, BiomeTemplate> _biomes;
	std::map<String, uint32_t> _texture_name_to_id;
	Ref<Texture2DArray> _textures_array;
	TypedArray<Image> _loading_textures;
	BiomeTemplate _default_biome;

	Vector2 _texture_size;

	// Loading State
	bool _textures_loaded = false;
	bool _textures_failed = false;

	// Biome loading async
	std::vector<std::tuple<String, String>> _pending_images;
	int _max_textures_per_tick = 5;
	String _resolution;

public:
	BiomeLoader();
	~BiomeLoader();
  static void _bind_methods();

	Vector2 get_texture_size() { return _texture_size; }
	
	void initialize(String resolution = "2k", String biome_path = "res://game_config/biomes/", String texture_path = "res://textures/biomes/");

	bool are_textures_ready() const { return _textures_loaded && !_textures_failed; }

	bool load_all_from_directory(const String &textures_path, const String &biomes_path);
	bool load_biomes_from_directory(const String &biomes_path = "res://game_config/biomes/");
	void load_textures_async(const String &textures_path);
	void process_pending_textures(float delta);
	bool wait_for_textures_ready(int timeout_ms);
	void finished_loading_textures();
	void clear_assets();

	Ref<Texture2DArray> get_texture_array() { return _textures_array; }

	Ref<Image> import_texture(const String &texture_path, const String &texture_type);

	const std::unordered_map<uint32_t, BiomeTemplate> &get_biomes() const {
		return _biomes;
	}

	uint32_t get_texture_id_by_name(const String &name) const {
    auto it = _texture_name_to_id.find(name);
    if (it != _texture_name_to_id.end()) {
        return it->second;
    }
    return -1;
}


	const BiomeTemplate &get_biome_by_index(uint32_t id) const {
			auto it = _biomes.find(id);
			if (it != _biomes.end()) {
					return it->second;
			}
			return _default_biome;
	}

	std::vector<int32_t> get_sorted_biome_ids(float height, float temperature, float humidity) const;

	const BiomeTemplate &get_default_biome() const {
		return _default_biome;
	}

	std::unordered_map<uint32_t, BiomeTemplate> get_biomes() { return _biomes; }

	// To handle signals and notifications
	virtual void _process(float delta);

};

#endif // BIOME_MANAGER_H
