
varying vec3 v_position;
varying vec3 v_world_position;
varying vec3 v_normal;
varying vec2 v_uv;
varying vec4 v_color;

uniform vec4 u_color;
uniform sampler2D u_texture;
uniform float u_time;
uniform float u_texture_tiling;

void main()
{
	vec2 uv = v_uv;

	vec2 tiled_uv = (uv * u_texture_tiling);

	vec2 waves_uv_offset;
	waves_uv_offset.x = cos(u_time + tiled_uv.x + tiled_uv.y);
	waves_uv_offset.y = sin(u_time + tiled_uv.x + tiled_uv.y);

	gl_FragColor = u_color * texture2D( u_texture, tiled_uv + waves_uv_offset * 0.02);
}
