#include "world_generator.h"

#include <godot_cpp/classes/resource_loader.hpp>
#include "../terrain/variable_lod/voxel_lod_terrain.h"

#include "biomes/biome_loader.h"

WorldGenerator::WorldGenerator() {
    _parent = nullptr;

    _biome_generator = memnew(BiomeGenerator);

    _biome_loader = memnew(BiomeLoader);

    _biome_generator->set_biome_loader(_biome_loader);
    _biome_generator->set_seed(1337);
}

WorldGenerator::~WorldGenerator(){
    /*if (_biome_generator) {
        memdelete(_biome_generator);
        _biome_generator = nullptr;
    }

    if (_biome_loader) {
        if (_parent) {
            if (_parent->get_children().find(_biome_loader)) {
                _parent->remove_child(_biome_loader);
            }
        }
        memdelete(_biome_loader);
        _biome_loader = nullptr;
    }*/
}

void WorldGenerator::_bind_methods() {

    ClassDB::bind_method(D_METHOD("set_parent"), &WorldGenerator::set_parent);
    ClassDB::bind_method(D_METHOD("set_terrain"), &WorldGenerator::set_terrain);
    ClassDB::bind_method(D_METHOD("get_biome_loader"), &WorldGenerator::get_biome_loader);
    ClassDB::bind_method(D_METHOD("on_biome_textures_loaded"), &WorldGenerator::on_biome_textures_loaded);
    
}

int WorldGenerator::get_used_channels_mask() const {
    return (1 << zylann::voxel::VoxelBuffer::CHANNEL_SDF) | (1 << zylann::voxel::VoxelBuffer::CHANNEL_COLOR);
}

zylann::voxel::VoxelGenerator::Result WorldGenerator::generate_block(VoxelQueryData input)
{
    VoxelGenerator::Result result;
	result.max_lod_hint = true;
    if (!_biome_generator){
        return result;
    }
    const int lod = input.lod;
    Vector3i origin = input.origin_in_voxels;
    zylann::voxel::VoxelBuffer &buffer = input.voxel_buffer;

	const float amplitude = 16.0f;                    // altura máxima constante


	// Recorre cada voxel y genera altura
	for (int z = 0; z < buffer.get_size().z; ++z) {
		for (int x = 0; x < buffer.get_size().x; ++x) {
			int world_x = origin.x + x;
			int world_z = origin.z + z;
			for (int y = 0; y < buffer.get_size().y; ++y) {
                int world_y = origin.y + y;

                // -- Biome --
                BiomeSelectionResult sel = _biome_generator->get_biome_selection(world_x, world_y, world_z);
                Color color(
                    float(sel.texture_index_1) / 15.0f,
                    float(sel.texture_index_2) / 15.0f,
                    sel.blend,
                    1.0f
                );
                buffer.set_voxel(color.to_rgba32(), x, y, z, zylann::voxel::VoxelBuffer::CHANNEL_COLOR);

                // -- Signed Distance Field
                //    < 0 = solid, > 0 = air
                float sdf = float(world_y) - sel.height;
                sdf = CLAMP(sdf, -20.f, 20.f);
                buffer.set_voxel_f(sdf, x, y, z, zylann::voxel::VoxelBuffer::CHANNEL_SDF);
            }
		}
	}
    return result;
}

void WorldGenerator::expose_classes() {
    UtilityFunctions::print("Exposing WorldGenerator");
    ClassDB::register_class<WorldGenerator>();
    ClassDB::register_class<BiomeLoader>();
    ClassDB::register_class<BiomeGenerator>();
}


void WorldGenerator::on_biome_textures_loaded(){
    _parent->remove_child(_biome_loader); // Disable _process on BiomeGenerator and allow this class to call destructor on it.
    //Shader assignation made on set_terrain, which holds the material.
}

void WorldGenerator::set_parent(Node* node){
    _parent = node;
    _biome_loader->initialize();
    _parent->add_child(_biome_loader);   // Enable _process on BiomeGenerator
}

void WorldGenerator::set_terrain(zylann::voxel::VoxelLodTerrain* terrain) {
    _terrain = terrain;
    // Configuration.
    _terrain->set_lod_distance(64);

    // Shader
    if (_biome_loader->get_texture_array().is_valid()){
        Ref<Shader> shader = ResourceLoader::get_singleton()->load("res://bioma.gdshader");
        if (shader.is_valid()) {
            Ref<ShaderMaterial> material;
            material.instantiate();
            material->set_shader(shader); // ← AQUÍ se asigna el shader cargado
            material->set_shader_parameter("biome_textures", _biome_loader->get_texture_array());
            material->set_shader_parameter("biome_texture_scales", PackedFloat32Array{
                1.0f, 1.0f, 1.5f, 2.0f, 0.8f, 1.2f, 1.0f, 1.8f,
                1.3f, 0.9f, 1.6f, 1.4f, 1.7f, 1.0f, 1.0f, 1.0f
            });
            terrain->set_material(material);
            UtilityFunctions::print("Shader loaded and material assigned.");
        } else {
            UtilityFunctions::printerr("Failed to load shader.");
        }
    }

}