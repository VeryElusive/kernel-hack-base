#include "overlay.h"
#include "../utils/utils.h"

#include <d2d1_3helper.h>
#include <d3d11_2.h>

using namespace Overlay;

ComPtr<IDXGISwapChain1> CreateSwapChain( const HWND window , ComPtr<IDXGIDevice>& dxgi_device ) {
    ComPtr<ID3D11Device> d3_device;
    D3D11CreateDevice( nullptr
        , D3D_DRIVER_TYPE_HARDWARE
        , nullptr
        , D3D11_CREATE_DEVICE_BGRA_SUPPORT
        , nullptr
        , 0
        , D3D11_SDK_VERSION
        , &d3_device
        , nullptr
        , nullptr );
    d3_device.As( &dxgi_device );

    ComPtr<IDXGIFactory2> dx_factory;
    CreateDXGIFactory2( 0 // do not load the debug dll
        , __uuidof( IDXGIFactory2 )
        , reinterpret_cast< void** >( dx_factory.GetAddressOf( ) ) );

    DXGI_SWAP_CHAIN_DESC1 description = { 0 };
    description.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    description.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    description.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    description.BufferCount = 2;
    description.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;
    description.SampleDesc.Count = 1;

    RECT rect;
    if ( !GetClientRect( window, &rect ) )
        return{ };

    description.Width = static_cast< UINT >( rect.right - rect.left );
    description.Height = static_cast< UINT >( rect.bottom - rect.top );

    ComPtr<IDXGISwapChain1> schain;
    dx_factory->CreateSwapChainForComposition( dxgi_device.Get( )
        , &description
        , nullptr
        , schain.GetAddressOf( ) );

    return schain;
}

ComPtr<ID2D1DeviceContext> create_device_ctx( ComPtr<IDXGIDevice>& dxgi_device ) {
    // if you manually close the window a breakpoint will be triggered
    // and say that you have leaks because the destructors are not run
    const D2D1_FACTORY_OPTIONS options = { D2D1_DEBUG_LEVEL_NONE };

    ComPtr<ID2D1Factory2> d2_factory;
    D2D1CreateFactory( D2D1_FACTORY_TYPE_SINGLE_THREADED
        , options
        , d2_factory.GetAddressOf( ) );

    // create the d2d device that links back to the d3d device
    ComPtr<ID2D1Device1> d2_device;
    d2_factory->CreateDevice( dxgi_device.Get( ), d2_device.GetAddressOf( ) );

    // Create the d2d device context that is the actual render target
    // and exposes drawing commands
    ComPtr<ID2D1DeviceContext> dev_context;
    d2_device->CreateDeviceContext( D2D1_DEVICE_CONTEXT_OPTIONS_ENABLE_MULTITHREADED_OPTIMIZATIONS
        , dev_context.GetAddressOf( ) );

    return dev_context;
}

ComPtr<IDCompositionTarget> create_composition_target( const HWND window, 
    ComPtr<IDXGIDevice>& dxgi_device, ComPtr<ID2D1DeviceContext>& dev_context , ComPtr<IDXGISwapChain1>& swap_chain ) {
    // retrieve back buffer
    ComPtr<IDXGISurface2> surface;
    swap_chain->GetBuffer( 0 // index
        , __uuidof( IDXGISurface2 )
        , reinterpret_cast< void** >( surface.GetAddressOf( ) ) );

    // create a d2d bitmap that points to the swap chain surface
    D2D1_BITMAP_PROPERTIES1 properties = {};
    properties.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
    properties.pixelFormat.format = DXGI_FORMAT_R8G8B8A8_UNORM;
    properties.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET
        | D2D1_BITMAP_OPTIONS_CANNOT_DRAW;

    ComPtr<ID2D1Bitmap1> bitmap;
    dev_context->CreateBitmapFromDxgiSurface( surface.Get( )
        , properties
        , bitmap.GetAddressOf( ) );
    dev_context->SetTarget( bitmap.Get( ) );

    ComPtr<IDCompositionDevice> comp_device;
    DCompositionCreateDevice( dxgi_device.Get( )
        , __uuidof( IDCompositionDevice )
        , reinterpret_cast< void** >( comp_device.GetAddressOf( ) ) );

    ComPtr<IDCompositionTarget> comp_target;
    comp_device->CreateTargetForHwnd( window
        , false // topmost
        , comp_target.GetAddressOf( ) );


    ComPtr<IDCompositionVisual> visual;
    comp_device->CreateVisual( visual.GetAddressOf( ) );

    visual->SetContent( swap_chain.Get( ) );

    comp_target->SetRoot( visual.Get( ) );

    comp_device->Commit( );

    return comp_target;
}


CLayerResource::CLayerResource( ID2D1DeviceContext* dc, ComPtr<ID2D1Layer>&& layer ) : m_pDeviceContext( dc ), m_pLayer( std::move( layer ) ) {}

void CLayerResource::Push( const Vector2D& pos, const Vector2D& size, float opacity ) {
    // i have not tested this and it may take size as pos + size /shrug

    D2D1_RECT_F rect{ pos.x, pos.y, pos.x + size.x, pos.y + size.y };

    // Push the layer with the content bounds.
    m_pDeviceContext->PushLayer( D2D1::LayerParameters( rect
        , nullptr
        , D2D1_ANTIALIAS_MODE_PER_PRIMITIVE
        , D2D1::IdentityMatrix( )
        , opacity
        , nullptr
        , D2D1_LAYER_OPTIONS_NONE )
        , m_pLayer.Get( ) );
}

void CLayerResource::Pop( ) {
    m_pDeviceContext->PopLayer( );
}

CLayerResource::Guard_t CLayerResource::SetClippedRect( const Vector2D& pos, const Vector2D& size, float opacity ) {
    Push( pos, size, opacity );
    return Guard_t{ m_pDeviceContext };

}

ID2D1SolidColorBrush* CDrawer::LocalBrush( const Color& color ) {
    static thread_local auto brush = [ ]( ComPtr<ID2D1DeviceContext>& dev_context ) {
        ComPtr<ID2D1SolidColorBrush> LocalBrush;
        dev_context->CreateSolidColorBrush( { 0,0,0,0 }
        , LocalBrush.GetAddressOf( ) );
        return LocalBrush;
    }( m_pDeviceContext );

    brush->SetColor( color.ToD2D( ) );

    return brush.Get( );
}

CDrawer::CDrawer( HWND window, HWND gameWindow ) : hwnd( window ), gameHWND( gameWindow ) {
    ComPtr<IDXGIDevice> dxgi_device;
    m_pSwapChain = CreateSwapChain( window, dxgi_device );

    m_pDeviceContext = create_device_ctx( dxgi_device );

    m_pCompositionTarget = create_composition_target( window, dxgi_device, m_pDeviceContext, m_pSwapChain );

    // create dwrite factory to generate text layouts
    DWriteCreateFactory( DWRITE_FACTORY_TYPE_SHARED
        , __uuidof( IDWriteFactory )
        , reinterpret_cast< IUnknown** >( m_pDWriteFactory.GetAddressOf( ) ) );

    // we need to begin drawing immediately because we exposed essentially
    // only the end method
    m_pDeviceContext->BeginDraw( );
    m_pDeviceContext->Clear( );
}

CDrawer::~CDrawer( ) noexcept {
    m_pDeviceContext->EndDraw( );
}

void CDrawer::Display( ) {
    SetWindowPos( gameHWND, hwnd, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE );

    m_pDeviceContext->EndDraw( );
    // should probably put error handling here but this can fail for stupid reasons
    m_pSwapChain->Present( 0, 0 );

    m_pDeviceContext->BeginDraw( );
    m_pDeviceContext->Clear( );

    //SetWindowPos( hwnd, gameHWND, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE );
}

void CDrawer::PumpMessages( ) const {
    MSG m;
    if ( PeekMessageA( &m, hwnd, 0, 0, PM_REMOVE ) ) {
        TranslateMessage( &m );
        DispatchMessageA( &m );
    }
}

void CDrawer::Circle( const Vector2D& center, float radius, const Color& color, float width ) {
    Ellipse( center, radius, radius, color, width );
}

void CDrawer::CircleFilled( const Vector2D& center, float radius, const Color& color ) {
    EllipseFilled( center, radius, radius, color );
}

void CDrawer::Ellipse( const Vector2D& center, float radius_x, float radius_y, const Color& color, float width ) {
    m_pDeviceContext->DrawEllipse( { center.ToD2D( ), radius_x, radius_y }, LocalBrush( color ), width );
}

void CDrawer::EllipseFilled( const Vector2D& center, float radius_x, float radius_y, const Color& color ) {
    m_pDeviceContext->FillEllipse( { center.ToD2D( ), radius_x, radius_y }, LocalBrush( color ) );
}

void CDrawer::Rect( const Vector2D& pos, const Vector2D& size, const Color& color, float width ) {
    D2D1_RECT_F rect{ pos.x, pos.y, pos.x + size.x, pos.y + size.y };
    m_pDeviceContext->DrawRectangle( rect, LocalBrush( color ), width );
}

void CDrawer::RectFilled( const Vector2D& pos, const Vector2D& size, const Color& color ) {
    D2D1_RECT_F rect{ pos.x, pos.y, pos.x + size.x, pos.y + size.y };
    m_pDeviceContext->FillRectangle( rect, LocalBrush( color ) );
}

void Overlay::CDrawer::RoundedRect( const Vector2D& pos, const Vector2D& size, const Color& color, float rounding, float width ) {
    D2D1_RECT_F rect{ pos.x, pos.y, pos.x + size.x, pos.y + size.y };
    m_pDeviceContext->DrawRoundedRectangle( { rect, rounding, rounding, }, LocalBrush( color ), width );
}

void Overlay::CDrawer::RoundedRectFilled( const Vector2D& pos, const Vector2D& size, const Color& color, float rounding ) {
    D2D1_RECT_F rect{ pos.x, pos.y, pos.x + size.x, pos.y + size.y };
    m_pDeviceContext->FillRoundedRectangle( { rect, rounding, rounding, }, LocalBrush( color ) );
}

void CDrawer::Line( const Vector2D& start, const Vector2D& end, const Color& color, float width ) {
    m_pDeviceContext->DrawLine( start.ToD2D( ), end.ToD2D( ), LocalBrush( color ), width );
}

CFontResource CDrawer::CreateFontResource( const std::string& font_family, float size, DWRITE_FONT_WEIGHT weight, DWRITE_FONT_STYLE style ) {
    ComPtr<IDWriteTextFormat> format;
    m_pDWriteFactory->CreateTextFormat( Utils::MultiByteToWide( font_family ).c_str( ), nullptr , weight, style, 
        DWRITE_FONT_STRETCH_NORMAL, size, L"", format.GetAddressOf( ) );

    return { std::move( format ), m_pDeviceContext.Get( ) };
}

CTextResource Overlay::CDrawer::CreateText( const CFontResource& wrapper, const std::string& text ) {
    ComPtr<IDWriteTextLayout> layout;
    m_pDWriteFactory->CreateTextLayout( Utils::MultiByteToWide( text ).c_str( ) , static_cast< UINT >( text.size( ) ), wrapper.Format( ),
        0, 0, layout.GetAddressOf( ) );

    return { std::move( layout ), m_pDeviceContext.Get( ) };
}

CLayerResource CDrawer::CreateLayer( ) {
    ComPtr<ID2D1Layer> layer;
    m_pDeviceContext->CreateLayer( layer.GetAddressOf( ) );

    return { m_pDeviceContext.Get( ), std::move( layer ) };
}

CDrawer::TranslationGuard_t CDrawer::MakeTranslation( const Vector2D& p, bool accumulate ) {
    const D2D1_POINT_2F point{ p.ToD2D( ) };

    if ( accumulate ) {
        auto translation = GetTransform( );
        translation._31 += point.x;
        translation._32 += point.y;
        m_pDeviceContext->SetTransform( translation );

        return TranslationGuard_t( m_pDeviceContext.Get( ),
            { translation._31 - point.x,  translation._32 - point.y } );
    }
    else {
        m_pDeviceContext->SetTransform( D2D1::Matrix3x2F::Translation( point.x, point.y ) );
        return TranslationGuard_t( m_pDeviceContext.Get( ) );
    }
}

Vector2D CDrawer::GetTranslation( ) const {
    const auto mat = GetTransform( );
    return { static_cast< int > ( mat._31 ), static_cast< int > ( mat._32 ) };
}

D2D1_MATRIX_3X2_F CDrawer::GetTransform( ) const {
    D2D1_MATRIX_3X2_F mat;
    m_pDeviceContext->GetTransform( &mat );
    return mat;
}

CFontResource::CFontResource( ComPtr<IDWriteTextFormat>&& format, ID2D1DeviceContext* context ) : m_pDeviceContext( context ), m_pTextFormat( std::move( format ) ) {
    // since we set the size of our text rect to 0 we need to disable wrapping
    m_pTextFormat->SetWordWrapping( DWRITE_WORD_WRAPPING_NO_WRAP );

    m_pDeviceContext->CreateSolidColorBrush( { 0,0,0,0 }, m_pBrush.GetAddressOf( ) );
}

void CFontResource::Align( ETextAlign horizontalAlignment ) {
    m_pTextFormat->SetTextAlignment( static_cast< DWRITE_TEXT_ALIGNMENT >( horizontalAlignment ) );
}

void Overlay::CFontResource::AlignVert( EVertTextAlign verticalAlignment ) {
    m_pTextFormat->SetParagraphAlignment( static_cast< DWRITE_PARAGRAPH_ALIGNMENT >( verticalAlignment ) );
}

void CFontResource::Draw( const std::string& text, const Vector2D pos, const Color col ) {
    return Draw( Utils::MultiByteToWide( text ), pos, col );
}

void CFontResource::Draw( const std::wstring& text, const Vector2D pos, const Color col ) {
    m_pBrush->SetColor( col.ToD2D( ) );
    m_pDeviceContext->DrawTextA( text.c_str( )
        , static_cast< UINT32 >( text.size( ) )
        , Format( )
        , { static_cast< float >( pos.x ), static_cast< float >( pos.y ), static_cast< float >( pos.x ), static_cast< float >( pos.y ) }
    , m_pBrush.Get( ) );
}

Overlay::CTextResource::CTextResource( ComPtr<IDWriteTextLayout>&& layout, ID2D1DeviceContext* context ) : m_pDeviceContext( context ), m_pLayout( std::move( layout ) ) {
    m_pDeviceContext->CreateSolidColorBrush( { 0,0,0,0 }, m_pBrush.GetAddressOf( ) );
    DWRITE_TEXT_METRICS metric;
    m_pLayout->GetMetrics( &metric );

    m_flWidth = metric.width;
    m_flHeight = metric.height;
}

void CTextResource::Align( ETextAlign horizontalAlignment ) {
    m_pLayout->SetTextAlignment( static_cast< DWRITE_TEXT_ALIGNMENT >( horizontalAlignment ) );
}

void CTextResource::AlignVert( EVertTextAlign verticalAlignment ) {
    m_pLayout->SetParagraphAlignment( static_cast< DWRITE_PARAGRAPH_ALIGNMENT >( verticalAlignment ) );
}

void CTextResource::Draw( const Vector2D pos, const Color col ) {
    m_pBrush->SetColor( col.ToD2D( ) );
    m_pDeviceContext->DrawTextLayout( pos.ToD2D( ), m_pLayout.Get( ), m_pBrush.Get( ) );
}

void CDrawer::_translation_destructor::operator()( ID2D1DeviceContext* dc ) {
    dc->SetTransform( D2D1::Matrix3x2F::Translation( previous.x, previous.y ) );
}