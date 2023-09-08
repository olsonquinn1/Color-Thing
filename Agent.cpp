#include "Agent.h"
#include "SFML\Graphics.hpp"
#include <random>
#include <vector>
#include <math.h>
#include <iostream>

using namespace std;
using namespace sf;

Edge::Edge(int x1, int y1,
    int x2, int y2)
{
    if (y1 < y2) {
        X1 = x1;
        Y1 = y1;
        X2 = x2;
        Y2 = y2;
    }
    else {
        X1 = x2;
        Y1 = y2;
        X2 = x1;
        Y2 = y1;
    }
}

Span::Span(int x1, int x2)
{
    if (x1 < x2) {
        X1 = x1;
        X2 = x2;
    }
    else {
        X1 = x2;
        X2 = x1;
    }
}

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

    Vector3f me = Vector3f(color.r, color.g, color.b);

    //treat colors as a 3d vector for comparing
    //normalize average color to ignore color intensity when deciding which way to go
    
    Vector3f l;
    Vector3f f;
    Vector3f r;
    if ((int) searchModel == 1) {
        l = norm(getAvgColor(im, searchSize, searchAngle, searchAngle + searchAngleOffset));
        f = norm(getAvgColor(im, searchSize, searchAngle, 0));
        r = norm(getAvgColor(im, searchSize, searchAngle, -1.0f * searchAngle - searchAngleOffset));
    }
    else if((int) searchModel == 2) {
        l = norm(alternateAvg(im, searchSize, searchAngle, searchAngle + searchAngleOffset));
        f = norm(alternateAvg(im, searchSize, searchAngle, 0));
        r = norm(alternateAvg(im, searchSize, searchAngle, -1.0f * searchAngle - searchAngleOffset));
    }
    
    float lval = compare(me, l);
    float rval = compare(me, r);
    float mval = compare(me, f);
    float bias = (lval - rval) * (1 - mval);

    //biasFactor affects how heavily the bias affects the decision
    bias = bias * biasFactor;
    float val = ((float)(rand() % 2000) / 1000.0f - 1.0f) + bias;
    alterDir(
        val * maxTurn
    );
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
    const float maxDist = sqrt(3); //maximum difference between two 3d normalized vectors 
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
    if (mag == 0) {
        return Vector3f(0, 0, 0);
    }
    return Vector3f((float)v.x / mag, (float)v.y / mag, (float)v.z / mag);
}

//rasterization algorithm, but instead of drawing the pixels within the triangle, we are reading them 
//to determine the average color within the area of the triangle
Vector3f Agent::getAvgColor(Image& im, float dist, float angle, float offset) {

    //calculate points on triangle to check
    vector<Vector2i> points = {
        Vector2i(pos.x, pos.y),
        Vector2i(
            pos.x + dist * cos(dir + offset + angle / 2),
            pos.y + dist * sin(dir + offset + angle / 2)
        ),
        Vector2i(
            pos.x + dist * cos(dir + offset - angle / 2),
            pos.y + dist * sin(dir + offset - angle / 2)
        )
    };

    //determine bounding box of triangle
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

    //get avg color of pixels within the triangle
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

Vector3f Agent::getSpanAvg(Image& im, const Span& span, int y)
{
    Vector2u size = im.getSize();
    int xdiff = span.X2 - span.X1;
    if (xdiff == 0)
        return Vector3f(0, 0, 0);

    float factor = 0.0f;
    float factorStep = 1.0f / (float)xdiff;

    Vector3f avg(0, 0, 0);
    int badCount = 0;
    int count = 0;
    for (int x = span.X1; x < span.X2; x++) {
        if (x < 0 || x >= size.x) 
            continue;

        count++;
        Color c = im.getPixel(x, y);
        avg += Vector3f(c.r, c.g, c.b);
        factor += factorStep;
    }
    if (count <= 0) count = 1;
    return Vector3f(avg.x / count, avg.y / count, avg.z / count);
}

Vector3f Agent::getEdgeAvg(Image& im, const Edge& e1, const Edge& e2)
{
    Vector2u size = im.getSize();

    //ignore if edge is horizontal
    float e1ydiff = (float)(e1.Y2 - e1.Y1);
    if (e1ydiff == 0.0f)
        return Vector3f(0, 0, 0);

    float e2ydiff = (float)(e2.Y2 - e2.Y1);
    if (e2ydiff == 0.0f)
        return Vector3f(0, 0, 0);

    float e1xdiff = (float)(e1.X2 - e1.X1);
    float e2xdiff = (float)(e2.X2 - e2.X1);

    float factor1 = (float)(e2.Y1 - e1.Y1) / e1ydiff;
    float factorStep1 = 1.0f / e1ydiff;
    float factor2 = 0.0f;
    float factorStep2 = 1.0f / e2ydiff;

    Vector3f avg(0, 0, 0);
    int count = 0;
    for (int y = e2.Y1; y < e2.Y2; y++) {
        if (y < 0 || y >= size.y) {
            continue;
        }

        count++;

        Span span(
            e1.X1 + (int)(e1xdiff * factor1),
            e2.X1 + (int)(e2xdiff * factor2));
        avg += getSpanAvg(im, span, y);

        factor1 += factorStep1;
        factor2 += factorStep2;
    }
    if (count <= 0) count = 1;
    return Vector3f(avg.x / count, avg.y / count, avg.z / count);
}

Vector3f Agent::alternateAvg(Image& im, float dist, float angle, float offset) {

    Vector2i pts[3] = {
        Vector2i(pos.x, pos.y),
        Vector2i(
            pos.x + dist * cos(dir + offset + angle / 2),
            pos.y + dist * sin(dir + offset + angle / 2)
        ),
        Vector2i(
            pos.x + dist * cos(dir + offset - angle / 2),
            pos.y + dist * sin(dir + offset - angle / 2)
        )
    };

    Edge edges[3] = {
        Edge(pts[0].x, pts[0].y, pts[1].x, pts[1].y),
        Edge(pts[1].x, pts[1].y, pts[2].x, pts[2].y),
        Edge(pts[2].x, pts[2].y, pts[0].x, pts[0].y)
    };

    //split triangle into two parts and search the pixels between each edge set
    int maxLength = 0;
    int longEdge = 0;
    for (int i = 0; i < 3; i++) {
        int length = edges[i].Y2 - edges[i].Y1;
        if (length > maxLength) {
            maxLength = length;
            longEdge = i;
        }
    }
    int shortEdge1 = (longEdge + 1) % 3;
    int shortEdge2 = (longEdge + 2) % 3;

    Vector3f avg(0, 0, 0);

    avg += getEdgeAvg(im, edges[longEdge], edges[shortEdge1]);
    avg += getEdgeAvg(im, edges[longEdge], edges[shortEdge2]);

    avg = Vector3f(avg.x / 2, avg.y / 2, avg.z / 2);

    return avg;
}