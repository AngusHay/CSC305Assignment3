layout(location = SCENE_POSITION_ATTRIB_LOCATION)
in vec4 Position;

layout(location = SCENE_TEXCOORD_ATTRIB_LOCATION)
in vec2 TexCoord;

layout(location = SCENE_NORMAL_ATTRIB_LOCATION)
in vec3 Normal;

layout(location = SCENE_DENSITY_ATTRIB_LOCATION)
in float Density;

uniform mat4 ModelWorld;
uniform mat4 ModelViewProjection;
uniform mat3 Normal_ModelWorld;

uniform mat4 lightMatrix;

in vec4 vertex_color;
//out vec4 fragment_color;

out vec4 position_worldspace;

out float vertex_density;

void main()
{
    // TODO: Set to MVP * P
    //gl_Position = vec4(0,0,0,1);
    gl_Position = ModelViewProjection * Position;
    //fragment_color = vertex_color;
    position_worldspace = (ModelWorld * Position);

    vertex_density = Density;

    gl_PointSize = (20 * Density);

    // TODO: Pass vertex attributes to fragment shader
}
