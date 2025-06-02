shader_type spatial;
render_mode cull_back;

uniform int u_transition_mask;

uniform sampler2D biome_0;
uniform sampler2D biome_1;
uniform sampler2D biome_2;
uniform sampler2D biome_3;

float get_biome_transvoxel_secondary_factor(int idata) {
	int transition_mask = u_transition_mask & 0xff;
	int cell_border_mask = idata & 63;
	int vertex_border_mask = (idata >> 8) & 63;
	int m = transition_mask & cell_border_mask;
	float t = float(m != 0);
	t *= float((vertex_border_mask & ~transition_mask) == 0);
	return t;
}

vec3 get_biome_transvoxel_position(vec3 vertex_pos, vec4 fdata) {
	int idata = floatBitsToInt(fdata.a);
	float secondary_factor = get_biome_transvoxel_secondary_factor(idata);
	vec3 secondary_position = fdata.xyz;
	vec3 pos = mix(vertex_pos, secondary_position, secondary_factor);
	int itransition = (idata >> 16) & 0xff;
	float transition_cull = float(itransition == 0 || (itransition & u_transition_mask) != 0);
	pos *= transition_cull;
	return pos;
}

void vertex() {
	VERTEX = get_biome_transvoxel_position(VERTEX, CUSTOM0);
}

void fragment() {
	int biome_index = int(COLOR.r * 255.0 + 0.5); // Biome codificado en canal R
	vec3 tex_color;

	if (biome_index == 0) {
		tex_color = texture(biome_0, UV).rgb;
	} else if (biome_index == 1) {
		tex_color = texture(biome_1, UV).rgb;
	} else if (biome_index == 2) {
		tex_color = texture(biome_2, UV).rgb;
	} else {
		tex_color = texture(biome_3, UV).rgb;
	}

	ALBEDO = tex_color;
}
