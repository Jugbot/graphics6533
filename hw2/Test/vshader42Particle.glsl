#version 150 
in  vec3 vVelocity;
in  vec4 vColor;
out vec4 color;
out float height;

uniform float time;
uniform vec3 initialPos = vec3(0.0, 0.1, 0.0);;
uniform mat4 model_view;
uniform mat4 projection;
void main()
{
	float a = -0.00000049;
	vec4 currPosition = vec4(initialPos.x + 0.001 * vVelocity.x * time,
		initialPos.y + 0.001 * vVelocity.y * time + 0.5 * a * time * time , 
		initialPos.z + 0.001 * vVelocity.z * time, 1.0);
		
	height = currPosition.y;
		
	color = vColor;
    gl_Position = projection * model_view * currPosition;
}