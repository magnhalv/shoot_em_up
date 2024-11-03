//
#version 460 core

layout(std140, binding = 0) uniform PerFrameData
{
    mat4 proj;
    mat4 view;
    mat4 model;
    vec4 camera_pos;
};

struct Vertex
{
    float p[3];
    float n[3];
    float tc[2];
};

layout(std430, binding = 1) restrict readonly buffer Vertices
{
    Vertex in_Vertices[];
};

layout(std430, binding = 2) restrict readonly buffer Matrices
{
    mat4 in_ModelMatrices[];
};

// extents of grid in world coordinates
float gridSize = 100.0;

// size of one cell
float gridCellSize = 0.001;

// color of thin lines
vec4 gridColorThin = vec4(0.5, 0.5, 0.5, 1.0);

// color of thick lines (every tenth line)
vec4 gridColorThick = vec4(0.0, 0.0, 0.0, 1.0);

// minimum number of pixels between cell lines before LOD switch should occur.
const float gridMinPixelsBetweenCells = 2.0;

const vec3 pos[4] = vec3[4](
vec3(-1.0, 0.0, -1.0),
vec3( 1.0, 0.0, -1.0),
vec3( 1.0, 0.0,  1.0),
vec3(-1.0, 0.0,  1.0)
);

const int indices[6] = int[6](
0, 1, 2, 2, 3, 0
);


layout (location=0) out vec2 uv;
layout (location=1) out vec2 out_camPos;

void main()
{
    mat4 MVP = proj * view;

    int idx = indices[gl_VertexID];
    vec3 position = pos[idx] * gridSize;

    position.x += camera_pos.x;
    position.z += camera_pos.z;

    out_camPos = camera_pos.xz;

    gl_Position = MVP * vec4(position, 1.0);
    uv = position.xz;
}
