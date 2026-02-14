#include "pch.h"
#include "math_compress_normal.h"
#include "math_bitwise.h" // дл€ negative, set_positive, set_negative, iFloor

XRAY_BEGIN

// маски битов (верхние 3 бита Ц знаки, далее 6 бит Ц xbits, 7 бит Ц ybits)
static const u16 SIGN_MASK = 0xe000;
static const u16 XSIGN_MASK = 0x8000;
static const u16 YSIGN_MASK = 0x4000;
static const u16 ZSIGN_MASK = 0x2000;
static const u16 TOP_MASK = 0x1f80;	   // биты 7-12 (6 бит)
static const u16 BOTTOM_MASK = 0x007f; // биты 0-6 (7 бит)

// таблица предвычисленных множителей дл€ декомпрессии (индекс 13 бит)
static float uv_adjustment[0x2000];

void init_normal_compression_table()
{
	for (int idx = 0; idx < 0x2000; ++idx)
	{
		int xbits = idx >> 7;		   // старшие 6 бит (0-126)
		int ybits = idx & BOTTOM_MASK; // младшие 7 бит (0-127)

		// отображаем обратно в треугольник (0,0)-(0,127)-(127,0)
		if (xbits + ybits >= 127)
		{
			xbits = 127 - xbits;
			ybits = 127 - ybits;
		}

		float x = float(xbits);
		float y = float(ybits);
		float z = float(126 - xbits - ybits);

		// длина вектора до нормализации
		float len = std::sqrt(x * x + y * y + z * z);
		uv_adjustment[idx] = (len > 0) ? (1.0f / len) : 0.0f;
	}
}

XRMATH_API u16 compress_normal(const float3& vec)
{
	float3 tmp = vec;

	u16 code = 0;

	if (negative(tmp.x))
	{
		code |= XSIGN_MASK;
		set_positive(tmp.x);
	}
	if (negative(tmp.y))
	{
		code |= YSIGN_MASK;
		set_positive(tmp.y);
	}
	if (negative(tmp.z))
	{
		code |= ZSIGN_MASK;
		set_positive(tmp.z);
	}

	// проекци€ на плоскость x+y+z = 1 (точнее, x+y+z = 1? в оригинале 126/(x+y+z))
	float sum = tmp.x + tmp.y + tmp.z;
	float w = 126.0f / sum; // sum > 0, так как все компоненты положительны

	int xbits = iFloor(tmp.x * w);
	int ybits = iFloor(tmp.y * w);

	// ќграничиваем (на вс€кий случай) Ц в оригинале есть VERIFY, но мы можем просто обрезать
	if (xbits < 0)
		xbits = 0;
	if (xbits > 126)
		xbits = 126;
	if (ybits < 0)
		ybits = 0;
	if (ybits > 126)
		ybits = 126;

	// преобразование треугольника в пр€моугольник дл€ упаковки
	if (xbits >= 64)
	{
		xbits = 127 - xbits;
		ybits = 127 - ybits;
	}

	code |= (xbits << 7);
	code |= ybits;

	return code;
}

void decompress_normal(float3& vec, u16 code)
{
	int xbits = (code & TOP_MASK) >> 7;
	int ybits = (code & BOTTOM_MASK);

	// обратное преобразование к треугольнику
	if (xbits + ybits >= 127)
	{
		xbits = 127 - xbits;
		ybits = 127 - ybits;
	}

	// нормализаци€ с помощью предвычисленного коэффициента
	float adj = uv_adjustment[code & ~SIGN_MASK]; // очищаем знаковые биты (индекс 0..0x1FFF)

	vec.x = adj * float(xbits);
	vec.y = adj * float(ybits);
	vec.z = adj * float(126 - xbits - ybits);

	if (code & XSIGN_MASK)
		set_negative(vec.x);
	if (code & YSIGN_MASK)
		set_negative(vec.y);
	if (code & ZSIGN_MASK)
		set_negative(vec.z);
}

XRAY_END
