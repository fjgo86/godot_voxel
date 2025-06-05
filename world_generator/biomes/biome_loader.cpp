#include "biome_loader.h"

#include <algorithm>
#include <utility>

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/main_loop.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/core/print_string.hpp>


#include "../world_generator.h"

const uint32_t MAX_LOADED_TEXTURES = 31; 

BiomeLoader::BiomeLoader() : _textures_loaded(false), _textures_failed(false) {
	clear_assets();
	_texture_size = Vector2(512, 512);
}

BiomeLoader::~BiomeLoader() {
  clear_assets();
}

void BiomeLoader::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_texture_array"), &BiomeLoader::get_texture_array);
	ADD_SIGNAL(MethodInfo("finished_loading_textures"));
}

void BiomeLoader::initialize(String resolution, String biome_path, String texture_path) {
	_resolution = resolution;
	load_textures_async(texture_path);
}

void BiomeLoader::_process(float delta) {
	if (_textures_loaded) {
		return;
	}
	process_pending_textures(delta);
}

void BiomeLoader::clear_assets() {
	_loading_textures.clear();
	_textures_array.instantiate();
	_texture_name_to_id.clear();
	_pending_images.clear();
	_textures_loaded = false;
	_textures_failed = false;
}

bool BiomeLoader::load_all_from_directory(const String &textures_path, const String &biomes_path) {
	load_textures_async(textures_path);
	bool biomes_ok = load_biomes_from_directory(biomes_path);
	return biomes_ok;
}

void BiomeLoader::load_textures_async(const String &textures_path) {
	
	String textures_path_final = textures_path.path_join("textures_" + _resolution);
	Ref<DirAccess> dir = DirAccess::open(textures_path_final);
	if (!dir.is_valid()) {
		UtilityFunctions::print("Invalid textures path: ", textures_path_final);
		return;
	}
	UtilityFunctions::print("Starting texture loading for path: ", textures_path_final);

	dir->list_dir_begin();
	String folder;
	while (!(folder = dir->get_next()).is_empty()) {
		if (!dir->current_is_dir() || folder.begins_with(".")) continue;

		String folder_path = textures_path_final.path_join(folder);
		String color_path = folder_path.path_join(folder + "_Color.png");

		_pending_images.emplace_back(folder, color_path);
		UtilityFunctions::print("Adding ", folder , " to pending textures.");
	}
}

void BiomeLoader::process_pending_textures(float delta) {
	int loaded = 0;
	while (!_pending_images.empty() && loaded < _max_textures_per_tick) {
		auto [folder, color_path] = _pending_images.back();
		_pending_images.pop_back();
		
		Ref<Image> texture = import_texture(color_path, "texture");
		

		if (!texture.is_valid()) {
			UtilityFunctions::printerr("Couldn't load color texture from: ", color_path, " skipping texture.");
			continue;
		}

		_loading_textures.push_back(texture);

		uint32_t tex_id = _texture_name_to_id.size();
		_texture_name_to_id[folder] = tex_id;

		UtilityFunctions::print("Loaded texture ", folder, " with ID: ", tex_id);

		loaded++;
		if (loaded >= MAX_LOADED_TEXTURES){
			UtilityFunctions::print("Processed ", loaded, " textures, can't process anymore.");
			break;
		}
	}

	if (_loading_textures.size() == 0) {
		UtilityFunctions::printerr("No textures loaded. Texture2DArray creation skipped.");
		return;
	}
	_textures_array.instantiate();
	_textures_array->create_from_images(_loading_textures);
	_loading_textures.clear();

	if (_pending_images.empty()) {
		finished_loading_textures();
	}
}

Ref<Image> BiomeLoader::import_texture(const String &texture_path, const String &texture_type){
	Ref<Image> texture_asset;
	
	Ref<FileAccess> texture_file = FileAccess::open(texture_path, FileAccess::READ);
	if (!texture_file.is_valid()) {
		UtilityFunctions::printerr("Failed to open ", texture_type, " texture: ", texture_path, ", error = ", texture_file->get_open_error());
		return nullptr;
	}
	
	Ref<Image> texture_image = memnew(Image);
	Error error_image = texture_image->load_png_from_buffer(texture_file->get_buffer(texture_file->get_length()));
	if (error_image != OK) {
		UtilityFunctions::printerr("Failed to load ", texture_type, " texture from: ", texture_path, " Error: ", error_image);
		return nullptr;
	}
	texture_image->convert(Image::FORMAT_RGBA8);	

	if (texture_image->get_format() != Image::FORMAT_RGBA8) {
		UtilityFunctions::printerr("Texture format is not RGBA8: ", texture_image->get_format(), " at: ", texture_path);
		return nullptr;
	}
	return texture_image;
}

bool BiomeLoader::load_biomes_from_directory(const String &biomes_path) {
	Ref<DirAccess> dir = DirAccess::open(biomes_path);
	if (!dir.is_valid()) {
			UtilityFunctions::printerr("Invalid biome directory: ", biomes_path);
			return false;
	}
	UtilityFunctions::print("Loading biomes from directory: ", biomes_path);

	dir->list_dir_begin();
	String folder;
	int32_t biome_count = 0;

	while (!(folder = dir->get_next()).is_empty()) {
			if (!dir->current_is_dir() || folder.begins_with(".")) continue;

			String file_path = biomes_path.path_join(folder).path_join("biome_template.json");
			Ref<FileAccess> f = FileAccess::open(file_path, FileAccess::READ);
			if (!f.is_valid()) {
					UtilityFunctions::printerr("Could not open biome_template.json in: ", file_path);
					continue;
			}

			String json_text = f->get_as_text();
			Ref<JSON> json = memnew(JSON);
			if (json->parse(json_text) != OK) {
					UtilityFunctions::printerr("Failed to parse biome_template.json in: ", file_path);
					continue;
			}

			Dictionary data = json->get_data();
			BiomeTemplate biome;
			biome.from_json(data);

			if (biome.textures.empty()) {
					UtilityFunctions::printerr("Biome ", data.get("name", ""), " has no textures defined.");
					continue;
			}
			// Map texture names to IDs.
			std::vector<int32_t> to_remove;
			for (int i = 0; i < biome.textures.size(); ++i) {
					String tex_name = biome.textures[i];
					std::map<String, uint32_t>::iterator it = _texture_name_to_id.find(tex_name);
					if ( it == _texture_name_to_id.end()) {
							UtilityFunctions::printerr("Texture not found for biome ", data.get("name", ""), ": ", tex_name);
							to_remove.push_back(i);
					}
			}
			// Delete from the biome the textures that are not found.
			if (!to_remove.empty()) {
					for (int i = to_remove.size() - 1; i >= 0; --i) {
							biome.textures.erase(biome.textures.begin() + to_remove[i]);
					}
			}

			// Check again if the biome still has textures.
			if (biome.textures.empty()) {
					UtilityFunctions::printerr("Biome ", data.get("name", ""), " had all it's textures removed and is invalid now.");
					continue;
			}

			// Run again (yes, again) the textures to map them by index.
			
			for (uint32_t i = 0; i < biome.textures.size(); ++i) {
					String tex_name = biome.textures[i];
					//auto it = _texture_name_to_id.find(tex_name);
					biome.texture_indexes.push_back(_texture_name_to_id.find(tex_name)->second);
			}
			
			UtilityFunctions::print("Loaded biome: ", data.get("name", ""), " with ID: ", biome_count);
			_biomes[biome_count] = biome;
			++biome_count;
	}

	UtilityFunctions::print("Loaded ", _biomes.size(), " biomes from directory: ", biomes_path);
	return true;
}


std::vector<int32_t> BiomeLoader::get_sorted_biome_ids(float height, float temperature, float humidity) const {
	std::vector<int32_t> sorted;
	std::vector<std::pair<int32_t, float>> scored;

	for (const auto &[id, biome] : _biomes) {
		float score = biome.match_score(height, temperature, humidity);
		scored.push_back({id, score});
	}

	std::sort(scored.begin(), scored.end(), [](auto &a, auto &b) {
		return a.second > b.second;
	});

	for (const auto &[id, _] : scored) {
		sorted.push_back(id);
	}
	return sorted;
}

void BiomeLoader::finished_loading_textures() {
	_textures_loaded = true;
	//_textures_array->update_texture_list();
	UtilityFunctions::print("BiomeLoader finished loading textures.");	

	emit_signal("finished_loading_textures");
	load_biomes_from_directory();
}