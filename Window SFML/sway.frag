uniform sampler2D textureCurrent;
uniform sampler2D textureNext;
uniform float time;
uniform float progress;

void main()
{
    vec2 uv = gl_TexCoord[0].xy;

    float swayFactor = pow(uv.y, 2.0);
    float speed = 2.2;
    float strength = 0.008;

    vec2 swayedUv = uv;
    swayedUv.x += sin(time * speed + uv.y * 6.0) * strength * swayFactor;

    vec4 colorCurrent = texture2D(textureCurrent, swayedUv);
    vec4 colorNext = texture2D(textureNext, swayedUv);

    gl_FragColor = mix(colorCurrent, colorNext, progress);
}
