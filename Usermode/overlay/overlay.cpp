#include "overlay.h"

#include <random>
#include <chrono>

void Overlay::Main( CDrawer* d ) {

    // font_resource
    auto font{ d->CreateFontResource( "Arial", 25 ) };

    // CLayerResource
    auto layer = d->CreateLayer( );

    // text_resource
    auto watermark = d->CreateText( font, "i hate fat people." );
    watermark.Align( ETextAlign::right );

    while ( true ) {
        d->PumpMessages( );
        if ( m_pVisualCallback )
            m_pVisualCallback( d );

        watermark.Draw( { 500,0 }, Color( 100, 255, 255 ) );

        d->Display( );
    }
}

 
LRESULT CALLBACK WndProc( HWND window, UINT message, WPARAM wparam, LPARAM lparam ) {
    if ( message == WM_DESTROY ) {
        PostQuitMessage( 0 );
        return 0;
    }
    return DefWindowProc( window, message, wparam, lparam );
}

HWND Overlay::CreateOverlayWindow( ) {
    WNDCLASSA wc = { 0 };
    wc.lpszClassName = xors( "HVC" );
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;

    const auto class_atom = RegisterClassA( &wc );
    if ( !class_atom )
        return { };

    const auto flags = WS_EX_NOREDIRECTIONBITMAP
        | WS_EX_LAYERED
        | WS_EX_TRANSPARENT;

    const auto width = GetSystemMetrics( SM_CXSCREEN ) - 2;
    const auto height = GetSystemMetrics( SM_CYSCREEN ) - 1;

    const auto window = CreateWindowEx( flags
        , wc.lpszClassName
        , xors( "HVC" )
        , WS_VISIBLE | WS_POPUP
        , -1
        , -1
        , width
        , height
        , nullptr
        , nullptr
        , nullptr
        , nullptr );
    if ( !window )
        return { };

    return window;
}

