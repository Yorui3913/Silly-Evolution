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

typedef enum ObjectiveType
{
    OBJ_NONE,
    OBJ_WATER,
    OBJ_FOOD,
    OBJ_FUNKY,
    OBJ_COUNT
} ObjectiveType;

typedef struct Position
{
    int x;
    int y;
} Position;

typedef struct Bush
{
    Position pos;
    float fruits[3];
    Vector2 offset;
} Bush;

typedef struct Creature
{
    Position pos;
    Color color;

    int eyesight;
    float wander;
    float thirst;
    float hunger;
    float funky;

    Vector2 velocity;
    Vector2 offset;

    int genID;
    int tileID;

    ObjectiveType mainObj;
    ObjectiveType secObj;
    bool foundMainObj;
    bool foundSecObj;
    Position objPos;
} Creature;

typedef struct Tile
{
    UT_hash_handle hh;
    Position key;
    Biome biome;

    Bush bush;
    bool bushExists;
    bool bushHasFruit;

    Creature *creatures;
    int creatureCount;
    bool creatureHasFunky;
} Tile;

#define DEBUG_CREATUREVISION false

#define TILESIZE 12
#define CHUNKSIZE 16

// ---CREATURE [START]---
#define CREATURE_VISION_MAX 64
#define CREATURE_SPEED_INC 0.075f

#define CREATURE_WANDER_TRESHOLD 18
#define CREATURE_WANDER_INC 0.012f

#define CREATURE_THRIST_MAX 64
#define CREATURE_THRIST_TRESHOLD 20
#define CREATURE_THIRST_INC 0.015f

#define CREATURE_HUNGER_MAX 64
#define CREATURE_HUNGER_TRESHOLD 20
#define CREATURE_HUNGER_INC 0.0125f

#define CREATURE_FUNKY_MAX 64
#define CREATURE_FUNKY_TRESHOLD 28
#define CREATURE_FUNKY_INC 0.0075f
// ---CREATURE [END]---

#define BUSH_FRUIT_MAX 64
#define BUSH_FRUIT_INC 0.03f 

#define sign(a) a < 0 ? -1 : 1

extern int camX;
extern int camY;
extern int screenWidth;
extern int screenHeight;
extern int seedHeight;
extern int seedTemp;
extern float noiseFreq;
extern unsigned int noiseDepth;

extern Color biomeColors[];
extern Vector2 fruitOffset[3];
extern Position *closestPositions[CREATURE_VISION_MAX];
extern int closestPositionCount[CREATURE_VISION_MAX];

extern Tile *tiles;
extern Position *bushesPos;
extern Creature **creatures;

extern int creatureCount;
extern int bushCount;

void InitGen();
void GenerateChunk(int xOff, int yOff);

void InitCreatures();
void GenerateLife();
bool GenerateBush(int xPos, int yPos);
Creature *CreateCreature(int xPos, int yPos, Color color, int id);
void DeleteCreature(Creature *creature, bool totalAnihilation);
void UpdateLife();

float VectorMagnitude(Vector2 vector);
Vector2 VectorNormalize(Vector2 vector);

#endif