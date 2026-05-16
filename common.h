#ifndef COMMON_DECLARATION
#define COMMON_DECLARATION

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <windef.h>

#include "raylib.h"
#include "uthash.h"

typedef enum Biome
{
    BIOME_GRASS,
    BIOME_FOREST,
    BIOME_MOUNT,
    BIOME_TUNDRA,
    BIOME_HIGH,
    BIOME_SAND,
    BIOME_JUNGLE,
    BIOME_WATER,
    BIOME_SIZE
} Biome;

typedef enum Objective
{
    OBJ_WATER,
    OBJ_FOOD,
    OBJ_COUNT
} Objective;

typedef struct Position
{
    int x;
    int y;
} Position;

typedef struct Tile
{
    UT_hash_handle hh;
    Position key;
    Biome biome;
} Tile;

typedef struct Bush
{
    UT_hash_handle hh;
    Position key;

    float fruits[3];
    Vector2 offset;
} Bush;

typedef struct Creature
{
    UT_hash_handle hh;
    Position key;
    Color color;

    int eyesight;
    float thirst;
    float hunger;

    Vector2 velocity;
    Vector2 offset;
} Creature;

#define DEBUG_CREATUREVISION false

#define TILESIZE 8
#define CHUNKSIZE 16

#define CREATURE_VISION_MAX 64
#define CREATURE_THRIST_MAX 64
#define CREATURE_HUNGER_MAX 64
#define CREATURE_THIRST_BASE 0.015f
#define CREATURE_HUNGER_BASE 0.015f
#define CREATURE_SPEED_BASE 0.025f

#define BUSH_FRUIT_MAX 64
#define BUSH_FRUIT_BASE 0.03f

#define sign(a) a < 0 ? -1 : 1

extern int camX;
extern int camY;
extern float noiseFreq;
extern unsigned int noiseDepth;
extern Color biomeColors[];
extern Vector2 fruitOffset[3];

extern Tile *tiles;
extern Creature *creatures;
extern Bush *bushes;
extern Position *closestPositions[CREATURE_VISION_MAX];
extern int closestPositionCount[CREATURE_VISION_MAX];

void InitGen();
void generateBiomeNoise(UINT8 *grid, int xOff, int yOff, unsigned int heightSeed, unsigned int tempSeed);

void InitCreatures();
float VectorMagnitude(Vector2 vector);
Vector2 VectorNormalize(Vector2 vector);
void UpdateLife();

#endif