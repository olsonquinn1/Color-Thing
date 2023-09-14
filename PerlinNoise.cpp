#include "PerlinNoise.h"
#include <vector>
#include <math.h>
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <list>
#include <numeric>
#include <random>

PerlinNoise::PerlinNoise(unsigned int seed) {
	int indx = 0;
	p.resize(256);
	generate(p.begin(), p.end(), [&indx] {return indx++; });
}