#pragma once

// TODO either generate a font h file in python based on an actual (not hacked)
//      bmfont file or write a better class

#include "platform.h"
#include <fstream>
#include <string>

// TODO do all of these have to be uints?
struct GlyphData
{
    uint x;        // The left position of the character image in the texture.
    uint y;        // The top position of the character image in the texture.
    uint width;    // The width of the character image in the texture.
    uint height;   // The height of the character image in the texture.
    uint xoffset;  // How much the current position should be offset when copying the image from the texture to the screen.
    uint yoffset;  // How much the current position should be offset when copying the image from the texture to the screen.
    uint xadvance; // How much the current position should be advanced after drawing the character.
    uint page;     // The texture page where the character image is found.
    uint chnl;     // The texture channel where the character image is found(1 = blue, 2 = green, 4 = red, 8 = alpha, 15 = all channels).
};

struct Font
{
    uint numGlyphs;
    uint lineHeight;
    uint lineBase;
    uint imageHeight;
    uint imageWidth;
    std::string imgFileName;
    GlyphData glyphs[255]; // TODO: not waste space 
};

static void ReadFont(Font& fontInfo, char* filePath)
{
    std::string token;
    std::ifstream fontFile;
    fontFile.open(filePath);

    memset(&fontInfo, 0, sizeof(Font));

    fontFile >> token;
    fontFile >> fontInfo.lineHeight;
    fontFile >> token;
    fontFile >> fontInfo.lineBase;
    fontFile >> token;
    fontFile >> fontInfo.imageWidth;
    fontFile >> token;
    fontFile >> fontInfo.imageHeight;
    fontFile >> token;
    fontFile >> fontInfo.imgFileName;
    fontFile >> token;
    fontFile >> fontInfo.numGlyphs;

    for (uint i = 0; i < fontInfo.numGlyphs; i++)
    {
        int id;
        fontFile >> token;
        fontFile >> id;

        fontFile >> token;
        fontFile >> fontInfo.glyphs[id].x;
        fontFile >> token;
        fontFile >> fontInfo.glyphs[id].y;
        fontFile >> token;
        fontFile >> fontInfo.glyphs[id].width;
        fontFile >> token;
        fontFile >> fontInfo.glyphs[id].height;
        fontFile >> token;
        fontFile >> fontInfo.glyphs[id].xoffset;
        fontFile >> token;
        fontFile >> fontInfo.glyphs[id].yoffset;
        fontFile >> token;
        fontFile >> fontInfo.glyphs[id].xadvance;
        fontFile >> token;
        fontFile >> fontInfo.glyphs[id].page;
        fontFile >> token;
        fontFile >> fontInfo.glyphs[id].chnl;
    }

    fontFile.close();
}