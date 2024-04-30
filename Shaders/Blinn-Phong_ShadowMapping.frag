#version 330


in vNormalVS


// BlinnPhong computed in "any" space
vec3 BlinnPhong(vec3 lightDir, vec3 viewDir, vec3 normal)
{
    // diffusive component
    float cosTheta = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = uDiffuse * cosTheta;

    //Specular component
    vec3 halfway = normalize(lightDir + viewDir);
    float cosAlpha = max(dot(normal, halfway), 0.0);
    vec3 specular = uSpecular * pow(cosAlpha, uShininess);


    return uEmmissive + uAmbient+ diffuse + specular;
}

void main() {

}
