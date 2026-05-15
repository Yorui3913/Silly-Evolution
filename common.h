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

typedef struct Creature
{
    UT_hash_handle hh;
    Position key;
    Color color;

    int eyesight;
    Vector2 velocity;
    Vector2 offset;
} Creature;

#define DEBUG_CREATUREVISION true

#define TILESIZE 6
#define CHUNKSIZE 16
#define CREATURE_VISION_MAX 64

#define sign(a) a < 0 ? -1 : 1

extern int camX;
extern int camY;
extern float noiseFreq;
extern unsigned int noiseDepth;
extern Color biomeColors[];

extern Tile *tiles;
extern Creature *creatures;
extern Position *closestPositions[CREATURE_VISION_MAX];
extern int closestPositionCount[CREATURE_VISION_MAX];

void InitGen();
void generateBiomeNoise(UINT8 *grid, int xOff, int yOff, unsigned int heightSeed, unsigned int tempSeed);

void InitCreatures();
float VectorMagnitude(Vector2 vector);
Vector2 VectorNormalize(Vector2 vector);
void UpdateCreatures();

#endif