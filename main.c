#include "common.h"

int trueScreenWidth = 1920;
int trueScreenHeight = 1080;
int screenWidth;
int screenHeight;
int tileSize;
int fps = 120;

int camX;
int camY;
int mousePosX;
int mousePosY;

int seedHeight;
int seedTemp;

Tile *tiles;
Creature *creatures;
Bush *bushes;

void GenerateChunk(int xOff, int yOff)
{
    int fixOffX = xOff * CHUNKSIZE;
    int fixOffY = yOff * CHUNKSIZE;

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
            HASH_ADD(hh, tiles, key, sizeof(Position), newTile);
        }
    }

    free(tileNoise);
}

void GenerateWorld()
{
    HASH_CLEAR(hh, tiles);

    seedHeight = 3913;
    seedTemp = 1000;

    for (int yOff = (camY / CHUNKSIZE) - 1; yOff <= ((screenHeight + camY) / CHUNKSIZE) + 1; yOff++)
        for (int xOff = (camX / CHUNKSIZE) - 1; xOff <= ((screenWidth + camX) / CHUNKSIZE) + 1; xOff++)
            GenerateChunk(xOff, yOff);
}

void CreateCreature(int xPos, int yPos, Color color)
{
    Creature *newCreature = (Creature *)malloc(sizeof(Creature));
    newCreature->key = (Position){xPos, yPos};
    newCreature->color = color;

    //
    newCreature->eyesight = 24;
    //

    HASH_ADD(hh, creatures, key, sizeof(Position), newCreature);
}

bool GenerateBush(int xPos, int yPos)
{
    Position key = {xPos, yPos};
    Tile *tile;
    HASH_FIND(hh, tiles, &key, sizeof(Position), tile);

    if (tile == NULL || tile->biome == BIOME_WATER)
        return false; // No tile to spawn, return false!

    Bush *newBush = (Bush *)malloc(sizeof(Bush));
    newBush->key = key;

    newBush->offset = (Vector2){GetRandomValue(-8, 8) / 20.0f, GetRandomValue(-8, 8) / 20.0f};
    newBush->fruits[0] = GetRandomValue(0, 8) * BUSH_FRUIT_MAX / 12.f;
    newBush->fruits[1] = GetRandomValue(0, 8) * BUSH_FRUIT_MAX / 12.f;
    newBush->fruits[2] = GetRandomValue(0, 8) * BUSH_FRUIT_MAX / 12.f;

    HASH_ADD(hh, bushes, key, sizeof(Position), newBush);
    return true; // It worked! I think... Return true :)
}

void GenerateLife()
{
    int c = 24;
    for (int i = 0; i < c; i++)
    {
        CreateCreature(GetRandomValue(-120, 360), GetRandomValue(-120, 360), ColorFromHSV(i * 360.0f / c, 1.0f, 1.0f));
    }

    int b = 30;
    for (int i = 0; i < b; i++)
    {
        while (!GenerateBush(GetRandomValue(-200, 400), GetRandomValue(-200, 400)))
        {
            // Idk man, I guess we just repeat the check again :/
        }
    }
}

void HandleInput()
{
    if (IsKeyDown(KEY_D))
    {
        camX++;
        if (camX / CHUNKSIZE != (camX - 1) / CHUNKSIZE)
        {
            for (int yOff = (camY / CHUNKSIZE) - 1; yOff <= ((screenHeight + camY) / CHUNKSIZE) + 1; yOff++)
                GenerateChunk(((screenWidth + camX) / CHUNKSIZE) + 1, yOff);
        }
    }
    if (IsKeyDown(KEY_A))
    {
        camX--;
        if (camX / CHUNKSIZE != (camX + 1) / CHUNKSIZE)
        {
            for (int yOff = (camY / CHUNKSIZE) - 1; yOff <= ((screenHeight + camY) / CHUNKSIZE) + 1; yOff++)
                GenerateChunk((camX / CHUNKSIZE) - 1, yOff);
        }
    }
    if (IsKeyDown(KEY_S))
    {
        camY++;
        if (camY / CHUNKSIZE != (camY - 1) / CHUNKSIZE)
        {
            for (int xOff = (camX / CHUNKSIZE) - 1; xOff <= ((screenWidth + camX) / CHUNKSIZE) + 1; xOff++)
                GenerateChunk(xOff, ((screenHeight + camY) / CHUNKSIZE) + 1);
        }
    }
    if (IsKeyDown(KEY_W))
    {
        camY--;
        if (camY / CHUNKSIZE != (camY + 1) / CHUNKSIZE)
        {
            for (int xOff = (camX / CHUNKSIZE) - 1; xOff <= ((screenWidth + camX) / CHUNKSIZE) + 1; xOff++)
                GenerateChunk(xOff, (camY / CHUNKSIZE) - 1);
        }
    }
}

void DrawTiles()
{
    Tile *tile;
    for (int y = camY; y < screenHeight + camY; y++)
    {
        for (int x = camX; x < screenWidth + camX; x++)
        {
            Position key = {x, y};
            HASH_FIND(hh, tiles, &key, sizeof(Position), tile);
            if (tile == NULL)
                continue;

            DrawRectangle((tile->key.x - camX) * tileSize, (tile->key.y - camY) * tileSize, tileSize, tileSize, biomeColors[tile->biome]);
        }
    }
}

void DrawLife()
{
    Creature *creature;
    Bush *bush;
    Tile *tile;
    for (int y = camY; y < screenHeight + camY; y++)
    {
        for (int x = camX; x < screenWidth + camX; x++)
        {
            Position key = {x, y};

            // BUSHES
            HASH_FIND(hh, bushes, &key, sizeof(Position), bush);
            if (bush != NULL)
            {
                Color bushColor = MAGENTA;
                HASH_FIND(hh, tiles, &key, sizeof(Position), tile);
                if (tile != NULL)
                    bushColor = biomeColors[tile->biome];

                Position bushPos = {bush->key.x + bush->offset.x + 0.5f - camX, bush->key.y + bush->offset.y + 0.5f - camY};
                DrawEllipse(bushPos.x * tileSize, bushPos.y * tileSize, tileSize * 2.25f, tileSize * 1.75f, WHITE);
                DrawEllipse(bushPos.x * tileSize, bushPos.y * tileSize, tileSize * 2.0f, tileSize * 1.5f, bushColor);

                for (int i = 0; i < 3; i++)
                {
                    Color fruitColor = bush->fruits[i] == BUSH_FRUIT_MAX ? (Color){255, 0, 0, 255} : (Color){bush->fruits[i] * 200 / BUSH_FRUIT_MAX , 255 - bush->fruits[i] * 200 / BUSH_FRUIT_MAX, 0, 255};
                    DrawCircle((bushPos.x + fruitOffset[i].x) * tileSize, (bushPos.y + fruitOffset[i].y) * tileSize, tileSize / 2.0f, WHITE);
                    DrawCircle((bushPos.x + fruitOffset[i].x) * tileSize, (bushPos.y + fruitOffset[i].y) * tileSize, tileSize / 2.5f, fruitColor);
                }
            }

            // CREATURES
            HASH_FIND(hh, creatures, &key, sizeof(Position), creature);
            if (creature != NULL)
            {
                float thirstMult = 255 - (creature->thirst * 255 / CREATURE_THRIST_MAX);
                float hungerMult = 255 - (creature->hunger * 255 / CREATURE_HUNGER_MAX);
                Color outColor = {thirstMult, 255, hungerMult, 255};
                DrawCircle((creature->key.x + creature->offset.x + 0.5f - camX) * tileSize, (creature->key.y + creature->offset.y + 0.5f - camY) * tileSize, tileSize * 1.5f, outColor);
                DrawCircle((creature->key.x + creature->offset.x + 0.5f - camX) * tileSize, (creature->key.y + creature->offset.y + 0.5f - camY) * tileSize, tileSize, creature->color);

                Color seekingColor = creature->hunger > creature->thirst ? (Color){255, 255, 0, 255} : (Color){0, 255, 255, 255};
                DrawRectangle((creature->key.x + creature->offset.x - 0.5f - camX) * tileSize, (creature->key.y + creature->offset.y - 1 - camY) * tileSize, tileSize * 2, tileSize / 2.0f, seekingColor);
            }
        }
    }
}

int main()
{
    SetConfigFlags(FLAG_WINDOW_UNDECORATED);
    InitWindow(trueScreenWidth, trueScreenHeight, "Natural Selection");

    trueScreenWidth = GetScreenWidth();
    trueScreenHeight = GetScreenHeight();
    tileSize = TILESIZE;

    screenWidth = trueScreenWidth / tileSize;
    screenHeight = trueScreenHeight / tileSize;

    SetWindowSize(trueScreenWidth, trueScreenHeight);
    SetWindowPosition(0, 0);
    SetTargetFPS(fps);
    // SetExitKey(0);

    // INIT HERE
    InitGen();
    GenerateWorld();

    InitCreatures();
    GenerateLife();

    while (!WindowShouldClose())
    {
        BeginDrawing();

        screenWidth = trueScreenWidth / tileSize;
        screenHeight = trueScreenHeight / tileSize;

        ClearBackground(BLACK);

        // LOGIC HERE
        HandleInput();

        DrawTiles();
        UpdateLife();
        DrawLife();

        EndDrawing();
    }

    CloseWindow();
    return 0;
}