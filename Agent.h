#include "SFML\Graphics.hpp"
#include <math.h>
#pragma once

using namespace std;
using namespace sf;

struct Vector3d {
    double x;
    double y;
    double z;
    Vector3d() : x(0), y(0), z(0) {};
    Vector3d(double x, double y, double z) : x(x), y(y), z(z) {};
};

struct Vector2d {
    double x;
    double y;
    Vector2d(double x, double y) : x(x), y(y) {};
};

class Agent {
public:

    inline static const float pi = 3.141592653589793f;

    inline static float speed = 1.0f;
    inline static float maxTurn = pi / 180.0f;
    inline static float biasFactor = 2.0f;
    inline static float searchSize = 10;
    inline static float searchAngle = pi / 6;

    inline static float alternate = -1.0f;
    inline static float globalPeriodR = 9.0f;
    inline static float globalPeriodG = 10.0f;
    inline static float globalPeriodB = 11.0f;

    inline static float distAlternate = -1.0f;
    inline static float distR = 100.0f;
    inline static float distG = 150.0f;
    inline static float distB = 200.0f;
    inline static float distPeriodR = 9.0f;
    inline static float distPeriodG = 10.0f;
    inline static float distPeriodB = 11.0f;

    inline static Vector3f palette = Vector3f(1.0f, 1.0f, 1.0f);

    inline static int width = 800;
    inline static int height = 450;

    bool debug = false;

    Agent();

    void updateColor();

    void alternateColor(float time);

    Vector2f getPos();

    void updatePos();

    void updateDir(Image& im);

private:

    //agent properties
    Vector2f pos;
    Vector2f vel;
    float dir;
    Color color;
    Vector3d colorBase;

    Vector3d v3d(Vector3i v);

    float cosfRatio(float val);

    float cosfRatio(float val, float offset, float scale);

    void alterDir(float delta);

    bool edgeFunc(Vector2i a, Vector2i b, Vector2i c);

    float compare(Vector3f a, Vector3f b);

    Vector3f norm(Vector3f v);

    Vector3f getAvgColor(Image& im, float dist, float dirDelta);
};