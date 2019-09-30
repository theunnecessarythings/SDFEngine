/**
 * Return the normalized direction to march in from the eye point for a single pixel.
 * 
 * fieldOfView: vertical field of view in degrees
 * size: resolution of the output image
 * fragCoord: the x,y coordinate of the pixel in the output image
 */
vec3 rayDirection(float fieldOfView, vec2 size, vec2 fragCoord) {
    vec2 xy = fragCoord - size / 2.0;
    float z = size.y / tan(radians(fieldOfView) / 2.0);
    return normalize(vec3(xy, -z));
}


/**
 * Return a transform matrix that will transform a ray from view space
 * to world coordinates, given the eye point, the camera target, and an up vector.
 *
 * This assumes that the center of the camera is aligned with the negative z axis in
 * view space when calculating the ray marching direction. See rayDirection.
 */
mat3 viewMatrix(vec3 eye, vec3 center, vec3 up) {
    // Based on gluLookAt man page
    vec3 f = normalize(center - eye);
    vec3 s = normalize(cross(f, up));
    vec3 u = cross(s, f);
    return mat3(s, u, -f);
}


out vec4 FragColor;
void main( void )
{
    vec4 outColor = vec4(0.0,0.0,0.0,1.0);
    vec3 viewDir = rayDirection(45.0, iResolution.xy, gl_FragCoord.xy);
    mat3 viewToWorld = viewMatrix(eye, target, up);
    vec3 worldDir = viewToWorld * viewDir;

    mainImage( outColor, gl_FragCoord.xy, eye, worldDir);
    FragColor = outColor;
    color = outColor;
}
