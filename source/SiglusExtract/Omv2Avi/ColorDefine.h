#pragma once

typedef unsigned char	U8;
typedef unsigned short	U16;
typedef unsigned int	U32;

__forceinline U8 Fix(int x) { return (U8)(x < 0 ? 0 : (x > 0xff ? 0xff : x)); }
