#version 330 core
layout (location = 0) in vec3 aPosition;
layout (location = 2) in vec3 aNormal;

out vec4 vSkyboxTexCoord;

uniform mat4 uP;
uniform mat4 uV;
uniform mat4 uT;

void main(void) {
    gl_Position = uP*uV*uT*vec4(aPosition, 1.0);
    vSkyboxTexCoord =  inverse(uV)*(uV*uT*vec4(aPosition, 1.0)-vec4(0,0,0,1.0));
}
