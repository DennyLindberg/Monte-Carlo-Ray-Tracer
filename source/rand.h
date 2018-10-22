#pragma once
#include <random>
#include <stdint.h>

class UniformRandomGenerator
{
protected:
	/*
		Xorshift
		https://stackoverflow.com/questions/35358501/what-is-performance-wise-the-best-way-to-generate-random-bools
	*/
	std::random_device rd;
	uint64_t xorseed[2] = { (uint64_t(rd()) << 32) ^ (rd()), (uint64_t(rd()) << 32) ^ (rd()) };

	static inline double to_double(uint64_t x) 
	{
		const union 
		{ 
			uint64_t i; 
			double d; 
		} u = { UINT64_C(0x3FF) << 52 | x >> 12 };

		return u.d - 1.0;
	}

public:
	UniformRandomGenerator()
	{
		//std::mt19937 mt(rd());
		//std::uniform_real_distribution<double> dist(1.0, 10.0);
	}

	// xorshift128plus
	inline uint64_t RandomInt()
	{
		uint64_t x = xorseed[0];
		uint64_t const y = xorseed[1];
		xorseed[0] = y;
		x ^= x << 23; // a
		xorseed[1] = x ^ y ^ (x >> 17) ^ (y >> 26); // b, c
		return xorseed[1] + y;
	}

	inline double RandomDouble()
	{
		return to_double(RandomInt());
	}

	inline double RandomDouble(double min, double max)
	{
		return min + (max - min) * to_double(RandomInt());
	}
	
	inline float RandomFloat()
	{
		return float(RandomDouble());
	}

	inline float RandomFloat(float min, float max)
	{
		return min + (max - min) * float(RandomDouble());
	}
};