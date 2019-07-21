#pragma once

#include <DirectXMath.h>

enum SHAPE_TYPE
{
    IBEAM           = 0,
    BLOCK           = 1,
    JBLOCK          = 2,
    LBLOCK          = 3,
    SBLOCK          = 4,
    TBLOCK          = 5,
    ZBLOCK          = 6,
    NUM_SHAPE_TYPES = 7
};

enum COLORS
{
    NO_COLOR   = 0,
    GREEN      = 1,
    YELLOW     = 2,
    ORANGE     = 3,
    RED        = 4,
    VOILET     = 5,
    BLUE       = 6,
    NUM_COLORS = 7
};

static DirectX::XMFLOAT4 shapeColors[NUM_COLORS] =
{
    {0.0f,          0.0f,          0.0f,          0.0f},
    {132.0f/256.0f, 184.0f/256.0f, 24.0f/256.0f,  1.0f},
    {251.0f/256.0f, 198.0f/256.0f, 20.0f/256.0f,  1.0f},
    {237.0f/256.0f, 130.0f/256.0f, 14.0f/256.0f,  1.0f},
    {220.0f/256.0f, 0,             48.0f/256.0f,  1.0f},
    {160.0f/256.0f, 31.0f/256.0f,  131.0f/256.0f, 1.0f},
    {0,             152.0f/256.0f, 212.0f/256.0f, 1.0f}
};

static constexpr unsigned char BLOCK_ROTATIONS[NUM_SHAPE_TYPES] = { 2, 1, 4, 4,  2,  4,  2 };
static constexpr unsigned char BLOCK_SIZES[NUM_SHAPE_TYPES]     = { 4, 4, 3, 3,  3,  3,  3 };
static constexpr unsigned char BLOCK_INDICES[NUM_SHAPE_TYPES]   = { 0, 2, 3, 7, 11, 13, 17 };

static constexpr unsigned char SHAPES[19][16] = 
{
    // IBEAN 0
    { 0, 0, 0, 0,
      0, 0, 0, 0,
      1, 1, 1, 1,
      0, 0, 0, 0 },

    { 0, 0, 1, 0,
      0, 0, 1, 0,
      0, 0, 1, 0,
      0, 0, 1, 0 },

    // BLOCK 2
    { 0, 0, 0, 0,
      0, 1, 1, 0,
      0, 1, 1, 0,
      0, 0, 0, 0 },

    // JBLOCK 3
    { 0, 0, 0, 0,
      1, 1, 1, 0,
      0, 0, 1, 0,
      0, 0, 0, 0 },

    { 0, 1, 0, 0,
      0, 1, 0, 0,
      1, 1, 0, 0,
      0, 0, 0, 0 },

    { 1, 0, 0, 0,
      1, 1, 1, 0,
      0, 0, 0, 0,
      0, 0, 0, 0 },

    { 0, 1, 1, 0,
      0, 1, 0, 0,
      0, 1, 0, 0,
      0, 0, 0, 0 },

    // LBLOCK 7
    { 0, 0, 0, 0,
      1, 1, 1, 0,
      1, 0, 0, 0,
      0, 0, 0, 0 },

    { 1, 1, 0, 0,
      0, 1, 0, 0,
      0, 1, 0, 0,
      0, 0, 0, 0 },

    { 0, 0, 1, 0,
      1, 1, 1, 0,
      0, 0, 0, 0,
      0, 0, 0, 0 },

    { 0, 1, 0, 0,
      0, 1, 0, 0,
      0, 1, 1, 0,
      0, 0, 0, 0 },

    // SBLOCK 11
    { 0, 0, 0, 0,
      0, 1, 1, 0,
      1, 1, 0, 0,
      0, 0, 0, 0 },

    { 0, 1, 0, 0,
      0, 1, 1, 0,
      0, 0, 1, 0,
      0, 0, 0, 0 },

    // TBLOCK 13
    { 0, 0, 0, 0,
      1, 1, 1, 0,
      0, 1, 0, 0,
      0, 0, 0, 0 },

    { 0, 1, 0, 0,
      1, 1, 0, 0,
      0, 1, 0, 0,
      0, 0, 0, 0 },

    { 0, 1, 0, 0,
      1, 1, 1, 0,
      0, 0, 0, 0,
      0, 0, 0, 0 },

    { 0, 1, 0, 0,
      0, 1, 1, 0,
      0, 1, 0, 0,
      0, 0, 0, 0 },

    // ZBLOCK 17
    { 0, 0, 0, 0,
      1, 1, 0, 0,
      0, 1, 1, 0,
      0, 0, 0, 0 },

    { 0, 0, 1, 0,
      0, 1, 1, 0,
      0, 1, 0, 0,
      0, 0, 0, 0 }
};
