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
bool paused;
bool advanceFrame;

int seedHeight;
int seedTemp;

Tile *tiles;
Position *bushesPos;
Creature **creatures;
int creatureCount;
int bushCount;

void HandleInput()
{
    if (IsKeyDown(KEY_D))
        camX++;
    if (IsKeyDown(KEY_A))
        camX--;
    if (IsKeyDown(KEY_S))
        camY++;
    if (IsKeyDown(KEY_W))
        camY--;

    if (IsKeyPressed(KEY_SPACE))
        paused = !paused;
    if (IsKeyPressed(KEY_LEFT_SHIFT))
        advanceFrame = true;

    if (IsKeyPressed(KEY_R))
    {
        for (int i = 0; i < creatureCount; i++)
            DeleteCreature(creatures[i], false);

        creatureCount = 0;
        GenerateLife();
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
            {
                GenerateChunk(x, y);
                HASH_FIND(hh, tiles, &key, sizeof(Position), tile);
            }

            DrawRectangle((tile->key.x - camX) * tileSize, (tile->key.y - camY) * tileSize, tileSize, tileSize, biomeColors[tile->biome]);
        }
    }
}

void DrawLife()
{
    Tile *tile;
    for (int y = camY; y < screenHeight + camY; y++)
    {
        for (int x = camX; x < screenWidth + camX; x++)
        {
            Position key = {x, y};

            // FIND THE TILE
            HASH_FIND(hh, tiles, &key, sizeof(Position), tile);
            if (tile == NULL)
                continue;

            // BUSHES
            if (tile->bushExists)
            {
                Color bushColor = biomeColors[tile->biome];

                Vector2 bushPos = {tile->bush.pos.x + tile->bush.offset.x + 0.5f - camX, tile->bush.pos.y + tile->bush.offset.y + 0.5f - camY};
                DrawEllipse(bushPos.x * tileSize, bushPos.y * tileSize, tileSize * 2.25f, tileSize * 1.75f, WHITE);
                DrawEllipse(bushPos.x * tileSize, bushPos.y * tileSize, tileSize * 2.0f, tileSize * 1.5f, bushColor);

                for (int i = 0; i < 3; i++)
                {
                    Color fruitColor = tile->bush.fruits[i] == BUSH_FRUIT_MAX ? (Color){255, 0, 0, 255} : (Color){tile->bush.fruits[i] * 200 / BUSH_FRUIT_MAX, 255 - tile->bush.fruits[i] * 200 / BUSH_FRUIT_MAX, 0, 255};
                    DrawCircle((bushPos.x + fruitOffset[i].x) * tileSize, (bushPos.y + fruitOffset[i].y) * tileSize, tileSize / 2.0f, WHITE);
                    DrawCircle((bushPos.x + fruitOffset[i].x) * tileSize, (bushPos.y + fruitOffset[i].y) * tileSize, tileSize / 2.5f, fruitColor);
                }
            }

            // CREATURES
            if (tile->creatureCount)
            {
                for (int i = 0; i < tile->creatureCount; i++)
                {
                    Creature creature = tile->creatures[i];
                    int thirstMult = 255 - (creature.thirst * 255 / CREATURE_THRIST_MAX);
                    int hungerMult = 255 - (creature.hunger * 255 / CREATURE_HUNGER_MAX);
                    int minMult = min(thirstMult, hungerMult);
                    Color outColor = {minMult, minMult, minMult, 255};

                    Vector2 creaturePos = {creature.pos.x + creature.offset.x + 0.5f - camX, creature.pos.y + creature.offset.y + 0.5f - camY};
                    DrawCircle(creaturePos.x * tileSize, creaturePos.y * tileSize, tileSize * 1.5f, outColor);
                    DrawCircle(creaturePos.x * tileSize, creaturePos.y * tileSize, tileSize, creature.color);
                    char buffer[8];
                    sprintf(buffer, "%d", creature.genID);
                    DrawRectangle((creaturePos.x - 0.65f) * tileSize, (creaturePos.y - 3.5f) * tileSize, tileSize * 1.5f, tileSize * 1.2f, BLACK);
                    DrawText(buffer, (creaturePos.x - 0.5f) * tileSize, (creaturePos.y - 3.35f) * tileSize, tileSize * 1.2f, WHITE);

                    Color mainObjColor = creature.mainObj == OBJ_FOOD ? (Color){255, 0, 0, 255} : (creature.mainObj == OBJ_WATER ? (Color){0, 0, 255, 255} : (creature.mainObj == OBJ_FUNKY ? (Color){0, 255, 0, 255} : WHITE));
                    Color secObjColor = creature.secObj == OBJ_FOOD ? (Color){255, 0, 0, 255} : (creature.secObj == OBJ_WATER ? (Color){0, 0, 255, 255} : (creature.secObj == OBJ_FUNKY ? (Color){0, 255, 0, 255} : WHITE));
                    DrawRectangle((creaturePos.x - 1.25f) * tileSize, (creaturePos.y - 2) * tileSize, tileSize, tileSize / 2.0f, mainObjColor);
                    DrawRectangle(creaturePos.x * tileSize, (creaturePos.y - 2) * tileSize, tileSize, tileSize / 2.0f, secObjColor);

                    if (creature.foundMainObj || creature.foundSecObj)
                        for (int yf = -1; yf <= 1; yf++)
                            for (int xf = -1; xf <= 1; xf++)
                                DrawRectangle((creature.objPos.x - camX + xf) * TILESIZE, (creature.objPos.y - camY + yf) * TILESIZE, TILESIZE, TILESIZE, (Color){255, 0, 0, 255});
                }
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
    HASH_CLEAR(hh, tiles);
    seedHeight = 3913;
    seedTemp = 1000;

    InitCreatures();
    GenerateLife();
    paused = true;

    while (!WindowShouldClose())
    {
        BeginDrawing();

        screenWidth = trueScreenWidth / tileSize;
        screenHeight = trueScreenHeight / tileSize;

        ClearBackground(BLACK);

        HandleInput();
        if (!paused || advanceFrame)
        {
            UpdateLife();
            advanceFrame = false;
        }
        DrawTiles();
        DrawLife();

        EndDrawing();
    }

    CloseWindow();
    return 0;
}