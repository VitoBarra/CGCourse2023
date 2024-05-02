#version 330 core
out vec4 color;

in vec3 vNormal;
in vec3 vLightDir;
in vec3 vViewDir;
in vec4 vLightFragPos;

in vec2 vTexCoord;


// Material Selection
uniform bool uUseMaterial;

// light model Selection
uniform bool uUsePhong;

// Material properties
uniform sampler2D uDiffuseTexture;
uniform vec3 uDiffuseColor;
uniform vec3 uAmbientColor;
uniform vec3 uEmissiveColor;
uniform vec3 uSpecularColor;
uniform float uShininess;
uniform float uRefractionIndex;

// ShadowMap Render Mode
uniform int uShadowMode;

// ShadowMap Paramether
uniform sampler2D uShadowMap;
uniform ivec2 uShadowMapSize;
uniform float uBias;
uniform int uKernelSize;

bool ValidTextCoord(vec2 texCoord)
{
    return texCoord.x >= 0.0 && texCoord.x <= 1.0 && texCoord.y >= 0.0 && texCoord.y <= 1.0;
}

float CalculateShadowFactor(vec3 LightDir, vec3 normal)
{
    vec3 projCoords = vLightFragPos.xyz/vLightFragPos.w;// divide by w to get NDC
    projCoords = projCoords * 0.5 + 0.5;// transform to [0,1] range

    if (!ValidTextCoord(projCoords.xy))
    return 1.0;


    float lightDepth = texture(uShadowMap, projCoords.xy).x; // closest depth in shadow map

    float shadowValue = 0.5;



    switch (uShadowMode)
    {
        case 1:// simple shadow
        return lightDepth <projCoords.z ? shadowValue: 1;
        case 2:// bias shadow
        return lightDepth+ uBias <projCoords.z ? shadowValue: 1;
        case 3:// slope bias shadow
        float bias = clamp(uBias*tan(acos(dot(normal, LightDir))), uBias, 0.05);
        return lightDepth+ bias <projCoords.z ? shadowValue: 1;
        case 4:// back face shadow
        return lightDepth + uBias<projCoords.z || dot(normal, LightDir)< 0? shadowValue: 1;
        case 5:// percentage closer filtering
        {
            float shadow = 0.0;
            int kenelRadius= int(floor(uKernelSize/2.0));
            int invalidCoord = 0;

            for (int x = -kenelRadius; x <= kenelRadius; x++)
            for (int y = -kenelRadius; y <= kenelRadius; y++)
            {
                vec2 offset = vec2(x, y)/uShadowMapSize;

                if (!ValidTextCoord(projCoords.xy + offset))
                {
                    invalidCoord++;
                    continue;
                }

                float pcfDepth = texture(uShadowMap, projCoords.xy + offset).x;

                if(pcfDepth + uBias <projCoords.z || dot(normal, LightDir)< 0)
                shadow +=  shadowValue;
                else shadow += 1;
            }

            return shadow/(uKernelSize * uKernelSize -invalidCoord);
        }

        default :// No shadow
        return 1.0;
    }
}

vec3 GetDiffusiveColor()
{
    if (uUseMaterial)
    return uDiffuseColor;
    else
    return texture(uDiffuseTexture,vTexCoord.xy).rgb;
}

// BlinnPhong computed in "any" space
vec3 BlinnPhong(vec3 lightDir, vec3 viewDir, vec3 normal)
{
    // diffusive component
    float cosTheta = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = GetDiffusiveColor()* cosTheta;

    float cosAlpha;
    //Specular component

    if(uUsePhong) // phong
    {
        vec3 reflection = reflect(-lightDir, normal);
        cosAlpha = max(dot(viewDir, reflection), 0.0);
    }
    else  // blinn-phong
    {
        vec3 halfway = normalize(lightDir + viewDir);
        cosAlpha = max(dot(normal, halfway), 0.0);
    }

    vec3 specular =uSpecularColor* pow(cosAlpha, uShininess);


    float shadowFactor = CalculateShadowFactor(lightDir, normal);

    //debug
    if (shadowFactor < 0.0)
    return vec3(1.0, 0.0, 0.0);

    return uEmissiveColor + uAmbientColor + (diffuse + specular) * shadowFactor;
}




void main() {

    color =vec4(BlinnPhong(normalize(vLightDir), normalize(vViewDir), normalize(vNormal)), 1.0);

}
