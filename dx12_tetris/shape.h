#pragma once

#include <DirectXMath.h>
#include "platform.h"
#include "shapes.h"

class Shape
{
public:
    Shape(DirectX::XMUINT2 position);
    
    void rotateForward();
    void rotateBackward();

    const unsigned char* getCurrentBlock() const { return shape; };
    char getSize() const { return size; };
    DirectX::XMFLOAT4 getColor() const { return floatColor; };
    unsigned char getColorIdx() const { return static_cast<unsigned char>(color); };
    DirectX::XMUINT2 getPosition() const { return position; };

    // don't feel like writing a million getters and setters
    DirectX::XMUINT2 position;

private:
    
    DirectX::XMFLOAT4 floatColor;
    COLORS color;
    SHAPE_TYPE blockType;
    const unsigned char* shape;
    char size;
    char numRotations;
    char curRotation;
};