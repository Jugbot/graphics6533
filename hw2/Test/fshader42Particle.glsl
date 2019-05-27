#version 150  
in vec4 color;
in float height;
out vec4 fColor;

void main() 
{ 
	if (height < 0.1) discard;
	fColor = color;
} 

