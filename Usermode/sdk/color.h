#pragma once
#include <stdint.h>


class Color {
public:
	Color( uint8_t red, uint8_t green, uint8_t blue ) : r( red ), g( green ), b( blue ) {};
	Color( uint8_t red, uint8_t green, uint8_t blue, float alpha ) : r( red ), g( green ), b( blue ), a( alpha ) {};
	Color( int hex ) {
		r = ( ( hex >> 16 ) & 0xFF ) / 255;
		g = ( ( hex >> 8 ) & 0xFF ) / 255;
		b = ( ( hex ) & 0xFF ) / 255;
	};

	uint32_t ToHex( ) {
		return ( ( r & 0xff ) << 16 ) + ( ( g & 0xff ) << 8 ) + ( b & 0xff );
	}

	D2D1_COLOR_F ToD2D( ) const {
		D2D1_COLOR_F color;
		color.r = static_cast< float >( r / 255.f );
		color.g = static_cast< float >( g / 255.f );
		color.b = static_cast< float >( b / 255.f );
		color.a = a;
		return color;
	}

	uint8_t r{ }, g{ }, b{ };
	float a{ 1.f };
};