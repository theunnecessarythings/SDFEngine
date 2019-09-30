#version 330
precision mediump float;
layout (location = 0) in vec3 position;
out vec2 texCoord;
void main(){
    gl_Position = vec4(position.xy, 0, 1);
    texCoord = (position.xy+vec2(1,1))/2.0;
}