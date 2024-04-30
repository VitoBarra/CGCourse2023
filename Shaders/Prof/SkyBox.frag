#version 330 core

out vec4 color;
in vec4 vSkyboxTexCoord;


uniform samplerCube uSkybox;

void main(void) {
    color = texture(uSkybox,normalize(vSkyboxTexCoord.xyz));
}
