//this var comes from the vertex shader
//they are baricentric interpolated by pixel according to the distance to every vertex
varying vec3 v_wPos;
varying vec3 v_wNormal;


//here create uniforms for all the data we need here
uniform float p;
uniform vec3 ka;
uniform vec3 kd;
uniform vec3 ks;

uniform vec3 id;
uniform vec3 is;
uniform vec3 ia;
uniform vec3 lp;

uniform vec3 eye;

void main()
{
	//here we set up the normal as a color to see them as a debug
	vec3 color = v_wNormal;

	//here write the computations for PHONG.
	vec3 l = normalize((lp - v_wPos));
	vec3 r = -reflect(l,v_wNormal);
	vec3 v = normalize((eye - v_wPos));
	
	vec3 c = ka*ia + kd*clamp(dot(l,v_wNormal),0.0,1.0)*id + ks*pow(clamp(dot(r,v),0.0,1.0),p)*is;

	//set the ouput color por the pixel
	gl_FragColor = vec4( c, 1.0 ) * 1.0;
}
