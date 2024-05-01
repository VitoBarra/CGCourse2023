#version 330

layout (location = 0) in vec3 aPosition;
layout (location = 2) in vec3 aNormal;
layout (location = 3) in vec2 aTexCoord;



out vec3 vNormal;
out vec3 vLightDir;
out vec3 vViewDir;
out vec4 vLightFragPos;
out vec2 vTexCoord;



uniform mat4 uT;
uniform mat4 uV;
uniform mat4 uP;


uniform vec4 uLdir;
uniform mat4 uLightMatrix;


void main() {

    mat4 ViewModel = uV * uT;
    vec4 fragPos = ViewModel *vec4(aPosition, 1.0);


    vNormal = (ViewModel* vec4(aNormal, 0.0)).xyz; // Normal ViewSpace
    vLightDir = (uV*vec4(uLdir)).xyz; // LightDir ViewSpace
    vViewDir = -fragPos.xyz; // ViewDir ViewSpace (camera position is (0,0,0) in ViewSpace)

    gl_Position = uP * fragPos;
    vLightFragPos = uLightMatrix * uT*vec4(aPosition, 1.0);

    vTexCoord = aTexCoord;
}
