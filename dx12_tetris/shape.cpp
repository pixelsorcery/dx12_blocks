#include "shape.h"

Shape::Shape(DirectX::XMUINT2 position)
    :
    curRotation(0),
    position(position)
{
    blockType    = static_cast<SHAPE_TYPE>(rand() % NUM_SHAPE_TYPES);
    shape        = SHAPES[BLOCK_INDICES[blockType]];
    size         = BLOCK_SIZES[blockType];
    numRotations = BLOCK_ROTATIONS[blockType];
    color        = static_cast<COLORS>((rand() % (NUM_COLORS-1)) + 1);
    floatColor   = shapeColors[color];

    ASSERT(color > 0 && color < NUM_COLORS);
}

void Shape::rotateForward()
{
    // compute rotation offset
    curRotation++;
    curRotation %= numRotations;

    // set our shape to the next index
    shape = SHAPES[BLOCK_INDICES[blockType] + curRotation];
}

void Shape::rotateBackward()
{
    // compute rotation offset
    if (curRotation == 0)
    {
        curRotation = numRotations-1;
    }
    else
    {
        curRotation--;
    }

    curRotation %= numRotations;

    // set our shape to the next index
    shape = SHAPES[BLOCK_INDICES[blockType] + curRotation];
}
