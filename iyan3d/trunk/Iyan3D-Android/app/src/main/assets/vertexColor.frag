//
//  Shader.fsh
//  SGEngine2
//
//  Created by Harishankar on 12/11/14.
//  Copyright (c) 2014 Smackall Games Pvt Ltd. All rights reserved.
//
precision highp float;

uniform sampler2D depthTexture;

uniform  float transparency,shadowDarkness;
uniform float reflection;
uniform  float shadowTextureSize;
uniform float numberOfLights;
uniform vec3 lightColor[5] , lightPos[5];
uniform float fadeEndDistance[5];
varying float shadowDist,lightingValue;
varying vec3 vertexColor;
varying vec4 texCoordsBias,normal,eyeVec,vertexPosCam;

float unpack(const in vec4 rgba_depth) {
    const vec3 bit_shift = vec3(1.0/65536.0, 1.0/256.0, 1.0);
    float depth = dot(rgba_depth.rgb, bit_shift);
    return depth;
}
float GetShadowValue(in vec2 offset) {
    vec2 pixelCoord = vec2(texCoordsBias.x, texCoordsBias.y);
    vec4 texelColor = texture2D(depthTexture, pixelCoord + offset);
    vec4 unpackValue = vec4(vec3(texelColor.xyz),1.0);
    float extractedDistance = unpack(unpackValue);
    return (extractedDistance < shadowDist) ? (shadowDarkness):(0.0);
}
void main()
{
    float maxSpecular = 30.0;
    
    // shadow Calculation ------
    float shadowValue = 0.0;
    if(shadowDist > 0.0) {
        float delta = 1.0/2048.0; // todo
        shadowValue = GetShadowValue(vec2(0.0, 0.0));
        shadowValue += GetShadowValue(vec2(-delta, 0.0));
        shadowValue += GetShadowValue(vec2(delta, 0.0));
        shadowValue += GetShadowValue(vec2(0.0, -delta));
        shadowValue += GetShadowValue(vec2(0.0, delta));
        shadowValue /= 5.0;
    }
    //------
    
    // Lighting calculation-----
    /*
    vec4 diffuse_color = vec4(vec3(vertexColor),1.0),specular = vec4(0.0),colorOfLight = vec4(1.0);
    if(lightingValue != 0.0){
        vec4 normal = normalize(normal);
        vec4 lightDir = normalize(lightDir);
        vec4 eyeVec = normalize(eyeVec);
        float n_dot_l = clamp(dot(normal,lightDir),0.0,1.0);
        vec4 diffuse = vec4(vec3(brightness * n_dot_l),1.0);
        diffuse_color = diffuse * vec4(vec3(vertexColor),1.0);
        
        vec4 reflection = -lightDir + 2.0 * n_dot_l * normal;
        float e_dot_r =  clamp(dot(eyeVec,reflection),0.0,1.0);
        specular = vec4(reflection * pow(e_dot_r,maxSpecular));
        float e_dot_l = dot(lightDir,eyeVec);
        if(e_dot_l < -0.8)
            specular = vec4(0.0);
        
        colorOfLight = vec4(vec3(lightColor),1.0);
    }
     */
    
    vec4 diffuse_color = vec4(vec3(vertexColor),1.0),specular = vec4(0.0),colorOfLight = vec4(1.0);
    if(lightingValue != 0.0){
        colorOfLight = vec4(0.0);
        
        int lightsCount = int(numberOfLights);
        for(int i = 0; i < lightsCount; i++) {
            
            vec4 light_position_cameraspace = vec4(vec3(lightPos[i]),1.0);
            vec4 lightDir = normalize(light_position_cameraspace - vertexPosCam);
            float distanceFromLight = distance(light_position_cameraspace , vertexPosCam);
            
            vec4 normal = normalize(normal);
            vec4 eyeVec = normalize(eyeVec);
            float n_dot_l = clamp(dot(normal,lightDir),0.0,1.0);
            vec4 diffuse = vec4(vec3(brightness * n_dot_l),1.0);
            
            vec4 reflectValue = -lightDir + 2.0 * n_dot_l * normal;
            float e_dot_r =  clamp(dot(eyeVec,reflectValue),0.0,1.0);
            specular += vec4(reflection * pow(e_dot_r,maxSpecular));
            
            float e_dot_l = dot(lightDir,eyeVec);
            if(e_dot_l < -0.8)
                specular = vec4(0.0);
            colorOfLight += vec4(vec3(lightColor[i]),1.0) * (1.0 - clamp((distanceFromLight/fadeEndDistance[i]) , 0.0,1.0)) * diffuse;
        }
    }

    //-------------
    
    vec4 finalColor = vec4(diffuse_color + specular) * colorOfLight;
    finalColor = finalColor + (vec4(0.0,0.0,0.0,0.0) - finalColor) * (shadowValue);
    
    gl_FragColor.xyz = finalColor.xyz;
    gl_FragColor.a = transparency;
}