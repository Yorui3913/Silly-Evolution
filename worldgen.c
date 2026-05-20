#include "common.h"

float noiseFreq = 0.035f;
unsigned int noiseDepth = 4;//4;

Color biomeColors[BIOME_SIZE];
Biome biomeBaseGrid[8][8];

// Most of the Perlin Noise functions are by "nowl"
float smooth(float x, float y, float s)
{
    return x + (s * s * (3 - 2 * s)) * (y - x);
}

float randNoise(int x, int y, int seed) 
{
    int n = x + y * 57 + seed * 131;
    n = (n << 13) ^ n;
    return 1.0f - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0f;
}

float noise2d(float x, float y, int seed)
{
    int x_int = x;
    int y_int = y;
    float x_frac = x - x_int;
    float y_frac = y - y_int;
    float s = randNoise(x_int, y_int, seed);
    float t = randNoise(x_int + 1, y_int, seed);
    float u = randNoise(x_int, y_int + 1, seed);
    float v = randNoise(x_int + 1, y_int + 1, seed);
    float low = smooth(s, t, x_frac);
    float high = smooth(u, v, x_frac);
    return smooth(low, high, y_frac);
}

float perlinNoise(float x, float y, int seed)
{
    float xa = x * noiseFreq;
    float ya = y * noiseFreq;
    float amp = 1.0;
    float fin = 0;
    float div = 0.0;

    for (int i = 0; i < noiseDepth; i++)
    {
        div += amp;
        fin += (noise2d(xa, ya, seed) + (1.1f - amp)) * amp;
        amp /= 2;
        xa *= 2;
        ya *= 2;
    }

    return fin / div;
}

void generateBiomeNoise(UINT8 *grid, int xOff, int yOff, unsigned int heightSeed, unsigned int tempSeed)
{
    for (int y = 0; y < CHUNKSIZE; y++)
    {
        for (int x = 0; x < CHUNKSIZE; x++)
        {
            int hVal = min((UINT8)(abs(perlinNoise(abs(x + xOff), abs(y + yOff), heightSeed) * 8)), 7);
            int tVal = min((UINT8)(abs(perlinNoise(abs(x + xOff), abs(y + yOff), tempSeed) * 8)), 7);
            grid[y * CHUNKSIZE + x] = biomeBaseGrid[tVal][hVal];
        }
    }
}

void GenerateChunk(int xOff, int yOff)
{
    int fixOffX = xOff < 0 ? ((xOff + 1) / CHUNKSIZE - 1) * CHUNKSIZE : (xOff / CHUNKSIZE) * CHUNKSIZE;
    int fixOffY = yOff < 0 ? ((yOff + 1) / CHUNKSIZE - 1) * CHUNKSIZE : (yOff / CHUNKSIZE) * CHUNKSIZE;

    Tile *check;
    Position checkPos = {fixOffX, fixOffY};
    HASH_FIND(hh, tiles, &checkPos, sizeof(Position), check);
    if (check != NULL)
        return;

    UINT8 *tileNoise = (UINT8 *)malloc(sizeof(UINT8) * CHUNKSIZE * CHUNKSIZE);
    generateBiomeNoise(tileNoise, fixOffX, fixOffY, seedHeight, seedTemp);

    for (int y = 0; y < CHUNKSIZE; y++)
    {
        for (int x = 0; x < CHUNKSIZE; x++)
        {
            Tile *newTile = (Tile *)malloc(sizeof(Tile));
            newTile->key = (Position){x + fixOffX, y + fixOffY};
            newTile->biome = (Biome)tileNoise[x + CHUNKSIZE * y];
            newTile->creatures = (Creature *)malloc(sizeof(Creature));

            newTile->bushExists = false;
            newTile->bushHasFruit = false;
            newTile->creatureCount = 0;
            newTile->creatureHasFunky = false;

            HASH_ADD(hh, tiles, key, sizeof(Position), newTile);
        }
    }

    free(tileNoise);
}

void InitGen()
{
    biomeColors[BIOME_GRASS] = (Color){0x00, 0xbf, 0x00, 0xff};
    biomeColors[BIOME_FOREST] = (Color){0x00, 0x8f, 0x3f, 0xff};
    biomeColors[BIOME_MOUNT] = (Color){0x1f, 0x4f, 0x3f, 0xff};
    biomeColors[BIOME_TUNDRA] = (Color){0x7f, 0xdf, 0xdf, 0xff};
    biomeColors[BIOME_HIGH] = (Color){0xdf, 0xff, 0xff, 0xff};
    biomeColors[BIOME_SAND] = (Color){0xff, 0xdf, 0x3f, 0xff};
    biomeColors[BIOME_JUNGLE] = (Color){0x1f, 0x6f, 0x00, 0xff};
    biomeColors[BIOME_WATER] = (Color){0x00, 0xbf, 0xff, 0xff};
    //biomeColors[BIOME_SIZE] = (Color){0x00, 0xff, 0x00, 0xff};

    Biome biomeBaseGridTemp[8][8] =
        {
            {BIOME_SAND, BIOME_SAND, BIOME_SAND, BIOME_JUNGLE, BIOME_JUNGLE, BIOME_MOUNT, BIOME_MOUNT, BIOME_MOUNT},
            {BIOME_WATER, BIOME_SAND, BIOME_SAND, BIOME_JUNGLE, BIOME_JUNGLE, BIOME_JUNGLE, BIOME_MOUNT, BIOME_MOUNT},
            {BIOME_WATER, BIOME_SAND, BIOME_GRASS, BIOME_GRASS, BIOME_JUNGLE, BIOME_JUNGLE, BIOME_MOUNT, BIOME_MOUNT},
            {BIOME_WATER, BIOME_GRASS, BIOME_GRASS, BIOME_GRASS, BIOME_FOREST, BIOME_FOREST, BIOME_MOUNT, BIOME_MOUNT},
            {BIOME_WATER, BIOME_GRASS, BIOME_GRASS, BIOME_GRASS, BIOME_FOREST, BIOME_FOREST, BIOME_MOUNT, BIOME_HIGH},
            {BIOME_WATER, BIOME_WATER, BIOME_GRASS, BIOME_GRASS, BIOME_FOREST, BIOME_FOREST, BIOME_HIGH, BIOME_HIGH},
            {BIOME_WATER, BIOME_WATER, BIOME_GRASS, BIOME_TUNDRA, BIOME_TUNDRA, BIOME_TUNDRA, BIOME_HIGH, BIOME_HIGH},
            {BIOME_WATER, BIOME_WATER, BIOME_WATER, BIOME_TUNDRA, BIOME_TUNDRA, BIOME_HIGH, BIOME_HIGH, BIOME_HIGH},
        };
    memcpy(biomeBaseGrid, biomeBaseGridTemp, sizeof(Biome) * 8 * 8);
}