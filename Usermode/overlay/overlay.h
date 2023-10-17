#include <string>
#include <memory>

#include <dwrite_3.h>
#include <dcomp.h>
#include <wrl.h>

#include "../sdk/vector.h"
#include "../sdk/color.h"
#include "../../xorstr.h"

// pragma comment is not portable
#ifdef _MSC_VER
#pragma comment(lib, "dxgi")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "d2d1")
#pragma comment(lib, "dcomp")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "user32.lib")
#endif

// TODO: stop using ComPtr and make your own memory management

namespace Overlay {
    template<typename T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    HWND CreateOverlayWindow( );
    void Main( );

    enum class ETextAlign {
        // on the left of drawing position
        left = DWRITE_TEXT_ALIGNMENT_TRAILING,
        center = DWRITE_TEXT_ALIGNMENT_CENTER,
        // on the right of drawing position
        right = DWRITE_TEXT_ALIGNMENT_LEADING
    };

    enum class EVertTextAlign {
        // above the drawing position
        top = DWRITE_PARAGRAPH_ALIGNMENT_FAR,
        center = DWRITE_PARAGRAPH_ALIGNMENT_CENTER,
        // below the drawing position
        bottom = DWRITE_PARAGRAPH_ALIGNMENT_NEAR
    };

    class CFontResource {
        // owned by overlay drawer
        ID2D1DeviceContext* m_pDeviceContext;

        ComPtr<IDWriteTextFormat>    m_pTextFormat;
        ComPtr<ID2D1SolidColorBrush> m_pBrush;

    public:
        CFontResource( ComPtr<IDWriteTextFormat>&& format, ID2D1DeviceContext* context );

        void Align( ETextAlign horizontalAlignment );
        void AlignVert( EVertTextAlign verticalAlignment );

        void Draw( const std::string& text, const Vector2D pos, const Color col );
        void Draw( const std::wstring& text, const Vector2D pos, const Color col );

        IDWriteTextFormat* Format( ) const noexcept { return m_pTextFormat.Get( ); }

        // TextFormat doesn't really allow to change much of it.
        // only getters about the params with which it was created are possible
    };

    class CTextResource {
        // owned by overlay drawer
        ID2D1DeviceContext* m_pDeviceContext;

        ComPtr<IDWriteTextLayout>    m_pLayout;
        ComPtr<ID2D1SolidColorBrush> m_pBrush;
        float m_flWidth;
        float m_flHeight;

    public:
        CTextResource( ComPtr<IDWriteTextLayout>&& layout, ID2D1DeviceContext* context );

        void Align( ETextAlign horizontalAlignment );
        void AlignVert( EVertTextAlign verticalAlignment );

        void Draw( const Vector2D pos, const Color col );

        float Width( ) const noexcept { return m_flWidth; }
        float Height( ) const noexcept { return m_flHeight; }

        IDWriteTextLayout* Layout( ) const noexcept { return m_pLayout.Get( ); }
    };

    class CLayerResource {
        ID2D1DeviceContext* m_pDeviceContext;
        ComPtr<ID2D1Layer>  m_pLayer;

        struct Popper_t {
            void operator()( ID2D1DeviceContext* dc ) const { dc->PopLayer( ); }
        };

    public:
        CLayerResource( ID2D1DeviceContext* dc, ComPtr<ID2D1Layer>&& layer );

        using Guard_t = std::unique_ptr <ID2D1DeviceContext, Popper_t>;

        void Push( const Vector2D& pos, const Vector2D& size, float opacity );

        void Pop( );

        Guard_t SetClippedRect( const Vector2D& pos, const Vector2D& size, float opacity = 1.f );
    };


    class CDrawer {
        // must be preserved for drawing
        ComPtr<IDXGISwapChain1>     m_pSwapChain;
        ComPtr<ID2D1DeviceContext>  m_pDeviceContext;
        ComPtr<IDCompositionTarget> m_pCompositionTarget;
        // needed for font layout creation
        ComPtr<IDWriteFactory>      m_pDWriteFactory;
        // can be retrieved only from swapchain that was created with ...ForHwnd function
        HWND                        hwnd;
        HWND                        gameHWND;

        // thread safe buffer storage
        ID2D1SolidColorBrush* LocalBrush( const Color& color );


        struct _translation_destructor {
            D2D1_POINT_2F previous{ 0.f,0.f };
            void operator()( ID2D1DeviceContext* dc );
        };

    public:

        using TranslationGuard_t = std::unique_ptr<ID2D1DeviceContext, _translation_destructor>;

        explicit CDrawer( HWND ourWindow, HWND gameWindow );
        // need to call EndDraw
        ~CDrawer( ) noexcept;

        HWND WindowHandle( ) const noexcept { return hwnd; }

        // call this after finishing to draw figures
        void Display( );

        void PumpMessages( ) const;


        void Circle( const Vector2D& center, float radius, const Color& color, float width = 1.f );
        void CircleFilled( const Vector2D& center, float radius, const Color& color );


        void Ellipse( const Vector2D& center
            , float radius_x
            , float radius_y
            , const Color& color
            , float width = 1.f );
        void EllipseFilled( const Vector2D& center
            , float radius_x
            , float radius_y
            , const Color& color );


        void Rect( const Vector2D& pos, const Vector2D& size, const Color& color, float width = 1 );
        void RectFilled( const Vector2D& pos, const Vector2D& size, const Color& color );

        void RoundedRect( const Vector2D& pos, const Vector2D& size, const Color& color, float rounding = 1, float width = 1 );
        void RoundedRectFilled( const Vector2D& pos, const Vector2D& size, const Color& color, float rounding = 1 );

        void Line( const Vector2D& start, const Vector2D& end, const Color& color, float width = 1.f );

        // the parameters cannot be changed after creation.
        // I would recommend not creating multiple copies of same font, but
        // they seem to be cached by DXGI so it may be fine
        CFontResource CreateFontResource( const std::string& font_family
            , float size
            , DWRITE_FONT_WEIGHT weight = DWRITE_FONT_WEIGHT_REGULAR
            , DWRITE_FONT_STYLE style = DWRITE_FONT_STYLE_NORMAL );

        // used to create static text
        CTextResource CreateText( const CFontResource& wrapper, const std::string& text );

        CLayerResource CreateLayer( );

        /// if accumulate is set to true previous translation value is added
        TranslationGuard_t MakeTranslation( const Vector2D& point, bool accumulate = true );

        Vector2D GetTranslation( ) const;

        D2D1_MATRIX_3X2_F GetTransform( ) const;
    };

}