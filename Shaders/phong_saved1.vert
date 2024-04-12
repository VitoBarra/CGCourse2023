#version 330 core 
layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec3 aNormal;

out vec3 vColor;
out vec3 vNormalVS;
out vec3 vLdirVS;
out vec3 vposVS;

uniform mat4 uP;
uniform mat4 uV;
uniform mat4 uT;
uniform vec4 uLdir;
uniform vec3 uDiffuseColor;
uniform int uShadingMode;

/* phong lighting */
vec3 phong_lighting(vec3 L, vec3 pos, vec3 N){
    float LN = max(0.0, dot(L, N));

    vec3 V = normalize(-pos);
    vec3 R = -L+2*dot(L, N)*N;
    float spec = max(0.0, pow(dot(V, R), 10));
    return LN*uDiffuseColor+spec*uDiffuseColor*vec3(0.2, 0.2, 0.9);
}

void main(void)
{
    /* lazy trick.  if uDiffuseColor.x is negative, it means I don't want to compute the diffuse lighting, just
    pass the per-vertex color attribute to the fragment Shader */
    if (uDiffuseColor.x >= 0){

        /* express normal, light directoin and position in view space */
        vec3 NormalVS= normalize((uV*uT*vec4(aNormal, 0.0)).xyz);
        vec3 LdirVS = (uV*uLdir).xyz;
        vec3 posVS = (uV*uT*vec4(aPosition, 1.0)).xyz;


        if (uShadingMode == 0){
            vColor = uDiffuseColor;
        }
        else
        if (uShadingMode == 1){
            phong_lighting(LdirVS, posVS, NormalVS);
            vColor = phong_lighting(LdirVS, posVS, NormalVS);
        }
        else
        if (uShadingMode >= 2){
            vNormalVS=NormalVS;
            vLdirVS  =LdirVS;
            vposVS   =posVS;
        }
    }
    else
    vColor = aColor;

    gl_Position = uP*uV*uT*vec4(aPosition, 1.0);
}
