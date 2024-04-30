#version 330 core 
layout (location = 0) in vec3 aPosition; 

uniform mat4 uT;
uniform mat4 uLightMatrix;


void main(void) 
{ 
    gl_Position = uLightMatrix*uT*vec4(aPosition, 1.0); 
}
