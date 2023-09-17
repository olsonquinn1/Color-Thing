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

    colorBase = Vector3f(
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
    while (pos.x < 0)
        pos.x += width;
    while(pos.x >= width)
        pos.x -= width;
    while(pos.y < 0)
        pos.y += height;
    while(pos.y >= height)
        pos.y -= height;
}

void Agent::updateDir(Image& im) {

    im.setPixel(pos.x, pos.y, color);

    //determine avg color in zones in front, left, and right of agent
    //treat colors as a 3d vector for comparing
    //normalize average color to ignore color intensity when deciding which way to go

    Vector3f me = norm(Vector3f(color.r, color.g, color.b));
    Vector3f l = norm(getAvgColor(im, searchSize, searchAngle, searchAngle + searchAngleOffset));
    Vector3f f = norm(getAvgColor(im, searchSize, searchAngle, 0));
    Vector3f r = norm(getAvgColor(im, searchSize, searchAngle, -1.0f * searchAngle - searchAngleOffset));

    float lval = compare(me, l);
    float rval = compare(me, r);
    float mval = compare(me, f);
    float bias = (lval - rval) * (1 - mval) * 255;
    //biasFactor affects how heavily the bias affects the decision
    float val = ((float)(rand() % 2000) / 1000.0f - 1.0f) + bias * biasFactor;
    alterDir( val * turnFactor );
    debug = false;
}

void Agent::mandelBrot(float time) {
    float c_re = (pos.x - width / 2.0) * 4.0 / width / 2;
    float c_im = (pos.y - height / 2.0) * 4.0 / height / 2;
    c_re *= 1000.0f / pow(time, 1.5f);
    c_im *= 1000.0f / pow(time, 1.5f);
    c_re += -1.34855;
    c_im += -0.0456763;
    float x = 0, y = 0;
    float max = 500;
    for (float it = 0; it < max; it++) {
        float x_new = x * x - y * y + c_re;
        y = 2 * x * y + c_im;
        x = x_new;
        if (x * x + y * y > 4) {
            //color.r *= it / max * 10;
            color.g *= it / max * 100;
            color.b *= it / max * 50;
            break;
        }
    }
}

//private members
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
    const float maxDist = sqrt(2); //maximum difference between two 3d normalized vectors 
    float val = 1 - (
        sqrt(
            pow(a.x - b.x, 2) +
            pow(a.y - b.y, 2) +
            pow(a.z - b.z, 2)
        )
    ) / maxDist;
    return val;
}

Vector3f Agent::norm(Vector3f v) {
    float mag = sqrt(pow(v.x, 2) + pow(v.y, 2) + pow(v.z, 2));
    if (mag == 0) {
        return Vector3f(1.0f / sqrt(3), 1.0f / sqrt(3), 1.0f / sqrt(3));
    }
    return Vector3f(v.x / mag, v.y / mag, v.z / mag);
}

//rasterization algorithm, but instead of drawing the pixels within the triangle, we are reading them 
//to determine the average color within the area of the triangle

Agent::Edge::Edge(int _x1, int _y1, int _x2, int _y2) {

    if (_y1 < _y2) {
        x1 = _x1;
        y1 = _y1;

        x2 = _x2;
        y2 = _y2;
    } else {
        x1 = _x2;
        y1 = _y2;

        x2 = _x1;
        y2 = _y1;
    }

    dx = x2 - x1;
    dy = y2 - y1;
}

Agent::Span::Span(int _x1, int _x2)
{
    if (_x1 < _x2) {
        x1 = _x1;
        x2 = _x2;
    }
    else {
        x1 = _x2;
        x2 = _x1;
    }

    dx = x2 - x1;
}

void Agent::getSpanSum(Vector3i& sum, int& count, Image& im, const Span& span, int y)
{
    Vector2u size = im.getSize();

    if (span.dx == 0)
        return;

    for (int x = span.x1; x < span.x2; x++) {
        if (x < 0)
            continue;
        if (x >= size.x)
            break;

        count++;
        Color c = im.getPixel(x, y);
        sum += Vector3i(c.r, c.g, c.b);
    }
}

void Agent::getEdgeSum(Vector3i& sum, int& count, Image& im, const Edge& e1, const Edge& e2)
{
    Vector2u size = im.getSize();

    //ignore if either edge is horizontal
    if (e1.dy == 0 || e2.dy == 0)
        return;

    //factors for walking the x values for each edge as we increase y
    float e1Pos = static_cast<float>(e2.y1 - e1.y1) / e1.dy;
    float e1Step = 1.0f / e1.dy;

    float e2Pos = 0.0f;
    float e2Step = 1.0f / e2.dy;

    for (int y = e2.y1; y < e2.y2; y++) {
        if (y < 0) 
            continue;
        if (y >= size.y)
            break;

        Span span(
            e1.x1 + static_cast<int>(e1.dx * e1Pos),
            e2.x1 + static_cast<int>(e2.dx * e2Pos)
        );

        getSpanSum(sum, count, im, span, y);

        e1Pos += e1Step;
        e2Pos += e2Step;
    }
}

Vector3f Agent::getAvgColor(Image& im, float dist, float angle, float offset) {

    Vector2f pts[3] = {
        Vector2f(pos.x, pos.y),
        Vector2f(
            pos.x + dist * cos(dir + offset + angle / 2),
            pos.y + dist * sin(dir + offset + angle / 2)
        ),
        Vector2f(
            pos.x + dist * cos(dir + offset - angle / 2),
            pos.y + dist * sin(dir + offset - angle / 2)
        )
    };

    Edge edges[3] = {
        Edge(pts[0].x, pts[0].y, pts[1].x, pts[1].y),
        Edge(pts[1].x, pts[1].y, pts[2].x, pts[2].y),
        Edge(pts[2].x, pts[2].y, pts[0].x, pts[0].y)
    };

    //split triangle into two parts and search the pixels in the spans between each edge set
    int max = 0;
    int l = 0;
    for (int i = 0; i < 3; i++) {
        if (edges[i].dy > max) {
            max = edges[i].dy;
            l = i;
        }
    }
    int s1 = (l + 1) % 3;
    int s2 = (l + 2) % 3;

    Vector3i sum(0, 0, 0);

    int count = 0;

    getEdgeSum(sum, count, im, edges[l], edges[s1]);
    getEdgeSum(sum, count, im, edges[l], edges[s2]);

    if (count == 0)
        count = 1;

    return Vector3f(static_cast<float>(sum.x) / count, static_cast<float>(sum.y) / count, static_cast<float>(sum.z) / count);
}