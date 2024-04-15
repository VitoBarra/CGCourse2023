#version 330 core

out vec4 color;

in vec4 vSkyboxTexCoord;

uniform mat4 uV;
uniform mat4 uT;

uniform samplerCube uSkybox;

void main(void) {
    color = texture(uSkybox,normalize(vSkyboxTexCoord.xyz));
}
