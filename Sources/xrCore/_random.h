#ifndef _LOCAL_RAND
#define _LOCAL_RAND

class CRandom
{
  private:
	volatile s64 holdrand;

  public:
	CRandom() : holdrand(1){};
	CRandom(s64 _seed) : holdrand(_seed){};

	IC void seed(s64 val)
	{
		holdrand = val;
	}
	IC s64 maxI()
	{
		return 32767;
	}

	ICN s64 randI()
	{
		holdrand++;
		return (((holdrand = holdrand * 214013L + 2531011L) >> 16) & 0x7fff);
	}
	IC s64 randI(s64 max)
	{
		VERIFY(max);
		holdrand++;
		return randI() % max;
	}
	IC s64 randI(s64 min, s64 max)
	{
		holdrand++;
		return min + randI(max - min);
	}
	IC s64 randIs(s64 range)
	{
		holdrand++;
		return randI(-range, range);
	}
	IC s64 randIs(s64 range, s64 offs)
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

XRCORE_API extern CRandom Random;

#endif
