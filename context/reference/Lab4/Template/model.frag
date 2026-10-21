#version 430 core
in vec2 TexCoords;
out vec4 FragColor;

//The texture for the model since models need to be textured
uniform sampler2D texture_diffuse1;

void main()
{
    FragColor = texture(texture_diffuse1, TexCoords);
}
