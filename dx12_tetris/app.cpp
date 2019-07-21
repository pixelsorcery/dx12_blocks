#include "app.h"
#include "Shaders.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_TGA
#include "stb_image.h"

BaseApp *app = new App();

using namespace DirectX;

App::App()
    :
    m_timeSinceLastDrop(0),
    m_boardWidth(10),
    m_boardHeight(20),
    m_startingBlockXPosition(3),
    m_startingBlockYPosition(0),
    m_nextBlockPosition({ 11, 12 }),
    m_pActiveShape(nullptr),
    m_dropLimit(.2)
{
    m_boardHeightPixels = m_windowHeight;
    m_boardWidthPixels = static_cast<uint>(m_windowWidth * (2.0f / 3.0f));
}

bool App::createFontPipeline()
{
    HRESULT hr = S_OK;

    fontVs = compileShaderFromFile("fontVs.hlsl", "vs_5_1", "main");
    fontPs = compileShaderFromFile("fontPs.hlsl", "ps_5_1", "main");

    //D3D12_DESCRIPTOR_RANGE uavRange[1];
    //uavRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    //uavRange[0].NumDescriptors = 1;
    //uavRange[0].OffsetInDescriptorsFromTableStart = 0;
    //uavRange[0].BaseShaderRegister = 0;
    //uavRange[0].RegisterSpace = 0;

    //D3D12_DESCRIPTOR_RANGE srvRange[1];
    //srvRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    //srvRange[0].NumDescriptors = 1;
    //srvRange[0].OffsetInDescriptorsFromTableStart = 0;
    //srvRange[0].BaseShaderRegister = 0;
    //srvRange[0].RegisterSpace = 0;

    D3D12_DESCRIPTOR_RANGE srvRange[2];
    srvRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    srvRange[0].NumDescriptors = 1;
    srvRange[0].OffsetInDescriptorsFromTableStart = 0;
    srvRange[0].BaseShaderRegister = 0;
    srvRange[0].RegisterSpace = 0;

    srvRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    srvRange[1].NumDescriptors = 1;
    srvRange[1].OffsetInDescriptorsFromTableStart = 0;
    srvRange[1].BaseShaderRegister = 0;
    srvRange[1].RegisterSpace = 0;

    D3D12_ROOT_PARAMETER param[4];
    param[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    param[0].DescriptorTable.NumDescriptorRanges = 1;
    param[0].DescriptorTable.pDescriptorRanges = srvRange;
    param[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

    param[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    param[1].DescriptorTable.NumDescriptorRanges = 1;
    param[1].DescriptorTable.pDescriptorRanges = &srvRange[1];
    param[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    param[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    param[2].Constants.Num32BitValues = 3;
    param[2].Constants.RegisterSpace = 0;
    param[2].Constants.ShaderRegister = 0;
    param[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    param[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    param[3].Constants.Num32BitValues = 16;
    param[3].Constants.RegisterSpace = 0;
    param[3].Constants.ShaderRegister = 0;
    param[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

    D3D12_STATIC_SAMPLER_DESC sampDesc = {};
    sampDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
    sampDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampDesc.MaxAnisotropy = 1;
    sampDesc.MaxLOD = D3D12_FLOAT32_MAX;
    sampDesc.ShaderRegister = 0;
    sampDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    {
        D3D12_ROOT_SIGNATURE_DESC desc;
        desc.NumParameters = 4;
        desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
        desc.NumStaticSamplers = 1;
        desc.pStaticSamplers = &sampDesc;
        desc.pParameters = param;

        // serialize it
        CComPtr<ID3DBlob> serializedRootSig;
        hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &serializedRootSig, NULL);
        assert(SUCCEEDED(hr));

        // create it
        ID3D12RootSignature *pRootSig;
        hr = device->CreateRootSignature(0, serializedRootSig->GetBufferPointer(), serializedRootSig->GetBufferSize(), IID_PPV_ARGS(&pRootSig));
        assert(SUCCEEDED(hr));

        fontRootSig.Attach(pRootSig);
    }

    // pipeline state object
    D3D12_DEPTH_STENCIL_DESC depthStencilDesc = { 0 };
    depthStencilDesc.DepthEnable = true;
    depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    depthStencilDesc.StencilEnable = FALSE;

    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
    setDefaultPipelineState(&psoDesc);
    psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
    psoDesc.pRootSignature = fontRootSig;
    psoDesc.VS = bytecodeFromBlob(fontVs);
    psoDesc.PS = bytecodeFromBlob(fontPs);
    psoDesc.DepthStencilState = depthStencilDesc;
    psoDesc.DSVFormat = getDepthBufferFmt();

    ID3D12PipelineState *pState;
    hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pState));

    if (S_OK != hr)
    {
        ErrorMsg("font pipeline creation failed");
        return false;
    }

    fontPipeline.Attach(pState);

    return true;
}

bool App::createBlockPipeline()
{
    HRESULT hr = S_OK;

    blockVs = compileShaderFromFile("blockVs.hlsl", "vs_5_1", "main");
    blockPs = compileShaderFromFile("blockPs.hlsl", "ps_5_1", "main");

    // two root constants 
    D3D12_ROOT_PARAMETER param[2];
    param[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    param[0].Constants.Num32BitValues = 16;
    param[0].Constants.RegisterSpace = 0;
    param[0].Constants.ShaderRegister = 0;
    param[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

    param[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    param[1].Constants.Num32BitValues = 3;
    param[1].Constants.RegisterSpace = 0;
    param[1].Constants.ShaderRegister = 0;
    param[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // empty root signature cause we don't have any SRVs
    D3D12_ROOT_SIGNATURE_DESC rootDesc;
    rootDesc.NumParameters = 2;
    rootDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    rootDesc.NumStaticSamplers = 0;
    rootDesc.pStaticSamplers = 0;
    rootDesc.pParameters = param;

    // serialize it
    CComPtr<ID3DBlob> serializedRS;
    hr = D3D12SerializeRootSignature(&rootDesc, D3D_ROOT_SIGNATURE_VERSION_1, &serializedRS, NULL);
    if (S_OK != hr)
    {
        ErrorMsg("block root sig serialization failed");
        return false;
    }

    // create rs
    ID3D12RootSignature *pRootSig;
    hr = device->CreateRootSignature(0, serializedRS->GetBufferPointer(), serializedRS->GetBufferSize(), IID_PPV_ARGS(&pRootSig));
    if (S_OK != hr)
    {
        ErrorMsg("block root sig creation failed");
        return false;
    }

    blockRootSig.Attach(pRootSig);

    // pipeline state object
    D3D12_DEPTH_STENCIL_DESC depthStencilDesc = { 0 };
    depthStencilDesc.DepthEnable = true;
    depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    depthStencilDesc.StencilEnable = FALSE;

    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
    setDefaultPipelineState(&psoDesc);
    psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
    psoDesc.pRootSignature = blockRootSig;
    psoDesc.VS = bytecodeFromBlob(blockVs);
    psoDesc.PS = bytecodeFromBlob(blockPs);
    psoDesc.DepthStencilState = depthStencilDesc;
    psoDesc.DSVFormat = getDepthBufferFmt();

    ID3D12PipelineState *state;
    hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&state));

    if (S_OK != hr)
    {
        ErrorMsg("block pipeline creation failed");
        return false;
    }

    blockPipeline.Attach(state);

    return true;
}

//void App::printText(uint x, uint y, char* string)
//{
//
//}

bool App::createBlockVB()
{
    HRESULT hr = S_OK;
    // Define the geometry for a triangle.
    Vertex blockVertices[] =
    {
        //{ { 330.0f, 20.0f, 0.0f },{ 0.0f, 0.0f } },
        //{ { 694.0f, 20.0f, 0.0f },{ 1.0f, 0.0f } },
        //{ { 694.0f, 748.0f, 0.0f },{ 1.0f, 1.0f } },
        //{ { 330.0f, 748.0f, 0.0f },{ 0.0f, 1.0f } }
        { { 0.0f, 0.0f, 0.0f },{ 0.0f, 0.0f } },
        { { 1.0f, 0.0f, 0.0f },{ 1.0f, 0.0f } },
        { { 1.0f, 1.0f, 0.0f },{ 1.0f, 1.0f } },
        { { 0.0f, 1.0f, 0.0f },{ 0.0f, 1.0f } }
    };

    const UINT vertexBufferSize = sizeof(blockVertices);

    D3D12_RESOURCE_DESC desc = {};
    desc.Alignment = 0;
    desc.DepthOrArraySize = 1;
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Flags = D3D12_RESOURCE_FLAG_NONE;
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.Height = 1;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.MipLevels = 1;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Width = (UINT64)vertexBufferSize;

    D3D12_HEAP_PROPERTIES heapProps;
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProps.CreationNodeMask = 1;
    heapProps.VisibleNodeMask = 1;

    hr = device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE,
        &desc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&blockVertexBuffer));

    if (FAILED(hr)) 
    { 
        ErrorMsg("block vb creation failed");
        return false;
    }

    transitionResource(blockVertexBuffer, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);

    hr = uploadBuffer(blockVertexBuffer, 0, blockVertices, vertexBufferSize, 0);

    if (FAILED(hr)) 
    { 
        ErrorMsg("vb upload failed");
        return false;
    }

    transitionResource(blockVertexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);

    blockVertexBufferView.BufferLocation = blockVertexBuffer->GetGPUVirtualAddress();
    blockVertexBufferView.SizeInBytes = vertexBufferSize;
    blockVertexBufferView.StrideInBytes = sizeof(Vertex);

    unsigned short indexBuffer[] = { 0, 1, 2, 2, 3, 0 };
    uint indexBufferSize = sizeof(indexBuffer);

    // change the width in the description of the buffer
    desc.Width = (UINT64)indexBufferSize;

    hr = device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE,
        &desc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&blockIndexBuffer));

    if (FAILED(hr))
    {
        ErrorMsg("block ib creation failed");
        return false;
    }

    transitionResource(blockIndexBuffer, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
    hr = uploadBuffer(blockIndexBuffer, 0, indexBuffer, indexBufferSize, 0);

    if (FAILED(hr))
    {
        ErrorMsg("vb upload failed");
        return false;
    }

    // now create the view
    transitionResource(blockIndexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);

    blockIndexBufferView.BufferLocation = blockIndexBuffer->GetGPUVirtualAddress();
    blockIndexBufferView.SizeInBytes = indexBufferSize;
    blockIndexBufferView.Format = DXGI_FORMAT_R16_UINT;

    return true;
}

bool App::createFontUav()
{
    HRESULT hr = S_OK;

    D3D12_RESOURCE_DESC desc = {};
    desc.Alignment = 0;
    desc.DepthOrArraySize = 1;
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Flags = D3D12_RESOURCE_FLAG_NONE;
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.Height = 1;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.MipLevels = 1;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Width = 1024 * 16; // 16k buffer for tex coordinates

    D3D12_HEAP_PROPERTIES heapProps;
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProps.CreationNodeMask = 1;
    heapProps.VisibleNodeMask = 1;

    hr = device->CreateCommittedResource(&heapProps, 
                                         D3D12_HEAP_FLAG_NONE,
                                         &desc, 
                                         D3D12_RESOURCE_STATE_GENERIC_READ, 
                                         nullptr, 
                                         IID_PPV_ARGS(&fontUav));

    // create view
    D3D12_SHADER_RESOURCE_VIEW_DESC viewDesc = {};
    //D3D12_UNORDERED_ACCESS_VIEW_DESC viewDesc = {};
    viewDesc.Format = DXGI_FORMAT_UNKNOWN;
    viewDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    viewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    viewDesc.Buffer.StructureByteStride = sizeof(fontUavData);
    viewDesc.Buffer.NumElements = (1024 * 16) / sizeof(fontUavData);
    viewDesc.Buffer.FirstElement = 0;
    viewDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

    D3D12_CPU_DESCRIPTOR_HANDLE h;
    h.ptr = viewHeap->GetCPUDescriptorHandleForHeapStart().ptr;
    device->CreateShaderResourceView(fontUav, &viewDesc, h);

    // map the resource
    hr = fontUav->Map(0, nullptr, reinterpret_cast<void**>(&fontUavPtr));
    ASSERT(hr == S_OK);

    // TODO fix this so it works
    viewHeapOffset = 0;

    return true;
}

bool App::init()
{
    done = false;

    // create pipelines we need to draw things
    if (!createBlockPipeline())
    {
        return false;
    }

    if (!createFontPipeline())
    {
        return false;
    }

    if (!createBlockVB())
    {
        return false;
    }

    createFontUav();

    // read in font info
    ReadFont(font, "font.fnt");

    // load font data into a comitted resource
    int width;
    int height;
    int comp;
    //unsigned char *data = static_cast<unsigned char *>(stbi_load(font.imgFileName.c_str(), &width, &height, &comp, 0));
    unsigned char *data = static_cast<unsigned char *>(stbi_load("font_0.tga", &width, &height, &comp, 0));
    const DXGI_FORMAT format = DXGI_FORMAT_R8_UNORM;

    // create upload buffer
    CComPtr<ID3D12Resource> uploadTemp;
    uploadTemp.Attach(createBuffer(D3D12_HEAP_TYPE_UPLOAD, width * height * comp, D3D12_RESOURCE_STATE_GENERIC_READ));
    
    unsigned char *pBufData;
    uploadTemp->Map(0, nullptr, reinterpret_cast<void**>(&pBufData));

    // copy data to buffer
    memcpy(pBufData, data, width * height * comp);

    uploadTemp->Unmap(0, nullptr);

    // create actual invisible vid mem texture
    fontTexture.Attach(createTexture(D3D12_HEAP_TYPE_DEFAULT, 256, 256, format));
    D3D12_RESOURCE_DESC desc = fontTexture->GetDesc();

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT fp;
    UINT64 rowSize, totalBytes;
    device->GetCopyableFootprints(&desc, 0, 1, 0, &fp, nullptr, &rowSize, &totalBytes);

    D3D12_TEXTURE_COPY_LOCATION srcLoc, destLoc;
    srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    srcLoc.pResource = uploadTemp;
    srcLoc.PlacedFootprint = fp;

    destLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    destLoc.pResource = fontTexture;
    destLoc.SubresourceIndex = 0;

    transitionResource(fontTexture, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);

    ID3D12GraphicsCommandList* cmdList = gfxCmdList;
    cmdList->CopyTextureRegion(&destLoc, 0, 0, 0, &srcLoc, nullptr);

    transitionResource(fontTexture, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);

    // create view
    D3D12_SHADER_RESOURCE_VIEW_DESC fontTexDesc = {};
    fontTexDesc.Format = format;
    fontTexDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    fontTexDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    fontTexDesc.Texture2D.MipLevels = 1;

    D3D12_CPU_DESCRIPTOR_HANDLE handle = viewHeap->GetCPUDescriptorHandleForHeapStart();
    // TODO: fix this so it's automatic
    handle.ptr += viewDescSize;
    device->CreateShaderResourceView(fontTexture, &fontTexDesc, handle);

    cmdSubmission[activeSubmission].deferredFrees.push_back((ID3D12DeviceChild *)uploadTemp);

    // initialize blocks and gameboard
    m_activeBlockPosition = { m_startingBlockXPosition, m_startingBlockYPosition };
    m_pActiveShape = std::make_unique<Shape>(Shape(m_activeBlockPosition));
    m_pNextShape = std::make_unique<Shape>(Shape(m_nextBlockPosition));

    memset(&m_gameBoard, 0, sizeof(m_gameBoard));

    return true;
}

void App::exit()
{
}

void App::updateGame()
{
    m_timeSinceLastDrop += frameTime;

    // d d d drop the block
    if (m_timeSinceLastDrop > m_dropLimit)
    {
        m_timeSinceLastDrop = 0;

        m_pActiveShape->position.y++;

        if (checkCollision() == true)
        {
            // bring the block back up
            m_pActiveShape->position.y--;

            // save block to game board
            retireBlock();

            checkLines();

            // make a new block
            m_pActiveShape = std::move(m_pNextShape);
            m_pActiveShape->position = { m_startingBlockXPosition, m_startingBlockYPosition };
            m_pNextShape = std::make_unique<Shape>(Shape(m_nextBlockPosition));
        }
    }
}

void App::checkLines()
{
    int hasGap = false;

    for (uint y = 0; y < m_boardHeight; y++)
    {
        hasGap = false;
        for (uint x = 0; x < m_boardWidth; x++)
        {
            if (m_gameBoard[y * m_boardWidth + x] == 0)
            {
                hasGap = true;
                break;
            }
        }

        if (hasGap == false)
        {
            uint* pOldLocation = m_gameBoard;
            uint* pNewLocation = m_gameBoard + m_boardWidth;
            uint blocksToMove = y * m_boardWidth * sizeof(uint);
            // erase line
            memmove_s(pNewLocation, blocksToMove + m_boardWidth, pOldLocation, blocksToMove);

            // erase top
            memset(&m_gameBoard, 0, sizeof(uint) * m_boardWidth);
        }
    }
}

void App::retireBlock()
{
    const unsigned char* shapeBlock = m_pActiveShape->getCurrentBlock();
    const XMUINT2 blockPosition = m_pActiveShape->getPosition();

    for (uint y = 0; y < 4; y++)
    {
        for (uint x = 0; x < 4; x++)
        {
            uint boardPositionX = blockPosition.x + x;
            uint boardPositionY = blockPosition.y + y;

            if (boardPositionY < m_boardHeight &&
                boardPositionX < m_boardWidth)
            {
                uint boardPositionX = blockPosition.x + x;
                uint boardPositionY = blockPosition.y + y;
                ASSERT((m_boardHeight * m_boardWidth) > (boardPositionY * m_boardWidth + boardPositionX));
                if (m_gameBoard[boardPositionY * m_boardWidth + boardPositionX] == 0)
                {
                    m_gameBoard[boardPositionY * m_boardWidth + boardPositionX] = *shapeBlock * m_pActiveShape->getColorIdx();
                }
            }

            shapeBlock++;
        }
    }
}

bool App::checkCollision()
{
    const unsigned char* shapeBlock = m_pActiveShape->getCurrentBlock();
    const XMUINT2 position = m_pActiveShape->getPosition();

    // TODO assert somewhere here the game block is within the board

    for (uint y = 0; y < 4; y++)
    {
        for (uint x = 0; x < 4; x++)
        {
            // if we are below the game board just check the block
            if ((position.y) + y >= m_boardHeight)
            {
                if (*shapeBlock != 0)
                {
                    return true;
                }
            }
            // check the sides too
            else if ((position.x + x) >= m_boardWidth) // note: unsigned int defined behavior assumed
            {
                if (*shapeBlock != 0)
                {
                    return true;
                }
            }
            else if (*shapeBlock != 0 && m_gameBoard[(m_boardWidth * (y + position.y)) + x + position.x] != 0)
            {
                return true;
            }

            shapeBlock++;
        }
    }

    return false;
}

void App::drawShape(const Shape& shape)
{
    // get the square dimensions, either 3x3 or 4x4
    //const uint shapeWH = shape.getSize(); // TODO or not
    const unsigned char* shapeBlock = shape.getCurrentBlock();
    const XMUINT2 position = shape.getPosition();

    for (uint y = 0; y < 4; y++)
    {
        for (uint x = 0; x < 4; x++)
        {
            if (*shapeBlock == 1)
            {
                drawBlock(position.x + x, position.y + y, shape.getColor());
            }
            shapeBlock++;
        }
    }
}

void App::drawBoard()
{
    for (uint y = 0; y < m_boardHeight; y++)
    {
        for (uint x = 0; x < m_boardWidth; x++)
        {
            if (m_gameBoard[y * m_boardWidth + x] > 0)
            {
                drawBlock(x, y, shapeColors[m_gameBoard[y * m_boardWidth + x]]);
            }
        }
    }
}

void App::drawGameBoardBackground()
{
    ID3D12GraphicsCommandList* cmdList = gfxCmdList;
    float boardColor[3] = { 0.0f, 0.0f, 0.0f };

    XMFLOAT4X4 viewproj;
    XMMATRIX project_m = XMMatrixOrthographicOffCenterLH(0.0f, static_cast<float>(m_windowWidth), 0.0f, static_cast<float>(m_windowHeight), -1.0f, 1.0f);
    XMMATRIX scale_m = XMMatrixScaling(static_cast<float>(m_boardWidthPixels), 
                                       static_cast<float>(m_boardHeightPixels), 1);

    project_m = scale_m * project_m;
    XMStoreFloat4x4(&viewproj, XMMatrixTranspose(project_m));

    // set root sig
    cmdList->SetGraphicsRootSignature(blockRootSig);

    // set projection matrix
    cmdList->SetGraphicsRoot32BitConstants(0, 16, viewproj.m, 0);

    // set color
    cmdList->SetGraphicsRoot32BitConstants(1, 3, boardColor, 0);

    cmdList->OMSetRenderTargets(1, &backbufDescHandle[backbufCurrent], false, &dsvHeap->GetCPUDescriptorHandleForHeapStart());
    cmdList->RSSetScissorRects(1, &defaultScissor);
    cmdList->RSSetViewports(1, &defaultViewport);

    //ID3D12DescriptorHeap *heaps[] = { viewHeap };
    //cmdList->SetDescriptorHeaps(1, heaps);
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->SetPipelineState(blockPipeline);

    cmdList->IASetVertexBuffers(0, 1, &blockVertexBufferView);
    cmdList->IASetIndexBuffer(&blockIndexBufferView);
    cmdList->DrawIndexedInstanced(6, 1, 0, 0, 0);
}

void App::drawBlock(const uint xpos, const uint ypos, const XMFLOAT4& color)
{
    ID3D12GraphicsCommandList* cmdList = gfxCmdList;

    float blockColor[3] = { color.x, color.y, color.z};
    float blockWidth = static_cast<float>(m_boardWidthPixels) / static_cast<float>(m_boardWidth);
    float blockHeight = static_cast<float>(m_boardHeightPixels) / static_cast<float>(m_boardHeight);

    XMFLOAT4X4 viewproj;
    XMMATRIX project_m = XMMatrixOrthographicOffCenterLH(0.0f, static_cast<float>(m_windowWidth), 0.0f, static_cast<float>(m_windowHeight), -1.0f, 1.0f);
    XMMATRIX scale_m = XMMatrixScaling(blockWidth, blockHeight, 1);
    float x = xpos * blockWidth;
    float y = (m_boardHeightPixels - (ypos *blockHeight)) - blockHeight;
    XMMATRIX transpose_m = XMMatrixTranslation(x, y, 0.0f);

    project_m = scale_m * transpose_m * project_m;
    XMStoreFloat4x4(&viewproj, XMMatrixTranspose(project_m));

    // set root sig
    cmdList->SetGraphicsRootSignature(blockRootSig);

    // set projection matrix
    cmdList->SetGraphicsRoot32BitConstants(0, 16, viewproj.m, 0);

    // set color
    cmdList->SetGraphicsRoot32BitConstants(1, 3, blockColor, 0);

    cmdList->OMSetRenderTargets(1, &backbufDescHandle[backbufCurrent], false, &dsvHeap->GetCPUDescriptorHandleForHeapStart());
    cmdList->RSSetScissorRects(1, &defaultScissor);
    cmdList->RSSetViewports(1, &defaultViewport);

    //ID3D12DescriptorHeap *heaps[] = { viewHeap };
    //cmdList->SetDescriptorHeaps(1, heaps);
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->SetPipelineState(blockPipeline);

    cmdList->IASetVertexBuffers(0, 1, &blockVertexBufferView);
    cmdList->IASetIndexBuffer(&blockIndexBufferView);
    cmdList->DrawIndexedInstanced(6, 1, 0, 0, 0);
}

void App::drawChar(const uint xpos, const uint ypos, const XMFLOAT4& color)
{
    ID3D12GraphicsCommandList* cmdList = gfxCmdList;

    // update the tex coordinates
    fontUavData uavDataArray[4] = { {0.0, 1.0},
                                    {1.0, 1.0},
                                    {1.0, 0.0},
                                    {0.0, 0.0} };

    // put data in mapped upload buffer
    memcpy(fontUavPtr, uavDataArray, sizeof(fontUavData) * 4);

    float blockColor[3] = { color.x, color.y, color.z };
    float blockWidth = 100;
    float blockHeight = 100;

    XMFLOAT4X4 viewproj;
    XMMATRIX project_m = XMMatrixOrthographicOffCenterLH(0.0f, static_cast<float>(m_windowWidth), 0.0f, static_cast<float>(m_windowHeight), -1.0f, 1.0f);
    XMMATRIX scale_m = XMMatrixScaling(blockWidth, blockHeight, 1);
    float x = xpos + blockWidth;
    float y = (m_boardHeightPixels - (ypos + blockHeight)) - blockHeight;
    XMMATRIX transpose_m = XMMatrixTranslation(x, y, 0.0f);

    project_m = scale_m * transpose_m * project_m;
    XMStoreFloat4x4(&viewproj, XMMatrixTranspose(project_m));

    cmdList->SetPipelineState(fontPipeline);
    // set root sig
    cmdList->SetGraphicsRootSignature(fontRootSig);

    ID3D12DescriptorHeap *heaps[] = { viewHeap };
    cmdList->SetDescriptorHeaps(1, heaps);

    // set uav and texture
    //D3D12_GPU_DESCRIPTOR_HANDLE uavHandle = uavHeap->GetGPUDescriptorHandleForHeapStart();
    //cmdList->SetGraphicsRootDescriptorTable(0, uavHandle);

    D3D12_GPU_DESCRIPTOR_HANDLE viewHandle = viewHeap->GetGPUDescriptorHandleForHeapStart();
    cmdList->SetGraphicsRootDescriptorTable(0, viewHandle);

    D3D12_GPU_DESCRIPTOR_HANDLE viewHandle2 = viewHeap->GetGPUDescriptorHandleForHeapStart();
    viewHandle2.ptr += viewDescSize;
    cmdList->SetGraphicsRootDescriptorTable(1, viewHandle2);

    // set projection matrix
    cmdList->SetGraphicsRoot32BitConstants(3, 16, viewproj.m, 0);

    // set color
    cmdList->SetGraphicsRoot32BitConstants(2, 3, blockColor, 0);

    cmdList->OMSetRenderTargets(1, &backbufDescHandle[backbufCurrent], false, &dsvHeap->GetCPUDescriptorHandleForHeapStart());
    cmdList->RSSetScissorRects(1, &defaultScissor);
    cmdList->RSSetViewports(1, &defaultViewport);

    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // null pointer for vertices
    cmdList->IASetVertexBuffers(0, 1, nullptr);
    cmdList->IASetIndexBuffer(&blockIndexBufferView);
    cmdList->DrawIndexedInstanced(6, 1, 0, 0, 0);
}

void App::drawFrame()
{
    HRESULT hr = S_OK;

    ID3D12GraphicsCommandList* cmdList = gfxCmdList;

    transitionResource(backbuf[backbufCurrent], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

    float clearCol[4] = { 0.2f, 0.5f, 0.0f, 1.0f };
    cmdList->ClearRenderTargetView(backbufDescHandle[backbufCurrent], clearCol, 0, nullptr);
    cmdList->ClearDepthStencilView(dsvHeap->GetCPUDescriptorHandleForHeapStart(),
        D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    drawGameBoardBackground();

    drawShape(*m_pActiveShape.get());
    drawShape(*m_pNextShape.get());

    drawBoard();

    XMFLOAT4 color { 1.0f, 1.0f, 1.0f, 1.0f };

    // uncomment this to draw font
    //drawChar(50, 50, color);

    transitionResource(backbuf[backbufCurrent], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

    // Submit graphics command list and swap
    swapBuffers(false);
    //++frame;
}

void App::onKey(const uint key, const bool pressed)
{
    if (pressed)
    {
        switch (key)
        {
        case 'W':
            // rotate shape (if it won't touch a wall nes style)
            m_pActiveShape->rotateForward();
            if (checkCollision() == true)
            {
                m_pActiveShape->rotateBackward();
            }
            break;
        case 'S':
            // decrease drop limit
            m_dropLimit = .1;
            break;
        case 'A':
            // move left
            m_pActiveShape->position.x--;
            if (checkCollision() == true)
            {
                m_pActiveShape->position.x++;
            }
            break;
        case 'D':
            // move right
            m_pActiveShape->position.x++;
            if (checkCollision() == true)
            {
                m_pActiveShape->position.x--;
            }
            break;
        }
    }
    else
    {
        switch (key)
        {
        case 'S':
            // decrease drop limit
            m_dropLimit = .2;
            break;
        }
    }
}
