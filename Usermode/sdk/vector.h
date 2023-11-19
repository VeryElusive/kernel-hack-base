#pragma once
#include <d2d1.h>

class Vector {
public:
	float x{ }, y{ }, z{ };

	bool IsValid( ) const {
		return x != -1.f
			&& y != -1.f
			&& z != -1.f;
	}

	inline float Dot( const Vector v ) { return x * v.x + y * v.y + z * v.z; }
};

class Vector2D {
public:
	int x{ }, y{ };

	constexpr bool operator > ( const Vector2D& rhs ) const {
		return ( this->x > rhs.x && this->y > rhs.y );
	}

	constexpr bool operator < ( const Vector2D& rhs ) const {
		return ( this->x < rhs.x&& this->y < rhs.y );
	}

	__forceinline Vector2D operator+( Vector2D v ) const {
		return Vector2D( x + v.x, y + v.y );
	}

	__forceinline Vector2D operator-( Vector2D v ) const {
		return Vector2D( x - v.x, y - v.y );
	}

	__forceinline Vector2D operator+( int v ) const {
		return Vector2D( x + v, y + v );
	}

	__forceinline Vector2D operator-( int v ) const {
		return Vector2D( x - v, y - v );
	}

	inline float Dot( const Vector2D v ) { return x * v.x + y * v.y; }

	D2D1_POINT_2F ToD2D( ) const { return { static_cast< float >( x ), static_cast< float >( y ) }; };

	Vector2D( ) {};

	Vector2D( int x, int y ) : x( x ), y( y ) {};
};

struct Matrix4x4_t {
	union {
		struct {
			float        _11, _12, _13, _14;
			float        _21, _22, _23, _24;
			float        _31, _32, _33, _34;
			float        _41, _42, _43, _44;

		}; float m[ 4 ][ 4 ];
	};
};