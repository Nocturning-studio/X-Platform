#pragma once

class CRandom
{
  private:
	volatile s32 holdrand;

  public:
	CRandom() : holdrand(1){};
	CRandom(s32 _seed) : holdrand(_seed){};

	IC void seed(s32 val)
	{
		holdrand = val;
	}
	IC s32 maxI()
	{
		return 32767;
	}

	ICN s32 randI()
	{
		holdrand++;
		return (((holdrand = holdrand * 214013L + 2531011L) >> 16) & 0x7fff);
	}
	IC s32 randI(s32 max)
	{
		holdrand++;
		return randI() % max;
	}
	IC s32 randI(s32 min, s32 max)
	{
		holdrand++;
		return min + randI(max - min);
	}
	IC s32 randIs(s32 range)
	{
		holdrand++;
		return randI(-range, range);
	}
	IC s32 randIs(s32 range, s32 offs)
	{
		holdrand++;
		return offs + randIs(range);
	}

	IC float maxF()
	{
		return 32767.f;
	}
	IC float randF()
	{
		holdrand++;
		return float(randI()) / maxF();
	}
	IC float randF(float max)
	{
		holdrand++;
		return randF() * max;
	}
	IC float randF(float min, float max)
	{
		holdrand++;
		return min + randF(max - min);
	}
	IC float randFs(float range)
	{
		holdrand++;
		return randF(-range, range);
	}
	IC float randFs(float range, float offs)
	{
		holdrand++;
		return offs + randFs(range);
	}
};

XRMATH_API extern CRandom Random;
