#pragma once
#include <vector>
#include <math.h>

using namespace std;

class PerlinNoise {
public:
	vector<int> p;

	PerlinNoise();

	PerlinNoise(unsigned int seed);

	double noise(double x, double y, double z);

private:

	double fage(double t);

	double lerp(double t, double a, double b);

	double grad(int hash, double x, double y, double z);
};