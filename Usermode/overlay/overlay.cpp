#include "overlay.h"

#include <random>
#include <chrono>

void Overlay::Main( ) {
    HWND hwnd = FindWindow( "SDL_app", NULL );
    CDrawer d{ CreateOverlayWindow( ), hwnd };

    // font_resource
    auto font{ d.CreateFontResource( "Arial", 25 ) };

    // CLayerResource
    auto layer = d.CreateLayer( );

    // text_resource
    auto watermark = d.CreateText( font, "i hate fat people." );
    watermark.Align( ETextAlign::right );

    std::uint64_t frametime = 1;
    std::uint64_t frametime_sum = 1;
    std::uint64_t frame_count = 1;
    std::uint64_t avg_fps = 1;

    while ( true ) {
        using std::chrono::steady_clock;
        using std::chrono::microseconds;
        using std::chrono::duration_cast;

        const auto begin = steady_clock::now( );

        // message pump -------------------------------------------------------
        d.PumpMessages( );

        // average fps --------------------------------------------------------
        font.Align( ETextAlign::right );
        font.Draw( L"current fps: " + std::to_wstring( 1000000 / frametime )
            + L"\nstabilized: " + std::to_wstring( 1000000 / ( frametime_sum / frame_count ) )
            + L"\naverage over 1000 frames: " + std::to_wstring( avg_fps )
            , { 0, 0 }
        , Color( 0, 255, 0 ) );

        watermark.Draw( { 500,0 }, Color( 0, 0, 255 ) );

        d.Display( );

        frametime = static_cast< std::uint64_t >(
            duration_cast< microseconds >( steady_clock::now( ) - begin ).count( ) );
        frametime_sum += frametime;

        if ( ++frame_count == 1000 ) {
            avg_fps = 1000000 / ( frametime_sum / 1000 );
            frametime_sum = 1;
            frame_count = 1;
        }
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

