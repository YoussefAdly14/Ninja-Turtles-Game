#version 330 core
in vec3 vColor;
out vec4 FragColor;

uniform vec4  u_Color;       // multiplies the vertex colour (Lab 2 u_Tint); alpha used for blending
uniform float u_DamageTint;  // 0 = normal, 1 = fully red   (Lab 3 u_Highlight + mix)
uniform float u_Grayscale;   // 0 = colour, 1 = grayscale    (learnopengl Framebuffers grayscale weights)

void main()
{
    vec4 c = vec4(vColor, 1.0) * u_Color;

    vec3 rgb = mix(c.rgb, vec3(0.9, 0.05, 0.05), u_DamageTint);

    float gray = 0.2126 * rgb.r + 0.7152 * rgb.g + 0.0722 * rgb.b;
    rgb = mix(rgb, vec3(gray), u_Grayscale);

    FragColor = vec4(rgb, c.a);
}
