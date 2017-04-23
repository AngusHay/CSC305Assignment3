uniform vec3 CameraPos;
uniform vec3 LightPos;

uniform vec3 Ambient;
uniform vec3 Diffuse;
uniform vec3 Specular;
uniform float Shininess;

uniform int HasDiffuseMap;
uniform sampler2D DiffuseMap;

uniform sampler1D HeightColorTexture;

//Nick
uniform sampler2D waterTex;
uniform sampler2D grassTex;
uniform sampler2D sandTex;
uniform sampler2D rockTex;
uniform sampler2D snowTex;

in vec4 fragment_color;
in vec3 surface_normal;
in vec4 position_worldspace;
in vec4 camera_position;
in float altitude;

in vec4 cloud_color;
in float cloud_block_ratio;

out vec4 FragColor;

void main()
{
    vec3 vector_to_camera = normalize(CameraPos - position_worldspace.xyz);
    vec3 vector_to_light = normalize(LightPos - position_worldspace.xyz);
    vec3 halfway_vector = vector_to_light + vector_to_camera;
    halfway_vector = normalize(halfway_vector);
	
	//Nick
	vec4 tex_ture;
	
	float textureCoordinate = altitude / 10.0f;

	vec3 BaseColor = texture(HeightColorTexture, textureCoordinate).xyz;


    vec3 Color = 0.2 * BaseColor
            + 0.5 * max(dot(vector_to_light, surface_normal), 0) * BaseColor
            + 0.6 * pow(max(0, dot(surface_normal, normalize(halfway_vector))), 4 * 1) * BaseColor;
	
	/*
	//Nick
	vec4 Colorw;

	if(altitude < 2.75)
	{
		Colorw = 0.2 * tex_ture
            + 0.5 * max(dot(vector_to_light, surface_normal), 0) * tex_ture
            + 0.6 * pow(max(0, dot(surface_normal, normalize(halfway_vector))), 4 * 1) * tex_ture;
	}
	*/

    vec3 origin = camera_position.xyz;    // location of the camera
    vec3 target = position_worldspace.xyz;

    FragColor = vec4(Color.x, Color.y, Color.z, 1);

	/*
	//Nick
	//texture is not working
	if(altitude < 2.75)
		FragColor = texture2D(waterTex, UV);
	*/

	FragColor = cloud_color * cloud_block_ratio + FragColor * (1 - cloud_block_ratio);
}
