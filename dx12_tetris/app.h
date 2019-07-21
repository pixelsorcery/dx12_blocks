#pragma once

#include "Dx12App.h"
#include "shape.h"
#include <memory>
#include "Font.h"

struct fontUavData
{
    float u;
    float v;
};

class App : public Dx12App
{
public:
    App();

    char *getTitle() const { return "DX12 TETRIS"; }

    bool init();
    void exit();

    void updateGame();

    void drawFrame();

    void onKey(const uint key, const bool pressed);
private:
    bool createBlockPipeline();
    bool printText(uint x, uint y, char * string);
    bool createFontPipeline();
    bool createBlockVB();

	bool createFontUav();

    // shaders
    CComPtr<ID3DBlob> blockVs, blockPs;
    CComPtr<ID3DBlob> fontVs, fontPs;

    // pipeline stuff
    CComPtr<ID3D12PipelineState> blockPipeline;
    CComPtr<ID3D12PipelineState> fontPipeline;
    CComPtr<ID3D12RootSignature> blockRootSig;
    CComPtr<ID3D12RootSignature> fontRootSig;

    // vertex buffer for drawing a square
    CComPtr<ID3D12Resource>  blockVertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW blockVertexBufferView;

    // uav for text
    CComPtr<ID3D12Resource>  fontUav;
    D3D12_VERTEX_BUFFER_VIEW fontUavView;
    fontUavData* fontUavPtr;

    // index buffer
    CComPtr<ID3D12Resource>    blockIndexBuffer;
    D3D12_INDEX_BUFFER_VIEW blockIndexBufferView;

    // font texture
    CComPtr<ID3D12Resource> fontTexture;

    Font font;

    void retireBlock();
    bool checkCollision();
    void drawShape(const Shape& shape);
    void drawBoard();
    void drawGameBoardBackground();
    void drawBlock(const uint xpos, const uint ypos, const DirectX::XMFLOAT4& color);
    void drawChar(const uint xpos, const uint ypos, const DirectX::XMFLOAT4& color);
    void checkLines();

    // standard 10x20 board
    uint m_gameBoard[200];

    uint m_boardWidth;
    uint m_boardHeight;

    uint m_boardWidthPixels;
    uint m_boardHeightPixels;

    const uint m_startingBlockXPosition;
    const uint m_startingBlockYPosition;

    DirectX::XMUINT2 m_activeBlockPosition;
    DirectX::XMUINT2 m_nextBlockPosition;
    std::unique_ptr<Shape> m_pActiveShape;
    std::unique_ptr<Shape> m_pNextShape;

    double m_timeSinceLastDrop;
    double m_dropLimit;
};