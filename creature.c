#include "common.h"

Position *closestPositions[CREATURE_VISION_MAX];
int closestPositionCount[CREATURE_VISION_MAX];
Vector2 fruitOffset[3] = {{-0.6f, 0.4}, {0.7, 0.55}, {0.2, -0.6}};

Position *oldCreaturePos;
Position *newCreaturePos;
int changeCreatureCount;

// --VECTOR HANDLING STUFF--
float VectorMagnitude(Vector2 vector)
{
    return sqrtf(vector.x * vector.x + vector.y * vector.y);
}

Vector2 VectorNormalize(Vector2 vector)
{
    float magnitude = VectorMagnitude(vector);
    if (!magnitude)
        return (Vector2){0, 0};

    return (Vector2){vector.x / magnitude, vector.y / magnitude};
}

// --ACTUAL CREATURE BEHAVIOUR--

bool CheckCreatureObjective(Creature *creature, Position posToCheck, Objective obj)
{
    Bush *bushToCheck;
    Tile *tileToCheck;

    switch (obj)
    {
    case OBJ_FOOD:
        // Find Food
        HASH_FIND(hh, bushes, &posToCheck, sizeof(Position), bushToCheck);
        return bushToCheck != NULL && (bushToCheck->fruits[0] == BUSH_FRUIT_MAX || bushToCheck->fruits[1] == BUSH_FRUIT_MAX || bushToCheck->fruits[2] == BUSH_FRUIT_MAX);

    case OBJ_WATER:
        // Find water
        HASH_FIND(hh, tiles, &posToCheck, sizeof(Position), tileToCheck);
        return tileToCheck != NULL && tileToCheck->biome == BIOME_WATER;

    default:
        // W H A T
        return false;
    }
}

void UpdateCreatureVelocity(Creature *creature)
{
    // Set some variables, like what are we even trying to find
    Objective objectives[OBJ_COUNT];
    if (creature->hunger > creature->thirst)
    {
        objectives[0] = OBJ_FOOD;
        objectives[1] = OBJ_WATER;
    }
    else
    {
        objectives[0] = OBJ_WATER;
        objectives[1] = OBJ_FOOD;
    }
    Position objectivePos = {0, 0};
    int foundObjective = -1;

    // Is it at the objective already?
    Position thisPos = creature->key;
    for (int obj = 0; obj < OBJ_COUNT; obj++)
    {
        if (CheckCreatureObjective(creature, thisPos, objectives[obj]))
        {
            // Oh wow it is, guess I can skip this whole part now :/
            creature->velocity = (Vector2){0, 0};

            if (objectives[obj] == OBJ_FOOD)
            {
                Bush *bushToCheck;
                HASH_FIND(hh, bushes, &thisPos, sizeof(Position), bushToCheck);
                if (bushToCheck == NULL)
                    continue;

                for (int fruit = 0; fruit < 3; fruit++)
                {
                    if (bushToCheck->fruits[fruit] == BUSH_FRUIT_MAX)
                    {
                        creature->hunger = 0;
                        bushToCheck->fruits[fruit] = 0;
                        return;
                    }
                }
            }
            else if (objectives[obj] == OBJ_WATER)
                creature->thirst = 0;

            return;
        }

        // Use the handy lookup table thing I did to search "efficiently"
        for (int i = 1; i < closestPositionCount[creature->eyesight - 1]; i++)
        {
            // What comes next is mirroring the values to account for a quarter circle, alongside flipping it on the x and y axis to count for the whole circle (prob has repeat checks at the 0s but whatever)
            for (int yMult = 1; yMult >= -1; yMult -= 2)
            {
                for (int xMult = 1; xMult >= -1; xMult -= 2)
                {
                    Position closePos = closestPositions[creature->eyesight - 1][i];

                    objectivePos.x = creature->key.x + closePos.x * xMult;
                    objectivePos.y = creature->key.y + closePos.y * yMult;
                    if (DEBUG_CREATUREVISION)
                        DrawRectangle((objectivePos.x - camX) * TILESIZE, (objectivePos.y - camY) * TILESIZE, TILESIZE, TILESIZE, (Color){creature->color.r, creature->color.g, creature->color.b, 159});

                    if (CheckCreatureObjective(creature, objectivePos, objectives[obj]))
                    {
                        foundObjective = obj;
                        break;
                    }

                    objectivePos.x = creature->key.x + closePos.y * xMult;
                    objectivePos.y = creature->key.y + closePos.x * yMult;
                    if (DEBUG_CREATUREVISION)
                        DrawRectangle((objectivePos.x - camX) * TILESIZE, (objectivePos.y - camY) * TILESIZE, TILESIZE, TILESIZE, (Color){creature->color.r, creature->color.g, creature->color.b, 159});

                    if (CheckCreatureObjective(creature, objectivePos, objectives[obj]))
                    {
                        foundObjective = obj;
                        break;
                    }
                }
                if (foundObjective != -1)
                    break;
            }
            if (foundObjective != -1)
                break;
        }
        if (foundObjective != -1)
            break;
    }

    // Check if the objective was found
    if (foundObjective != -1)
    {
        // Get make difference vector, then normalize it and apply it as velocity, it's as shrimple as that :)
        Vector2 diffVector = {objectivePos.x - creature->key.x, objectivePos.y - creature->key.y};
        creature->velocity = VectorNormalize(diffVector);
        return;
    }

    // If it doesn't find anything it just stays still I guess, poor guy :(
    creature->velocity = (Vector2){0, 0};
}

void UpdateCreaturePosition(Creature *creature)
{
    Position creaturePos = creature->key;

    // X Position and Offset
    creature->offset.x += creature->velocity.x * CREATURE_SPEED_BASE;
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
    creature->offset.y += creature->velocity.y * CREATURE_SPEED_BASE;
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

    // Add to the "change list" if changed
    if (creaturePos.x != creature->key.x || creaturePos.y != creature->key.y)
    {
        changeCreatureCount++;
        oldCreaturePos = (Position *)realloc(oldCreaturePos, sizeof(Position) * changeCreatureCount);
        newCreaturePos = (Position *)realloc(newCreaturePos, sizeof(Position) * changeCreatureCount);
        oldCreaturePos[changeCreatureCount - 1] = creature->key;
        newCreaturePos[changeCreatureCount - 1] = creaturePos;
    }
}

void UpdateCreature(Creature *creature)
{
    creature->thirst += CREATURE_THIRST_BASE;
    creature->hunger += CREATURE_HUNGER_BASE;
    if (creature->thirst > CREATURE_THRIST_MAX || creature->hunger > CREATURE_HUNGER_MAX)
    {
        // DEATH
        HASH_DEL(creatures, creature);
        return;
    }

    UpdateCreatureVelocity(creature);
    UpdateCreaturePosition(creature);
}

// --BUSH BEHAVIOUR--
void UpdateBush(Bush *bush)
{
    bush->fruits[0] = min(bush->fruits[0] + BUSH_FRUIT_BASE, BUSH_FRUIT_MAX);
    bush->fruits[1] = min(bush->fruits[1] + BUSH_FRUIT_BASE, BUSH_FRUIT_MAX);
    bush->fruits[2] = min(bush->fruits[2] + BUSH_FRUIT_BASE, BUSH_FRUIT_MAX);
}

void UpdateLife()
{
    // BUSHES
    Bush *bush, *tmpBush;
    HASH_ITER(hh, bushes, bush, tmpBush)
    {
        UpdateBush(bush);
    }

    // CREATURES
    free(oldCreaturePos);
    free(newCreaturePos);
    oldCreaturePos = (Position *)malloc(sizeof(Position));
    newCreaturePos = (Position *)malloc(sizeof(Position));
    changeCreatureCount = 0;

    Creature *creature, *tmpCreature;
    HASH_ITER(hh, creatures, creature, tmpCreature)
    {
        UpdateCreature(creature);
    }

    if (!changeCreatureCount)
        return;

    for (int i = 0; i < changeCreatureCount; i++)
    {
        HASH_FIND(hh, creatures, &oldCreaturePos[i], sizeof(Position), creature);
        if (creature == NULL)
            continue;

        HASH_DEL(creatures, creature);
        creature->key = newCreaturePos[i];
        HASH_ADD(hh, creatures, key, sizeof(Position), creature);
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