 #version 150  
 #define M_PI 3.1415926535897932384626433832795

in  vec3 vPosition;
in  vec4 vColor;
in  vec3 vNormal;
in  vec2 vTexture;
out vec2 texcoord;
out vec2 latcoord;
out vec4 color;
out float dist;

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

uniform int f_sphereTexture = 1;
uniform int f_latticeType = 1;
uniform bool f_lattice = false;
uniform bool f_relTexture = true;
uniform bool f_tiltTexture = true;

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
    vec3 E = normalize( -Position );
	if ( dot(Obj_Normal, E) < 0 ) Obj_Normal = -Obj_Normal;
//	DIRECTIONAL LIGHT
    vec3 Light_Normal = normalize(-dir_light.xyz);
    vec3 H = normalize( Light_Normal + E );

	vec4 directional_ambient = dir_ambient * ambient;
	float d = max( dot(Light_Normal, Obj_Normal), 0.0 );
	vec4 directional_diffuse = d * dir_diffuse * diffuse;
    float s = pow( max(dot(Obj_Normal, H), 0.0), shininess );
	vec4 directional_specular = s * dir_specular * specular;

	if( dot(Light_Normal, Obj_Normal) < 0.0 ) {
		directional_specular = vec4(0.0, 0.0, 0.0, 1.0);
    } 
	
	color = global_ambient + directional_ambient + directional_diffuse + directional_specular;

//	POINT & SPOT LIGHT
	vec3 dist_v = (camera * point_light).xyz - Position;
	Light_Normal = normalize(dist_v);
	H = normalize( Light_Normal + E );
	float dist_m = length(dist_v);
	float attenuation = 1/(2 + 0.01 * dist_m + 0.001 * dist_m * dist_m);

	vec4 pnt_ambient = point_ambient * ambient;
	d = max( dot(Light_Normal, Obj_Normal), 0.0 );
	vec4 pnt_diffuse = d * point_diffuse * diffuse;
	s = pow(max(dot(Obj_Normal, H), 0.0), shininess);
	vec4 pnt_specular = s * point_specular * specular;

	if( dot(Light_Normal, Obj_Normal) < 0.0 ) {
		pnt_specular = vec4(0.0, 0.0, 0.0, 1.0);
	} 

	if (f_spotlight) {
		vec3 spotlight_dir = normalize((camera * point_to).xyz - (camera * point_light).xyz);
		if (dot(-Light_Normal, spotlight_dir) >= cos(spotlight_cutoff * M_PI / 180))
			attenuation *= pow(dot(-Light_Normal, spotlight_dir), spotlight_exp);
		else 
			attenuation = 0;
	}

	color += attenuation * (pnt_ambient + pnt_diffuse + pnt_specular);
	dist = length(Position.xyz);

	if (f_lattice){
		if (f_latticeType == 1){
			latcoord = vec2(0.5 * (vPosition4.x + 1), 0.5 * (vPosition4.y + 1));
		}
		else if (f_latticeType == 2) {
			latcoord = vec2(0.3 * (vPosition4.x + vPosition4.y + vPosition4.z), 0.3 * (vPosition4.x - vPosition4.y + vPosition4.z));
		}
	}
	
	if (f_sphereTexture != 0) {
		float s = 0.0;
		float t = 0.0;
		vec4 pos;

		if (f_relTexture){
			pos = vPosition4;
		} else {
			pos = camera * model_view * vPosition4;
		}

		if (f_sphereTexture == 1){
			if (f_tiltTexture){
				s = 1.5 * (pos.x + pos.y + pos.z);
			}
			else{
				s = 2.5 * pos.x;
			}
		}
		else if (f_sphereTexture == 2) {
			if (f_tiltTexture){
				s = 0.45 * (pos.x + pos.y + pos.z);
				t = 0.45 * (pos.x - pos.y + pos.z);
			}
			else{
				s = 0.75 * (pos.x + 1);
				t = 0.75 * (pos.y + 1);
			}
		}
		// Use x for 1D
		texcoord = vec2(s, t);
	}
	else {
		texcoord = vTexture;
	}
//	texcoord = vTexture;
} 
