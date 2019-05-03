/***************************
 * File: vshader42.glsl:
 *   A simple vertex shader.
 *
 * - Vertex attributes (positions & colors) for all vertices are sent
 *   to the GPU via a vertex buffer object created in the OpenGL program.
 *
 * - This vertex shader uses the Model-View and Projection matrices passed
 *   on from the OpenGL program as uniform variables of type mat4.
 ***************************/

 #version 150  
 #define M_PI 3.1415926535897932384626433832795

in  vec3 vPosition;
in  vec4 vColor;
in  vec3 vNormal;
out vec4 color;

uniform mat4 camera;
uniform mat4 model_view;
uniform mat4 projection;

uniform bool smooth_shading;
uniform vec4 ambient, diffuse, specular;
uniform float shininess;
uniform vec4 global_illum = vec4(1.f,1.f,1.f,1.f);
uniform vec4 dir_ambient = vec4(0, 0, 0, 1);
uniform vec4 dir_diffuse = vec4(0.8, 0.8, 0.8, 1);
uniform vec4 dir_specular = vec4(0.2, 0.2, 0.2, 1);
uniform vec4 dir_light = vec4(0.1,0,-1,0);
uniform vec4 point_ambient = vec4(0, 0, 0, 1);
uniform vec4 point_diffuse = vec4(1,1,1, 1);
uniform vec4 point_specular = vec4(1,1,1, 1);
uniform vec4 point_light;
uniform vec4 point_to = vec4(-6,0,-4.5,1);
uniform float spotlight_exp = 15;
uniform float spotlight_cutoff = 20;

uniform bool f_lighting = true;
uniform bool f_shading = false;
uniform bool f_spotlight = true;

void main() 
{
	vec4 vPosition4 = vec4(vPosition, 1.0);
    gl_Position = projection * camera * model_view * vPosition4;

	if (!f_lighting) {
		color = vColor;
		return;
	}
	
	vec4 global_ambient = global_illum * ambient;

	vec3 Position = (camera * model_view * vPosition4).xyz;
	vec3 Obj_Normal;
	if (f_shading)
		Obj_Normal = normalize( camera * model_view * vec4(vPosition, 0.0) ).xyz;
	else
		Obj_Normal = normalize( camera * model_view * vec4(vNormal, 0.0) ).xyz;
//	DIRECTIONAL LIGHT
    vec3 Light_Normal = normalize(-dir_light.xyz);
    vec3 E = normalize( -Position );
    vec3 H = normalize( Light_Normal + E );
	//	if ( dot(Obj_Normal, E) < 0 ) Obj_Normal = -Obj_Normal;

	vec4 directional_ambient = dir_ambient * ambient;
	float d = max( dot(Light_Normal, Obj_Normal), 0.0 );
	vec4 directional_diffuse = d * dir_diffuse * diffuse;
    float s = pow( max(dot(Obj_Normal, H), 0.0), shininess );
	vec4 directional_specular = s * dir_specular * specular;

	if( dot(Light_Normal, Obj_Normal) < 0.0 ) {
		directional_specular = vec4(0.0, 0.0, 0.0, 1.0);
    } 
	
//	color = global_ambient + directional_ambient + directional_diffuse;
	color = global_ambient + directional_ambient + directional_diffuse + directional_specular;
//	color = directional_specular;

//	POINT & SPOT LIGHT
	vec3 dist_v = (camera * point_light).xyz - Position;
	Light_Normal = normalize(dist_v);
	E = normalize( -Position );
	H = normalize( Light_Normal + E );
	float dist_m = length(dist_v);
	float attenuation = 1/(2 + 0.01 * dist_m + 0.001 * dist_m * dist_m);

	vec4 point_ambient = point_ambient * ambient;
	d = max( dot(Light_Normal, Obj_Normal), 0.0 );
	vec4 point_diffuse = d * point_diffuse * diffuse;
	s = pow( max(dot(Obj_Normal, H), 0.0), shininess );
	vec4 point_specular = s * point_specular * specular;

	if( dot(Light_Normal, Obj_Normal) < 0.0 ) {
		directional_specular = vec4(0.0, 0.0, 0.0, 1.0);
	} 

	if (f_spotlight) {
		vec3 spotlight_dir = normalize((camera * point_to).xyz - (camera * point_light).xyz);
		if (dot(-Light_Normal, spotlight_dir) >= cos(spotlight_cutoff * M_PI / 180))
			attenuation *= pow(dot(-Light_Normal, spotlight_dir), spotlight_exp);
		else 
			attenuation = 0;
	}

	color += attenuation * (point_ambient + point_diffuse + point_specular);
	
} 
