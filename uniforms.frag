#version 330
in vec2 texCoord;

uniform vec3 iResolution;
uniform float iTime;
uniform mat4 tCameraMatrix;
uniform vec3 eye;
uniform vec3 target;
uniform vec3 up;

uniform sampler2D channel;

layout(location = 0) out vec4 color;

uniform int MAX_MARCHING_STEPS;
uniform float MIN_DIST;
uniform float MAX_DIST;
uniform float EPSILON;
