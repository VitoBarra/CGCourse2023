#version 330 core
out vec4 color;

in vec4 vCoordLS;
in vec3 vNormalWS;
in vec3 vVWS;
in vec3 vLWS;

/*Phong*/
in vec3 vNormalVS;
in vec3 vLdirVS;
in vec3 vposVS;

uniform int uRenderMode;

uniform sampler2D  uShadowMap;
uniform ivec2 uShadowMapSize;
uniform float uBias;

uniform int uShadingMode;
uniform vec3 uDiffuseColor;
uniform vec3 uAmbientColor;
uniform vec3 uSpecularColor;
uniform vec3 uEmissiveColor;
uniform float uShininess;
uniform float uRefractionIndex;


/* phong lighting */
vec3 phong_lighting (vec3 LightDir, vec3 pos, vec3 Normal){
    vec3 ViewDir = normalize(-pos);
    float cosRule = max(0.0, dot(LightDir, Normal));

    vec3 reflectDir = -LightDir+2*dot(LightDir, Normal)*Normal;

    float specu = max(0.0, pow(dot(ViewDir, reflectDir), uShininess));

    return uEmissiveColor+ uDiffuseColor*cosRule +  uSpecularColor*specu +uAmbientColor;
}

void main(void)
{
    vec3 N = normalize(vNormalWS);
    vec3 L = normalize(vLWS);
    vec3 V = normalize(vVWS);

    //vec3 shaded = (vec3(max(0.0,dot(L,N))) +vec3(max(0.0,dot(V,N)))*0.0)*uDiffuseColor;
    //vec3 shaded = (vec3(max(0.0, dot(L, N))) +vec3(0.6, 0.6, 0.6))*uDiffuseColor;
    //vec3 shaded = phong_lighting(L, V, N);
    vec3 shaded = phong_lighting(vLdirVS, normalize(vposVS), normalize(vNormalVS));
    float lit = 1.0;

    if (uRenderMode==0)// just diffuse gray
    {
        color = vec4(shaded, 1.0);
    }
    else
    if (uRenderMode==1)// basic shadow
    {
        vec4 pLS = (vCoordLS/vCoordLS.w)*0.5+0.5;
        float depth = texture(uShadowMap, pLS.xy).x;
        if (depth < pLS.z)
        lit = 0.5;
        color = vec4(shaded*lit, 1.0);
    }
    else
    if (uRenderMode==2)// bias
    {
        vec4 pLS = (vCoordLS/vCoordLS.w)*0.5+0.5;
        float depth = texture(uShadowMap, pLS.xy).x;
        if (depth + uBias < pLS.z)
        lit = 0.5;
        color = vec4(shaded*lit, 1.0);
    } else
    if (uRenderMode==3)// slope bias
    {
        float bias = clamp(uBias*tan(acos(dot(N, L))), uBias, 0.05);
        vec4 pLS = (vCoordLS/vCoordLS.w)*0.5+0.5;
        float depth = texture(uShadowMap, pLS.xy).x;
        if (depth + bias < pLS.z)
        lit = 0.5;
        color = vec4(shaded*lit, 1.0);
    } else
    if (uRenderMode==4)// backfaces for watertight objects
    {
        vec4 pLS = (vCoordLS/vCoordLS.w)*0.5+0.5;
        float depth = texture(uShadowMap, pLS.xy).x;
        if (depth  < pLS.z  || dot(N, L)< 0.f)
        lit = 0.5;
        color = vec4(shaded*lit, 1.0);
    } else
    if (uRenderMode==5)// Percentage Closest Filtering
    {
        float storedDepth;
        vec4 pLS = (vCoordLS/vCoordLS.w)*0.5+0.5;
        for (float  x = 0.0; x < 5.0;x+=1.0)
        for (float y = 0.0; y < 5.0;y+=1.0)
        {
            storedDepth =  texture(uShadowMap, pLS.xy+vec2(-2.0+x, -2.0+y)/uShadowMapSize).x;
            if (storedDepth + uBias < pLS.z || dot(N, L)<0.f)
            lit  -= 0.5/25.0;
        }
        color = vec4(shaded*lit, 1.0);
    } else
    if (uRenderMode==6)// VARIANCE SHADOW MAPPING
    {
        vec4 pLS = (vCoordLS/vCoordLS.w)*0.5+0.5;
        vec2 m = texture(uShadowMap, pLS.xy).xy;
        float mu = m.x-uBias;
        float diff = pLS.z - mu;
        if (diff > 0.0){
            lit = m.y / (m.y+diff*diff);
        }
        color = vec4(shaded*lit, 1.0);
    }


}