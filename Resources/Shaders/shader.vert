#version 450
layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texCoord;

out vec2 TexCoord;

void main() 
{ 
    TexCoord = vec2(texCoord.x, 1.0 - texCoord.y);
    gl_Position = vec4(position, 1.0);
}