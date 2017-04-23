// https://learnopengl.com/#!Advanced-OpenGL/Cubemaps
layout (location = 0) in vec4 position;
out vec3 TexCoords;

uniform mat4 projection;
uniform mat4 view;

void main()
{
    gl_Position = projection * view * position;
    gl_Position.z = gl_Position.w;
    TexCoords = vec3(position.xyz);
}
