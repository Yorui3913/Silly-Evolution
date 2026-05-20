#include "common.h"

Position *closestPositions[CREATURE_VISION_MAX];
int closestPositionCount[CREATURE_VISION_MAX];
Vector2 fruitOffset[3] = {{-0.6f, 0.4}, {0.7, 0.55}, {0.2, -0.6}};

int *IDsToKill;
int idCount;

// --VECTOR HANDLING STUFF--
float VectorMagnitude(Vector2 vector)
{
    return sqrtf(vector.x * vector.x + vector.y * vector.y);
}

Vector2 VectorNormalize(Vector2 vector)
{
    if (vector.x == 0.0f && vector.y == 0.0f)
        return vector;

    float magnitude = VectorMagnitude(vector);
    return (Vector2){vector.x / magnitude, vector.y / magnitude};
}

// --GENERATE LIFE--
void DeleteCreature(Creature *creature, bool totalAnihilation)
{
    // If you REALLY want it to get killed, this should do the trick
    if (totalAnihilation)
    {
        idCount++;
        IDsToKill = (int *)realloc(IDsToKill, sizeof(int) * idCount);
        IDsToKill[idCount - 1] = creature->genID;
    }

    // And now we remove it from the tile list
    Tile *thisTile;
    HASH_FIND(hh, tiles, &creature->pos, sizeof(Position), thisTile);
    if (thisTile == NULL)
        return;

    if (creature->tileID + 1 < thisTile->creatureCount)
    {
        printf("MOVING BACK! (%d)\n", thisTile->creatureCount);
        for (int i = creature->tileID + 1; i < thisTile->creatureCount; i++)
        {
            thisTile->creatures[i - 1] = thisTile->creatures[i];
            thisTile->creatures[i - 1].tileID--;
            creatures[thisTile->creatures[i - 1].genID] = &(thisTile->creatures[i - 1]);
            Color c = thisTile->creatures[i - 1].color;
            printf("[%d] at [%d] of color <%d, %d, %d, %d>\n", thisTile->creatures[i].genID, thisTile->key.x, thisTile->key.y, i - 1, c.r, c.b, c.r, c.a);
        }
    }
    thisTile->creatureCount--;
}

Creature *CreateCreature(int xPos, int yPos, Color color, int id)
{
    Position pos = (Position){xPos, yPos};
    Tile *tile;
    HASH_FIND(hh, tiles, &pos, sizeof(Position), tile);
    if (tile == NULL)
    {
        GenerateChunk(xPos, yPos);
        HASH_FIND(hh, tiles, &pos, sizeof(Position), tile);
    }

    tile->creatureCount++;
    tile->creatures = (Creature *)realloc(tile->creatures, sizeof(Creature) * tile->creatureCount);
    int tileID = tile->creatureCount - 1;

    if (id >= 0)
    {
        tile->creatures[tileID] = *(creatures[id]);
        tile->creatures[tileID].pos = pos;
        tile->creatures[tileID].tileID = tileID;

        DeleteCreature(creatures[id], false);
        creatures[id] = &(tile->creatures[tileID]);
        return creatures[id];
    }

    creatureCount++;
    tile->creatures[tileID].pos = pos;
    tile->creatures[tileID].color = color;

    tile->creatures[tileID].genID = creatureCount - 1;
    tile->creatures[tileID].tileID = tileID;
    tile->creatures[tileID].eyesight = 24;

    tile->creatures[tileID].wander = 0.0f;
    tile->creatures[tileID].thirst = 0.0f;
    tile->creatures[tileID].hunger = 0.0f;
    tile->creatures[tileID].funky = 0.0f;

    tile->creatures[tileID].mainObj = OBJ_NONE;
    tile->creatures[tileID].secObj = OBJ_NONE;
    tile->creatures[tileID].foundMainObj = false;
    tile->creatures[tileID].foundSecObj = false;
    tile->creatures[tileID].objPos = (Position){0, 0};

    creatures = (Creature **)realloc(creatures, sizeof(Creature) * creatureCount);
    creatures[creatureCount - 1] = &(tile->creatures[tileID]);
    return creatures[creatureCount - 1];
}

bool GenerateBush(int xPos, int yPos)
{
    Position pos = (Position){xPos, yPos};
    Tile *tile;
    HASH_FIND(hh, tiles, &pos, sizeof(Position), tile);
    if (tile == NULL)
    {
        GenerateChunk(xPos, yPos);
        HASH_FIND(hh, tiles, &pos, sizeof(Position), tile);
    }

    if (tile->biome == BIOME_WATER)
        return false; // No tile to spawn, return false!

    Bush newBush;
    newBush.pos = pos;
    newBush.offset = (Vector2){GetRandomValue(-8, 8) / 20.0f, GetRandomValue(-8, 8) / 20.0f};
    newBush.fruits[0] = GetRandomValue(0, 8) * BUSH_FRUIT_MAX / 12.f;
    newBush.fruits[1] = GetRandomValue(0, 8) * BUSH_FRUIT_MAX / 12.f;
    newBush.fruits[2] = GetRandomValue(0, 8) * BUSH_FRUIT_MAX / 12.f;
    tile->bush = newBush;
    tile->bushExists = true;

    bushCount++;
    bushesPos = (Position *)realloc(bushesPos, sizeof(Position) * bushCount);
    bushesPos[bushCount - 1] = pos;
    return true; // It worked! I think... Return true :)
}

void GenerateLife()
{
    int c = 24;
    for (int i = 0; i < c; i++)
    {
        CreateCreature(GetRandomValue(60, 300), GetRandomValue(60, 300), ColorFromHSV(i * 360.0f / c, 1.0f, 1.0f), -1);
    }

    int b = 56;
    for (int i = 0; i < b; i++)
    {
        while (!GenerateBush(GetRandomValue(0, 360), GetRandomValue(0, 360)))
        {
            // Idk man, I guess we just repeat the check again :/
        }
    }
}

// --ACTUAL CREATURE BEHAVIOUR--
void CreatureWander(Creature *creature)
{
    if (creature->wander > 0)
        return;

    Position pos = closestPositions[creature->eyesight][GetRandomValue(1, closestPositionCount[creature->eyesight] - 1)];
    Position posArray[8] = {
        (Position){pos.x, pos.y},
        (Position){-pos.x, pos.y},
        (Position){pos.x, -pos.y},
        (Position){-pos.x, -pos.y},
        (Position){pos.y, pos.x},
        (Position){-pos.y, pos.x},
        (Position){pos.y, -pos.x},
        (Position){-pos.y, -pos.x},
    };

    Position objective = posArray[GetRandomValue(0, 7)];
    creature->velocity = VectorNormalize(((Vector2){objective.x, objective.y}));
    creature->wander = CREATURE_WANDER_TRESHOLD;
}

bool *CheckCreatureObjective(Creature *creature, Position posToCheck, ObjectiveType mainObj, ObjectiveType secObj)
{
    bool *returnArray = (bool *)malloc(sizeof(bool) * 2);
    returnArray[0] = false;
    returnArray[1] = false;

    Tile *tileToCheck;
    HASH_FIND(hh, tiles, &posToCheck, sizeof(Position), tileToCheck);
    if (tileToCheck == NULL)
    {
        GenerateChunk(creature->pos.x, creature->pos.y);
        HASH_FIND(hh, tiles, &creature->pos, sizeof(Position), tileToCheck);
    }

    for (int obj = 0; obj < 2; obj++)
    {
        ObjectiveType currentObj = obj ? secObj : mainObj;
        switch (currentObj)
        {
        case OBJ_FOOD:
            if (!tileToCheck->bushExists)
                break;

            returnArray[obj] = (tileToCheck->bush.fruits[0] == BUSH_FRUIT_MAX || tileToCheck->bush.fruits[1] == BUSH_FRUIT_MAX || tileToCheck->bush.fruits[2] == BUSH_FRUIT_MAX);
            break;

        case OBJ_WATER:
            returnArray[obj] = tileToCheck->biome == BIOME_WATER;
            break;

        case OBJ_FUNKY:
            if (!tileToCheck->creatureCount)
                break;

            for (int i = 0; i < tileToCheck->creatureCount; i++)
            {
                if (tileToCheck->creatures[i].funky >= CREATURE_FUNKY_TRESHOLD)
                {
                    returnArray[obj] = true;
                    break;
                }
            }
            break;

        default:
            return returnArray;
        }
    }

    return returnArray;
}

void UpdateCreatureObjective(Creature *creature)
{
    // Figure out what even are our objective priorities
    ObjectiveType mainObj = OBJ_NONE;
    ObjectiveType secObj = OBJ_NONE;

    if (creature->thirst > CREATURE_THRIST_TRESHOLD)
    {
        if (creature->hunger > CREATURE_HUNGER_TRESHOLD)
        {
            if (creature->hunger > creature->thirst)
            {
                mainObj = OBJ_FOOD;
                secObj = OBJ_WATER;
            }
            else
            {
                mainObj = OBJ_WATER;
                secObj = OBJ_FOOD;
            }
        }
        else
        {
            mainObj = OBJ_WATER;
            if (creature->funky > CREATURE_FUNKY_TRESHOLD)
            {
                secObj = OBJ_FOOD;
            }
        }
    }
    else
    {
        if (creature->hunger > CREATURE_HUNGER_TRESHOLD)
        {
            mainObj = OBJ_FOOD;
        }
        else if (creature->funky > CREATURE_FUNKY_TRESHOLD)
        {
            mainObj = OBJ_FUNKY;
        }
    }

    if (mainObj == OBJ_NONE)
    {
        CreatureWander(creature);
        return;
    }

    if ((creature->foundMainObj && creature->mainObj == mainObj) || (creature->foundSecObj && creature->secObj == secObj))
        return;

    creature->mainObj = mainObj;
    creature->secObj = secObj;

    Position objectivePos = (Position){0, 0};
    ObjectiveType foundObjective = OBJ_NONE;
    bool foundMain = false;

    // Use the handy lookup table thing I did to search "efficiently"
    for (int i = 1; i < closestPositionCount[creature->eyesight - 1]; i++)
    {
        // What comes next is mirroring the values to account for a quarter circle, alongside flipping it on the x and y axis to count for the whole circle (prob has repeat checks at the 0s but whatever)
        for (int yMult = 1; yMult >= -1; yMult -= 2)
        {
            for (int xMult = 1; xMult >= -1; xMult -= 2)
            {
                Position closePos = closestPositions[creature->eyesight - 1][i];

                // ---CHECK POS 1---
                objectivePos.x = creature->pos.x + closePos.x * xMult;
                objectivePos.y = creature->pos.y + closePos.y * yMult;
                if (DEBUG_CREATUREVISION)
                    DrawRectangle((objectivePos.x - camX) * TILESIZE, (objectivePos.y - camY) * TILESIZE, TILESIZE, TILESIZE, (Color){creature->color.r, creature->color.g, creature->color.b, 159});

                bool *objectiveCheck1 = CheckCreatureObjective(creature, objectivePos, creature->mainObj, creature->secObj);
                for (int obj = 0; obj < 2; obj++)
                {
                    ObjectiveType objToCheck = obj ? creature->secObj : creature->mainObj;
                    if (objToCheck == OBJ_NONE)
                        break;

                    if (objectiveCheck1[obj])
                    {
                        foundObjective = objToCheck;
                        foundMain = !obj;
                        break;
                    }
                }
                if (foundMain)
                    break;

                // ---CHECK POS 2---
                objectivePos.x = creature->pos.x + closePos.y * xMult;
                objectivePos.y = creature->pos.y + closePos.x * yMult;
                if (DEBUG_CREATUREVISION)
                    DrawRectangle((objectivePos.x - camX) * TILESIZE, (objectivePos.y - camY) * TILESIZE, TILESIZE, TILESIZE, (Color){creature->color.r, creature->color.g, creature->color.b, 159});

                bool *objectiveCheck2 = (bool *)malloc(sizeof(bool) * OBJ_COUNT);
                objectiveCheck2 = CheckCreatureObjective(creature, objectivePos, creature->mainObj, creature->secObj);
                for (int obj = 0; obj < 2; obj++)
                {
                    ObjectiveType objToCheck = obj ? creature->secObj : creature->mainObj;
                    if (objToCheck == OBJ_NONE)
                        break;

                    if (objectiveCheck2[obj])
                    {
                        foundObjective = objToCheck;
                        foundMain = !obj;
                        break;
                    }
                }
                if (foundMain)
                    break;
            }
            if (foundMain)
                break;
        }
        if (foundMain)
            break;
    }

    // Check if the objective was found
    if (foundObjective != OBJ_NONE)
    {
        // Get make difference vector, then normalize it and apply it as velocity, it's as shrimple as that :)
        Vector2 diffVector = {objectivePos.x - creature->pos.x, objectivePos.y - creature->pos.y};
        creature->velocity = VectorNormalize(diffVector);
        creature->objPos = objectivePos;

        if (foundMain)
            creature->foundMainObj = true;
        else
            creature->foundSecObj = true;

        return;
    }

    CreatureWander(creature);
}

void UpdateCreaturePosition(Creature *creature)
{
    Position creaturePos = creature->pos;

    // X Position and Offset
    creature->offset.x += creature->velocity.x * CREATURE_SPEED_INC;
    if (creature->offset.x > 0.5f)
    {
        creature->offset.x -= 1.0f;
        creaturePos.x += 1;
    }
    else if (creature->offset.x < -0.5f)
    {
        creature->offset.x += 1.0f;
        creaturePos.x -= 1;
    }

    // Y Position and Offset
    creature->offset.y += creature->velocity.y * CREATURE_SPEED_INC;
    if (creature->offset.y > 0.5f)
    {
        creature->offset.y -= 1.0f;
        creaturePos.y += 1;
    }
    else if (creature->offset.y < -0.5f)
    {
        creature->offset.y += 1.0f;
        creaturePos.y -= 1;
    }

    // If the creature has moved, move it to the tile it moved to
    if (creaturePos.x == creature->pos.x && creaturePos.y == creature->pos.y)
        return;

    Tile *oldTile, *newTile;
    HASH_FIND(hh, tiles, &creature->pos, sizeof(Position), oldTile);
    HASH_FIND(hh, tiles, &creaturePos, sizeof(Position), newTile);
    if (newTile == NULL)
    {
        GenerateChunk(creaturePos.x, creaturePos.y);
        HASH_FIND(hh, tiles, &creaturePos, sizeof(Position), newTile);
    }
    if (oldTile == NULL)
        return;

    // Actual moving of the creature
    creature = CreateCreature(creaturePos.x, creaturePos.y, creature->color, creature->genID);

    if (!creature->foundMainObj)
        UpdateCreatureObjective(creature);

    // Check if the creature is at its objective (and if even has one)
    ObjectiveType objToCheck = creature->foundMainObj ? creature->mainObj : (creature->foundSecObj ? creature->secObj : OBJ_NONE);
    if (objToCheck == OBJ_NONE || creaturePos.x != creature->objPos.x || creaturePos.y != creature->objPos.y)
        return;

    switch (objToCheck)
    {
    case OBJ_FOOD:
        if (!newTile->bushExists)
            break;

        for (int fruit = 0; fruit < 3; fruit++)
        {
            if (newTile->bush.fruits[fruit] == BUSH_FRUIT_MAX)
            {
                creature->hunger = 0;
                newTile->bush.fruits[fruit] = 0;
                break;
            }
        }
        break;

    case OBJ_WATER:
        if (newTile->biome != BIOME_WATER)
            break;

        creature->thirst = 0;
        break;

    case OBJ_FUNKY:
        if (newTile->creatureCount < 2)
            break;

        for (int i = 0; i < newTile->creatureCount; i++)
        {
            if (i == creature->tileID)
                continue;

            Creature *creatureToCheck = &newTile->creatures[i];
            if (creatureToCheck->funky > CREATURE_FUNKY_TRESHOLD)
            {
                creature->funky -= CREATURE_FUNKY_TRESHOLD;
                creatureToCheck -= CREATURE_FUNKY_TRESHOLD;
                Color childColor = {((int)creature->color.r + (int)creatureToCheck->color.r) / 2, ((int)creature->color.g + (int)creatureToCheck->color.g) / 2, ((int)creature->color.b + (int)creatureToCheck->color.b) / 2, 255};
                CreateCreature(creaturePos.x, creaturePos.y, childColor, -1);
                break;
            }
        }
        break;

    default:
        break;
    }
    creature->mainObj = OBJ_NONE;
    creature->secObj = OBJ_NONE;
    creature->foundMainObj = false;
    creature->foundSecObj = false;

    UpdateCreatureObjective(creature);
}

void UpdateCreature(Creature *creature)
{
    // Update creature stats
    creature->thirst += CREATURE_THIRST_INC;
    creature->hunger += CREATURE_HUNGER_INC;
    if (creature->thirst > CREATURE_THRIST_MAX || creature->hunger > CREATURE_HUNGER_MAX)
    {
        // Kill it if it should be obliterated
        DeleteCreature(creature, true);
        return;
    }
    creature->funky += creature->funky < CREATURE_FUNKY_MAX ? CREATURE_FUNKY_INC : 0;
    creature->wander -= creature->wander > 0 ? CREATURE_WANDER_INC : 0;

    // And lastly run the updating functions
    UpdateCreatureObjective(creature);
    UpdateCreaturePosition(creature);
}

// --BUSH BEHAVIOUR--
void UpdateBush(Bush *bush)
{
    bush->fruits[0] = min(bush->fruits[0] + BUSH_FRUIT_INC, BUSH_FRUIT_MAX);
    bush->fruits[1] = min(bush->fruits[1] + BUSH_FRUIT_INC, BUSH_FRUIT_MAX);
    bush->fruits[2] = min(bush->fruits[2] + BUSH_FRUIT_INC, BUSH_FRUIT_MAX);
}

void UpdateLife()
{
    // BUSHES
    Tile *tile;
    if (bushCount)
    {
        for (int i = 0; i < bushCount; i++)
        {
            HASH_FIND(hh, tiles, &bushesPos[i], sizeof(Position), tile);
            if (tile == NULL || !tile->bushExists)
                continue;

            UpdateBush(&tile->bush);
        }
    }

    idCount = 0;
    free(IDsToKill);
    IDsToKill = (int *)malloc(sizeof(int));

    if (creatureCount)
    {
        for (int i = 0; i < creatureCount; i++)
        {
            UpdateCreature(creatures[i]);
        }
    }

    if (!idCount)
        return;

    for (int id = 0; id < idCount; id++)
    {
        if (IDsToKill[id] + 1 < creatureCount)
        {
            for (int i = IDsToKill[id] + 1; i < creatureCount; i++)
            {
                creatures[i]->genID = i - 1;
                creatures[i - 1] = creatures[i];
            }
        }
        creatureCount--;
    }
}

void InitCreatures()
{
    // VISION LOOKUP TABLE THINGY, GO!
    // Oh boi is this inneficient, but if it works it works... Right?
    for (int vision = 1; vision <= CREATURE_VISION_MAX; vision++)
    {
        int visionSquared = vision * vision;
        int *maxCircleLength = (int *)malloc(sizeof(int) * (vision + 1));
        for (int maxC = 0; maxC <= vision; maxC++)
            maxCircleLength[maxC] = ceil(sqrt(visionSquared - (maxC * maxC)));

        int sizeThing = vision * vision;
        int indexer = 0;

        Position *vectorThemselves = (Position *)malloc(sizeof(Position) * sizeThing);
        float *vectorDistance = (float *)malloc(sizeof(float) * sizeThing);
        for (int y = 0; y < vision + 1; y++)
        {
            if (y >= maxCircleLength[y])
                break;

            for (int x = y; x < maxCircleLength[y]; x++)
            {
                vectorThemselves[indexer] = (Position){x, y};
                vectorDistance[indexer] = VectorMagnitude((Vector2){x + 1, y + 1});
                indexer++;
            }
        }

        closestPositions[vision - 1] = (Position *)malloc(sizeof(Position) * indexer);
        closestPositionCount[vision - 1] = indexer;
        for (int orgLoop = 0; orgLoop < indexer; orgLoop++)
        {
            float currentSmallest = -1.0f;
            int currentSmallestID = -1;
            for (int org = 0; org < indexer; org++)
            {
                if (vectorThemselves[org].x == -1 || (vectorDistance[org] >= currentSmallest && currentSmallest >= 0.0f))
                    continue;

                currentSmallest = vectorDistance[org];
                currentSmallestID = org;
            }

            if (currentSmallestID >= 0)
            {
                closestPositions[vision - 1][orgLoop] = vectorThemselves[currentSmallestID];
                vectorThemselves[currentSmallestID].x = -1;
            }
            else
                closestPositionCount[vision - 1]--;
        }
    }
}