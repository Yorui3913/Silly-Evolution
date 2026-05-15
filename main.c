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

void GenerateCreatures()
{
    int n = 16;
    for (int i = 0; i < n; i++)
    {
        CreateCreature(GetRandomValue(0, 180), GetRandomValue(0, 180), ColorFromHSV(i * 360.0f / n, 1.0f, 1.0f));
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

void DrawCreatures()
{
    Creature *creature;
    for (int y = camY; y < screenHeight + camY; y++)
    {
        for (int x = camX; x < screenWidth + camX; x++)
        {
            Position key = {x, y};
            HASH_FIND(hh, creatures, &key, sizeof(Position), creature);
            if (creature == NULL)
                continue;

            DrawCircle((creature->key.x + creature->offset.x + 0.5f - camX) * tileSize, (creature->key.y + creature->offset.y + 0.5f - camY) * tileSize, tileSize * 1.5f, WHITE);
            DrawCircle((creature->key.x + creature->offset.x + 0.5f - camX) * tileSize, (creature->key.y + creature->offset.y + 0.5f - camY) * tileSize, tileSize, creature->color);
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
    GenerateCreatures();

    while (!WindowShouldClose())
    {
        BeginDrawing();

        screenWidth = trueScreenWidth / tileSize;
        screenHeight = trueScreenHeight / tileSize;

        ClearBackground(BLACK);

        // LOGIC HERE
        HandleInput();
        //UpdateCreatures();

        DrawTiles();
        UpdateCreatures();
        DrawCreatures();

        EndDrawing();
    }

    CloseWindow();
    return 0;
}