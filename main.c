#include <stdlib.h>

#include <raylib.h>
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include "main.h"

#define RL_VEC(v) ((Vector2){(v)[0], (v)[1]})

i32 windowWidth = 1200, windowHeight = 900;

f32 mutationChance = DEFAULT_MUTATION_CHANCE;
f32 mutationMagnitude = DEFAULT_MUTATION_MAGNITUDE;
f32 popMagnitude = DEFAULT_POP_MAGNITUDE;

i32 main(void) {
    SetTargetFPS(60);
    // TODO get resizing to work, currently the mouse is misaligned
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(windowWidth, windowHeight, "Genetic Entities");

    GuiSetStyle(DEFAULT, BASE_COLOR_NORMAL, 0x10101060);

    State state = { .fastMode = 0 };
    state.entitySpawnPos[0] = 100.f;
    state.entitySpawnPos[1] = windowWidth / 2.f;
    state.entityTargetPos[0] = windowWidth - 100.f;
    state.entityTargetPos[1] = windowHeight / 2.f;
    state_init(&state);

    while (!WindowShouldClose()) {
        windowWidth = GetScreenWidth();
        windowHeight = GetScreenHeight();

        // TODO moving obstacles and start/end pos with mouse
        state.obstacles[0] = (Obstacle){
            {windowWidth / 3.f, 100},
            {windowWidth / 3.f + 50.f, windowHeight - 100}
        };
        state.obstacles[1] = (Obstacle){
            {windowWidth / 3.f * 2.f, -10000},
            {windowWidth / 3.f * 2.f + 80.f, windowHeight / 2.f - 70}
        };
        state.obstacles[2] = (Obstacle){
            {windowWidth / 3.f * 2.f, windowHeight / 2.f + 70},
            {windowWidth / 3.f * 2.f + 80.f, windowHeight + 10000}
        };

        state.entitySpawnPos[0] = 100.f;
        state.entitySpawnPos[1] = windowHeight / 2.f;
        state.entityTargetPos[0] = windowWidth - 100.f;
        state.entityTargetPos[1] = windowHeight / 2.f;

        if (IsWindowResized()) {
            state_release(&state);
            state_init(&state);
        }

        // 1 gen per frame + 1 frame for some additional movement
        // or 1 frame per frame
        const i32 n = state.fastMode ? FRAMES_MAX + 1 : 1;
        for (i32 i = 0; i < n; i += 1) {
            state_update(&state);
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
            for (i32 i = 0; i < ENTITIES_MAX; i += 1) {
                const vec2 p = { state.entities->posX[i], state.entities->posY[i] };
                const vec2 v = { state.entities->velX[i], state.entities->velY[i] };

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
                    STATE_ALIVE == state.entities->state[i] ? (Color){10, 10, 255, 80} :
                    STATE_COMPLETED == state.entities->state[i] ? (Color){200, 250, 10, 80} :
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

            // DrawFPS(10, windowHeight - 30);

            DrawText("NOTE: The magenta circle is where the browser thinks your cursor is,", 10, windowHeight - 52, 20, WHITE);
            DrawText("it may be wrong and I don't currently know how to fix that.", 10, windowHeight - 30, 20, WHITE);

            const Vector2 mp = GetMousePosition();
            DrawCircle(mp.x, mp.y, 8.f, Fade(MAGENTA, 0.7f));
        } EndDrawing();
    }

    state_release(&state);
    CloseWindow();
    return 0;
}

// :state
void state_init(State* state) {
    state->gen = state->currFrame = 0;
    
    if (!state->entities) {
        state->entities = (Entities*)malloc(sizeof(Entities));
        if (!state->entities) {
            ERR_EXIT("state_init: to allocate entities");
        }
        memset(state->entities, 0, sizeof(Entities));
    }

    if (!state->nextEntities) {
        state->nextEntities = (Entities*)malloc(sizeof(Entities));
        if (!state->nextEntities) {
            ERR_EXIT("state_init: to allocate nextEntities");
        }
        memset(state->nextEntities, 0, sizeof(Entities));
    }

    for (i32 i = 0; i < ENTITIES_MAX; i += 1) {
        state_reset_entity(state, i);
        state_reset_next_entity(state, i);
    }
}

void state_release(State* state) {
    free(state->entities);
    free(state->nextEntities);
    state->entities = state->nextEntities = NULL;
}

void state_new_generation(State* state) {
    // update fitness
    for (i32 i = 0; i < ENTITIES_MAX; i += 1) {
        // TODO some pathfinding or raycasting to work better with obstacles
        const vec2 diff = {
            state->entities->posX[i] - state->entityTargetPos[0],
            state->entities->posY[i] - state->entityTargetPos[1]
        };
        const f32 dist = VEC_LENGTH2(diff);

        if (STATE_COMPLETED == state->entities->state[i]) {
            const f32 timeMult = 1.f - (state->entities->framesToFinish[i] / (f32)FRAMES_MAX);
            state->entities->fitness[i] = 1000.f + (500.f * timeMult);
        } else {
            state->entities->fitness[i] = 100.f / (dist + 100.f);
        }
    }

    const i32 popCount = (i32)(ENTITIES_MAX * popMagnitude);

    for (i32 i = 0; i < ENTITIES_MAX; i += 1) {
        state_reset_next_entity(state, i);
        if (i < popCount) {
            continue;
        }

        // tournament selection instead of gene pool
        const i32 parent1 = entities_tournament_select(state->entities, 6);
        const i32 parent2 = entities_tournament_select(state->entities, 6);

        // single split point works better than continuous for some reason
        const i32 split = GetRandomValue(0, FRAMES_MAX - 1);

        // crossover genes
        for (i32 j = 0; j < split; j += 1) {
            state->nextEntities->genesX[j][i] = state->entities->genesX[j][parent1];
            state->nextEntities->genesY[j][i] = state->entities->genesY[j][parent1];
        }
        for (i32 j = split; j < FRAMES_MAX; j += 1) {
            state->nextEntities->genesX[j][i] = state->entities->genesX[j][parent2];
            state->nextEntities->genesY[j][i] = state->entities->genesY[j][parent2];
        }

        // mutate genes
        for (i32 j = 0; j < FRAMES_MAX; j += 1) {
            if (randf(0.f, 1.f) > mutationChance) continue;

            state->nextEntities->genesX[j][i] += randf(-1.f, 1.f) * mutationMagnitude;
            state->nextEntities->genesX[j][i] = CLAMP(state->nextEntities->genesX[j][i], -1.f, 1.f);
            
            state->nextEntities->genesY[j][i] += randf(-1.f, 1.f) * mutationMagnitude;
            state->nextEntities->genesY[j][i] = CLAMP(state->nextEntities->genesY[j][i], -1.f, 1.f);
        }
    }

    // swap buffers
    Entities* temp = state->entities;
    state->entities = state->nextEntities;
    state->nextEntities = temp;
}

void state_end_generation(State* state) {
    // move to next generation
    state->gen += 1;
    state->currFrame = 0;
}

void state_update(State* state) {
    // this generation is done
    if (state->currFrame >= FRAMES_MAX) {
        state_end_generation(state);
        state_new_generation(state);
    }

    // update entities
    Entities* e = state->entities;
    for (i32 i = 0; i < ENTITIES_MAX; i += 1) {
        if (STATE_ALIVE != e->state[i]) {
            continue;
        }
        // handle obstacles
        const vec2 p = { e->posX[i], e->posY[i] };
        for (i32 j = 0; j < OBSTACLE_MAX; j += 1) {
            if (VEC_INRANGE(p,
                state->obstacles[j].min[0], state->obstacles[j].max[0],
                state->obstacles[j].min[1], state->obstacles[j].max[1]))
            {
                e->state[i] = STATE_FAILED;
                goto CONTINUE;
            }
        }

        // handle reaching the finish
        const vec2 diff = {
            p[0] - state->entityTargetPos[0],
            p[1] - state->entityTargetPos[1]
        };
        const f32 len = VEC_LENGTH2(diff);
        if (len <= TARGET_RAD * TARGET_RAD) {
            e->state[i] = STATE_COMPLETED;
            e->framesToFinish[i] = state->currFrame;
            goto CONTINUE;
        }

        // handle movement
        e->velX[i] = (e->velX[i] + state->entities->genesX[state->currFrame][i]) * 0.97f;
        e->velY[i] = (e->velY[i] + state->entities->genesY[state->currFrame][i]) * 0.97f;

        e->posX[i] += e->velX[i];
        e->posY[i] += e->velY[i];

        CONTINUE:;
    }

    state->currFrame += 1;
}

void state_reset_entity(State* state, i32 i) {
    state->entities->posX[i] = state->entitySpawnPos[0];
    state->entities->posY[i] = state->entitySpawnPos[1];
    state->entities->velX[i] = state->entities->velY[i] = 0.f;

    state->entities->state[i] = STATE_ALIVE;

    state->entities->fitness[i] = 0.f;
    state->entities->framesToFinish[i] = FRAMES_MAX;

    for (i32 j = 0; j < FRAMES_MAX; j += 1) {
        state->entities->genesX[j][i] = randf(-1.f, 1.f);
        state->entities->genesY[j][i] = randf(-1.f, 1.f);
    }
}

void state_reset_next_entity(State* state, i32 i) {
    state->nextEntities->posX[i] = state->entitySpawnPos[0];
    state->nextEntities->posY[i] = state->entitySpawnPos[1];
    state->nextEntities->velX[i] = state->nextEntities->velY[i] = 0.f;

    state->nextEntities->state[i] = STATE_ALIVE;

    state->nextEntities->fitness[i] = 0.f;
    state->nextEntities->framesToFinish[i] = FRAMES_MAX;

    for (i32 j = 0; j < FRAMES_MAX; j += 1) {
        state->nextEntities->genesX[j][i] = randf(-1.f, 1.f);
        state->nextEntities->genesY[j][i] = randf(-1.f, 1.f);
    }
}
// :end state

// :entities
i32 entities_tournament_select(const Entities* entities, i32 size) {
    i32 best = GetRandomValue(0, ENTITIES_MAX - 1);
    for (i32 i = 1; i < size; i += 1) {
        const i32 candidate = GetRandomValue(0, ENTITIES_MAX - 1);
        if (entities->fitness[candidate] > entities->fitness[best]) {
            best = candidate;
        }
    }
    return best;
}
// :end entities

f32 randf(f32 min, f32 max) {
    // precision, because this isn't called with big min and max this is fine
    #define P 10000000
    return GetRandomValue((i32)(min * P), (i32)(max * P)) / (f32)P;
}
