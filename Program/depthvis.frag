uniform sampler2D DepthMap;
in vec2 fTexCoord;
out vec4 FragColor;
void main()
{
 FragColor = vec4(pow(texture(DepthMap, fTexCoord).xxx, vec3(500.0)), 0.9);
}
