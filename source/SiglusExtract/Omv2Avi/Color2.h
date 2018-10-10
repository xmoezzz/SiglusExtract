#pragma once

#include "ColorDefine.h"
#include "my.h"


#ifdef __GNUC__
#ifndef __forceinline
#define __forceinline __attribute__((__always_inline__)) inline
#endif
#endif

//__forceinline U8 Fix(int x) { return (U8)(x < 0 ? 0 : (x > 255 ? 255 : x)); }
#define BGR_Y(b, g, r) ((U8)(((b) * 120041 + (g) * 615105 + (r) * 313430 + 0x007ffff) >> 20))
#define BGR_U(b, g, r) ((U8)(((b) * 524288 - (g) * 347351 - (r) * 176937 + 0x807ffff) >> 20))
#define BGR_V(b, g, r) ((U8)(((b) * -85260 - (g) * 439028 + (r) * 524288 + 0x807ffff) >> 20))
#define YUV_B(y, u, v) (Fix(((y) * 0x100000 + 0x7ffff + ((u) - 0x80) * 1857070						   ) >> 20))
#define YUV_G(y, u, v) (Fix(((y) * 0x100000 + 0x7ffff - ((u) - 0x80) *  361883 - ((v) - 0x80) *  748988) >> 20))
#define YUV_R(y, u, v) (Fix(((y) * 0x100000 + 0x7ffff						   + ((v) - 0x80) * 1470292) >> 20))

// w和h必须是2的倍数,并假设u(stride_d[1])和v(stride_d[2])相等
__forceinline void BGR_YUV420_F(U8* const dst[3], const int stride_d[3], const void* src, int stride_s, int w, int h)
{
	const int stride_y = stride_d[0];
	const int stride_u = stride_d[1];
	const U8* src_s = (U8*)src;
	U8* dst_y = (U8*)dst[0];
	U8* dst_u = (U8*)dst[1];
	U8* dst_v = (U8*)dst[2];

	for (w /= 2, h /= 2; h > 0; --h)
	{
		U8* dst_v_end = dst_v + w;
		while (dst_v < dst_v_end)
		{
			int b, g, r, u, v;
			b = src_s[0]; g = src_s[1]; r = src_s[2];
			dst_y[0] = BGR_Y(b, g, r); u = BGR_U(b, g, r); v = BGR_V(b, g, r);
			b = src_s[3]; g = src_s[4]; r = src_s[5];
			dst_y[1] = BGR_Y(b, g, r); u += BGR_U(b, g, r); v += BGR_V(b, g, r);
			b = src_s[stride_s]; g = src_s[stride_s + 1]; r = src_s[stride_s + 2];
			dst_y[stride_y] = BGR_Y(b, g, r); u += BGR_U(b, g, r); v += BGR_V(b, g, r);
			b = src_s[stride_s + 3]; g = src_s[stride_s + 4]; r = src_s[stride_s + 5];
			dst_y[stride_y + 1] = BGR_Y(b, g, r); u += BGR_U(b, g, r); v += BGR_V(b, g, r);
			dst_u[0] = (U8)((u + 2) / 4);
			dst_v[0] = (U8)((v + 2) / 4);

			src_s += 6;
			dst_y += 2;
			dst_u += 1;
			dst_v += 1;
		}
		src_s += stride_s * 2 - w * 6;
		dst_y += stride_y * 2 - w * 2;
		dst_u += stride_u - w;
		dst_v += stride_u - w;
	}
}

// w和h必须是2的倍数,并假设u(stride_d[1])和v(stride_d[2])相等
__forceinline void BGRA_YUVA420_F(U8* const dst[4], const int stride_d[4], const void* src, int stride_s, int w, int h)
{
	const int stride_y = stride_d[0];
	const int stride_u = stride_d[1];
	const int stride_a = stride_d[3];
	const U8* src_s = (U8*)src;
	U8* dst_y = (U8*)dst[0];
	U8* dst_u = (U8*)dst[1];
	U8* dst_v = (U8*)dst[2];
	U8* dst_a = (U8*)dst[3];

	for (w /= 2, h /= 2; h > 0; --h)
	{
		U8* dst_v_end = dst_v + w;
		while (dst_v < dst_v_end)
		{
			int b, g, r, u, v;
			b = src_s[0]; g = src_s[1]; r = src_s[2]; dst_a[0] = src_s[3];
			dst_y[0] = BGR_Y(b, g, r); u = BGR_U(b, g, r); v = BGR_V(b, g, r);
			b = src_s[4]; g = src_s[5]; r = src_s[6]; dst_a[1] = src_s[7];
			dst_y[1] = BGR_Y(b, g, r); u += BGR_U(b, g, r); v += BGR_V(b, g, r);
			b = src_s[stride_s]; g = src_s[stride_s + 1]; r = src_s[stride_s + 2]; dst_a[stride_a] = src_s[stride_s + 3];
			dst_y[stride_y] = BGR_Y(b, g, r); u += BGR_U(b, g, r); v += BGR_V(b, g, r);
			b = src_s[stride_s + 4]; g = src_s[stride_s + 5]; r = src_s[stride_s + 6]; dst_a[stride_a + 1] = src_s[stride_s + 7];
			dst_y[stride_y + 1] = BGR_Y(b, g, r); u += BGR_U(b, g, r); v += BGR_V(b, g, r);
			dst_u[0] = (U8)((u + 2) / 4);
			dst_v[0] = (U8)((v + 2) / 4);

			src_s += 8;
			dst_y += 2;
			dst_u += 1;
			dst_v += 1;
			dst_a += 2;
		}
		src_s += stride_s * 2 - w * 8;
		dst_y += stride_y * 2 - w * 2;
		dst_u += stride_u - w;
		dst_v += stride_u - w;
		dst_a += stride_a * 2 - w * 2;
	}
}

__forceinline void BGR_YUV444_F(U8* const dst[3], const int stride_d[3], const void* src, int stride_s, int w, int h)
{
	const U8* src_s = (U8*)src;
	U8* dst_y = (U8*)dst[0];
	U8* dst_u = (U8*)dst[1];
	U8* dst_v = (U8*)dst[2];
	for (; h > 0; --h)
	{
		U8* dst_v_end = dst_v + w;
		while (dst_v < dst_v_end)
		{
			int b = src_s[0];
			int g = src_s[1];
			int r = src_s[2];
			src_s += 3;
			*dst_y++ = BGR_Y(b, g, r);
			*dst_u++ = BGR_U(b, g, r);
			*dst_v++ = BGR_V(b, g, r);
		}
		src_s += stride_s - w * 3;
		dst_y += stride_d[0] - w;
		dst_u += stride_d[1] - w;
		dst_v += stride_d[2] - w;
	}
}

__forceinline void BGRA_YUVA444_F(U8* const dst[4], const int stride_d[4], const void* src, int stride_s, int w, int h)
{
	const U8* src_s = (U8*)src;
	U8* dst_y = (U8*)dst[0];
	U8* dst_u = (U8*)dst[1];
	U8* dst_v = (U8*)dst[2];
	U8* dst_a = (U8*)dst[3];
	for (; h > 0; --h)
	{
		U8* dst_v_end = dst_v + w;
		while (dst_v < dst_v_end)
		{
			int b = src_s[0];
			int g = src_s[1];
			int r = src_s[2];
			*dst_a++ = src_s[3]; src_s += 4;
			*dst_y++ = BGR_Y(b, g, r);
			*dst_u++ = BGR_U(b, g, r);
			*dst_v++ = BGR_V(b, g, r);
		}
		src_s += stride_s - w * 4;
		dst_y += stride_d[0] - w;
		dst_u += stride_d[1] - w;
		dst_v += stride_d[2] - w;
		dst_a += stride_d[3] - w;
	}
}

// w和h必须是2的倍数,并假设u(stride_s[1])和v(stride_s[2])相等. 注意uv通道会边缘越界一字节
__forceinline void YUV420_BGR_F(void* dst, int stride_d, const U8* const src[3], const int stride_s[3], int w, int h)
{
	const int stride_y = stride_s[0];
	const int stride_u = stride_s[1];
	const U8* src_y = (U8*)src[0];
	const U8* src_u = (U8*)src[1] - stride_u;
	const U8* src_v = (U8*)src[2] - stride_u;
	U8* dst_d = (U8*)dst;

	for (w /= 2, h /= 2; h > 0; --h)
	{
		int u0, v0, u1, v1, y, u, v;
		const U8* src_v_end;

		u0 = src_u[-1] + src_u[stride_u - 1] * 3;
		v0 = src_v[-1] + src_v[stride_u - 1] * 3;
		for (src_v_end = src_v + w; src_v < src_v_end;)
		{
			u1 = src_u[0] + src_u[stride_u] * 3;
			v1 = src_v[0] + src_v[stride_u] * 3;
			u = (u0 + u1 * 3 + 8) / 16;
			v = (v0 + v1 * 3 + 8) / 16;
			y = src_y[0];
			dst_d[0] = YUV_B(y, u, v);
			dst_d[1] = YUV_G(y, u, v);
			dst_d[2] = YUV_R(y, u, v);
			u0 = src_u[1] + src_u[stride_u + 1] * 3;
			v0 = src_v[1] + src_v[stride_u + 1] * 3;
			u = (u0 + u1 * 3 + 8) / 16;
			v = (v0 + v1 * 3 + 8) / 16;
			y = src_y[1];
			dst_d[3] = YUV_B(y, u, v);
			dst_d[4] = YUV_G(y, u, v);
			dst_d[5] = YUV_R(y, u, v);

			u0 = u1;
			v0 = v1;
			src_y += 2;
			src_u += 1;
			src_v += 1;
			dst_d += 6;
		}

		src_y += stride_y - w * 2;
		src_u -= w;
		src_v -= w;
		dst_d += stride_d - w * 6;

		u0 = src_u[stride_u - 1] * 3 + src_u[stride_u * 2 - 1];
		v0 = src_v[stride_u - 1] * 3 + src_v[stride_u * 2 - 1];
		for (src_v_end = src_v + w; src_v < src_v_end;)
		{
			u1 = src_u[stride_u] * 3 + src_u[stride_u * 2];
			v1 = src_v[stride_u] * 3 + src_v[stride_u * 2];
			u = (u0 + u1 * 3 + 8) / 16;
			v = (v0 + v1 * 3 + 8) / 16;
			y = src_y[0];
			dst_d[0] = YUV_B(y, u, v);
			dst_d[1] = YUV_G(y, u, v);
			dst_d[2] = YUV_R(y, u, v);

			u0 = src_u[stride_u + 1] * 3 + src_u[stride_u * 2 + 1];
			v0 = src_v[stride_u + 1] * 3 + src_v[stride_u * 2 + 1];
			u = (u0 + u1 * 3 + 8) / 16;
			v = (v0 + v1 * 3 + 8) / 16;
			y = src_y[1];
			dst_d[3] = YUV_B(y, u, v);
			dst_d[4] = YUV_G(y, u, v);
			dst_d[5] = YUV_R(y, u, v);

			u0 = u1;
			v0 = v1;
			src_y += 2;
			src_u += 1;
			src_v += 1;
			dst_d += 6;
		}

		src_y += stride_y - w * 2;
		src_u += stride_u - w;
		src_v += stride_u - w;
		dst_d += stride_d - w * 6;
	}
}

// w和h必须是2的倍数,并假设u(stride_s[1])和v(stride_s[2])相等
__forceinline void YUVA420_BGRA_F(void* dst, int stride_d, const U8* const src[4], const int stride_s[4], int w, int h)
{
	const int stride_y = stride_s[0];
	const int stride_u = stride_s[1];
	const int stride_a = stride_s[3];
	const U8* src_y = (U8*)src[0];
	const U8* src_u = (U8*)src[1] - stride_u;
	const U8* src_v = (U8*)src[2] - stride_u;
	const U8* src_a = (U8*)src[3];
	U8* dst_d = (U8*)dst;

	for (w /= 2, h /= 2; h > 0; --h)
	{
		int u0, v0, u1, v1, y, u, v;
		const U8* src_v_end;

		u0 = src_u[-1] + src_u[stride_u - 1] * 3;
		v0 = src_v[-1] + src_v[stride_u - 1] * 3;
		for (src_v_end = src_v + w; src_v < src_v_end;)
		{
			u1 = src_u[0] + src_u[stride_u] * 3;
			v1 = src_v[0] + src_v[stride_u] * 3;
			u = (u0 + u1 * 3 + 8) / 16;
			v = (v0 + v1 * 3 + 8) / 16;
			y = src_y[0];
			dst_d[0] = YUV_B(y, u, v);
			dst_d[1] = YUV_G(y, u, v);
			dst_d[2] = YUV_R(y, u, v);
			dst_d[3] = src_a[0];

			u0 = src_u[1] + src_u[stride_u + 1] * 3;
			v0 = src_v[1] + src_v[stride_u + 1] * 3;
			u = (u0 + u1 * 3 + 8) / 16;
			v = (v0 + v1 * 3 + 8) / 16;
			y = src_y[1];
			dst_d[4] = YUV_B(y, u, v);
			dst_d[5] = YUV_G(y, u, v);
			dst_d[6] = YUV_R(y, u, v);
			dst_d[7] = src_a[1];

			u0 = u1;
			v0 = v1;
			src_y += 2;
			src_u += 1;
			src_v += 1;
			src_a += 2;
			dst_d += 8;
		}

		src_y += stride_y - w * 2;
		src_u -= w;
		src_v -= w;
		src_a += stride_a - w * 2;
		dst_d += stride_d - w * 8;

		u0 = src_u[stride_u - 1] * 3 + src_u[stride_u * 2 - 1];
		v0 = src_v[stride_u - 1] * 3 + src_v[stride_u * 2 - 1];
		for (src_v_end = src_v + w; src_v < src_v_end;)
		{
			u1 = src_u[stride_u] * 3 + src_u[stride_u * 2];
			v1 = src_v[stride_u] * 3 + src_v[stride_u * 2];
			u = (u0 + u1 * 3 + 8) / 16;
			v = (v0 + v1 * 3 + 8) / 16;
			y = src_y[0];
			dst_d[0] = YUV_B(y, u, v);
			dst_d[1] = YUV_G(y, u, v);
			dst_d[2] = YUV_R(y, u, v);
			dst_d[3] = src_a[0];

			u0 = src_u[stride_u + 1] * 3 + src_u[stride_u * 2 + 1];
			v0 = src_v[stride_u + 1] * 3 + src_v[stride_u * 2 + 1];
			u = (u0 + u1 * 3 + 8) / 16;
			v = (v0 + v1 * 3 + 8) / 16;
			y = src_y[1];
			dst_d[4] = YUV_B(y, u, v);
			dst_d[5] = YUV_G(y, u, v);
			dst_d[6] = YUV_R(y, u, v);
			dst_d[7] = src_a[1];

			u0 = u1;
			v0 = v1;
			src_y += 2;
			src_u += 1;
			src_v += 1;
			src_a += 2;
			dst_d += 8;
		}

		src_y += stride_y - w * 2;
		src_u += stride_u - w;
		src_v += stride_u - w;
		src_a += stride_a - w * 2;
		dst_d += stride_d - w * 8;
	}
}

// w和h必须是2的倍数,并假设u(stride_s[1])和v(stride_s[2])相等. 注意uv通道会边缘越界一字节
__forceinline void YUV420_BGRA_F(void* dst, int stride_d, const U8* const src[3], const int stride_s[3], int w, int h)
{
	const int stride_y = stride_s[0];
	const int stride_u = stride_s[1];
	const U8* src_y = (U8*)src[0];
	const U8* src_u = (U8*)src[1] - stride_u;
	const U8* src_v = (U8*)src[2] - stride_u;
	U8* dst_d = (U8*)dst;

	for (w /= 2, h /= 2; h > 0; --h)
	{
		int u0, v0, u1, v1, y, u, v;
		const U8* src_v_end;

		u0 = src_u[-1] + src_u[stride_u - 1] * 3;
		v0 = src_v[-1] + src_v[stride_u - 1] * 3;
		for (src_v_end = src_v + w; src_v < src_v_end;)
		{
			u1 = src_u[0] + src_u[stride_u] * 3;
			v1 = src_v[0] + src_v[stride_u] * 3;
			u = (u0 + u1 * 3 + 8) / 16;
			v = (v0 + v1 * 3 + 8) / 16;
			y = src_y[0];
			dst_d[0] = YUV_B(y, u, v);
			dst_d[1] = YUV_G(y, u, v);
			dst_d[2] = YUV_R(y, u, v);

			u0 = src_u[1] + src_u[stride_u + 1] * 3;
			v0 = src_v[1] + src_v[stride_u + 1] * 3;
			u = (u0 + u1 * 3 + 8) / 16;
			v = (v0 + v1 * 3 + 8) / 16;
			y = src_y[1];
			dst_d[4] = YUV_B(y, u, v);
			dst_d[5] = YUV_G(y, u, v);
			dst_d[6] = YUV_R(y, u, v);

			u0 = u1;
			v0 = v1;
			src_y += 2;
			src_u += 1;
			src_v += 1;
			dst_d += 8;
		}

		src_y += stride_y - w * 2;
		src_u -= w;
		src_v -= w;
		dst_d += stride_d - w * 8;

		u0 = src_u[stride_u - 1] * 3 + src_u[stride_u * 2 - 1];
		v0 = src_v[stride_u - 1] * 3 + src_v[stride_u * 2 - 1];
		for (src_v_end = src_v + w; src_v < src_v_end;)
		{
			u1 = src_u[stride_u] * 3 + src_u[stride_u * 2];
			v1 = src_v[stride_u] * 3 + src_v[stride_u * 2];
			u = (u0 + u1 * 3 + 8) / 16;
			v = (v0 + v1 * 3 + 8) / 16;
			y = src_y[0];
			dst_d[0] = YUV_B(y, u, v);
			dst_d[1] = YUV_G(y, u, v);
			dst_d[2] = YUV_R(y, u, v);

			u0 = src_u[stride_u + 1] * 3 + src_u[stride_u * 2 + 1];
			v0 = src_v[stride_u + 1] * 3 + src_v[stride_u * 2 + 1];
			u = (u0 + u1 * 3 + 8) / 16;
			v = (v0 + v1 * 3 + 8) / 16;
			y = src_y[1];
			dst_d[4] = YUV_B(y, u, v);
			dst_d[5] = YUV_G(y, u, v);
			dst_d[6] = YUV_R(y, u, v);

			u0 = u1;
			v0 = v1;
			src_y += 2;
			src_u += 1;
			src_v += 1;
			dst_d += 8;
		}

		src_y += stride_y - w * 2;
		src_u += stride_u - w;
		src_v += stride_u - w;
		dst_d += stride_d - w * 8;
	}
}

__forceinline void YUV444_BGR_F(void* dst, int stride_d, const U8* const src[3], const int stride_s[3], int w, int h)
{
	const U8* src_y = (U8*)src[0];
	const U8* src_u = (U8*)src[1];
	const U8* src_v = (U8*)src[2];
	U8* dst_d = (U8*)dst;
	for (; h > 0; --h)
	{
		const U8* src_v_end = src_v + w;
		while (src_v < src_v_end)
		{
			int y = *src_y++;
			int u = *src_u++;
			int v = *src_v++;
			dst_d[0] = YUV_B(y, u, v);
			dst_d[1] = YUV_G(y, u, v);
			dst_d[2] = YUV_R(y, u, v);
			dst_d += 3;
		}
		src_y += stride_s[0] - w;
		src_u += stride_s[1] - w;
		src_v += stride_s[2] - w;
		dst_d += stride_d - w * 3;
	}
}

__forceinline void YUVA444_BGRA_F(void* dst, int stride_d, const U8* const src[4], const int stride_s[4], int w, int h)
{
	const U8* src_y = (U8*)src[0];
	const U8* src_u = (U8*)src[1];
	const U8* src_v = (U8*)src[2];
	const U8* src_a = (U8*)src[3];
	U8* dst_d = (U8*)dst;
	for (; h > 0; --h)
	{
		const U8* src_v_end = src_v + w;
		while (src_v < src_v_end)
		{
			int y = *src_y++;
			int u = *src_u++;
			int v = *src_v++;
			dst_d[0] = YUV_B(y, u, v);
			dst_d[1] = YUV_G(y, u, v);
			dst_d[2] = YUV_R(y, u, v);
			dst_d[3] = *src_a++;
			dst_d += 4;
		}
		src_y += stride_s[0] - w;
		src_u += stride_s[1] - w;
		src_v += stride_s[2] - w;
		src_a += stride_s[3] - w;
		dst_d += stride_d - w * 4;
	}
}

__forceinline void YUV444_BGRA_F(void* dst, int stride_d, const U8* const src[3], const int stride_s[3], int w, int h)
{
	const U8* src_y = (U8*)src[0];
	const U8* src_u = (U8*)src[1];
	const U8* src_v = (U8*)src[2];
	U8* dst_d = (U8*)dst;
	for (; h > 0; --h)
	{
		const U8* src_v_end = src_v + w;
		while (src_v < src_v_end)
		{
			int y = *src_y++;
			int u = *src_u++;
			int v = *src_v++;
			dst_d[0] = YUV_B(y, u, v);
			dst_d[1] = YUV_G(y, u, v);
			dst_d[2] = YUV_R(y, u, v);
			dst_d += 4;
		}
		src_y += stride_s[0] - w;
		src_u += stride_s[1] - w;
		src_v += stride_s[2] - w;
		dst_d += stride_d - w * 4;
	}
}

// w和h必须是2的倍数,并假设u(stride_s[1])和v(stride_s[2])相等. 注意uv通道会边缘越界一字节
__forceinline void YUV420_BGR_F_slow(void* dst, int stride_d, const U8* const src[3], const int stride_s[3], int w, int h)
{
	const int stride_y = stride_s[0];
	const int stride_u = stride_s[1];
	const U8* src_y = (U8*)src[0];
	const U8* src_u = (U8*)src[1] - stride_u - 1;
	const U8* src_v = (U8*)src[2] - stride_u - 1;
	U8* dst_d = (U8*)dst;
	for (w /= 2, h /= 2; h > 0; --h)
	{
		const U8* src_v_end = src_v + w;
		while (src_v < src_v_end)
		{
			int u0, u1, u2, u3, u4, u5;
			int 	v1, v2, v3, v5;
			int 	y1, y2, y3, y5;

			u0 = 9 * src_u[stride_u + 1];
			u1 = 3 * src_u[1];
			u2 = 3 * src_u[stride_u];
			u3 = 3 * src_u[stride_u + 2];
			u4 = 3 * src_u[stride_u * 2 + 1];
			u5 = (u0 + u1 + u2 + src_u[0] + 8) / 16;
			u1 = (u0 + u1 + u3 + src_u[2] + 8) / 16;
			u2 = (u0 + u2 + u4 + src_u[stride_u * 2] + 8) / 16;
			u3 = (u0 + u3 + u4 + src_u[stride_u * 2 + 2] + 8) / 16;

			u0 = 9 * src_v[stride_u + 1];
			v1 = 3 * src_v[1];
			v2 = 3 * src_v[stride_u];
			v3 = 3 * src_v[stride_u + 2];
			u4 = 3 * src_v[stride_u * 2 + 1];
			v5 = (u0 + v1 + v2 + src_v[0] + 8) / 16;
			v1 = (u0 + v1 + v3 + src_v[2] + 8) / 16;
			v2 = (u0 + v2 + u4 + src_v[stride_u * 2] + 8) / 16;
			v3 = (u0 + v3 + u4 + src_v[stride_u * 2 + 2] + 8) / 16;

			y5 = src_y[0];
			y1 = src_y[1];
			y2 = src_y[stride_y];
			y3 = src_y[stride_y + 1];

			dst_d[0] = YUV_B(y5, u5, v5);
			dst_d[1] = YUV_G(y5, u5, v5);
			dst_d[2] = YUV_R(y5, u5, v5);
			dst_d[3] = YUV_B(y1, u1, v1);
			dst_d[4] = YUV_G(y1, u1, v1);
			dst_d[5] = YUV_R(y1, u1, v1);
			dst_d[stride_d] = YUV_B(y2, u2, v2);
			dst_d[stride_d + 1] = YUV_G(y2, u2, v2);
			dst_d[stride_d + 2] = YUV_R(y2, u2, v2);
			dst_d[stride_d + 3] = YUV_B(y3, u3, v3);
			dst_d[stride_d + 4] = YUV_G(y3, u3, v3);
			dst_d[stride_d + 5] = YUV_R(y3, u3, v3);

			src_u += 1;
			src_v += 1;
			src_y += 2;
			dst_d += 6;
		}

		src_y += stride_y * 2 - w * 2;
		src_u += stride_u - w;
		src_v += stride_u - w;
		dst_d += stride_d * 2 - w * 6;
	}
}

// w和h必须是2的倍数,并假设u(stride_s[1])和v(stride_s[2])相等. 注意uv通道会边缘越界一字节
__forceinline void YUVA420_BGRA_F_slow(void* dst, int stride_d, const U8* const src[3], const int stride_s[3], int w, int h)
{
	const int stride_y = stride_s[0];
	const int stride_u = stride_s[1];
	const int stride_a = stride_s[3];
	const U8* src_y = (U8*)src[0];
	const U8* src_u = (U8*)src[1] - stride_u - 1;
	const U8* src_v = (U8*)src[2] - stride_u - 1;
	const U8* src_a = (U8*)src[3];
	U8* dst_d = (U8*)dst;
	for (w /= 2, h /= 2; h > 0; --h)
	{
		const U8* src_v_end = src_v + w;
		while (src_v < src_v_end)
		{
			int u0, u1, u2, u3, u4, u5;
			int v0, v1, v2, v3, v4, v5;
			int y5, y1, y2, y3;

			u0 = 9 * src_u[stride_u + 1];
			u1 = 3 * src_u[1];
			u2 = 3 * src_u[stride_u];
			u3 = 3 * src_u[stride_u + 2];
			u4 = 3 * src_u[stride_u * 2 + 1];
			u5 = (u0 + u1 + u2 + src_u[0] + 8) / 16;
			u1 = (u0 + u1 + u3 + src_u[2] + 8) / 16;
			u2 = (u0 + u2 + u4 + src_u[stride_u * 2] + 8) / 16;
			u3 = (u0 + u3 + u4 + src_u[stride_u * 2 + 2] + 8) / 16;

			v0 = 9 * src_v[stride_u + 1];
			v1 = 3 * src_v[1];
			v2 = 3 * src_v[stride_u];
			v3 = 3 * src_v[stride_u + 2];
			v4 = 3 * src_v[stride_u * 2 + 1];
			v5 = (v0 + v1 + v2 + src_v[0] + 8) / 16;
			v1 = (v0 + v1 + v3 + src_v[2] + 8) / 16;
			v2 = (v0 + v2 + v4 + src_v[stride_u * 2] + 8) / 16;
			v3 = (v0 + v3 + v4 + src_v[stride_u * 2 + 2] + 8) / 16;

			y5 = src_y[0];
			y1 = src_y[1];
			y2 = src_y[stride_y];
			y3 = src_y[stride_y + 1];

			dst_d[0] = YUV_B(y5, u5, v5);
			dst_d[1] = YUV_G(y5, u5, v5);
			dst_d[2] = YUV_R(y5, u5, v5);
			dst_d[3] = src_a[0];
			dst_d[4] = YUV_B(y1, u1, v1);
			dst_d[5] = YUV_G(y1, u1, v1);
			dst_d[6] = YUV_R(y1, u1, v1);
			dst_d[3] = src_a[1];
			dst_d[stride_d] = YUV_B(y2, u2, v2);
			dst_d[stride_d + 1] = YUV_G(y2, u2, v2);
			dst_d[stride_d + 2] = YUV_R(y2, u2, v2);
			dst_d[stride_d + 3] = src_a[stride_a];
			dst_d[stride_d + 4] = YUV_B(y3, u3, v3);
			dst_d[stride_d + 5] = YUV_G(y3, u3, v3);
			dst_d[stride_d + 6] = YUV_R(y3, u3, v3);
			dst_d[stride_d + 7] = src_a[stride_a + 1];

			src_u += 1;
			src_v += 1;
			src_y += 2;
			dst_d += 8;
		}

		src_y += stride_y * 2 - w * 2;
		src_u += stride_u - w;
		src_v += stride_u - w;
		src_a += stride_a * 2 - w * 2;
		dst_d += stride_d * 2 - w * 8;
	}
}

#undef BGR_Y
#undef BGR_U
#undef BGR_V
#undef YUV_B
#undef YUV_G
#undef YUV_R

// 生成一像素边框,w和h最小为1
__forceinline void MakeYUV420Frame(U8* dst, int w, int h, int stride)
{
	int y; U8* p = dst;
	for (y = 0; y < h; ++y)
	{
		p[-1] = p[0];
		p[w] = p[w - 1];
		p += stride;
	}
	memcpy(dst - stride - 1, dst - 1, w + 2);
	memcpy(dst + stride*h - 1, dst + stride*h - stride - 1, w + 2);
}
// 把紧凑YUV420数据中UV通道转换成有1像素边框的数据,w和h必须是2的倍数,w和h最小为2
__forceinline U8* CreateYUV420Frame(const U8* src, int w, int h)
{
	int wh = w*h, ww = w / 2, hh = h / 2, ww2 = ww + 2, hh2 = hh + 2, ww2hh2 = ww2*hh2, y;
	U8* dst = (U8*)malloc(wh + ww2hh2 * 2), *db = dst + w + 3; const U8* sb;
	memcpy(dst, src, wh); // Y通道
	db = dst + wh + ww2 + 1; sb = src + wh;
	for (y = 0; y < hh; ++y) // U通道
		memcpy(db + y*ww2, sb + y*ww, ww);
	db += ww2hh2; sb += wh / 4;
	for (y = 0; y < hh; ++y) // V通道
		memcpy(db + y*ww2, sb + y*ww, ww);
	MakeYUV420Frame(dst + wh + ww2 + 1, ww, hh, ww2);
	MakeYUV420Frame(dst + wh + ww2hh2 + ww2 + 1, ww, hh, ww2);
	return dst;
}
