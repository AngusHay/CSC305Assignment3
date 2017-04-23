uniform mat4 Transform2D;
uniform mat4 OrthoProjection;
out vec2 fTexCoord;
void main()
{
 vec4 pos;
 if (gl_VertexID == 0)
 {
 pos = vec4(0, 0, 0, 1);
 fTexCoord = vec2(0, 0);
 }
 else if (gl_VertexID == 1)
 {
 pos = vec4(1, 0, 0, 1);
 fTexCoord = vec2(1, 0);
 }
 else if (gl_VertexID == 2)
 {
 pos = vec4(1, 1, 0, 1);
 fTexCoord = vec2(1, 1);
 }
 else if (gl_VertexID == 3)
 {
 pos = vec4(0, 1, 0, 1);
 fTexCoord = vec2(0, 1);
 }
 else
 {
 pos = vec4(0, 0, 0, 1);
 fTexCoord = vec2(0, 0);
 }
 gl_Position = OrthoProjection * Transform2D * pos;
}
