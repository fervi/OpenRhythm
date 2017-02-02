#version 330

in vec2 position;
in vec2 vertexUV;
in vec4 color;

out vec2 UV;
out vec4 fragColor;

uniform mat4 ortho;
uniform mat4 models[64]; // batch size of 64
uniform uint modelIndices[128]; // Currently we will have 2x triangles vs models

void main(void)
{
	int faceID = int(gl_VertexID/3);
	gl_Position = ortho * models[modelIndices[faceID]] * vec4(position, 0.0, 1.0);
	UV = vertexUV;
	fragColor = color;
}