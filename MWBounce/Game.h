#pragma once

struct Matrix4
{
    struct { float x, y, z, w; } x, y, z, p;
};

extern Matrix4& CarScaleMatrix;