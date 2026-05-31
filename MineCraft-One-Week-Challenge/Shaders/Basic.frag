#version 330

out vec4 outColour;
in  vec2 passTextureCoord;

uniform sampler2D texSampler;
uniform vec3 tintColor = vec3(1.0, 1.0, 1.0);

void main()
{
    vec4 color = texture(texSampler, passTextureCoord);

    outColour = vec4(color.rgb * tintColor, color.a);
}
