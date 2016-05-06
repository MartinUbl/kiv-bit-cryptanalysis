#ifndef KIV_BIT_WORKER_GENERAL_H
#define KIV_BIT_WORKER_GENERAL_H

#include <iostream>
#include <list>
#include <vector>
#include <map>
#include <set>
#include <stdint.h>
#include <thread>
#include <random>
#include <cstdio>
#include <cstring>
#include <climits>
#include <cfloat>

using namespace std;

// global chance generator (0-100)
static std::default_random_engine chanceGenerator;
static std::uniform_int_distribution<int> chanceDist(0, 100);
// bound generator for uniform number generation (range 0-100)
static auto standardChance = std::bind(chanceDist, chanceGenerator);

// global general generator (0-INT_MAX)
static std::default_random_engine generalGenerator;
static std::uniform_int_distribution<int> generalDist(0, INT_MAX);
// bound generator for uniform number generation (range 0-INT_MAX)
static auto generalChance = std::bind(generalDist, generalGenerator);

#endif
