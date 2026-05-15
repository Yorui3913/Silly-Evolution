#include "common.h"

Position *closestPositions[CREATURE_VISION_MAX];
int closestPositionCount[CREATURE_VISION_MAX];
float baseCreatureSpeed = 0.0125f;

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
void UpdateCreatureVelocity(Creature *creature)
{
    // Set some variables, like what are we even trying to find
    Biome objective = BIOME_WATER;
    Position objectivePos = {0, 0};
    bool foundObjective = false;
    Tile *tileToCheck;

    // Is it at the objective already?
    Position thisPos = creature->key;
    HASH_FIND(hh, tiles, &thisPos, sizeof(Position), tileToCheck);
    if (tileToCheck != NULL && tileToCheck->biome == objective)
    {
        // Oh wow it is, guess I can skip this whole part now :/
        creature->velocity = (Vector2){0, 0};
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

                Position relativePos = {creature->key.x + closePos.x * xMult, creature->key.y + closePos.y * yMult};

                if (DEBUG_CREATUREVISION)
                    DrawRectangle((relativePos.x - camX) * TILESIZE, (relativePos.y - camY) * TILESIZE, TILESIZE, TILESIZE, (Color){creature->color.r, creature->color.g, creature->color.b, 159});

                HASH_FIND(hh, tiles, &relativePos, sizeof(Position), tileToCheck);
                if (tileToCheck != NULL && tileToCheck->biome == objective)
                {
                    objectivePos = relativePos;
                    foundObjective = true;
                    break;
                }

                Position relativePosRev = {creature->key.x + closePos.y * xMult, creature->key.y + closePos.x * yMult};
                
                if (DEBUG_CREATUREVISION)
                    DrawRectangle((relativePosRev.x - camX) * TILESIZE, (relativePosRev.y - camY) * TILESIZE, TILESIZE, TILESIZE, (Color){creature->color.r, creature->color.g, creature->color.b, 159});
                
                HASH_FIND(hh, tiles, &relativePos, sizeof(Position), tileToCheck);
                if (tileToCheck != NULL && tileToCheck->biome == objective)
                {
                    objectivePos = relativePosRev;
                    foundObjective = true;
                    break;
                }
            }
            if (foundObjective)
                break;
        }
        if (foundObjective)
                break;
    }

    /*for (int off = 0; off < creature->eyesight; off++)
    {
        // ABOVE AND BELLOW CHECK
        for (int xRange = -(off + 1); xRange <= off + 1; xRange++)
        {
            Position abovePos = {creature->key.x + xRange, creature->key.y - (off + 1)};
            HASH_FIND(hh, tiles, &abovePos, sizeof(Position), tileToCheck);
            if (tileToCheck != NULL && tileToCheck->biome == objective)
            {
                objectivePos = abovePos;
                foundObjective = true;
                break;
            }

            Position bellowPos = {creature->key.x + xRange, creature->key.y + off + 1};
            HASH_FIND(hh, tiles, &bellowPos, sizeof(Position), tileToCheck);
            if (tileToCheck != NULL && tileToCheck->biome == objective)
            {
                objectivePos = bellowPos;
                foundObjective = true;
                break;
            }
        }
        if (foundObjective)
            break;

        // SIDE CHECK
        for (int yRange = -off; yRange <= off; yRange++)
        {
            Position rightPos = {creature->key.x + off + 1, creature->key.y + yRange};
            HASH_FIND(hh, tiles, &rightPos, sizeof(Position), tileToCheck);
            if (tileToCheck != NULL && tileToCheck->biome == objective)
            {
                objectivePos = rightPos;
                foundObjective = true;
                break;
            }

            Position leftPos = {creature->key.x - (off + 1), creature->key.y + yRange};
            HASH_FIND(hh, tiles, &leftPos, sizeof(Position), tileToCheck);
            if (tileToCheck != NULL && tileToCheck->biome == objective)
            {
                objectivePos = leftPos;
                foundObjective = true;
                break;
            }
        }
        if (foundObjective)
            break;
    }*/

    // Check if the objective was found
    if (foundObjective)
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
    creature->offset.x += creature->velocity.x * baseCreatureSpeed;
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
    creature->offset.y += creature->velocity.y * baseCreatureSpeed;
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
    UpdateCreatureVelocity(creature);
    UpdateCreaturePosition(creature);
}

void UpdateCreatures()
{
    free(oldCreaturePos);
    free(newCreaturePos);
    oldCreaturePos = (Position *)malloc(sizeof(Position));
    newCreaturePos = (Position *)malloc(sizeof(Position));
    changeCreatureCount = 0;

    Creature *creature, *tmp;
    HASH_ITER(hh, creatures, creature, tmp)
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