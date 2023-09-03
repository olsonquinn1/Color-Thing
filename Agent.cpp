#include "Agent.h"
#include "SFML\Graphics.hpp"
#include <random>
#include <vector>
#include <math.h>
#include <iostream>

using namespace std;
using namespace sf;

//public members
Agent::Agent() {
    //spawn agent with a random position, velocity, and base color
    pos.x = rand() % width;
    pos.y = rand() % height;
    dir = (float)(rand() % 10000) / 10000 * 2 * pi;
    vel = Vector2f(cos(dir), sin(dir));

    colorBase = Vector3d(
        (double)(rand() % 206 + 50),
        (double)(rand() % 206 + 50),
        (double)(rand() % 206 + 50)
    );

    updateColor();
}

void Agent::updateColor() {
    color.r = colorBase.x * palette.x;
    color.g = colorBase.y * palette.y;
    color.b = colorBase.z * palette.z;
}

void Agent::alternateColor(float time) {

    Vector2f center(width / 2, height / 2);
    float dist = sqrt(pow(center.x - pos.x, 2) + pow(center.y - pos.y, 2));

    color.r = colorBase.x * palette.x;
    color.g = colorBase.y * palette.y;
    color.b = colorBase.z * palette.z;

    if (alternate > 0) {
        if (globalPeriodR != 0)
            color.r *= cosfRatio(
                time / (globalPeriodR * 1000)
            );
        if (globalPeriodG != 0)
            color.g *= cosfRatio(
                time / (globalPeriodG * 1000)
            );
        if (globalPeriodB != 0)
            color.b *= cosfRatio(
                time / (globalPeriodB * 1000)
            );
    }

    if (distAlternate > 0) {
        if(distR != 0 && distPeriodR != 0)
            color.r *= cosfRatio(
                dist / distR / cosfRatio(
                    time / (distPeriodR * 1000)
                )
            );
        if (distG != 0 && distPeriodG != 0)
            color.g *= cosfRatio(
                dist / distG / cosfRatio(
                    time / (distPeriodG * 1000)
                )
            );
        if (distB != 0 && distPeriodB != 0)
            color.b *= cosfRatio(
                dist / distB / cosfRatio(
                    time / (distPeriodB * 1000)
                )
            );
    }
}

Vector2f Agent::getPos() { return pos; }

void Agent::updatePos() {

    pos.x += vel.x * speed;
    pos.y += vel.y * speed;

    //wrap agent positions around screen
    if (pos.x < 0)
        pos.x = width - 1;
    else if (pos.x >= width)
        pos.x = 0;
    if (pos.y < 0)
        pos.y = height - 1;
    else if (pos.y >= height)
        pos.y = 0;
}

void Agent::updateDir(Image& im) {

    im.setPixel(pos.x, pos.y, color);

    //determine avg color in zones in front, left, and right of agent
    float bias = 0;

    Vector3f me = Vector3f(color.r, color.g, color.b);

    //normalize average color to ignore color intensity when deciding which way to go
    Vector3f l = norm(getAvgColor(im, searchSize, dir + searchAngle / 2));
    Vector3f f = norm(getAvgColor(im, searchSize, dir));
    Vector3f r = norm(getAvgColor(im, searchSize, dir - searchAngle / 2));

    float lval = compare(me, l);
    float rval = compare(me, r);
    float mval = compare(me, f);
    bias = (lval - rval) * (1 - mval);
    if (debug) {
        cout << "l: " << lval << "\tr: " << rval << "\tm: " << mval << "\tbias: " << bias << "\n";
    }
    //biasFactor affects how heavily the bias affects the decision
    bias = bias * biasFactor;
    float val = (float)(rand() % 2000) / 1000.0f - 1.0f + bias;
    alterDir(
        val * maxTurn
    );
    debug = false;
}

//private members
Vector3f cross(Vector3f a, Vector3f b) { return Vector3f(a.x * b.x, a.y * b.y, a.z * b.z); }
Vector3f cross(Vector3f a, float b) { return Vector3f(a.x * b, a.y * b, a.z * b); }

Vector3d Agent::v3d(Vector3i v) {
    return Vector3d((double) v.x, (double) v.y, (double) v.z);
}

float Agent::cosfRatio(float val) {
    return cosfRatio(val, 0, 1);
}

float Agent::cosfRatio(float val, float offset, float scale) {
    return (cosf(val * 2 * pi) + offset) / scale;
}

void Agent::alterDir(float delta) {
    dir = dir + delta;
    vel = Vector2f(cos(dir), sin(dir));
}

bool Agent::edgeFunc(Vector2i a, Vector2i b, Vector2i c) {
    return (c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x) >= 0;
}

float Agent::compare(Vector3f a, Vector3f b) {
    //const float maxDist = sqrt(3 * pow(255, 2));
    const float maxDist = sqrt(3);
    float val = (
        maxDist - sqrt(
            pow(a.x - b.x, 2) +
            pow(a.y - b.y, 2) +
            pow(a.z - b.z, 2)
        )
    ) / maxDist;
    return val;
}

Vector3f Agent::norm(Vector3f v) {
    float mag = sqrt(pow(v.x, 2) + pow(v.y, 2) + pow(v.z, 2));
    if (false && mag == 0) {
        return Vector3f(0, 0, 0);
    }
    return Vector3f((float)v.x / mag, (float)v.y / mag, (float)v.z / mag);
}

Vector3f Agent::getAvgColor(Image& im, float dist, float dirDelta) {

    vector<Vector2i> points = {
        Vector2i(pos.x, pos.y),
        Vector2i(pos.x + dist * cos(dirDelta + pi / 12), pos.y + dist * sin(dirDelta + pi / 12)),
        Vector2i(pos.x + dist * cos(dirDelta - pi / 12), pos.y + dist * sin(dirDelta - pi / 12))
    };
    /*
    sort(points.begin(), points.end(), [](Vector2i l, Vector2i r) {
        return l.y < r.y;
    });
    */
    Vector2f search[2] = {
        Vector2f(
            min(min(points[0].x, points[1].x), points[2].x),
            min(min(points[0].y, points[1].y), points[2].y)
        ),
        Vector2f(
            max(max(points[0].x, points[1].x), points[2].x),
            max(max(points[0].y, points[1].y), points[2].y)
        )
    };

    int count = 0;
    int avg[3] = { 0, 0, 0 };

    for (int x = search[0].x; x <= search[1].x; x++) {

        if (x < 0 || x >= width) continue;

        for (int y = search[0].y; y <= search[1].y; y++) {

            if (y < 0 || y >= height) continue;

            bool inside =
                edgeFunc(points[0], points[1], Vector2i(x, y))
                && edgeFunc(points[1], points[2], Vector2i(x, y))
                && edgeFunc(points[2], points[0], Vector2i(x, y));

            if (inside) {

                count++;

                Color c = im.getPixel(x, y);

                avg[0] += c.r;
                avg[1] += c.g;
                avg[2] += c.b;
            }
        }
    }
    if (count > 0) {
        avg[0] = (float)avg[0] / count;
        avg[1] = (float)avg[1] / count;
        avg[2] = (float)avg[2] / count;
    }

    return Vector3f(avg[0], avg[1], avg[2]);
}