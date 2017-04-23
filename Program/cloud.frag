uniform vec3 CameraPos;
uniform vec3 LightPos;

uniform vec3 CloudColor;

in vec4 fragment_color;
in vec4 position_worldspace;
in float vertex_density;

out vec4 FragColor;

void main()
{
    vec3 vector_to_camera = normalize(CameraPos - position_worldspace.xyz);
    vec3 vector_to_light = normalize(LightPos - position_worldspace.xyz);
    //vec3 halfway_vector = normalize(position_worldspace.xyz) - normalize(CameraPos);
    //halfway_vector = normalize(halfway_vector);
    vec3 halfway_vector = vector_to_light + vector_to_camera;
    halfway_vector = normalize(halfway_vector);

    /*vec3 Color = 0.1 * Ambient
            + 0.5 * max(dot(vector_to_light, surface_normal), 0) * Diffuse
            + 0.5 * pow(max(0, dot(surface_normal, normalize(halfway_vector))), 4 * Shininess) * Specular;*/

    //vec3 CloudColor = vec3(1, 1, 1);

    vec3 Color = 1.0 * CloudColor;

    FragColor = vec4(Color.x, Color.y, Color.z, 0.5);
}
