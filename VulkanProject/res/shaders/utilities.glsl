#ifndef UTILITIES_GLSL
#define UTILITIES_GLSL

void Swap(inout float A, inout float B) 
{
    float Temp = A;
    A = B;
    B = Temp;
}

void Swap(inout int A, inout int B) 
{
    int Temp = A;
    A = B;
    B = Temp;
}

void Swap(inout uint A, inout uint B) 
{
    uint Temp = A;
    A = B;
    B = Temp;
}

#endif