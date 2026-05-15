#include "common.h"

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

    for (int off = 0; off < creature->eyesight; off++)
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
    }

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