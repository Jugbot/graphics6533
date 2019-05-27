/*****************************
 * File: fshader42.glsl
 *       A simple fragment shader
 *****************************/

 #version 150  

in vec4 color;
in float dist;
varying vec2 texcoord;
varying vec2 latcoord;
uniform sampler2D checkerTex;
uniform sampler1D stripeTex;
uniform bool floorTexture = true;
out vec4 fColor;

uniform vec4 fogColor = vec4(0.7, 0.7, 0.7, 0.5);
uniform float fogstart = 0.f;
uniform float fogend = 18.f;
uniform float fogdensity = .09f;

uniform int f_fog = 0;
uniform int f_sphereTexture = 1;
uniform bool f_lattice = false;

void main() 
{ 
	if (f_lattice && fract(4 * latcoord.x) < 0.35 && fract(4 * latcoord.y) < 0.35)
		discard;
	
	float fogScale = 1;
	if (f_fog == 1) {
		fogScale = clamp((fogend - dist) / (fogend - fogstart), 0, 1);
	} else if (f_fog == 2) {
		fogScale = exp(-dist * fogdensity);
	} else if (f_fog == 3) {
		fogScale = exp(-pow(dist * fogdensity, 2));
	} 
	fColor = vec4(mix(fogColor, color, fogScale).xyz, color.a);
	if (floorTexture)
		fColor *= texture(checkerTex, texcoord);
	else if (f_sphereTexture == 1)
		fColor *= texture(stripeTex, texcoord.x);
	else if (f_sphereTexture == 2) {
		vec4 texc = texture(checkerTex, texcoord);
		if (texc.x == 0)
			texc = vec4(0.9, 0.1, 0.1, 1.0);
		fColor *= texc;
	}

} 

