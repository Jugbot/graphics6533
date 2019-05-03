/* 
File Name: "vshader53.glsl":
Vertex shader:
  - Per vertex shading for a single point light source;
    distance attenuation is Yet To Be Completed.
  - Entire shading computation is done in the Eye Frame.
*/

// #version 150  // YJC: Comment/un-comment this line to resolve compilation errors
                 //      due to different settings of the default GLSL version

in  vec4 vPosition;
in  vec3 vNormal;
out vec4 color;

uniform vec4 AmbientProduct, DiffuseProduct, SpecularProduct;
uniform mat4 ModelView;
uniform mat4 Projection;
uniform mat3 Normal_Matrix;
uniform vec4 LightPosition;   // Must be in Eye Frame
uniform float Shininess;

uniform float ConstAtt;  // Constant Attenuation
uniform float LinearAtt; // Linear Attenuation
uniform float QuadAtt;   // Quadratic Attenuation

void main()
{
    // Transform vertex  position into eye coordinates
    vec3 pos = (ModelView * vPosition).xyz;
	
    vec3 Light_Normal = normalize( LightPosition.xyz - pos );
    vec3 E = normalize( -pos );
    vec3 H = normalize( Light_Normal + E );

    // Transform vertex normal into eye coordinates
      // vec3 Obj_Normal = normalize( ModelView*vec4(vNormal, 0.0) ).xyz;
    vec3 Obj_Normal = normalize(Normal_Matrix * vNormal);

	// YJC Note: Obj_Normal must use the one pointing *toward* the viewer
	//     ==> If (Obj_Normal dot E) < 0 then Obj_Normal must be changed to -Obj_Normal
	//
	if ( dot(Obj_Normal, E) < 0 ) Obj_Normal = -Obj_Normal;


	/*--- To Do: Compute attenuation ---*/
	float attenuation = 1.0; 

	 // Compute terms in the illumination equation
    vec4 ambient = AmbientProduct;

    float d = max( dot(Light_Normal, Obj_Normal), 0.0 );
    vec4  diffuse = d * DiffuseProduct;

    float s = pow( max(dot(Obj_Normal, H), 0.0), Shininess );
    vec4  specular = s * SpecularProduct;
    
    if( dot(Light_Normal, Obj_Normal) < 0.0 ) {
	specular = vec4(0.0, 0.0, 0.0, 1.0);
    } 

    gl_Position = Projection * ModelView * vPosition;

/*--- attenuation below must be computed properly ---*/
    color = attenuation * (ambient + diffuse + specular);
}
