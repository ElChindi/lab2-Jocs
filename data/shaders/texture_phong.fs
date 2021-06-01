
varying vec3 v_position;
varying vec3 v_world_position;
varying vec3 v_normal;
varying vec2 v_uv;
varying vec4 v_color;

uniform vec4 u_color;
uniform sampler2D u_texture;
uniform float u_time;
uniform vec3 u_eye;

void main()
{

	float p = 0.2;
	vec3 ka = vec3(1, 1, 1);
	vec3 kd = vec3(0.7, 0.6, 0.6);
	vec3 ks = vec3(0.1, 0.1, 0.1);

	vec3 id = vec3(0.7, 0.6, 0.6);
	vec3 is = vec3(0.3, 0.3, 0.3);
	vec3 ia = vec3(1, 1, 1);
	vec3 lp = v_position - vec3(10.0, 0.0, 50.0);;

	//here we set up the normal as a color to see them as a debug
	vec3 c = v_normal;

	//here write the computations for PHONG.
	vec3 l = normalize((lp - v_position));
	vec3 r = -reflect(l,v_normal);
	vec3 v = normalize((u_eye - v_position));
	
	c = ka*ia + kd*clamp(dot(l,v_normal),0.0,1.0)*id + ks*pow(clamp(dot(r,v),0.0,1.0),p)*is;

	vec2 uv = v_uv;
	gl_FragColor = vec4(c, 1.0) * texture2D( u_texture, uv );
}
