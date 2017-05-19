//
//    FILE: RunningAverage.cpp
//  AUTHOR: Rob Tillaart
// VERSION: 0.1.01
// PURPOSE: RunningAverage library for Arduino
//
// The library does store the last N individual values, to
// calculate the running average.
//
// HISTORY:
// 0.1.00 - 2011-01-30 initial version
// 0.1.01 - 2011-02-28 fixed missing destructor in .h
//
// Released to the public domain
//
// updated by Anton Viktorov to use double instead of float

#include "RunningAverage.h"
#include <stdlib.h>

//
RunningAverage::RunningAverage(uint8_t n){
    _size = n;
    _ar = (double *) malloc(_size * sizeof(double));
    clr();
}

RunningAverage::~RunningAverage(){
    free(_ar);
}

// resets all counters
void RunningAverage::clr(){
    _count = 0;
    _idx = 0;
    _sum = 0.0;
    for (uint8_t i = 0; i < _size; i++) _ar[i] = 0.0;
}

// adds a new value to the data-set
void RunningAverage::add(double f){
    _sum -= _ar[_idx];
    _ar[_idx] = f;
    _sum += _ar[_idx];
    _idx = (_idx + 1) % _size;
    if (_count < _size) _count++;
}

// returns the average of the data-set added sofar
double RunningAverage::avg(){
    if (_count == 0) return 0;
    return _sum / _count;
}

uint8_t RunningAverage::count(){
    return _count;
}

// returns the last value of the data-set added sofar
double RunningAverage::peek(){
    if (_count == 0) return 0;
    return _ar[_idx];
}
