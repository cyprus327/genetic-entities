#include <stdlib.h>

#include <raylib.h>
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include "main.h"

#define RL_VEC(v) ((Vector2){(v)[0], (v)[1]})

i32 windowWidth = 800, windowHeight = 600;

f32 mutationChance = DEFAULT_MUTATION_CHANCE;
f32 mutationMagnitude = DEFAULT_MUTATION_MAGNITUDE;
f32 popMagnitude = DEFAULT_POP_MAGNITUDE;

i32 main(void) {
    SetTargetFPS(60);
    InitWindow(windowWidth, windowHeight, "Genetic Entities");

    GuiSetStyle(DEFAULT, BASE_COLOR_NORMAL, 0x10101060);

    State state = { 
        .entitySpawnPos = {100, windowHeight / 2.f}, 
        .entityTargetPos = {windowWidth - 100, windowHeight / 2.f},
        .fastMode = 0
    };
    state_init(&state);

    // TODO moving obstacles and end pos with mouse
    state.obstacles[0] = (Obstacle){
        {320, 100}, {370, windowHeight - 100} };
    state.obstacles[1] = (Obstacle){
        {500, -1000}, {550, windowHeight / 2.f - 70} };
    state.obstacles[2] = (Obstacle){
        {500, windowHeight / 2.f + 70}, {550, windowHeight + 1000} };

    while (!WindowShouldClose()) {
        windowWidth = GetScreenWidth();
        windowHeight = GetScreenHeight();

        // 1 gen per frame or 1 frame per frame
        const i32 n = state.fastMode ? FRAMES_MAX : 1;
        for (i32 i = 0; i < n; i += 1) {
            state_update(&state, 0.8f);
        }

        BeginDrawing(); {
            ClearBackground(BLACK);

            DrawCircle(
                state.entityTargetPos[0], state.entityTargetPos[1],
                TARGET_RAD, GREEN
            );

            DrawCircle(
                state.entitySpawnPos[0], state.entitySpawnPos[1],
                30.f, (Color){20, 100, 180, 255}
            );

            // draw obstacles
            for (i32 i = 0; i < OBSTACLE_MAX; i += 1) {
                const i32 w = state.obstacles[i].max[0] - state.obstacles[i].min[0];
                const i32 h = state.obstacles[i].max[1] - state.obstacles[i].min[1];
                DrawRectangle(state.obstacles[i].min[0], state.obstacles[i].min[1], w, h, GRAY);
            }

            // draw entities
            for (i32 i = 0; i < ENTITY_MAX; i += 1) {
                const Entity* e = &state.entities[i];
                const vec2 p = { e->pos[0], e->pos[1] };
                const vec2 v = { e->vel[0], e->vel[1] };

                // triangle dimensions
                const f32 height = ENTITY_SIZE;
                const f32 width = ENTITY_SIZE * 0.6f;

                // triangle vertices
                vec2 top = { p[0], p[1] - height / 2.f };
                vec2 bottomLeft = { p[0] - width / 2.f, p[1] + height / 2.f };
                vec2 bottomRight = { p[0] + width / 2.f, p[1] + height / 2.f };

                // rotation based on velocity
                const f32 rot = atan2f(v[1], v[0]) + 3.14159f / 2.f;
                VEC_ROTATE(top, p, rot);
                VEC_ROTATE(bottomLeft, p, rot);
                VEC_ROTATE(bottomRight, p, rot);

                const Color col =
                    STATE_ALIVE == e->state ? (Color){10, 10, 255, 80} :
                    STATE_COMPLETED == e->state ? (Color){200, 250, 10, 80} :
                    (Color){255, 0, 0, 80};
                DrawTriangle(RL_VEC(top), RL_VEC(bottomLeft), RL_VEC(bottomRight), col);
            }

            // ui
            i32 x = 20, y = 20;
            DrawText(
                TextFormat("GEN: %d (frame %3d)", state.gen, state.currFrame),
                x, y, 30, GRAY
            );

            y += 40;
            if (GuiButton((Rectangle){x, y, 120, 20}, "Reset Sliders")) {
                mutationChance = DEFAULT_MUTATION_CHANCE;
                mutationMagnitude = DEFAULT_MUTATION_MAGNITUDE;
                popMagnitude = DEFAULT_POP_MAGNITUDE;
            }

            x += 130;
            if (GuiButton((Rectangle){x, y, 120, 20}, "Reset Sim")) {
                state_release(&state);
                state_init(&state);
            }

            x += 130;
            GuiToggle((Rectangle){x, y, 80, 20}, "Fast Mode", (_Bool*)(&state.fastMode));

            x = 20;
            y += 30;
            DrawText(TextFormat("%.3f | Mutation Chance", mutationChance), x, y, 14, GRAY);
            GuiSlider((Rectangle){x, y + 20, 150, 20}, "", "", &mutationChance, 0.f, 1.f);

            y += 50;
            DrawText(TextFormat("%.3f | Mutation Magnitude", mutationMagnitude), x, y, 14, GRAY);
            GuiSlider((Rectangle){x, y + 20, 150, 20}, "", "", &mutationMagnitude, 0.f, 1.f);

            y += 50;
            DrawText(TextFormat("%.3f | Pop Magnitude", popMagnitude), x, y, 14, GRAY);
            GuiSlider((Rectangle){x, y + 20, 150, 20}, "", "", &popMagnitude, 0.f, 1.f);

            DrawFPS(10, windowHeight - 30);
        } EndDrawing();
    }

    state_release(&state);
    CloseWindow();
    return 0;
}

// :state
void state_init(State* state) {
    state->gen = state->currFrame = 0;

    if (state->entities) {
        free(state->entities);
    }
    if (state->nextEntities) {
        free(state->nextEntities);
    }
    
    state->entities = (Entity*)malloc(sizeof(Entity) * ENTITY_MAX);
    if (!state->entities) {
        ERR_EXIT("state_init: failed to allocate entities");
    }

    state->nextEntities = (Entity*)malloc(sizeof(Entity) * ENTITY_MAX);
    if (!state->nextEntities) {
        ERR_EXIT("state_init: failed to allocate nextEntities");
    }

    for (i32 i = 0; i < ENTITY_MAX; i += 1) {
        entity_init(&state->entities[i], state->entitySpawnPos);
        entity_init(&state->nextEntities[i], state->entitySpawnPos);
    }
}

void state_release(State* state) {
    for (i32 i = 0; i < ENTITY_MAX; i += 1) {
        entity_release(&state->entities[i]);
        entity_release(&state->nextEntities[i]);
    }

    free(state->entities);
    free(state->nextEntities);
    state->entities = state->nextEntities = NULL;
}

void state_new_generation(State* state) {
    // update fitness
    for (i32 i = 0; i < ENTITY_MAX; i += 1) {
        f32 fitness = entity_calc_fitness(&state->entities[i], state->entityTargetPos);
        state->entities[i].fitness = fitness;
    }

    // pop some for variation
    const i32 popCount = (i32)(ENTITY_MAX * popMagnitude);

    for (i32 i = 0; i < popCount; i += 1) {
        entity_reset(&state->nextEntities[i], state->entitySpawnPos);
    }

    // create new population
    for (i32 i = popCount; i < ENTITY_MAX; i += 1) {
        // tournament selection instead of gene pool
        const i32 parent1 = entity_tournament_select(state, 7);
        const i32 parent2 = entity_tournament_select(state, 7);

        entity_crossover_genes(
            &state->nextEntities[i],
            &state->entities[parent1],
            &state->entities[parent2],
            state
        );
        
        entity_mutate_genes(&state->nextEntities[i]);
    }

    // swap buffers
    Entity* temp = state->entities;
    state->entities = state->nextEntities;
    state->nextEntities = temp;
}

void state_end_generation(State* state) {
    // move to next generation
    state->gen += 1;
    state->currFrame = 0;
}

void state_update(State* state, f32 delta) {
    // this generation is done
    if (state->currFrame >= FRAMES_MAX) {
        state_end_generation(state);
        state_new_generation(state);
    }

    // update
    for (i32 i = 0; i < ENTITY_MAX; i += 1) {
        entity_update(&state->entities[i], state, delta);
    }

    state->currFrame += 1;
}
// :end state

// :entity
void entity_init(Entity* entity, const vec2 spawnPos) {
    entity->genes = (vec2*)malloc(sizeof(vec2) * FRAMES_MAX);
    if (!entity->genes) {
        ERR_EXIT("entity_init: failed to allocate genes");
    }

    entity_reset(entity, spawnPos);
}

void entity_release(Entity* entity) {
    if (entity->genes) {
        free(entity->genes);
        entity->genes = NULL;
    }
}

void entity_reset(Entity* entity, const vec2 spawnPos) {
    VEC_SET(entity->pos, spawnPos);
    VEC_SET(entity->vel, VEC_ZERO);

    entity->state = STATE_ALIVE;

    entity->fitness = 0.f;
    entity->framesToFinish = FRAMES_MAX;

    for (i32 i = 0; i < FRAMES_MAX; i += 1) {
        entity->genes[i][0] = randf(-1.f, 1.f);
        entity->genes[i][1] = randf(-1.f, 1.f);
    }
}

void entity_update(Entity* entity, const State* state, f32 delta) {
    if (STATE_ALIVE != entity->state) {
        return;
    }

    // handle obstacles
    for (i32 i = 0; i < OBSTACLE_MAX; i += 1) {
        if (VEC_INRANGE(entity->pos,
            state->obstacles[i].min[0], state->obstacles[i].max[0],
            state->obstacles[i].min[1], state->obstacles[i].max[1]))
        {
            entity->state = STATE_FAILED;
            return;
        }
    }

    // handle reaching the finish
    vec2 diff;
    VEC_SUB(diff, entity->pos, state->entityTargetPos);
    const f32 len = VEC_LENGTH2(diff);
    if (len <= TARGET_RAD * TARGET_RAD) {
        entity->state = STATE_COMPLETED;
        entity->framesToFinish = state->currFrame;
        return;
    }

    // keep 0.97-0.99
    VEC_ADD(entity->vel, entity->vel, entity->genes[state->currFrame]);
    VEC_MULF(entity->vel, entity->vel, 0.97f);

    // TODO
    // delta isn't frame time so that decresing fps decreases sim's speed
    // so there's no point of this right now
    vec2 scaledVel;
    VEC_MULF(scaledVel, entity->vel, delta);

    VEC_ADD(entity->pos, entity->pos, scaledVel);
}

// TODO with obstacles added this sucks
f32 entity_calc_fitness(const Entity* entity, const vec2 targetPos) {
    vec2 diff;
    VEC_SUB(diff, entity->pos, targetPos);
    const f32 dist = VEC_LENGTH2(diff);
    
    if (entity->state == STATE_COMPLETED) {
        const f32 timeMult = 1.f - (entity->framesToFinish / (f32)FRAMES_MAX);
        return 1000.f + (500.f * timeMult);
    }
    return 100.f / (dist + 100.f);
}

void entity_crossover_genes(Entity* child, Entity* first, Entity* second, const State* state) {
    entity_reset(child, state->entitySpawnPos);

    // second is better but it seems like it shouldn't be

    /*
    for (i32 i = 0; i < FRAMES_MAX; i += 1) {
        if (randf(0.f, 1.f) < 0.5f) {
            VEC_SET(child->genes[i], first->genes[i]);
        } else {
            VEC_SET(child->genes[i], second->genes[i]);
        }
    }
    // */

    // /*
    const i32 split = GetRandomValue(0, FRAMES_MAX - 1);
    for (i32 i = 0; i < split; i += 1) {
        VEC_SET(child->genes[i], first->genes[i]);
    }
    for (i32 i = split; i < FRAMES_MAX; i += 1) {
        VEC_SET(child->genes[i], second->genes[i]);
    }
    // */
}

void entity_mutate_genes(Entity* entity) {
    for (i32 i = 0; i < FRAMES_MAX; i += 1) {
        if (randf(0.f, 1.f) > mutationChance) {
            continue;
        }

        // small perturbations
        entity->genes[i][0] += randf(-1.f, 1.f) * mutationMagnitude;
        entity->genes[i][0] = CLAMP(entity->genes[i][0], -1.f, 1.f);
        entity->genes[i][1] += randf(-1.f, 1.f) * mutationMagnitude;
        entity->genes[i][1] = CLAMP(entity->genes[i][1], -1.f, 1.f);
    }
}

i32 entity_tournament_select(const State* state, i32 size) {
    i32 best = GetRandomValue(0, ENTITY_MAX - 1);
    for (i32 i = 1; i < size; i += 1) {
        const i32 candidate = GetRandomValue(0, ENTITY_MAX - 1);
        if (state->entities[candidate].fitness > state->entities[best].fitness) {
            best = candidate;
        }
    }
    return best;
}
// :end entity

f32 randf(f32 min, f32 max) {
    // precision, because this isn't called with big min and max this is fine
    #define P 10000000
    return GetRandomValue((i32)(min * P), (i32)(max * P)) / (f32)P;
}
