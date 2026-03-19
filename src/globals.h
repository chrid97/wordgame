#include <stdint.h>

// The number of times a letter appears in your bag
// 100 tiles total
uint8_t letter_occurrence[26] = {
    8,  // a
    2,  // b
    4,  // c
    4,  // d
    11, // e
    1,  // f
    3,  // g
    2,  // h
    8,  // i
    1,  // j
    1,  // k
    5,  // l
    3,  // m
    7,  // n
    6,  // o
    3,  // p
    1,  // q
    7,  // r
    8,  // s
    5,  // t
    4,  // u
    1,  // v
    1,  // w
    1,  // x
    2,  // y
    1   // z
};

const uint8_t letter_scores[26] = {
    1,  // a
    3,  // b
    3,  // c
    2,  // d
    1,  // e
    4,  // f
    2,  // g
    4,  // h
    1,  // i
    8,  // j
    5,  // k
    1,  // l
    3,  // m
    1,  // n
    1,  // o
    3,  // p
    10, // q
    1,  // r
    1,  // s
    1,  // t
    1,  // u
    4,  // v
    4,  // w
    8,  // x
    4,  // y
    10  // z
};
