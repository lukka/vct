#version 430 core

out vec4 color;

in VS_OUT {
    vec2 tc;
} fs_in;

const int warpDim = 4;
uniform float warpTexture[warpDim * warpDim];
uniform float occupancyTexture[(warpDim * 2) * (warpDim * 2)];
uniform int warpPartialsX[warpDim * warpDim], warpPartialsY[warpDim * warpDim];
uniform float warpWeights[2 * (warpDim+1)];

uniform vec2 click;
uniform bool toggle;
uniform bool toggleFilter;

layout(location = 0) uniform sampler2D warpmap;

vec2 calculateWarpedPosition(vec2 tc) {
    vec2 linearTexcoord = tc * warpDim;   // convert [0, 1] -> [0, warpDim] for indexing into warp texture
    vec2 warpCellIndex;
    vec2 warpCellPosition = modf(linearTexcoord, warpCellIndex);
    int x = int(warpCellIndex.x), y = int(warpCellIndex.y);
    // if (warpCellOccupied) color.g = 1;

    bool warpCellOccupied = warpTexture[y*warpDim + x] > 0.5;
    vec2 totalOccupied = vec2(warpPartialsX[y*warpDim + warpDim - 1], warpPartialsY[(warpDim - 1)*warpDim + x]);
    vec2 partialSum = vec2(warpPartialsX[y*warpDim + x], warpPartialsY[y*warpDim + x]);
    // color.r = totalOccupied.y / warpDim;

    vec2 warpCellWeightsLow = vec2(warpWeights[0*(warpDim+1)+int(totalOccupied.x)], warpWeights[0*(warpDim+1)+int(totalOccupied.y)]);
    vec2 warpCellWeightsHigh = vec2(warpWeights[1*(warpDim+1)+int(totalOccupied.x)], warpWeights[1*(warpDim+1)+int(totalOccupied.y)]);

    vec2 warpCellResolution = warpCellOccupied ? warpCellWeightsHigh : warpCellWeightsLow;
    // vec2 warpCellResolution;
    // vec2 previousCellWeights
    // if (warpCellOccupied) {
    //     warpCellResolution = smoothstep();
    // }
    // else {
    //     warpCellResolution = smoothstep();
    // }

    // if totalOccupied = 0 or warpDim then it doesn't matter which we choose (edge case for all linear cells)
    if (totalOccupied.x == 0) totalOccupied.x = warpDim;
    if (totalOccupied.y == 0) totalOccupied.y = warpDim;
    vec2 previousPartial = warpCellOccupied ? partialSum - vec2(1) : partialSum;
    vec2 warpCellOffset = warpCellWeightsLow * (warpCellIndex - previousPartial) + warpCellWeightsHigh * previousPartial;
    vec2 warpCellInnerOffset = warpCellPosition * warpCellResolution;
    // color.rb = warpCellOffset / warpDim;

    vec2 warpedTexcoord = (warpCellOffset + warpCellInnerOffset) / warpDim;
    return warpedTexcoord;
}

bool inCircle(vec2 p, vec2 c, float r) {
    return distance(p, c) < r * r;
}

vec4 hqfilter(sampler2D t, vec2 tc) {
    vec2 texSize = textureSize(t, 0);
    vec2 uvScaled = tc * texSize + 0.5;
    vec2 uvInt = floor(uvScaled);
    vec2 uvFrac = fract(uvScaled);
    uvFrac = smoothstep(0.0, 1.0, uvFrac);
    vec2 uv = (uvInt + uvFrac - 0.5) / texSize;
    return texture(t, uv);
}

vec4 sampleWarpmap(vec2 tc) {
    return (toggleFilter ? hqfilter(warpmap, tc) : texture(warpmap, tc));
}

void main() {
    color = vec4(0, 0, 0, 1);
    vec2 tc = fs_in.tc;
    // vec2 warpedPoint = calculateWarpedPosition(click);
    vec2 warpedTexcoord = calculateWarpedPosition(tc);

    if (toggle) {
        // color.r =  warpedTexcoord.r;
        // color.rb = warpedTexcoord;
        // if (inCircle(warpedTexcoord, warpedPoint, 0.1)) color.g = 1;
        // color.rbga = vec4(hqfilter(warpmap, tc).rg, 0, 1);
        // color.rb = tc;
        color.rbga = vec4(sampleWarpmap(tc).rg, 0, 1);
    }
    else {
        // color.r = tc.r;
        color.rb = tc;
        // color.rb = floor(tc * warpDim) / warpDim;
        // if (inCircle(tc, click, 0.1)) color.g = 1;
        // color.rbga = vec4(texture(warpmap, tc).rg, 0, 1);
    }

    // if (toggle) {
    //     ivec2 i = ivec2(tc * warpDim);
    //     color.r = warpTexture[i.y * warpDim + i.x];
    // }
    // else {
    //     ivec2 i = ivec2(tc * warpDim * 2);
    //     color.r = occupancyTexture[i.y * warpDim * 2 + i.x];
    // }
}