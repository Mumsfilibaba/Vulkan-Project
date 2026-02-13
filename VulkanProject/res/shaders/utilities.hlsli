#ifndef UTILITIES_HLSLI
#define UTILITIES_HLSLI

void Swap(inout float a, inout float b)
{
    const float temp = a;
    a = b;
    b = temp;
}

void Swap(inout int a, inout int b)
{
    const int temp = a;
    a = b;
    b = temp;
}

void Swap(inout uint a, inout uint b)
{
    const uint temp = a;
    a = b;
    b = temp;
}

#endif

