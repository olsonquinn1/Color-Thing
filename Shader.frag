#version 120

uniform sampler2D texture;
uniform vec2 imageSize;
uniform float disperseFactor;
uniform float dimRate;
uniform bool checkOutside;

varying out vec4 pixel;

void main() {
    float xStep = 1 / imageSize.x;
    float yStep = 1 / imageSize.y;
    vec3 avg;
    avg.xyz = vec3(0.0, 0.0, 0.0);
    vec2 myPos = gl_TexCoord[0].xy;
    
    int count = 0;

    for(int dx = -1; dx <= 1; dx++) {
        float x = myPos.x + xStep * dx;

        if(x < 0 || x > 1 && !checkOutside) continue;

        for(int dy = -1; dy <= 1; dy++) {
            float y = myPos.y + yStep * dy;

            if(y < 0 || y > 1 && !checkOutside) continue;
            if(dx == 0 && dy == 0) continue;

            vec4 c = texture2D(texture, vec2(x, y));
            count++;
            avg.x += c.x;
            avg.y += c.y;
            avg.z += c.z;
        }
    }
    avg.x /= count;
    avg.y /= count;
    avg.z /= count;

    vec4 original = texture2D(texture, gl_TexCoord[0].xy);

    pixel.x = original.x + (avg.x - original.x) * disperseFactor - dimRate;
    pixel.y = original.y + (avg.y - original.y) * disperseFactor - dimRate;
    pixel.z = original.z + (avg.z - original.z) * disperseFactor - dimRate;
    pixel.w = original.w;
}