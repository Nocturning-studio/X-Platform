////////////////////////////////////////////////////////////////////////////////
// Created: 14.06.2026
// Author: NSDeathman
// Nocturning studio for NS Platform X
////////////////////////////////////////////////////////////////////////////////
#include "Stdafx.h"
#include "CPUOcclusion.h"
#include "CPUOcclusionShaders.h"
////////////////////////////////////////////////////////////////////////////////
#define SPHERE_VERTEX_COUNT 92
float3 sphere_vertices_array[SPHERE_VERTEX_COUNT] =
{
    float3(0.0000f, 1.0000f, 0.0000f), float3(0.8944f, 0.4472f, 0.0000f), float3(0.2764f, 0.4472f, 0.8507f),
    float3(-0.7236f, 0.4472f, 0.5257f), float3(-0.7236f, 0.4472f, -0.5257f), float3(0.2764f, 0.4472f, -0.8507f),
    float3(0.7236f, -0.4472f, 0.5257f), float3(-0.2764f, -0.4472f, 0.8507f), float3(-0.8944f, -0.4472f, -0.0000f),
    float3(-0.2764f, -0.4472f, -0.8507f), float3(0.7236f, -0.4472f, -0.5257f), float3(0.0000f, -1.0000f, 0.0000f),
    float3(0.3607f, 0.9327f, 0.0000f), float3(0.6729f, 0.7397f, 0.0000f), float3(0.1115f, 0.9327f, 0.3431f),
    float3(0.2079f, 0.7397f, 0.6399f), float3(-0.2918f, 0.9327f, 0.2120f), float3(-0.5444f, 0.7397f, 0.3955f),
    float3(-0.2918f, 0.9327f, -0.2120f), float3(-0.5444f, 0.7397f, -0.3955f), float3(0.1115f, 0.9327f, -0.3431f),
    float3(0.2079f, 0.7397f, -0.6399f), float3(0.7844f, 0.5168f, 0.3431f), float3(0.5687f, 0.5168f, 0.6399f),
    float3(-0.0839f, 0.5168f, 0.8520f), float3(-0.4329f, 0.5168f, 0.7386f), float3(-0.8362f, 0.5168f, 0.1835f),
    float3(-0.8362f, 0.5168f, -0.1835f), float3(-0.4329f, 0.5168f, -0.7386f), float3(-0.0839f, 0.5168f, -0.8520f),
    float3(0.5687f, 0.5168f, -0.6399f), float3(0.7844f, 0.5168f, -0.3431f), float3(0.9647f, 0.1561f, 0.2120f),
    float3(0.9051f, -0.1561f, 0.3955f), float3(0.0965f, 0.1561f, 0.9830f), float3(-0.0965f, -0.1561f, 0.9830f),
    float3(-0.9051f, 0.1561f, 0.3955f), float3(-0.9647f, -0.1561f, 0.2120f), float3(-0.6558f, 0.1561f, -0.7386f),
    float3(-0.4998f, -0.1561f, -0.8520f), float3(0.4998f, 0.1561f, -0.8520f), float3(0.6558f, -0.1561f, -0.7386f),
    float3(0.9647f, 0.1561f, -0.2120f), float3(0.9051f, -0.1561f, -0.3955f), float3(0.4998f, 0.1561f, 0.8520f),
    float3(0.6558f, -0.1561f, 0.7386f), float3(-0.6558f, 0.1561f, 0.7386f), float3(-0.4998f, -0.1561f, 0.8520f),
    float3(-0.9051f, 0.1561f, -0.3955f), float3(-0.9647f, -0.1561f, -0.2120f), float3(0.0965f, 0.1561f, -0.9830f),
    float3(-0.0965f, -0.1561f, -0.9830f), float3(0.4329f, -0.5168f, 0.7386f), float3(0.0839f, -0.5168f, 0.8520f),
    float3(-0.5687f, -0.5168f, 0.6399f), float3(-0.7844f, -0.5168f, 0.3431f), float3(-0.7844f, -0.5168f, -0.3431f),
    float3(-0.5687f, -0.5168f, -0.6399f), float3(0.0839f, -0.5168f, -0.8520f), float3(0.4329f, -0.5168f, -0.7386f),
    float3(0.8362f, -0.5168f, -0.1835f), float3(0.8362f, -0.5168f, 0.1835f), float3(0.2918f, -0.9327f, 0.2120f),
    float3(0.5444f, -0.7397f, 0.3955f), float3(-0.1115f, -0.9327f, 0.3431f), float3(-0.2079f, -0.7397f, 0.6399f),
    float3(-0.3607f, -0.9327f, -0.0000f), float3(-0.6729f, -0.7397f, -0.0000f), float3(-0.1115f, -0.9327f, -0.3431f),
    float3(-0.2079f, -0.7397f, -0.6399f), float3(0.2918f, -0.9327f, -0.2120f), float3(0.5444f, -0.7397f, -0.3955f),
    float3(0.4795f, 0.8054f, 0.3484f), float3(-0.1832f, 0.8054f, 0.5637f), float3(-0.5927f, 0.8054f, -0.0000f),
    float3(-0.1832f, 0.8054f, -0.5637f), float3(0.4795f, 0.8054f, -0.3484f), float3(0.9855f, -0.1699f, 0.0000f),
    float3(0.3045f, -0.1699f, 0.9372f), float3(-0.7973f, -0.1699f, 0.5792f), float3(-0.7973f, -0.1699f, -0.5792f),
    float3(0.3045f, -0.1699f, -0.9372f), float3(0.7973f, 0.1699f, 0.5792f), float3(-0.3045f, 0.1699f, 0.9372f),
    float3(-0.9855f, 0.1699f, -0.0000f), float3(-0.3045f, 0.1699f, -0.9372f), float3(0.7973f, 0.1699f, -0.5792f),
    float3(0.1832f, -0.8054f, 0.5637f), float3(-0.4795f, -0.8054f, 0.3484f), float3(-0.4795f, -0.8054f, -0.3484f),
    float3(0.1832f, -0.8054f, -0.5637f), float3(0.5927f, -0.8054f, 0.0000f)
};

#define SPHERE_FACES_COUNT 180
u16 sphere_faces_array[SPHERE_FACES_COUNT * 3] =
{
    14, 12, 0,	72, 13, 12, 14, 72, 12, 15, 72, 14, 22, 1,	13, 72, 22, 13, 23, 22, 72, 15, 23, 72, 2,	23, 15, 16, 14,
    0,	73, 15, 14, 16, 73, 14, 17, 73, 16, 24, 2,	15, 73, 24, 15, 25, 24, 73, 17, 25, 73, 3,	25, 17, 18, 16, 0,	74,
    17, 16, 18, 74, 16, 19, 74, 18, 26, 3,	17, 74, 26, 17, 27, 26, 74, 19, 27, 74, 4,	27, 19, 20, 18, 0,	75, 19, 18,
    20, 75, 18, 21, 75, 20, 28, 4,	19, 75, 28, 19, 29, 28, 75, 21, 29, 75, 5,	29, 21, 12, 20, 0,	76, 21, 20, 12, 76,
    20, 13, 76, 12, 30, 5,	21, 76, 30, 21, 31, 30, 76, 13, 31, 76, 1,	31, 13, 32, 42, 1,	77, 43, 42, 32, 77, 42, 33,
    77, 32, 60, 10, 43, 77, 60, 43, 61, 60, 77, 33, 61, 77, 6,	61, 33, 34, 44, 2,	78, 45, 44, 34, 78, 44, 35, 78, 34,
    52, 6,	45, 78, 52, 45, 53, 52, 78, 35, 53, 78, 7,	53, 35, 36, 46, 3,	79, 47, 46, 36, 79, 46, 37, 79, 36, 54, 7,
    47, 79, 54, 47, 55, 54, 79, 37, 55, 79, 8,	55, 37, 38, 48, 4,	80, 49, 48, 38, 80, 48, 39, 80, 38, 56, 8,	49, 80,
    56, 49, 57, 56, 80, 39, 57, 80, 9,	57, 39, 40, 50, 5,	81, 51, 50, 40, 81, 50, 41, 81, 40, 58, 9,	51, 81, 58, 51,
    59, 58, 81, 41, 59, 81, 10, 59, 41, 33, 45, 6,	82, 44, 45, 33, 82, 45, 32, 82, 33, 23, 2,	44, 82, 23, 44, 22, 23,
    82, 32, 22, 82, 1,	22, 32, 35, 47, 7,	83, 46, 47, 35, 83, 47, 34, 83, 35, 25, 3,	46, 83, 25, 46, 24, 25, 83, 34,
    24, 83, 2,	24, 34, 37, 49, 8,	84, 48, 49, 37, 84, 49, 36, 84, 37, 27, 4,	48, 84, 27, 48, 26, 27, 84, 36, 26, 84,
    3,	26, 36, 39, 51, 9,	85, 50, 51, 39, 85, 51, 38, 85, 39, 29, 5,	50, 85, 29, 50, 28, 29, 85, 38, 28, 85, 4,	28,
    38, 41, 43, 10, 86, 42, 43, 41, 86, 43, 40, 86, 41, 31, 1,	42, 86, 31, 42, 30, 31, 86, 40, 30, 86, 5,	30, 40, 62,
    64, 11, 87, 65, 64, 62, 87, 64, 63, 87, 62, 53, 7,	65, 87, 53, 65, 52, 53, 87, 63, 52, 87, 6,	52, 63, 64, 66, 11,
    88, 67, 66, 64, 88, 66, 65, 88, 64, 55, 8,	67, 88, 55, 67, 54, 55, 88, 65, 54, 88, 7,	54, 65, 66, 68, 11, 89, 69,
    68, 66, 89, 68, 67, 89, 66, 57, 9,	69, 89, 57, 69, 56, 57, 89, 67, 56, 89, 8,	56, 67, 68, 70, 11, 90, 71, 70, 68,
    90, 70, 69, 90, 68, 59, 10, 71, 90, 59, 71, 58, 59, 90, 69, 58, 90, 9,	58, 69, 70, 62, 11, 91, 63, 62, 70, 91, 62,
    71, 91, 70, 61, 6,	63, 91, 61, 63, 60, 61, 91, 71, 60, 91, 10, 60, 71,
};

void CPUOcclusion::CreateLightPointGeometry()
{
    std::vector<SoftX::VertexInput> vertices;
    vertices.reserve(SPHERE_VERTEX_COUNT);
    for (int i = 0; i < SPHERE_VERTEX_COUNT; ++i)
    {
        SoftX::VertexInput vi;
        vi.Position = sphere_vertices_array[i];
        vi.Normal = float3(0, 0, 0);
        vi.Color = float4(0, 0, 0, 0);
        vi.UV = float2(0, 0);
        vertices.push_back(vi);
    }

    std::vector<uint> indices;
    indices.reserve(SPHERE_FACES_COUNT * 3);
    for (int i = 0; i < SPHERE_FACES_COUNT * 3; ++i)
        indices.push_back(sphere_faces_array[i]);

    m_lightPointVB = std::make_unique<SoftX::VertexBuffer>(vertices);
    m_lightPointIB = std::make_unique<SoftX::IndexBuffer>(indices);

    Msg("[CPU-OCC] Point geometry created: %u vertices, %u indices", m_lightPointVB->Size(), m_lightPointIB->Size());
}

#define SPHERE_PART_VERTEX_COUNT 82
float3 sphere_part_vertices_array[SPHERE_PART_VERTEX_COUNT] =
{
    float3(-0.288675f, -0.288675f, 0.288675f), float3(0.288675f, -0.288675f, 0.288675f), float3(-0.288675f, 0.288675f, 0.288675f),
    float3(0.288675f, 0.288675f, 0.288675f),   float3(0.000000f, 0.000000f, 0.500000f),   float3(0.000000f, -0.353553f, 0.353553f),
    float3(0.353553f, 0.000000f, 0.353553f),   float3(0.000000f, 0.353553f, 0.353553f),   float3(-0.353553f, 0.000000f, 0.353553f),
    float3(-0.204124f, -0.204124f, 0.408248f), float3(0.204124f, -0.204124f, 0.408248f), float3(0.204124f, 0.204124f, 0.408248f),
    float3(-0.204124f, 0.204124f, 0.408248f),  float3(-0.166667f, -0.333333f, 0.333333f), float3(0.000000f, -0.223607f, 0.447214f),
    float3(-0.223607f, 0.000000f, 0.447214f),  float3(-0.333333f, -0.166667f, 0.333333f), float3(0.333333f, -0.166667f, 0.333333f),
    float3(0.223607f, 0.000000f, 0.447214f),   float3(0.166667f, -0.333333f, 0.333333f), float3(0.166667f, 0.333333f, 0.333333f),
    float3(0.000000f, 0.223607f, 0.447214f),   float3(0.333333f, 0.166667f, 0.333333f),   float3(-0.333333f, 0.166667f, 0.333333f),
    float3(-0.166667f, 0.333333f, 0.333333f),  float3(-0.257248f, -0.257248f, 0.342997f), float3(-0.098058f, -0.294174f, 0.392232f),
    float3(-0.117851f, -0.117851f, 0.471405f), float3(-0.294174f, -0.098058f, 0.392232f), float3(0.257248f, -0.257248f, 0.342997f),
    float3(0.294174f, -0.098058f, 0.392232f),  float3(0.117851f, -0.117851f, 0.471405f),  float3(0.098058f, -0.294174f, 0.392232f),
    float3(0.257248f, 0.257248f, 0.342997f),   float3(0.098058f, 0.294174f, 0.392232f),   float3(0.117851f, 0.117851f, 0.471405f),
    float3(0.294174f, 0.098058f, 0.392232f),   float3(-0.257248f, 0.257248f, 0.342997f),  float3(-0.294174f, 0.098058f, 0.392232f),
    float3(-0.117851f, 0.117851f, 0.471405f),  float3(-0.098058f, 0.294174f, 0.392232f),  float3(-0.234261f, -0.312348f, 0.312348f),
    float3(-0.185695f, -0.278543f, 0.371391f), float3(-0.278543f, -0.185695f, 0.371391f), float3(-0.312348f, -0.234261f, 0.312348f),
    float3(0.000000f, -0.300000f, 0.400000f),  float3(-0.109109f, -0.218218f, 0.436436f), float3(-0.087039f, -0.348155f, 0.348155f),
    float3(-0.121268f, 0.000000f, 0.485071f),  float3(-0.218218f, -0.109109f, 0.436436f), float3(0.000000f, -0.121268f, 0.485071f),
    float3(-0.348155f, -0.087039f, 0.348155f), float3(-0.300000f, 0.000000f, 0.400000f),  float3(0.312348f, -0.234261f, 0.312348f),
    float3(0.278543f, -0.185695f, 0.371391f),  float3(0.185695f, -0.278543f, 0.371391f),  float3(0.234261f, -0.312348f, 0.312348f),
    float3(0.300000f, 0.000000f, 0.400000f),   float3(0.218218f, -0.109109f, 0.436436f),  float3(0.348155f, -0.087039f, 0.348155f),
    float3(0.109109f, -0.218218f, 0.436436f),  float3(0.121268f, 0.000000f, 0.485071f),   float3(0.087039f, -0.348155f, 0.348155f),
    float3(0.234261f, 0.312348f, 0.312348f),   float3(0.185695f, 0.278543f, 0.371391f),   float3(0.278543f, 0.185695f, 0.371391f),
    float3(0.312348f, 0.234261f, 0.312348f),   float3(0.000000f, 0.300000f, 0.400000f),   float3(0.109109f, 0.218218f, 0.436436f),
    float3(0.087039f, 0.348155f, 0.348155f),   float3(0.218218f, 0.109109f, 0.436436f),   float3(0.000000f, 0.121268f, 0.485071f),
    float3(0.348155f, 0.087039f, 0.348155f),   float3(-0.312348f, 0.234261f, 0.312348f),  float3(-0.278543f, 0.185695f, 0.371391f),
    float3(-0.185695f, 0.278543f, 0.371391f),  float3(-0.234261f, 0.312348f, 0.312348f),  float3(-0.218218f, 0.109109f, 0.436436f),
    float3(-0.348155f, 0.087039f, 0.348155f),  float3(-0.109109f, 0.218218f, 0.436436f),  float3(-0.087039f, 0.348155f, 0.348155f),
    float3(0.000000f, 0.000000f, 0.000000f)
};

#define SPHERE_PART_FACES_COUNT 160
u16 sphere_part_faces_array[SPHERE_PART_FACES_COUNT * 3] = {
    0,	41, 25, 25, 44, 0,	13, 42, 25, 25, 41, 13, 9,	43, 25, 25, 42, 9,	16, 44, 25, 25, 43, 16, 5,	45, 26, 26, 47,
    5,	14, 46, 26, 26, 45, 14, 9,	42, 26, 26, 46, 9,	13, 47, 26, 26, 42, 13, 4,	48, 27, 27, 50, 4,	15, 49, 27, 27,
    48, 15, 9,	46, 27, 27, 49, 9,	14, 50, 27, 27, 46, 14, 8,	51, 28, 28, 52, 8,	16, 43, 28, 28, 51, 16, 9,	49, 28,
    28, 43, 9,	15, 52, 28, 28, 49, 15, 1,	53, 29, 29, 56, 1,	17, 54, 29, 29, 53, 17, 10, 55, 29, 29, 54, 10, 19, 56,
    29, 29, 55, 19, 6,	57, 30, 30, 59, 6,	18, 58, 30, 30, 57, 18, 10, 54, 30, 30, 58, 10, 17, 59, 30, 30, 54, 17, 4,
    50, 31, 31, 61, 4,	14, 60, 31, 31, 50, 14, 10, 58, 31, 31, 60, 10, 18, 61, 31, 31, 58, 18, 5,	62, 32, 32, 45, 5,
    19, 55, 32, 32, 62, 19, 10, 60, 32, 32, 55, 10, 14, 45, 32, 32, 60, 14, 3,	63, 33, 33, 66, 3,	20, 64, 33, 33, 63,
    20, 11, 65, 33, 33, 64, 11, 22, 66, 33, 33, 65, 22, 7,	67, 34, 34, 69, 7,	21, 68, 34, 34, 67, 21, 11, 64, 34, 34,
    68, 11, 20, 69, 34, 34, 64, 20, 4,	61, 35, 35, 71, 4,	18, 70, 35, 35, 61, 18, 11, 68, 35, 35, 70, 11, 21, 71, 35,
    35, 68, 21, 6,	72, 36, 36, 57, 6,	22, 65, 36, 36, 72, 22, 11, 70, 36, 36, 65, 11, 18, 57, 36, 36, 70, 18, 2,	73,
    37, 37, 76, 2,	23, 74, 37, 37, 73, 23, 12, 75, 37, 37, 74, 12, 24, 76, 37, 37, 75, 24, 8,	52, 38, 38, 78, 8,	15,
    77, 38, 38, 52, 15, 12, 74, 38, 38, 77, 12, 23, 78, 38, 38, 74, 23, 4,	71, 39, 39, 48, 4,	21, 79, 39, 39, 71, 21,
    12, 77, 39, 39, 79, 12, 15, 48, 39, 39, 77, 15, 7,	80, 40, 40, 67, 7,	24, 75, 40, 40, 80, 24, 12, 79, 40, 40, 75,
    12, 21, 67, 40, 40, 79, 21, 41, 0,	81, 0,	44, 81, 13, 41, 81, 44, 16, 81, 5,	47, 81, 47, 13, 81, 51, 8,	81, 16,
    51, 81, 53, 1,	81, 1,	56, 81, 17, 53, 81, 56, 19, 81, 6,	59, 81, 59, 17, 81, 62, 5,	81, 19, 62, 81, 63, 3,	81,
    3,	66, 81, 20, 63, 81, 66, 22, 81, 7,	69, 81, 69, 20, 81, 72, 6,	81, 22, 72, 81, 73, 2,	81, 2,	76, 81, 23, 73,
    81, 76, 24, 81, 8,	78, 81, 78, 23, 81, 80, 7,	81, 24, 80, 81,
};

void CPUOcclusion::CreateLightOmniPartGeometry()
{
    std::vector<SoftX::VertexInput> vertices;
    vertices.reserve(SPHERE_PART_VERTEX_COUNT);
    for (int i = 0; i < SPHERE_PART_VERTEX_COUNT; ++i)
    {
        SoftX::VertexInput vi;
        vi.Position = sphere_part_vertices_array[i];
        vi.Normal = float3(0, 0, 0);
        vi.Color = float4(0, 0, 0, 0);
        vi.UV = float2(0, 0);
        vertices.push_back(vi);
    }

    std::vector<uint> indices;
    indices.reserve(SPHERE_PART_FACES_COUNT * 3);
    for (int i = 0; i < SPHERE_PART_FACES_COUNT * 3; ++i)
        indices.push_back(sphere_part_faces_array[i]);

    m_lightOmniPartVB = std::make_unique<SoftX::VertexBuffer>(vertices);
    m_lightOmniPartIB = std::make_unique<SoftX::IndexBuffer>(indices);

    Msg("[CPU-OCC] Omni geometry created: %u vertices, %u indices", m_lightOmniPartVB->Size(), m_lightOmniPartIB->Size());
}

#define CONE_VERTEX_COUNT 18
float3 cone_vertices_array[CONE_VERTEX_COUNT] =
{
	float3(0.0000f, 0.0000f, 0.0000f), float3(0.5000f, 0.0000f, 1.0000f), float3(0.4619f,  0.1913f, 1.0000f),  
    float3(0.3536f, 0.3536f, 1.0000f), float3(0.1913f, 0.4619f, 1.0000f), float3(-0.0000f, 0.5000f, 1.0000f),  
    float3(-0.1913f, 0.4619f, 1.0000f), float3(-0.3536f, 0.3536f,  1.0000f), float3(-0.4619f, 0.1913f, 1.0000f),  
    float3(-0.5000f, -0.0000f, 1.0000f), float3(-0.4619f, -0.1913f, 1.0000f), float3(-0.3536f, -0.3536f, 1.0000f),  
    float3(-0.1913f, -0.4619f, 1.0000f), float3(0.0000f, -0.5000f, 1.0000f), float3(0.1913f, -0.4619f, 1.0000f),  
    float3(0.3536f,	-0.3536f, 1.0000f),	float3(0.4619f, -0.1913f, 1.0000f), float3(0.0000f, 0.0000f, 1.0000f + EPS_L)
};

#define CONE_FACES_COUNT 32
u16 cone_faces_array[CONE_FACES_COUNT * 3] = {
	0,	2,	1,	0,	3,	2,	0,	4,	3,	0,	5,	4,	0,	6,	5,	0,	7,	6,	0,	8,	7,	0,	9,	8,
	0,	10, 9,	0,	11, 10, 0,	12, 11, 0,	13, 12, 0,	14, 13, 0,	15, 14, 0,	16, 15, 0,	1,	16,
	17, 1,	2,	17, 2,	3,	17, 3,	4,	17, 4,	5,	17, 5,	6,	17, 6,	7,	17, 7,	8,	17, 8,	9,
	17, 9,	10, 17, 10, 11, 17, 11, 12, 17, 12, 13, 17, 13, 14, 17, 14, 15, 17, 15, 16, 17, 16, 1};

void CPUOcclusion::CreateLightSpotGeometry()
{
    std::vector<SoftX::VertexInput> vertices;
    vertices.reserve(CONE_VERTEX_COUNT);
    for (int i = 0; i < CONE_VERTEX_COUNT; ++i)
    {
        SoftX::VertexInput vi;
        vi.Position = cone_vertices_array[i];
        vi.Normal = float3(0, 0, 0);
        vi.Color = float4(0, 0, 0, 0);
        vi.UV = float2(0, 0);
        vertices.push_back(vi);
    }

    std::vector<uint> indices;
    indices.reserve(CONE_FACES_COUNT * 3);
    for (int i = 0; i < CONE_FACES_COUNT * 3; ++i)
        indices.push_back(cone_faces_array[i]);

    m_lightSpotVB = std::make_unique<SoftX::VertexBuffer>(vertices);
    m_lightSpotIB = std::make_unique<SoftX::IndexBuffer>(indices);

    Msg("[CPU-OCC] Spot geometry created: %u vertices, %u indices", m_lightSpotVB->Size(), m_lightSpotIB->Size());
}

SoftX::OcclusionQuery::queryID CPUOcclusion::AddLightVolume(light* L)
{
    if (!L)
    {
        Msg("[CPU-OCC] AddLightVolume: light is null");
        return 0;
    }
       
    if (!m_activeQuery)
    {
        Msg("[CPU-OCC] AddLightVolume: m_activeQuery is null");
        return 0;
    }

    SoftX::VertexBuffer* vb = nullptr;
    SoftX::IndexBuffer* ib = nullptr;

    switch (L->LightFlags.type)
    {
    case IRender_Light::REFLECTED:
    case IRender_Light::POINT:
        vb = m_lightPointVB.get();
        ib = m_lightPointIB.get();
        break;
    case IRender_Light::SPOT:
        vb = m_lightSpotVB.get();
        ib = m_lightSpotIB.get();
        break;
    case IRender_Light::OMNIPART:
        vb = m_lightOmniPartVB.get();
        ib = m_lightOmniPartIB.get();
        break;
    default:
        Msg("[CPU-OCC] AddLightVolume: L->LightFlags.type is unknown %d", L->LightFlags.type);
        return 0;
    }

    if (!vb || !ib)
    {
        Msg("[CPU-OCC] AddLightVolume: null vb or ib for light type %d", L->LightFlags.type);
        return 0;
    }

    fmat4x4 world = L->get_transform();
    fmat4x4 mvp = Fidentity;
    mvp.mul(m_currentViewProj, world);   // world * viewProj
    SoftX::ConstantBuffer cb(&mvp, sizeof(mvp));

    m_activeQuery->SetVertexBuffer(*vb);
    m_activeQuery->SetIndexBuffer(*ib);
    m_activeQuery->SetConstantBuffer(cb);
    m_activeQuery->SetVertexShader(LightVolumeQueryVS);
    m_activeQuery->SetDepthFunc(SoftX::ComparisonFunc::Less);
    m_activeQuery->SetCullMode(SoftX::CullMode::None);

    return m_activeQuery->DrawIndexed();
}

void CPUOcclusion::ResetOcclusionVolumes()
{
    m_lightPointVB.reset();
    m_lightPointIB.reset();
    m_lightSpotVB.reset();
    m_lightSpotIB.reset();
    m_lightOmniPartVB.reset();
    m_lightOmniPartIB.reset();
}
////////////////////////////////////////////////////////////////////////////////
