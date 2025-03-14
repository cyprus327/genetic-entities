#pragma once

#include <stdio.h>
#include <math.h>

#define EPSILON 0.0001f

#define CLAMP(x, a, b) ((x) < (a) ? (a) : (x) > (b) ? (b) : (x))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define ARRLEN(arr) (sizeof(arr) / sizeof((arr)[0]))

#define ERR_EXIT(msg) { printf("FATAL: %s\n", msg); exit(1); }
#define ERR_RET(rv, msg) { printf("%s\n", msg); return rv; }

typedef unsigned char u8;
typedef int i32;
typedef unsigned u32;
typedef long long i64;
typedef unsigned long long u64;
typedef float f32;

typedef f32 vec2[2];
typedef i32 vec2i[2];

#define VEC_ZERO ((vec2){0.f, 0.f})
#define VEC_ONE ((vec2){1.f, 1.f})

#define VEC_SET(v, a) { (v)[0] = (a)[0]; (v)[1] = (a)[1]; }

#define VEC_INRANGE(v, minX, maxX, minY, maxY) ( \
    (v)[0] >= (minX) && (v)[0] <= (maxX) && (v)[1] >= (minY) && (v)[1] <= (maxY) \
)

#define VEC_ADD(v, a, b) { (v)[0] = (a)[0] + (b)[0]; (v)[1] = (a)[1] + (b)[1]; }
#define VEC_SUB(v, a, b) { (v)[0] = (a)[0] - (b)[0]; (v)[1] = (a)[1] - (b)[1]; }
#define VEC_MUL(v, a, b) { (v)[0] = (a)[0] * (b)[0]; (v)[1] = (a)[1] * (b)[1]; }
#define VEC_DIV(v, a, b) { (v)[0] = (a)[0] / (b)[0]; (v)[1] = (a)[1] / (b)[1]; }

#define VEC_ADDF(v, a, x) { (v)[0] = (a)[0] + (x); (v)[1] = (a)[1] + (x); }
#define VEC_SUBF(v, a, x) { (v)[0] = (a)[0] - (x); (v)[1] = (a)[1] - (x); }
#define VEC_MULF(v, a, x) { (v)[0] = (a)[0] * (x); (v)[1] = (a)[1] * (x); }
#define VEC_DIVF(v, a, x) { (v)[0] = (a)[0] / (x); (v)[1] = (a)[1] / (x); }

#define VEC_DOT(a, b) ((a)[0] * (b)[0] + (a)[1] * (b)[1])
#define VEC_LENGTH2(a) ((a)[0] * (a)[0] + (a)[1] * (a)[1])
#define VEC_LENGTH(a) sqrtf(VEC_LENGTH2(a))

#define VEC_PERPENDICULAR(v, a) { (v)[0] = -(a)[1]; (v)[1] = (a)[0]; }

#define VEC_NORMALIZE(v, a) { \
    const f32 _len = VEC_LENGTH(a); \
    if (_len > EPSILON) { \
        VEC_DIVF((v), (a), _len); \
    } else { \
        (v)[0] = (v)[1] = 0.f; \
    } \
}

#define VEC_ROTATE(v, p, a) { \
    const f32 _s = sinf(a), _c = cosf(a); \
    vec2 _o; VEC_SUB(_o, (v), (p)); \
    const f32 _x = _o[0] * _c - _o[1] * _s; \
    const f32 _y = _o[0] * _s + _o[1] * _c; \
    (v)[0] = _x + (p)[0]; (v)[1] = _y + (p)[1]; \
}

#define FRAMES_MAX 256
#define ENTITY_MAX 1000
#define OBSTACLE_MAX 3

#define ENTITY_SIZE 15.f
#define TARGET_RAD 20.f

#define DEFAULT_MUTATION_CHANCE 0.2f
#define DEFAULT_MUTATION_MAGNITUDE 0.15f
#define DEFAULT_POP_MAGNITUDE 0.1f

typedef struct entity {
    vec2 pos, vel;

    enum stateType {
        STATE_ALIVE,
        STATE_FAILED,
        STATE_COMPLETED,
    } state;

    f32 fitness;
    i32 framesToFinish;

    vec2* genes;
} Entity;

typedef struct obstacle {
    vec2 min, max;
} Obstacle;

typedef struct state {
    i32 gen, currFrame;
    i32 slowMode;
    
    Entity* entities;
    Entity* nextEntities;
    vec2 entitySpawnPos, entityTargetPos;

    Obstacle obstacles[OBSTACLE_MAX];
} State;

f32 randf(f32 min, f32 max);

void state_init(State* state);
void state_release(State* state);
void state_new_generation(State* state);
void state_end_generation(State* state);
void state_update(State* state, f32 delta);

void entity_init(Entity* entity, const vec2 spawnPos);
void entity_release(Entity* entity);
void entity_reset(Entity* entity, const vec2 spawnPos);
void entity_update(Entity* entity, const State* state, f32 delta);
f32 entity_calc_fitness(const Entity* entity, const vec2 targetPos);
void entity_crossover_genes(Entity* child, Entity* first, Entity* second, const State* state);
void entity_mutate_genes(Entity* entity);
i32 entity_tournament_select(const State* state, i32 size);
