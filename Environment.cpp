#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <queue>
#include <sstream>
#include <string>
#include <vector>

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#ifndef __has_include
#define __has_include(x) 0
#endif

#if __has_include(<GL/freeglut.h>)
#include <GL/freeglut.h>
#elif __has_include(<GL/glut.h>)
#include <GL/glut.h>
#else
#error "No GLUT header found. Install freeglut/glut development headers and libraries."
#endif
#endif

enum GamePhase
{
    PHASE_PLAYING = 0,
    PHASE_GAME_OVER,
    PHASE_WIN
};

struct GameState
{
    float camX;
    float camY;
    float camZ;
    float moveStep;

    float stamina;
    float camBaseY;
    float camBobOffset;
    float camRollOffset;
    float panicJitterX;
    float panicJitterY;
    float flashlightCutoff;

    float guardianX;
    float guardianZ;
    float guardianDir;
    float guardianSpeed;
    float guardianScale;

    float headAngle;
    float headDir;

    float timeValue;
    float portalAngle;
    float portalSpinSpeed;

    GamePhase phase;
    bool loseAnnounced;
    bool winAnnounced;

    int lastTicks;
};

GameState gState =
{
    0.0f, 2.0f, 14.5f, 0.55f,
    100.0f, 2.0f, 0.0f, 0.0f, 0.0f, 0.0f, 25.0f,
    -9.0f, -7.8f, 1.0f, 2.6f, 1.85f,
    0.0f, 1.0f,
    0.0f, 0.0f, 24.0f,
    PHASE_PLAYING, false, false,
    0
};

const float ROOM_HALF = 18.0f;
const float PEDESTAL_HALF = 1.35f;
const float PEDESTAL_HEIGHT = 2.2f;

const int STATE_MENU = 1;
const int STATE_GAME_OVER = 2;
const int STATE_WIN = 3;
const int STATE_PLAYING = 4;

int gameState = STATE_MENU;
int windowWidth = 1280;
int windowHeight = 720;
float yaw = 0.0f;

const int MAP_SIZE = 20;
const int MAX_INTERACTABLES = 8;
const float MAP_CELL_SIZE = (ROOM_HALF * 2.0f) / static_cast<float>(MAP_SIZE);
const float PLAYER_WALK_SPEED = 3.4f;
const float PLAYER_TURN_SPEED = 1.9f;
const float PLAYER_SPRINT_MULT = 1.75f;
const float STAMINA_MAX = 100.0f;
const float STAMINA_DRAIN_RATE = 32.0f;
const float STAMINA_RECOVER_RATE = 18.0f;
const float STAMINA_PANIC_THRESHOLD = 30.0f;
const float FLASHLIGHT_BASE_CUTOFF = 25.0f;
const float FLASHLIGHT_PANIC_CUTOFF = 20.5f;
const float PANIC_JITTER_X_MAX = 0.028f;
const float PANIC_JITTER_Y_MAX = 0.020f;
const float HEAD_BOB_FREQ_WALK = 9.5f;
const float HEAD_BOB_FREQ_SPRINT = 14.0f;
const float HEAD_BOB_AMP_WALK = 0.020f;
const float HEAD_BOB_AMP_SPRINT = 0.034f;
const float HEAD_ROLL_AMP_WALK = 0.008f;
const float HEAD_ROLL_AMP_SPRINT = 0.014f;
const float ADMIN_HIT_RADIUS = 1.2f;
const float ADMIN_ROAM_STEP = 0.02f;
const float ADMIN_HUNT_STEP = 0.03f;
const float ADMIN_PROXIMITY_AGGRO_RADIUS = 3.0f;
const float ADMIN_VISION_MAX_RANGE = 12.0f;
const float ADMIN_VISION_DOT_THRESHOLD = 0.70710678f;
const float ADMIN_WAYPOINT_REACHED_RADIUS = 0.55f;
const int ADMIN_STUCK_FRAME_LIMIT = 24;
const float INTERACT_MAX_RANGE = 2.75f;
const float INTERACT_VIEW_DOT = 0.94f;

struct LevelData
{
    int grid[MAP_SIZE][MAP_SIZE];
    float roomHeight;
    float wallColor[3];
    float fogDensity;
    float adminScaleY;
    float adminSpeedMult;
    int requiredScore;
};

const int LEVEL_1_GRID[MAP_SIZE][MAP_SIZE] =
{
    { 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
    { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
    { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
    { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
    { 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
    { 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
    { 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1 },
    { 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1 },
    { 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1 },
    { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1 },
    { 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1 },
    { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
    { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
    { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
    { 1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
    { 1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
    { 1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
    { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
    { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
    { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }
};

const int LEVEL_2_GRID[MAP_SIZE][MAP_SIZE] =
{
    { 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
    { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
    { 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 1, 1 },
    { 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 1, 1 },
    { 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 1, 1 },
    { 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 1, 1 },
    { 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0, 0, 1, 1 },
    { 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 1, 0, 1, 1 },
    { 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 0, 0, 0, 1, 1, 1 },
    { 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1, 1 },
    { 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1 },
    { 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1 },
    { 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1 },
    { 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 0, 0, 0, 1, 1, 1 },
    { 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 1, 0, 1, 1 },
    { 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0, 0, 1, 1 },
    { 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 1, 1 },
    { 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 1, 1 },
    { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
    { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }
};

enum InteractableTypeFlags
{
    TYPE_PICKUP = 1 << 0,
    TYPE_SWITCH = 1 << 1,
    TYPE_READABLE = 1 << 2
};

struct Interactable
{
    float x;
    float y;
    float z;
    int typeFlags;
    bool active;
    int linkId;
    int sequenceOrder;
};

struct GridPoint
{
    int x;
    int z;
};

struct ObjMesh
{
    std::vector<float> vertices;
    std::vector<float> normals;
};

float gridToWorld(int gridCoord);
bool loadOBJ(const char* filePath, ObjMesh& mesh);
GLuint buildMeshDisplayList(const ObjMesh& mesh);
void initializeMeshAssets();
void drawGuardianFallbackPrimitive();
void drawArtifactCrystalFallback();
void updateDoorLockState();
void updateInteractionTarget();
void loadLevel(int levelIndex);
bool findAdminPath(int startX, int startZ, int goalX, int goalZ);
bool followAdminPath(float moveStep);

LevelData createLevelData(
    const int sourceGrid[MAP_SIZE][MAP_SIZE],
    float roomHeight,
    const float wallColor[3],
    float fogDensity,
    float adminScaleY,
    float adminSpeedMult,
    int requiredScore)
{
    LevelData level = {};
    for (int z = 0; z < MAP_SIZE; ++z)
    {
        for (int x = 0; x < MAP_SIZE; ++x)
        {
            level.grid[z][x] = sourceGrid[z][x];
        }
    }

    level.roomHeight = roomHeight;
    level.wallColor[0] = wallColor[0];
    level.wallColor[1] = wallColor[1];
    level.wallColor[2] = wallColor[2];
    level.fogDensity = fogDensity;
    level.adminScaleY = adminScaleY;
    level.adminSpeedMult = adminSpeedMult;
    level.requiredScore = requiredScore;
    return level;
}

std::vector<LevelData> buildLevels()
{
    const float level1WallColor[3] = { 0.13f, 0.13f, 0.16f };
    const float level2WallColor[3] = { 0.18f, 0.08f, 0.05f };

    std::vector<LevelData> result;
    result.reserve(2);
    result.push_back(createLevelData(LEVEL_1_GRID, 10.5f, level1WallColor, 0.040f, 1.0f, 1.0f, 3));
    result.push_back(createLevelData(LEVEL_2_GRID, 18.0f, level2WallColor, 0.065f, 2.5f, 1.30f, 4));
    return result;
}

const int LEVEL_1_INTERACTABLE_COUNT = 4;
const int LEVEL_2_INTERACTABLE_COUNT = 4;

const Interactable LEVEL_1_INTERACTABLES[LEVEL_1_INTERACTABLE_COUNT] =
{
    { gridToWorld(3), 0.72f, gridToWorld(3), TYPE_PICKUP, true, -1, 0 },
    { gridToWorld(9), 0.72f, gridToWorld(12), TYPE_PICKUP, true, -1, 0 },
    { gridToWorld(15), 0.72f, gridToWorld(6), TYPE_PICKUP, true, -1, 0 },
    { gridToWorld(2), 1.15f, gridToWorld(17), TYPE_READABLE, true, -1, 0 }
};

const Interactable LEVEL_2_INTERACTABLES[LEVEL_2_INTERACTABLE_COUNT] =
{
    { gridToWorld(2), 0.72f, gridToWorld(2), TYPE_PICKUP, true, -1, 1 },
    { gridToWorld(17), 0.72f, gridToWorld(2), TYPE_PICKUP, true, -1, 2 },
    { gridToWorld(17), 0.72f, gridToWorld(17), TYPE_PICKUP, true, -1, 3 },
    { gridToWorld(2), 0.72f, gridToWorld(17), TYPE_PICKUP, true, -1, 4 }
};

std::vector<LevelData> levels = buildLevels();
int activeMapGrid[MAP_SIZE][MAP_SIZE] = {};
int currentLevelIndex = 0;

int score = 0;
Interactable interactables[MAX_INTERACTABLES] = {};
int interactableCount = 0;
int currentSequenceTarget = 1;

bool canInteract = false;
int targetedInteractableIndex = -1;
bool readableVisited = false;
bool doorUnlocked = false;

ObjMesh guardianMesh;
ObjMesh artifactMesh;
GLuint guardianMeshList = 0;
GLuint artifactMeshList = 0;

float adminDirX = ADMIN_ROAM_STEP;
float adminDirZ = 0.0f;
float adminWaypointX = -9.0f;
float adminWaypointZ = -7.8f;
float adminForwardX = 0.0f;
float adminForwardZ = 1.0f;
bool adminHasWaypoint = false;
bool adminChasing = false;
bool adminForcedChase = false;
int adminStuckFrames = 0;
std::vector<GridPoint> adminPath;
int adminPathGoalX = -1;
int adminPathGoalZ = -1;
int adminPathLevelIndex = -1;

bool keyHeld[256] = { false };

const LevelData& getCurrentLevel()
{
    return levels[static_cast<std::size_t>(currentLevelIndex)];
}

void loadLevel(int levelIndex)
{
    if (levels.empty())
    {
        return;
    }

    if (levelIndex < 0)
    {
        levelIndex = 0;
    }
    if (levelIndex >= static_cast<int>(levels.size()))
    {
        levelIndex = static_cast<int>(levels.size()) - 1;
    }

    currentLevelIndex = levelIndex;
    const LevelData& level = getCurrentLevel();
    for (int z = 0; z < MAP_SIZE; ++z)
    {
        for (int x = 0; x < MAP_SIZE; ++x)
        {
            activeMapGrid[z][x] = level.grid[z][x];
        }
    }

    score = 0;
    currentSequenceTarget = 1;
    canInteract = false;
    targetedInteractableIndex = -1;
    readableVisited = false;
    doorUnlocked = false;

    gState.camBaseY = 2.0f;
    if (currentLevelIndex == 0)
    {
        gState.camX = 0.0f;
        gState.camZ = 14.5f;
        gState.guardianX = -9.0f;
        gState.guardianZ = -7.8f;
    }
    else
    {
        gState.camX = gridToWorld(10);
        gState.camZ = gridToWorld(18);
        gState.guardianX = gridToWorld(5);
        gState.guardianZ = gridToWorld(5);
    }

    yaw = 0.0f;
    gState.stamina = STAMINA_MAX;
    gState.camBobOffset = 0.0f;
    gState.camRollOffset = 0.0f;
    gState.panicJitterX = 0.0f;
    gState.panicJitterY = 0.0f;
    gState.flashlightCutoff = FLASHLIGHT_BASE_CUTOFF;
    gState.camY = gState.camBaseY;
    gState.guardianScale = 1.85f;
    gState.headAngle = 0.0f;
    gState.headDir = 1.0f;
    gState.phase = PHASE_PLAYING;
    gState.loseAnnounced = false;
    gState.winAnnounced = false;

    adminDirX = ADMIN_ROAM_STEP;
    adminDirZ = 0.0f;
    adminWaypointX = gState.guardianX;
    adminWaypointZ = gState.guardianZ;
    adminForwardX = 0.0f;
    adminForwardZ = 1.0f;
    adminHasWaypoint = false;
    adminChasing = false;
    adminForcedChase = false;
    adminStuckFrames = 0;
    adminPath.clear();
    adminPathGoalX = -1;
    adminPathGoalZ = -1;
    adminPathLevelIndex = -1;
    gState.portalAngle = 0.0f;
    gState.portalSpinSpeed = 9.0f;

    const Interactable* sourceInteractables = LEVEL_1_INTERACTABLES;
    int sourceCount = LEVEL_1_INTERACTABLE_COUNT;
    if (currentLevelIndex == 1)
    {
        sourceInteractables = LEVEL_2_INTERACTABLES;
        sourceCount = LEVEL_2_INTERACTABLE_COUNT;
    }

    interactableCount = (sourceCount < MAX_INTERACTABLES) ? sourceCount : MAX_INTERACTABLES;
    for (int i = 0; i < interactableCount; ++i)
    {
        interactables[i] = sourceInteractables[i];
    }
    for (int i = interactableCount; i < MAX_INTERACTABLES; ++i)
    {
        interactables[i].x = 0.0f;
        interactables[i].y = 0.0f;
        interactables[i].z = 0.0f;
        interactables[i].typeFlags = 0;
        interactables[i].active = false;
        interactables[i].linkId = -1;
        interactables[i].sequenceOrder = 0;
    }

    updateDoorLockState();
    updateInteractionTarget();
}

float gridToWorld(int gridCoord)
{
    return -ROOM_HALF + (static_cast<float>(gridCoord) + 0.5f) * MAP_CELL_SIZE;
}

int worldToGrid(float worldCoord)
{
    return static_cast<int>(std::floor((worldCoord + ROOM_HALF) / MAP_CELL_SIZE));
}

bool isBlockedCell(int gridX, int gridZ)
{
    if (gridX < 0 || gridZ < 0 || gridX >= MAP_SIZE || gridZ >= MAP_SIZE)
    {
        return true;
    }

    const int cell = activeMapGrid[gridZ][gridX];
    if (cell == 1)
    {
        return true;
    }

    if (cell == 2)
    {
        return !doorUnlocked;
    }

    return false;
}

bool canMoveTo(float nextX, float nextZ)
{
    const float halfExtent = 0.25f;
    const float cornerX[4] =
    {
        nextX - halfExtent,
        nextX + halfExtent,
        nextX - halfExtent,
        nextX + halfExtent
    };
    const float cornerZ[4] =
    {
        nextZ - halfExtent,
        nextZ - halfExtent,
        nextZ + halfExtent,
        nextZ + halfExtent
    };

    for (int i = 0; i < 4; ++i)
    {
        const int gridX = worldToGrid(cornerX[i]);
        const int gridZ = worldToGrid(cornerZ[i]);
        if (isBlockedCell(gridX, gridZ))
        {
            return false;
        }
    }

    return true;
}

bool canAdminMoveTo(float nextX, float nextZ)
{
    const float halfExtent = 0.45f;
    const float cornerX[4] =
    {
        nextX - halfExtent,
        nextX + halfExtent,
        nextX - halfExtent,
        nextX + halfExtent
    };
    const float cornerZ[4] =
    {
        nextZ - halfExtent,
        nextZ - halfExtent,
        nextZ + halfExtent,
        nextZ + halfExtent
    };

    for (int i = 0; i < 4; ++i)
    {
        const int gridX = worldToGrid(cornerX[i]);
        const int gridZ = worldToGrid(cornerZ[i]);
        if (isBlockedCell(gridX, gridZ))
        {
            return false;
        }
    }

    return true;
}

void updateDoorLockState()
{
    doorUnlocked = (score >= getCurrentLevel().requiredScore);
    if (gState.phase == PHASE_PLAYING)
    {
        gState.portalSpinSpeed = doorUnlocked ? 68.0f : 9.0f;
    }
}

bool hasLineOfSightToTarget(float fromX, float fromZ, float toX, float toZ)
{
    const float dx = toX - fromX;
    const float dz = toZ - fromZ;
    const float dist = std::sqrt(dx * dx + dz * dz);
    if (dist < 0.0001f)
    {
        return true;
    }

    const int targetGridX = worldToGrid(toX);
    const int targetGridZ = worldToGrid(toZ);

    int steps = static_cast<int>(dist / 0.18f);
    if (steps < 1)
    {
        steps = 1;
    }

    const float xInc = dx / static_cast<float>(steps);
    const float zInc = dz / static_cast<float>(steps);

    float traceX = fromX;
    float traceZ = fromZ;
    for (int i = 0; i < steps; ++i)
    {
        traceX += xInc;
        traceZ += zInc;

        const int gridX = worldToGrid(traceX);
        const int gridZ = worldToGrid(traceZ);

        if (gridX == targetGridX && gridZ == targetGridZ)
        {
            return true;
        }

        if (isBlockedCell(gridX, gridZ))
        {
            return false;
        }
    }

    return true;
}

void updateInteractionTarget()
{
    canInteract = false;
    targetedInteractableIndex = -1;

    const float lookX = std::cos(yaw);
    const float lookZ = std::sin(yaw);
    float closestDistance = INTERACT_MAX_RANGE + 0.001f;

    for (int i = 0; i < interactableCount; ++i)
    {
        const Interactable& item = interactables[i];
        if (!item.active)
        {
            continue;
        }

        const float dx = item.x - gState.camX;
        const float dz = item.z - gState.camZ;
        const float distance = std::sqrt(dx * dx + dz * dz);
        if (distance > INTERACT_MAX_RANGE || distance < 0.0001f)
        {
            continue;
        }

        const float dot = (dx * lookX + dz * lookZ) / distance;
        if (dot < INTERACT_VIEW_DOT)
        {
            continue;
        }

        if (!hasLineOfSightToTarget(gState.camX, gState.camZ, item.x, item.z))
        {
            continue;
        }

        if (distance < closestDistance)
        {
            closestDistance = distance;
            canInteract = true;
            targetedInteractableIndex = i;
        }
    }
}

int resolveObjIndex(int index, int count)
{
    if (index > 0)
    {
        return index - 1;
    }
    if (index < 0)
    {
        return count + index;
    }
    return -1;
}

bool parseObjFaceToken(const std::string& token, int& vertexIndex, int& normalIndex)
{
    vertexIndex = 0;
    normalIndex = 0;

    const std::size_t firstSlash = token.find('/');
    if (firstSlash == std::string::npos)
    {
        vertexIndex = std::atoi(token.c_str());
        return vertexIndex != 0;
    }

    const std::string vertexPart = token.substr(0, firstSlash);
    if (!vertexPart.empty())
    {
        vertexIndex = std::atoi(vertexPart.c_str());
    }

    const std::size_t lastSlash = token.rfind('/');
    if (lastSlash != std::string::npos && lastSlash + 1 < token.size())
    {
        const std::string normalPart = token.substr(lastSlash + 1);
        if (!normalPart.empty())
        {
            normalIndex = std::atoi(normalPart.c_str());
        }
    }

    return vertexIndex != 0;
}

bool loadOBJ(const char* filePath, ObjMesh& mesh)
{
    mesh.vertices.clear();
    mesh.normals.clear();

    if (filePath == nullptr)
    {
        return false;
    }

    std::ifstream file(filePath);
    if (!file.is_open())
    {
        return false;
    }

    std::vector<float> positions;
    std::vector<float> normals;

    std::string line;
    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == '#')
        {
            continue;
        }

        std::istringstream lineStream(line);
        std::string type;
        lineStream >> type;

        if (type == "v")
        {
            float x = 0.0f;
            float y = 0.0f;
            float z = 0.0f;
            if (lineStream >> x >> y >> z)
            {
                positions.push_back(x);
                positions.push_back(y);
                positions.push_back(z);
            }
        }
        else if (type == "vn")
        {
            float nx = 0.0f;
            float ny = 0.0f;
            float nz = 0.0f;
            if (lineStream >> nx >> ny >> nz)
            {
                normals.push_back(nx);
                normals.push_back(ny);
                normals.push_back(nz);
            }
        }
        else if (type == "f")
        {
            std::vector<int> faceVertices;
            std::vector<int> faceNormals;

            std::string token;
            while (lineStream >> token)
            {
                int rawVertexIndex = 0;
                int rawNormalIndex = 0;
                if (!parseObjFaceToken(token, rawVertexIndex, rawNormalIndex))
                {
                    continue;
                }

                const int vertexCount = static_cast<int>(positions.size() / 3);
                const int normalCount = static_cast<int>(normals.size() / 3);
                const int resolvedVertexIndex = resolveObjIndex(rawVertexIndex, vertexCount);
                const int resolvedNormalIndex = (rawNormalIndex != 0) ? resolveObjIndex(rawNormalIndex, normalCount) : -1;

                if (resolvedVertexIndex < 0 || resolvedVertexIndex >= vertexCount)
                {
                    continue;
                }

                faceVertices.push_back(resolvedVertexIndex);
                faceNormals.push_back(resolvedNormalIndex);
            }

            if (faceVertices.size() < 3)
            {
                continue;
            }

            for (std::size_t i = 1; i + 1 < faceVertices.size(); ++i)
            {
                const int triVertex[3] = { faceVertices[0], faceVertices[i], faceVertices[i + 1] };
                const int triNormal[3] = { faceNormals[0], faceNormals[i], faceNormals[i + 1] };

                const float x0 = positions[triVertex[0] * 3 + 0];
                const float y0 = positions[triVertex[0] * 3 + 1];
                const float z0 = positions[triVertex[0] * 3 + 2];
                const float x1 = positions[triVertex[1] * 3 + 0];
                const float y1 = positions[triVertex[1] * 3 + 1];
                const float z1 = positions[triVertex[1] * 3 + 2];
                const float x2 = positions[triVertex[2] * 3 + 0];
                const float y2 = positions[triVertex[2] * 3 + 1];
                const float z2 = positions[triVertex[2] * 3 + 2];

                const float ax = x1 - x0;
                const float ay = y1 - y0;
                const float az = z1 - z0;
                const float bx = x2 - x0;
                const float by = y2 - y0;
                const float bz = z2 - z0;

                float flatNx = ay * bz - az * by;
                float flatNy = az * bx - ax * bz;
                float flatNz = ax * by - ay * bx;
                const float flatLength = std::sqrt(flatNx * flatNx + flatNy * flatNy + flatNz * flatNz);
                if (flatLength > 0.00001f)
                {
                    flatNx /= flatLength;
                    flatNy /= flatLength;
                    flatNz /= flatLength;
                }
                else
                {
                    flatNx = 0.0f;
                    flatNy = 1.0f;
                    flatNz = 0.0f;
                }

                for (int k = 0; k < 3; ++k)
                {
                    const int vIndex = triVertex[k] * 3;
                    mesh.vertices.push_back(positions[vIndex + 0]);
                    mesh.vertices.push_back(positions[vIndex + 1]);
                    mesh.vertices.push_back(positions[vIndex + 2]);

                    const int nIndex = triNormal[k] * 3;
                    if (triNormal[k] >= 0 && nIndex + 2 < static_cast<int>(normals.size()))
                    {
                        mesh.normals.push_back(normals[nIndex + 0]);
                        mesh.normals.push_back(normals[nIndex + 1]);
                        mesh.normals.push_back(normals[nIndex + 2]);
                    }
                    else
                    {
                        mesh.normals.push_back(flatNx);
                        mesh.normals.push_back(flatNy);
                        mesh.normals.push_back(flatNz);
                    }
                }
            }
        }
    }

    return !mesh.vertices.empty();
}

GLuint buildMeshDisplayList(const ObjMesh& mesh)
{
    if (mesh.vertices.empty() || mesh.normals.size() != mesh.vertices.size())
    {
        return 0;
    }

    const GLuint listId = glGenLists(1);
    if (listId == 0)
    {
        return 0;
    }

    glNewList(listId, GL_COMPILE);
    glBegin(GL_TRIANGLES);
    for (std::size_t i = 0; i < mesh.vertices.size(); i += 3)
    {
        glNormal3f(mesh.normals[i + 0], mesh.normals[i + 1], mesh.normals[i + 2]);
        glVertex3f(mesh.vertices[i + 0], mesh.vertices[i + 1], mesh.vertices[i + 2]);
    }
    glEnd();
    glEndList();

    return listId;
}

void drawMeshFromArrays(const ObjMesh& mesh)
{
    if (mesh.vertices.empty() || mesh.normals.size() != mesh.vertices.size())
    {
        return;
    }

    glBegin(GL_TRIANGLES);
    for (std::size_t i = 0; i < mesh.vertices.size(); i += 3)
    {
        glNormal3f(mesh.normals[i + 0], mesh.normals[i + 1], mesh.normals[i + 2]);
        glVertex3f(mesh.vertices[i + 0], mesh.vertices[i + 1], mesh.vertices[i + 2]);
    }
    glEnd();
}

void initializeMeshAssets()
{
    if (loadOBJ("assets/guardian.obj", guardianMesh))
    {
        guardianMeshList = buildMeshDisplayList(guardianMesh);
        std::printf("Loaded mesh: assets/guardian.obj\n");
    }

    if (loadOBJ("assets/artifact.obj", artifactMesh))
    {
        artifactMeshList = buildMeshDisplayList(artifactMesh);
        std::printf("Loaded mesh: assets/artifact.obj\n");
    }
}

void randomizeAdminDirection()
{
    const int pick = std::rand() % 4;
    switch (pick)
    {
    case 0:
        adminDirX = ADMIN_ROAM_STEP;
        adminDirZ = 0.0f;
        break;
    case 1:
        adminDirX = -ADMIN_ROAM_STEP;
        adminDirZ = 0.0f;
        break;
    case 2:
        adminDirX = 0.0f;
        adminDirZ = ADMIN_ROAM_STEP;
        break;
    default:
        adminDirX = 0.0f;
        adminDirZ = -ADMIN_ROAM_STEP;
        break;
    }
}

void normalizeXZ(float& x, float& z)
{
    const float len = std::sqrt(x * x + z * z);
    if (len > 0.00001f)
    {
        x /= len;
        z /= len;
    }
    else
    {
        x = 0.0f;
        z = 1.0f;
    }
}

float randomSignedUnit()
{
    const float unit = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
    return (unit * 2.0f) - 1.0f;
}

bool findAdminPath(int startX, int startZ, int goalX, int goalZ)
{
    adminPath.clear();

    if (startX < 0 || startZ < 0 || startX >= MAP_SIZE || startZ >= MAP_SIZE)
    {
        return false;
    }
    if (goalX < 0 || goalZ < 0 || goalX >= MAP_SIZE || goalZ >= MAP_SIZE)
    {
        return false;
    }
    if (isBlockedCell(startX, startZ) || isBlockedCell(goalX, goalZ))
    {
        return false;
    }
    if (startX == goalX && startZ == goalZ)
    {
        return true;
    }

    bool visited[MAP_SIZE][MAP_SIZE] = { false };
    int parentX[MAP_SIZE][MAP_SIZE];
    int parentZ[MAP_SIZE][MAP_SIZE];
    for (int z = 0; z < MAP_SIZE; ++z)
    {
        for (int x = 0; x < MAP_SIZE; ++x)
        {
            parentX[z][x] = -1;
            parentZ[z][x] = -1;
        }
    }

    std::queue<GridPoint> frontier;
    frontier.push({ startX, startZ });
    visited[startZ][startX] = true;

    const int dirX[4] = { 1, -1, 0, 0 };
    const int dirZ[4] = { 0, 0, 1, -1 };

    bool foundGoal = false;
    while (!frontier.empty())
    {
        const GridPoint current = frontier.front();
        frontier.pop();

        if (current.x == goalX && current.z == goalZ)
        {
            foundGoal = true;
            break;
        }

        for (int i = 0; i < 4; ++i)
        {
            const int nextX = current.x + dirX[i];
            const int nextZ = current.z + dirZ[i];
            if (nextX < 0 || nextZ < 0 || nextX >= MAP_SIZE || nextZ >= MAP_SIZE)
            {
                continue;
            }
            if (visited[nextZ][nextX])
            {
                continue;
            }
            if (isBlockedCell(nextX, nextZ))
            {
                continue;
            }

            visited[nextZ][nextX] = true;
            parentX[nextZ][nextX] = current.x;
            parentZ[nextZ][nextX] = current.z;
            frontier.push({ nextX, nextZ });
        }
    }

    if (!foundGoal)
    {
        return false;
    }

    std::vector<GridPoint> reversedPath;
    int traceX = goalX;
    int traceZ = goalZ;
    while (!(traceX == startX && traceZ == startZ))
    {
        reversedPath.push_back({ traceX, traceZ });
        const int prevX = parentX[traceZ][traceX];
        const int prevZ = parentZ[traceZ][traceX];
        if (prevX < 0 || prevZ < 0)
        {
            adminPath.clear();
            return false;
        }
        traceX = prevX;
        traceZ = prevZ;
    }

    for (int i = static_cast<int>(reversedPath.size()) - 1; i >= 0; --i)
    {
        adminPath.push_back(reversedPath[static_cast<std::size_t>(i)]);
    }

    return true;
}

bool chooseRandomAdminWaypoint()
{
    const int startGridX = worldToGrid(gState.guardianX);
    const int startGridZ = worldToGrid(gState.guardianZ);

    for (int attempt = 0; attempt < 96; ++attempt)
    {
        const int gridX = 1 + (std::rand() % (MAP_SIZE - 2));
        const int gridZ = 1 + (std::rand() % (MAP_SIZE - 2));
        if (activeMapGrid[gridZ][gridX] != 0)
        {
            continue;
        }

        const float candidateX = gridToWorld(gridX);
        const float candidateZ = gridToWorld(gridZ);
        const float dx = candidateX - gState.guardianX;
        const float dz = candidateZ - gState.guardianZ;
        const float dist = std::sqrt(dx * dx + dz * dz);
        if (dist < 1.2f)
        {
            continue;
        }

        if (!findAdminPath(startGridX, startGridZ, gridX, gridZ))
        {
            continue;
        }

        adminWaypointX = candidateX;
        adminWaypointZ = candidateZ;
        adminHasWaypoint = true;
        adminStuckFrames = 0;
        adminPathGoalX = gridX;
        adminPathGoalZ = gridZ;
        adminPathLevelIndex = currentLevelIndex;
        return true;
    }

    adminPath.clear();
    adminWaypointX = gState.guardianX;
    adminWaypointZ = gState.guardianZ;
    adminHasWaypoint = true;
    adminStuckFrames = 0;
    adminPathGoalX = -1;
    adminPathGoalZ = -1;
    adminPathLevelIndex = -1;
    return false;
}

bool moveAdminTowards(float targetX, float targetZ, float moveStep)
{
    const float toX = targetX - gState.guardianX;
    const float toZ = targetZ - gState.guardianZ;
    const float dist = std::sqrt(toX * toX + toZ * toZ);
    if (dist < 0.0001f)
    {
        return true;
    }

    const float dirX = toX / dist;
    const float dirZ = toZ / dist;

    const float oldX = gState.guardianX;
    const float oldZ = gState.guardianZ;
    bool moved = false;

    const float nextX = gState.guardianX + dirX * moveStep;
    const float nextZ = gState.guardianZ + dirZ * moveStep;
    if (canAdminMoveTo(nextX, nextZ))
    {
        gState.guardianX = nextX;
        gState.guardianZ = nextZ;
        moved = true;
    }
    else if (std::fabs(dirX) > std::fabs(dirZ))
    {
        if (canAdminMoveTo(gState.guardianX + dirX * moveStep, gState.guardianZ))
        {
            gState.guardianX += dirX * moveStep;
            moved = true;
        }
        if (!moved && canAdminMoveTo(gState.guardianX, gState.guardianZ + dirZ * moveStep))
        {
            gState.guardianZ += dirZ * moveStep;
            moved = true;
        }
    }
    else
    {
        if (canAdminMoveTo(gState.guardianX, gState.guardianZ + dirZ * moveStep))
        {
            gState.guardianZ += dirZ * moveStep;
            moved = true;
        }
        if (!moved && canAdminMoveTo(gState.guardianX + dirX * moveStep, gState.guardianZ))
        {
            gState.guardianX += dirX * moveStep;
            moved = true;
        }
    }

    if (moved)
    {
        float movedX = gState.guardianX - oldX;
        float movedZ = gState.guardianZ - oldZ;
        normalizeXZ(movedX, movedZ);
        adminForwardX = movedX;
        adminForwardZ = movedZ;
        adminStuckFrames = 0;
    }
    else
    {
        ++adminStuckFrames;
    }

    return moved;
}

bool followAdminPath(float moveStep)
{
    if (adminPath.empty())
    {
        return false;
    }

    const float targetX = gridToWorld(adminPath.front().x);
    const float targetZ = gridToWorld(adminPath.front().z);
    const bool moved = moveAdminTowards(targetX, targetZ, moveStep);

    const float dx = targetX - gState.guardianX;
    const float dz = targetZ - gState.guardianZ;
    const float dist = std::sqrt(dx * dx + dz * dz);
    if (dist < 0.15f)
    {
        adminPath.erase(adminPath.begin());
    }

    return moved;
}

bool hasAdminLineOfSightToPlayer()
{
    const float dx = gState.camX - gState.guardianX;
    const float dz = gState.camZ - gState.guardianZ;
    const float dist = std::sqrt(dx * dx + dz * dz);
    if (dist < 0.0001f)
    {
        return true;
    }

    const int playerGridX = worldToGrid(gState.camX);
    const int playerGridZ = worldToGrid(gState.camZ);

    // Grid raycast: advance a short segment along the Guardian->player vector.
    // If we hit a blocking cell before reaching the player's grid, sight is broken.
    int steps = static_cast<int>(dist / 0.16f);
    if (steps < 1)
    {
        steps = 1;
    }

    const float xInc = dx / static_cast<float>(steps);
    const float zInc = dz / static_cast<float>(steps);

    float rayX = gState.guardianX;
    float rayZ = gState.guardianZ;
    for (int i = 0; i < steps; ++i)
    {
        rayX += xInc;
        rayZ += zInc;

        const int gridX = worldToGrid(rayX);
        const int gridZ = worldToGrid(rayZ);
        if (gridX == playerGridX && gridZ == playerGridZ)
        {
            return true;
        }

        if (gridX < 0 || gridZ < 0 || gridX >= MAP_SIZE || gridZ >= MAP_SIZE)
        {
            return false;
        }

        const int cell = activeMapGrid[gridZ][gridX];
        if (cell == 1 || (cell == 2 && !doorUnlocked))
        {
            return false;
        }
    }

    return true;
}

bool canAdminDetectPlayer()
{
    float toPlayerX = gState.camX - gState.guardianX;
    float toPlayerZ = gState.camZ - gState.guardianZ;
    const float distToPlayer = std::sqrt(toPlayerX * toPlayerX + toPlayerZ * toPlayerZ);

    if (distToPlayer <= ADMIN_PROXIMITY_AGGRO_RADIUS)
    {
        return hasAdminLineOfSightToPlayer();
    }

    if (distToPlayer > ADMIN_VISION_MAX_RANGE)
    {
        return false;
    }

    normalizeXZ(toPlayerX, toPlayerZ);

    // Vision cone dot product math:
    // dot = forward . toPlayerNormalized, with threshold cos(45deg) ~= 0.707.
    // Values above threshold mean the player lies inside a 90deg field of view.
    const float dot = adminForwardX * toPlayerX + adminForwardZ * toPlayerZ;
    if (dot < ADMIN_VISION_DOT_THRESHOLD)
    {
        return false;
    }

    return hasAdminLineOfSightToPlayer();
}

float clampf(float value, float minValue, float maxValue)
{
    if (value < minValue)
    {
        return minValue;
    }
    if (value > maxValue)
    {
        return maxValue;
    }
    return value;
}

void clampCameraToRoom()
{
    const float margin = 1.0f;
    gState.camX = clampf(gState.camX, -ROOM_HALF + margin, ROOM_HALF - margin);
    gState.camZ = clampf(gState.camZ, -ROOM_HALF + margin, ROOM_HALF - margin);
}

float getArtifactY()
{
    return PEDESTAL_HEIGHT + 0.95f + 0.28f * std::sin(gState.timeValue * 2.0f);
}

bool checkRobotCollision()
{
    const float playerHalf = 0.35f;
    const float robotHalfX = 0.92f * gState.guardianScale;
    const float robotHalfZ = 0.44f * gState.guardianScale;

    return (std::fabs(gState.camX - gState.guardianX) <= (playerHalf + robotHalfX) &&
            std::fabs(gState.camZ - gState.guardianZ) <= (playerHalf + robotHalfZ));
}

bool checkArtifactCollision()
{
    const float artifactY = getArtifactY();
    const float dx = gState.camX;
    const float dz = gState.camZ;
    const float dist2 = dx * dx + dz * dz;
    const float yDelta = std::fabs(gState.camY - artifactY);

    return dist2 <= (1.25f * 1.25f) && yDelta <= 1.05f;
}

void triggerGameOver()
{
    if (gState.phase != PHASE_PLAYING)
    {
        return;
    }

    gState.phase = PHASE_GAME_OVER;
    gameState = STATE_GAME_OVER;
    gState.portalSpinSpeed = 8.0f;
    if (!gState.loseAnnounced)
    {
        std::printf("GAME OVER: The guardian has seized you.\n");
        gState.loseAnnounced = true;
    }
}

void triggerWin()
{
    if (gState.phase != PHASE_PLAYING)
    {
        return;
    }

    gState.phase = PHASE_WIN;
    gState.portalSpinSpeed = 220.0f;
    if (!gState.winAnnounced)
    {
        std::printf("SUCCESS: Lock bypass complete. The portal surges awake.\n");
        gState.winAnnounced = true;
    }
}

void drawBoxPolygons(float width, float height, float depth, float r, float g, float b)
{
    const float hx = width * 0.5f;
    const float hy = height * 0.5f;
    const float hz = depth * 0.5f;

    glColor3f(r, g, b);

    glBegin(GL_POLYGON);
    glNormal3f(0.0f, 0.0f, 1.0f);
    glVertex3f(-hx, -hy, hz);
    glVertex3f(hx, -hy, hz);
    glVertex3f(hx, hy, hz);
    glVertex3f(-hx, hy, hz);
    glEnd();

    glBegin(GL_POLYGON);
    glNormal3f(0.0f, 0.0f, -1.0f);
    glVertex3f(hx, -hy, -hz);
    glVertex3f(-hx, -hy, -hz);
    glVertex3f(-hx, hy, -hz);
    glVertex3f(hx, hy, -hz);
    glEnd();

    glBegin(GL_POLYGON);
    glNormal3f(-1.0f, 0.0f, 0.0f);
    glVertex3f(-hx, -hy, -hz);
    glVertex3f(-hx, -hy, hz);
    glVertex3f(-hx, hy, hz);
    glVertex3f(-hx, hy, -hz);
    glEnd();

    glBegin(GL_POLYGON);
    glNormal3f(1.0f, 0.0f, 0.0f);
    glVertex3f(hx, -hy, hz);
    glVertex3f(hx, -hy, -hz);
    glVertex3f(hx, hy, -hz);
    glVertex3f(hx, hy, hz);
    glEnd();

    glBegin(GL_POLYGON);
    glNormal3f(0.0f, 1.0f, 0.0f);
    glVertex3f(-hx, hy, hz);
    glVertex3f(hx, hy, hz);
    glVertex3f(hx, hy, -hz);
    glVertex3f(-hx, hy, -hz);
    glEnd();

    glBegin(GL_POLYGON);
    glNormal3f(0.0f, -1.0f, 0.0f);
    glVertex3f(-hx, -hy, -hz);
    glVertex3f(hx, -hy, -hz);
    glVertex3f(hx, -hy, hz);
    glVertex3f(-hx, -hy, hz);
    glEnd();
}

void drawRoomShell()
{
    const LevelData& level = getCurrentLevel();
    const float roomHeight = level.roomHeight;
    const float wallR = level.wallColor[0];
    const float wallG = level.wallColor[1];
    const float wallB = level.wallColor[2];

    glBegin(GL_QUADS);

    glColor3f(0.06f, 0.06f, 0.07f);
    glNormal3f(0.0f, 1.0f, 0.0f);
    glVertex3f(-ROOM_HALF, 0.0f, -ROOM_HALF);
    glVertex3f(ROOM_HALF, 0.0f, -ROOM_HALF);
    glVertex3f(ROOM_HALF, 0.0f, ROOM_HALF);
    glVertex3f(-ROOM_HALF, 0.0f, ROOM_HALF);

    glColor3f(wallR * 0.36f + 0.02f, wallG * 0.36f + 0.02f, wallB * 0.36f + 0.02f);
    glNormal3f(0.0f, -1.0f, 0.0f);
    glVertex3f(-ROOM_HALF, roomHeight, ROOM_HALF);
    glVertex3f(ROOM_HALF, roomHeight, ROOM_HALF);
    glVertex3f(ROOM_HALF, roomHeight, -ROOM_HALF);
    glVertex3f(-ROOM_HALF, roomHeight, -ROOM_HALF);

    glColor3f(wallR, wallG, wallB);
    glNormal3f(0.0f, 0.0f, 1.0f);
    glVertex3f(-ROOM_HALF, 0.0f, -ROOM_HALF);
    glVertex3f(ROOM_HALF, 0.0f, -ROOM_HALF);
    glVertex3f(ROOM_HALF, roomHeight, -ROOM_HALF);
    glVertex3f(-ROOM_HALF, roomHeight, -ROOM_HALF);

    glColor3f(wallR * 0.82f, wallG * 0.82f, wallB * 0.82f);
    glNormal3f(0.0f, 0.0f, -1.0f);
    glVertex3f(ROOM_HALF, 0.0f, ROOM_HALF);
    glVertex3f(-ROOM_HALF, 0.0f, ROOM_HALF);
    glVertex3f(-ROOM_HALF, roomHeight, ROOM_HALF);
    glVertex3f(ROOM_HALF, roomHeight, ROOM_HALF);

    glColor3f(wallR * 0.72f, wallG * 0.72f, wallB * 0.72f);
    glNormal3f(1.0f, 0.0f, 0.0f);
    glVertex3f(-ROOM_HALF, 0.0f, ROOM_HALF);
    glVertex3f(-ROOM_HALF, 0.0f, -ROOM_HALF);
    glVertex3f(-ROOM_HALF, roomHeight, -ROOM_HALF);
    glVertex3f(-ROOM_HALF, roomHeight, ROOM_HALF);

    glColor3f(wallR * 0.72f, wallG * 0.72f, wallB * 0.72f);
    glNormal3f(-1.0f, 0.0f, 0.0f);
    glVertex3f(ROOM_HALF, 0.0f, -ROOM_HALF);
    glVertex3f(ROOM_HALF, 0.0f, ROOM_HALF);
    glVertex3f(ROOM_HALF, roomHeight, ROOM_HALF);
    glVertex3f(ROOM_HALF, roomHeight, -ROOM_HALF);

    glEnd();
}

void drawPedestal()
{
    const float y0 = 0.0f;
    const float y1 = PEDESTAL_HEIGHT;
    const float x0 = -PEDESTAL_HALF;
    const float x1 = PEDESTAL_HALF;
    const float z0 = -PEDESTAL_HALF;
    const float z1 = PEDESTAL_HALF;

    glBegin(GL_QUADS);

    glColor3f(0.15f, 0.15f, 0.18f);
    glNormal3f(0.0f, 1.0f, 0.0f);
    glVertex3f(x0, y1, z1);
    glVertex3f(x1, y1, z1);
    glVertex3f(x1, y1, z0);
    glVertex3f(x0, y1, z0);

    glColor3f(0.10f, 0.10f, 0.12f);
    glNormal3f(0.0f, -1.0f, 0.0f);
    glVertex3f(x0, y0, z0);
    glVertex3f(x1, y0, z0);
    glVertex3f(x1, y0, z1);
    glVertex3f(x0, y0, z1);

    glColor3f(0.12f, 0.10f, 0.14f);
    glNormal3f(0.0f, 0.0f, 1.0f);
    glVertex3f(x0, y0, z1);
    glVertex3f(x1, y0, z1);
    glVertex3f(x1, y1, z1);
    glVertex3f(x0, y1, z1);

    glColor3f(0.10f, 0.09f, 0.12f);
    glNormal3f(0.0f, 0.0f, -1.0f);
    glVertex3f(x1, y0, z0);
    glVertex3f(x0, y0, z0);
    glVertex3f(x0, y1, z0);
    glVertex3f(x1, y1, z0);

    glColor3f(0.11f, 0.10f, 0.13f);
    glNormal3f(-1.0f, 0.0f, 0.0f);
    glVertex3f(x0, y0, z0);
    glVertex3f(x0, y0, z1);
    glVertex3f(x0, y1, z1);
    glVertex3f(x0, y1, z0);

    glColor3f(0.11f, 0.10f, 0.13f);
    glNormal3f(1.0f, 0.0f, 0.0f);
    glVertex3f(x1, y0, z1);
    glVertex3f(x1, y0, z0);
    glVertex3f(x1, y1, z0);
    glVertex3f(x1, y1, z1);

    glEnd();
}

void drawMapCollisionCubes()
{
    const LevelData& level = getCurrentLevel();
    const float cubeSize = MAP_CELL_SIZE * 0.98f;
    float cubeHeight = level.roomHeight - 1.0f;

    if (cubeHeight < 2.35f)
    {
        cubeHeight = 2.35f;
    }

    for (int z = 1; z < MAP_SIZE - 1; ++z)
    {
        for (int x = 1; x < MAP_SIZE - 1; ++x)
        {
            const int cell = activeMapGrid[z][x];
            if (cell == 0)
            {
                continue;
            }

            if (cell == 2 && doorUnlocked)
            {
                continue;
            }

            const float wx = -ROOM_HALF + (static_cast<float>(x) + 0.5f) * MAP_CELL_SIZE;
            const float wz = -ROOM_HALF + (static_cast<float>(z) + 0.5f) * MAP_CELL_SIZE;

            glPushMatrix();
            glTranslatef(wx, cubeHeight * 0.5f, wz);
            if (cell == 2)
            {
                drawBoxPolygons(cubeSize, cubeHeight, cubeSize, 0.22f, 0.06f, 0.09f);
            }
            else
            {
                drawBoxPolygons(cubeSize, cubeHeight, cubeSize, level.wallColor[0], level.wallColor[1], level.wallColor[2]);
            }
            glPopMatrix();
        }
    }
}

void plotPortalOctants(float cx, float cy, float cz, int x, int y, float step)
{
    glVertex3f(cx + x * step, cy + y * step, cz);
    glVertex3f(cx - x * step, cy + y * step, cz);
    glVertex3f(cx + x * step, cy - y * step, cz);
    glVertex3f(cx - x * step, cy - y * step, cz);
    glVertex3f(cx + y * step, cy + x * step, cz);
    glVertex3f(cx - y * step, cy + x * step, cz);
    glVertex3f(cx + y * step, cy - x * step, cz);
    glVertex3f(cx - y * step, cy - x * step, cz);
}

void drawMidpointCirclePortal(float cx, float cy, float cz, int radius, float step)
{
    int x = 0;
    int y = radius;
    int decision = 1 - radius;

    glBegin(GL_POINTS);
    while (x <= y)
    {
        plotPortalOctants(cx, cy, cz, x, y, step);
        if (decision < 0)
        {
            decision += (2 * x) + 3;
        }
        else
        {
            decision += (2 * (x - y)) + 5;
            --y;
        }
        ++x;
    }
    glEnd();
}

void drawPortalOnBackWall()
{
    const float centerX = 0.0f;
    const float centerY = 5.2f;
    const float centerZ = -ROOM_HALF + 0.03f;
    const float step = 0.03f;

    glDisable(GL_LIGHTING);
    glPointSize((gState.phase == PHASE_WIN) ? 4.0f : 3.0f);

    glPushMatrix();
    glTranslatef(centerX, centerY, centerZ);
    glRotatef(gState.portalAngle, 0.0f, 0.0f, 1.0f);
    glTranslatef(-centerX, -centerY, -centerZ);

    if (gState.phase == PHASE_WIN)
    {
        glColor4f(0.82f, 0.26f, 0.20f, 0.88f);
        drawMidpointCirclePortal(centerX, centerY, centerZ, 94, step);

        glColor4f(0.74f, 0.46f, 0.24f, 0.80f);
        drawMidpointCirclePortal(centerX, centerY, centerZ, 80, step);

        glColor4f(0.90f, 0.83f, 0.78f, 0.72f);
        drawMidpointCirclePortal(centerX, centerY, centerZ, 12, step);
    }
    else if (doorUnlocked)
    {
        glColor4f(0.10f, 0.22f, 0.60f, 0.82f);
        drawMidpointCirclePortal(centerX, centerY, centerZ, 94, step);

        glColor4f(0.16f, 0.34f, 0.74f, 0.72f);
        drawMidpointCirclePortal(centerX, centerY, centerZ, 80, step);

        glColor4f(0.32f, 0.62f, 0.96f, 0.58f);
        drawMidpointCirclePortal(centerX, centerY, centerZ, 64, step);

        glColor4f(0.64f, 0.84f, 0.96f, 0.48f);
        drawMidpointCirclePortal(centerX, centerY, centerZ, 10, step);
    }
    else
    {
        glColor4f(0.18f, 0.05f, 0.08f, 0.50f);
        drawMidpointCirclePortal(centerX, centerY, centerZ, 94, step);

        glColor4f(0.13f, 0.05f, 0.08f, 0.40f);
        drawMidpointCirclePortal(centerX, centerY, centerZ, 80, step);

        glColor4f(0.34f, 0.30f, 0.34f, 0.36f);
        drawMidpointCirclePortal(centerX, centerY, centerZ, 6, step);
    }

    glPopMatrix();
    glEnable(GL_LIGHTING);
}

void drawBresenhamLineXZ(float x0, float z0, float x1, float z1, float y)
{
    const float scale = 80.0f;
    int ix0 = static_cast<int>(std::round(x0 * scale));
    int iz0 = static_cast<int>(std::round(z0 * scale));
    int ix1 = static_cast<int>(std::round(x1 * scale));
    int iz1 = static_cast<int>(std::round(z1 * scale));

    int dx = std::abs(ix1 - ix0);
    int dz = std::abs(iz1 - iz0);
    int sx = (ix0 < ix1) ? 1 : -1;
    int sz = (iz0 < iz1) ? 1 : -1;
    int err = dx - dz;

    glBegin(GL_POINTS);
    while (true)
    {
        glVertex3f(ix0 / scale, y, iz0 / scale);

        if (ix0 == ix1 && iz0 == iz1)
        {
            break;
        }

        int e2 = err * 2;
        if (e2 > -dz)
        {
            err -= dz;
            ix0 += sx;
        }
        if (e2 < dx)
        {
            err += dx;
            iz0 += sz;
        }
    }
    glEnd();
}

void drawDDALineXZ(float x0, float z0, float x1, float z1, float y)
{
    float dx = x1 - x0;
    float dz = z1 - z0;
    float maxDelta = (std::fabs(dx) > std::fabs(dz)) ? std::fabs(dx) : std::fabs(dz);

    int steps = static_cast<int>(maxDelta * 120.0f);
    if (steps < 1)
    {
        steps = 1;
    }

    float x = x0;
    float z = z0;
    float xInc = dx / static_cast<float>(steps);
    float zInc = dz / static_cast<float>(steps);

    glBegin(GL_POINTS);
    for (int i = 0; i <= steps; ++i)
    {
        glVertex3f(x, y, z);
        x += xInc;
        z += zInc;
    }
    glEnd();
}

void drawRuneOnPedestal()
{
    glDisable(GL_LIGHTING);
    glPointSize(2.0f);
    glColor3f(0.64f, 0.60f, 0.66f);

    const float y = PEDESTAL_HEIGHT + 0.04f;

    drawDDALineXZ(0.00f, 0.92f, 0.78f, 0.00f, y);
    drawDDALineXZ(0.78f, 0.00f, 0.00f, -0.92f, y);
    drawDDALineXZ(0.00f, -0.92f, -0.78f, 0.00f, y);
    drawDDALineXZ(-0.78f, 0.00f, 0.00f, 0.92f, y);

    drawDDALineXZ(-0.56f, 0.00f, 0.56f, 0.00f, y);
    drawDDALineXZ(0.00f, 0.56f, 0.00f, -0.56f, y);

    drawDDALineXZ(-0.42f, 0.54f, 0.42f, 0.54f, y);
    drawDDALineXZ(0.42f, 0.54f, 0.62f, -0.20f, y);
    drawDDALineXZ(0.62f, -0.20f, -0.62f, -0.20f, y);
    drawDDALineXZ(-0.62f, -0.20f, -0.42f, 0.54f, y);

    glEnable(GL_LIGHTING);
}

void drawArtifactCrystalFallback()
{
    const float s = 0.32f;
    const float top = 0.56f;
    const float bottom = -0.56f;

    glColor3f(0.74f, 0.70f, 0.68f);
    glBegin(GL_POLYGON);
    glNormal3f(0.58f, 0.58f, 0.58f);
    glVertex3f(0.0f, top, 0.0f);
    glVertex3f(s, 0.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, s);
    glEnd();

    glBegin(GL_POLYGON);
    glNormal3f(-0.58f, 0.58f, 0.58f);
    glVertex3f(0.0f, top, 0.0f);
    glVertex3f(0.0f, 0.0f, s);
    glVertex3f(-s, 0.0f, 0.0f);
    glEnd();

    glBegin(GL_POLYGON);
    glNormal3f(-0.58f, 0.58f, -0.58f);
    glVertex3f(0.0f, top, 0.0f);
    glVertex3f(-s, 0.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, -s);
    glEnd();

    glBegin(GL_POLYGON);
    glNormal3f(0.58f, 0.58f, -0.58f);
    glVertex3f(0.0f, top, 0.0f);
    glVertex3f(0.0f, 0.0f, -s);
    glVertex3f(s, 0.0f, 0.0f);
    glEnd();

    glColor3f(0.50f, 0.16f, 0.18f);
    glBegin(GL_POLYGON);
    glNormal3f(0.58f, -0.58f, 0.58f);
    glVertex3f(0.0f, bottom, 0.0f);
    glVertex3f(0.0f, 0.0f, s);
    glVertex3f(s, 0.0f, 0.0f);
    glEnd();

    glBegin(GL_POLYGON);
    glNormal3f(-0.58f, -0.58f, 0.58f);
    glVertex3f(0.0f, bottom, 0.0f);
    glVertex3f(-s, 0.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, s);
    glEnd();

    glBegin(GL_POLYGON);
    glNormal3f(-0.58f, -0.58f, -0.58f);
    glVertex3f(0.0f, bottom, 0.0f);
    glVertex3f(0.0f, 0.0f, -s);
    glVertex3f(-s, 0.0f, 0.0f);
    glEnd();

    glBegin(GL_POLYGON);
    glNormal3f(0.58f, -0.58f, -0.58f);
    glVertex3f(0.0f, bottom, 0.0f);
    glVertex3f(s, 0.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, -s);
    glEnd();
}

void drawArtifactCrystal()
{
    if (artifactMeshList != 0)
    {
        glColor3f(0.70f, 0.68f, 0.72f);
        glCallList(artifactMeshList);
        return;
    }

    if (!artifactMesh.vertices.empty())
    {
        glColor3f(0.70f, 0.68f, 0.72f);
        drawMeshFromArrays(artifactMesh);
        return;
    }

    drawArtifactCrystalFallback();
}

void drawFloatingArtifact()
{
    const float bob = 0.28f * std::sin(gState.timeValue * 2.0f);
    float pulse = 0.94f + 0.08f * std::sin(gState.timeValue * 3.0f);
    if (gState.phase == PHASE_WIN)
    {
        pulse = 1.10f + 0.16f * std::sin(gState.timeValue * 9.0f);
    }

    glPushMatrix();
    glTranslatef(0.0f, PEDESTAL_HEIGHT + 0.95f + bob, 0.0f);
    glScalef(pulse, pulse, pulse);
    drawArtifactCrystal();
    glPopMatrix();
}

void drawCodeFragments()
{
    for (int i = 0; i < interactableCount; ++i)
    {
        const Interactable& item = interactables[i];
        if ((item.typeFlags & TYPE_PICKUP) == 0 || !item.active)
        {
            continue;
        }

        float bob = 0.18f * std::sin(gState.timeValue * 3.6f + static_cast<float>(i));
        float pulse = 1.0f;
        float colorR = 0.95f;
        float colorG = 0.86f;
        float colorB = 0.20f;

        if (item.sequenceOrder > 0)
        {
            if (item.sequenceOrder == currentSequenceTarget)
            {
                const float pulseWave = 0.5f + 0.5f * std::sin(gState.timeValue * 14.0f + static_cast<float>(item.sequenceOrder));
                bob = 0.22f * std::sin(gState.timeValue * 11.0f + static_cast<float>(item.sequenceOrder));
                pulse = 1.03f + 0.25f * pulseWave;
                colorR = 0.98f;
                colorG = 0.90f + 0.08f * pulseWave;
                colorB = 0.24f;
            }
            else if (item.sequenceOrder > currentSequenceTarget)
            {
                bob = 0.0f;
                pulse = 1.0f;
                colorR = 0.18f;
                colorG = 0.18f;
                colorB = 0.18f;
            }
        }

        glPushMatrix();
        glTranslatef(item.x, item.y + bob, item.z);
        glScalef(pulse, pulse, pulse);
        drawBoxPolygons(0.46f, 0.46f, 0.46f, colorR, colorG, colorB);
        glPopMatrix();
    }
}

void drawReadableTerminal()
{
    for (int i = 0; i < interactableCount; ++i)
    {
        const Interactable& item = interactables[i];
        if ((item.typeFlags & TYPE_READABLE) == 0 || !item.active)
        {
            continue;
        }

        glPushMatrix();
        glTranslatef(item.x, 0.65f, item.z);
        drawBoxPolygons(0.68f, 1.30f, 0.44f, 0.10f, 0.10f, 0.13f);
        glTranslatef(0.0f, 0.26f, 0.23f);
        if (readableVisited)
        {
            drawBoxPolygons(0.46f, 0.46f, 0.02f, 0.12f, 0.42f, 0.84f);
        }
        else
        {
            drawBoxPolygons(0.46f, 0.46f, 0.02f, 0.24f, 0.05f, 0.10f);
        }
        glPopMatrix();
    }
}

void drawGuardianFallbackPrimitive()
{
    const float armSwing = 8.0f * std::sin(gState.timeValue * 2.2f);

    glPushMatrix();
    glTranslatef(-0.24f, 0.62f, 0.0f);
    drawBoxPolygons(0.30f, 1.24f, 0.34f, 0.15f, 0.15f, 0.18f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.24f, 0.62f, 0.0f);
    drawBoxPolygons(0.30f, 1.24f, 0.34f, 0.15f, 0.15f, 0.18f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, 1.28f, 0.0f);
    drawBoxPolygons(0.74f, 0.34f, 0.40f, 0.17f, 0.16f, 0.20f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, 2.02f, 0.0f);
    drawBoxPolygons(0.96f, 1.20f, 0.52f, 0.19f, 0.17f, 0.23f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-0.66f, 2.08f, 0.0f);
    glRotatef(-armSwing, 1.0f, 0.0f, 0.0f);
    glTranslatef(0.0f, -0.48f, 0.0f);
    drawBoxPolygons(0.24f, 0.96f, 0.26f, 0.18f, 0.16f, 0.20f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.66f, 2.08f, 0.0f);
    glRotatef(armSwing, 1.0f, 0.0f, 0.0f);
    glTranslatef(0.0f, -0.48f, 0.0f);
    drawBoxPolygons(0.24f, 0.96f, 0.26f, 0.18f, 0.16f, 0.20f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, 2.85f, 0.0f);
    glRotatef(gState.headAngle, 0.0f, 1.0f, 0.0f);
    drawBoxPolygons(0.58f, 0.56f, 0.46f, 0.26f, 0.23f, 0.29f);

    glColor3f(0.36f, 0.09f, 0.11f);
    glBegin(GL_POLYGON);
    glNormal3f(-0.2f, 0.9f, 0.2f);
    glVertex3f(-0.17f, 0.30f, 0.08f);
    glVertex3f(-0.32f, 0.62f, 0.02f);
    glVertex3f(-0.08f, 0.35f, -0.05f);
    glEnd();

    glBegin(GL_POLYGON);
    glNormal3f(0.2f, 0.9f, 0.2f);
    glVertex3f(0.17f, 0.30f, 0.08f);
    glVertex3f(0.08f, 0.35f, -0.05f);
    glVertex3f(0.32f, 0.62f, 0.02f);
    glEnd();

    glColor3f(0.61f, 0.58f, 0.62f);
    glBegin(GL_POLYGON);
    glNormal3f(0.0f, 0.0f, 1.0f);
    glVertex3f(-0.17f, 0.06f, 0.24f);
    glVertex3f(-0.05f, 0.06f, 0.24f);
    glVertex3f(-0.05f, -0.02f, 0.24f);
    glVertex3f(-0.17f, -0.02f, 0.24f);
    glEnd();

    glBegin(GL_POLYGON);
    glNormal3f(0.0f, 0.0f, 1.0f);
    glVertex3f(0.05f, 0.06f, 0.24f);
    glVertex3f(0.17f, 0.06f, 0.24f);
    glVertex3f(0.17f, -0.02f, 0.24f);
    glVertex3f(0.05f, -0.02f, 0.24f);
    glEnd();

    glPopMatrix();
}

void drawGuardian()
{
    glPushMatrix();
    glTranslatef(gState.guardianX, 0.0f, gState.guardianZ);

    const float facingAngleDegrees = std::atan2(adminForwardX, adminForwardZ) * 57.2957795f;
    glRotatef(facingAngleDegrees, 0.0f, 1.0f, 0.0f);

    const float guardianScaleY = gState.guardianScale * getCurrentLevel().adminScaleY;
    glScalef(gState.guardianScale, guardianScaleY, gState.guardianScale);

    if (guardianMeshList != 0)
    {
        const float breathe = 1.0f + 0.02f * std::sin(gState.timeValue * 2.4f);
        glScalef(breathe, breathe, breathe);
        glRotatef(gState.headAngle * 0.45f, 0.0f, 1.0f, 0.0f);
        glColor3f(0.17f, 0.16f, 0.19f);
        glCallList(guardianMeshList);
    }
    else if (!guardianMesh.vertices.empty())
    {
        const float breathe = 1.0f + 0.02f * std::sin(gState.timeValue * 2.4f);
        glScalef(breathe, breathe, breathe);
        glRotatef(gState.headAngle * 0.45f, 0.0f, 1.0f, 0.0f);
        glColor3f(0.17f, 0.16f, 0.19f);
        drawMeshFromArrays(guardianMesh);
    }
    else
    {
        drawGuardianFallbackPrimitive();
    }

    glPopMatrix();
}

void setupCamera()
{
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    const float forwardX = std::cos(yaw);
    const float forwardZ = std::sin(yaw);
    const float lookX = gState.camX + forwardX + gState.panicJitterX;
    const float lookY = gState.camY + gState.panicJitterY;
    const float lookZ = gState.camZ + forwardZ;

    const float rightX = std::cos(yaw + 1.5708f);
    const float rightZ = std::sin(yaw + 1.5708f);
    const float rollSin = std::sin(gState.camRollOffset);
    const float rollCos = std::cos(gState.camRollOffset);
    const float upX = rightX * rollSin;
    const float upY = rollCos;
    const float upZ = rightZ * rollSin;

    gluLookAt(
        gState.camX, gState.camY, gState.camZ,
        lookX, lookY, lookZ,
        upX, upY, upZ
    );

    // Update flashlight after the view matrix is established so it tracks the camera.
    const GLfloat flashlightPos[] = { gState.camX, gState.camY, gState.camZ, 1.0f };
    const GLfloat flashlightDir[] = { forwardX, -0.10f, forwardZ };
    glLightfv(GL_LIGHT0, GL_POSITION, flashlightPos);
    glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, flashlightDir);
    glLightf(GL_LIGHT0, GL_SPOT_CUTOFF, gState.flashlightCutoff);

    const GLfloat lamp1Pos[] = { -9.0f, 4.0f, -9.0f, 1.0f };
    glLightfv(GL_LIGHT2, GL_POSITION, lamp1Pos);
    const GLfloat lamp2Pos[] = { 9.0f, 4.0f, 9.0f, 1.0f };
    glLightfv(GL_LIGHT3, GL_POSITION, lamp2Pos);
    const GLfloat lamp3Pos[] = { -9.0f, 4.0f, 9.0f, 1.0f };
    glLightfv(GL_LIGHT4, GL_POSITION, lamp3Pos);
    const GLfloat lamp4Pos[] = { 9.0f, 4.0f, -9.0f, 1.0f };
    glLightfv(GL_LIGHT5, GL_POSITION, lamp4Pos);

    const float adminGlowHeight = 2.6f * gState.guardianScale * getCurrentLevel().adminScaleY;
    const GLfloat adminGlowPos[] = { gState.guardianX, adminGlowHeight, gState.guardianZ, 1.0f };
    glLightfv(GL_LIGHT1, GL_POSITION, adminGlowPos);
}

void configureDynamicAtmosphere()
{
    GLfloat fogColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    float fogDensity = getCurrentLevel().fogDensity;

    if (gState.phase == PHASE_GAME_OVER)
    {
        glClearColor(0.01f, 0.0f, 0.0f, 1.0f);
        fogDensity = getCurrentLevel().fogDensity + 0.008f;
    }
    else if (gState.phase == PHASE_WIN)
    {
        glClearColor(0.0f, 0.0f, 0.01f, 1.0f);
        fogDensity = getCurrentLevel().fogDensity - 0.005f;
        if (fogDensity < 0.010f)
        {
            fogDensity = 0.010f;
        }
    }
    else
    {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    }

    glFogi(GL_FOG_MODE, GL_EXP2);
    glFogfv(GL_FOG_COLOR, fogColor);
    glFogf(GL_FOG_DENSITY, fogDensity);
}

void configureLight()
{
    const GLfloat globalAmbient[] = { 0.02f, 0.02f, 0.02f, 1.0f };
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmbient);

    const GLfloat ambient[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    const GLfloat diffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    const GLfloat specular[] = { 0.80f, 0.80f, 0.86f, 1.0f };
    const GLfloat adminAmbient[] = { 0.02f, 0.0f, 0.0f, 1.0f };
    const GLfloat adminDiffuse[] = { 0.8f, 0.1f, 0.1f, 1.0f };
    const GLfloat adminSpecular[] = { 0.35f, 0.08f, 0.08f, 1.0f };
    const GLfloat lampColor[] = { 0.8f, 0.1f, 0.1f, 1.0f };
    const GLfloat zeroAmbient[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    const bool adminGlowEnabled = (currentLevelIndex == 1) && (gameState == STATE_PLAYING);

    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, specular);
    glLightf(GL_LIGHT0, GL_SPOT_CUTOFF, gState.flashlightCutoff); // Panic can narrow cone dynamically
    glLightf(GL_LIGHT0, GL_SPOT_EXPONENT, 12.0f); // Softer center-to-edge falloff inside cone
    glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, 1.0f); // Keep baseline intensity stable
    glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, 0.005f); // Lower linear distance dimming
    glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0.001f); // Gentle far-distance drop-off

    glLightfv(GL_LIGHT1, GL_AMBIENT, adminAmbient);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, adminDiffuse);
    glLightfv(GL_LIGHT1, GL_SPECULAR, adminSpecular);
    glLightf(GL_LIGHT1, GL_CONSTANT_ATTENUATION, 0.12f);
    glLightf(GL_LIGHT1, GL_LINEAR_ATTENUATION, 1.10f);
    glLightf(GL_LIGHT1, GL_QUADRATIC_ATTENUATION, 0.15f);

    for (GLenum light = GL_LIGHT2; light <= GL_LIGHT5; ++light)
    {
        glLightfv(light, GL_DIFFUSE, lampColor);
        glLightfv(light, GL_SPECULAR, lampColor);
        glLightfv(light, GL_AMBIENT, zeroAmbient);
        glLightf(light, GL_CONSTANT_ATTENUATION, 0.1f);
        glLightf(light, GL_LINEAR_ATTENUATION, 0.08f);
        glLightf(light, GL_QUADRATIC_ATTENUATION, 0.02f);
    }

    if (adminGlowEnabled)
    {
        glEnable(GL_LIGHT1);
    }
    else
    {
        glDisable(GL_LIGHT1);
    }
}

void renderText(float x, float y, void* font, const char* string)
{
    if (font == nullptr || string == nullptr)
    {
        return;
    }

    glRasterPos2f(x, y);
    for (const char* c = string; *c != '\0'; ++c)
    {
        glutBitmapCharacter(font, *c);
    }
}

float getTextWidth(void* font, const char* string)
{
    if (font == nullptr || string == nullptr)
    {
        return 0.0f;
    }

    int width = 0;
    for (const char* c = string; *c != '\0'; ++c)
    {
        width += glutBitmapWidth(font, *c);
    }
    return static_cast<float>(width);
}

float centeredTextX(void* font, const char* string)
{
    return (windowWidth - getTextWidth(font, string)) * 0.5f;
}

void beginOrthoTextPass()
{
    glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glDisable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0, static_cast<double>(windowWidth), 0.0, static_cast<double>(windowHeight), -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
}

void endOrthoTextPass()
{
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glPopAttrib();
    glMatrixMode(GL_MODELVIEW);
}

void drawFrameLines(float left, float bottom, float right, float top)
{
    glBegin(GL_LINES);
    glVertex2f(left, bottom);
    glVertex2f(right, bottom);

    glVertex2f(right, bottom);
    glVertex2f(right, top);

    glVertex2f(right, top);
    glVertex2f(left, top);

    glVertex2f(left, top);
    glVertex2f(left, bottom);
    glEnd();
}

void renderMenuScreen()
{
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    beginOrthoTextPass();

    const float left = windowWidth * 0.14f;
    const float right = windowWidth * 0.86f;
    const float bottom = windowHeight * 0.16f;
    const float top = windowHeight * 0.86f;

    glLineWidth(1.0f);
    glColor3f(1.0f, 1.0f, 1.0f);
    drawFrameLines(left, bottom, right, top);

    glColor3f(0.8f, 0.1f, 0.1f);
    drawFrameLines(left + 10.0f, bottom + 10.0f, right - 10.0f, top - 10.0f);

    glBegin(GL_LINES);
    glColor3f(1.0f, 1.0f, 1.0f);
    glVertex2f(left + 24.0f, top - 42.0f);
    glVertex2f(right - 24.0f, top - 42.0f);
    glVertex2f(left + 40.0f, top - 42.0f);
    glVertex2f(left + 40.0f, top - 74.0f);
    glVertex2f(right - 40.0f, top - 42.0f);
    glVertex2f(right - 40.0f, top - 74.0f);

    glColor3f(0.8f, 0.1f, 0.1f);
    glVertex2f(left + 24.0f, bottom + 48.0f);
    glVertex2f(right - 24.0f, bottom + 48.0f);
    glVertex2f(left + 42.0f, bottom + 48.0f);
    glVertex2f(left + 42.0f, bottom + 80.0f);
    glVertex2f(right - 42.0f, bottom + 48.0f);
    glVertex2f(right - 42.0f, bottom + 80.0f);
    glEnd();

    const char* title = "[ THE_GLITCHED_SERVER ]";
    const char* author = "[ OPERATOR_CHANNEL : COFFEEHEAD ]";
    const char* sys = "[ SYSTEM : INITIALIZED ]";
    const char* line1 = "> NAV_WASD : MOVE";
    const char* line2 = "> INPUT_E : INTERACT";
    const char* line3 = "> OBJECTIVE : RECOVER_CODE_FRAGMENTS";
    const char* line4 = "> THREAT : ADMIN_UNIT_PATROLLING";
    const char* boot = "[ ENTER ] : START_SESSION";

    glColor3f(1.0f, 1.0f, 1.0f);
    renderText(centeredTextX(GLUT_BITMAP_HELVETICA_18, title), windowHeight * 0.74f, GLUT_BITMAP_HELVETICA_18, title);

    glColor3f(0.78f, 0.78f, 0.78f);
    renderText(centeredTextX(GLUT_BITMAP_HELVETICA_12, author), windowHeight * 0.69f, GLUT_BITMAP_HELVETICA_12, author);

    glColor3f(0.8f, 0.1f, 0.1f);
    renderText(centeredTextX(GLUT_BITMAP_HELVETICA_12, sys), windowHeight * 0.58f, GLUT_BITMAP_HELVETICA_12, sys);

    glColor3f(0.92f, 0.92f, 0.92f);
    renderText(centeredTextX(GLUT_BITMAP_8_BY_13, line1), windowHeight * 0.52f, GLUT_BITMAP_8_BY_13, line1);
    renderText(centeredTextX(GLUT_BITMAP_8_BY_13, line2), windowHeight * 0.48f, GLUT_BITMAP_8_BY_13, line2);
    renderText(centeredTextX(GLUT_BITMAP_8_BY_13, line3), windowHeight * 0.44f, GLUT_BITMAP_8_BY_13, line3);
    renderText(centeredTextX(GLUT_BITMAP_8_BY_13, line4), windowHeight * 0.40f, GLUT_BITMAP_8_BY_13, line4);

    glColor3f(1.0f, 1.0f, 1.0f);
    renderText(centeredTextX(GLUT_BITMAP_HELVETICA_12, boot), windowHeight * 0.30f, GLUT_BITMAP_HELVETICA_12, boot);

    endOrthoTextPass();
}

void renderGameOverScreen()
{
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    beginOrthoTextPass();

    const float left = windowWidth * 0.22f;
    const float right = windowWidth * 0.78f;
    const float bottom = windowHeight * 0.33f;
    const float top = windowHeight * 0.68f;

    glLineWidth(1.0f);
    glColor3f(1.0f, 1.0f, 1.0f);
    drawFrameLines(left, bottom, right, top);

    glColor3f(0.8f, 0.1f, 0.1f);
    drawFrameLines(left + 8.0f, bottom + 8.0f, right - 8.0f, top - 8.0f);

    glBegin(GL_LINES);
    glColor3f(0.8f, 0.1f, 0.1f);
    glVertex2f(left + 20.0f, top - 40.0f);
    glVertex2f(right - 20.0f, top - 40.0f);
    glColor3f(1.0f, 1.0f, 1.0f);
    glVertex2f(left + 20.0f, bottom + 44.0f);
    glVertex2f(right - 20.0f, bottom + 44.0f);
    glEnd();

    const char* headline = "[ SYSTEM : FATAL_EXCEPTION ]";
    const char* sub = "[ ADMIN_OVERRIDE : COMPLETE ]";
    const char* quitHint = "> PRESS [ESC] TO TERMINATE_LINK <";

    glColor3f(0.8f, 0.1f, 0.1f);
    renderText(centeredTextX(GLUT_BITMAP_HELVETICA_18, headline), windowHeight * 0.56f, GLUT_BITMAP_HELVETICA_18, headline);
    glColor3f(0.92f, 0.92f, 0.92f);
    renderText(centeredTextX(GLUT_BITMAP_HELVETICA_12, sub), windowHeight * 0.50f, GLUT_BITMAP_HELVETICA_12, sub);
    glColor3f(1.0f, 1.0f, 1.0f);
    renderText(centeredTextX(GLUT_BITMAP_HELVETICA_12, quitHint), windowHeight * 0.43f, GLUT_BITMAP_HELVETICA_12, quitHint);

    endOrthoTextPass();
}

void renderWinScreen()
{
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    beginOrthoTextPass();

    const float left = windowWidth * 0.22f;
    const float right = windowWidth * 0.78f;
    const float bottom = windowHeight * 0.34f;
    const float top = windowHeight * 0.68f;

    glLineWidth(1.0f);
    glColor3f(1.0f, 1.0f, 1.0f);
    drawFrameLines(left, bottom, right, top);

    glColor3f(0.8f, 0.1f, 0.1f);
    drawFrameLines(left + 8.0f, bottom + 8.0f, right - 8.0f, top - 8.0f);

    glBegin(GL_LINES);
    glColor3f(1.0f, 1.0f, 1.0f);
    glVertex2f(left + 20.0f, top - 40.0f);
    glVertex2f(right - 20.0f, top - 40.0f);
    glColor3f(0.8f, 0.1f, 0.1f);
    glVertex2f(left + 20.0f, bottom + 44.0f);
    glVertex2f(right - 20.0f, bottom + 44.0f);
    glEnd();

    const char* title = "[ LEVEL : CLEARED ]";
    const char* subtitle = "[ CORE_LINK : RESTORED ]";
    const char* status = "> EXIT_CHANNEL : STABILIZED <";

    glColor3f(1.0f, 1.0f, 1.0f);
    renderText(centeredTextX(GLUT_BITMAP_HELVETICA_18, title), windowHeight * 0.56f, GLUT_BITMAP_HELVETICA_18, title);
    glColor3f(0.88f, 0.88f, 0.88f);
    renderText(centeredTextX(GLUT_BITMAP_HELVETICA_12, subtitle), windowHeight * 0.50f, GLUT_BITMAP_HELVETICA_12, subtitle);
    glColor3f(0.8f, 0.1f, 0.1f);
    renderText(centeredTextX(GLUT_BITMAP_HELVETICA_12, status), windowHeight * 0.44f, GLUT_BITMAP_HELVETICA_12, status);

    endOrthoTextPass();
}

void renderPickupPrompt()
{
    if (!canInteract || targetedInteractableIndex < 0 || targetedInteractableIndex >= interactableCount)
    {
        return;
    }

    beginOrthoTextPass();

    const Interactable& item = interactables[targetedInteractableIndex];
    const char* prompt = "> INTERACTION_READY : PRESS [E] <";
    if ((item.typeFlags & TYPE_PICKUP) != 0)
    {
        prompt = "> FRAGMENT_DETECTED : PRESS [E] <";
    }
    else if ((item.typeFlags & TYPE_READABLE) != 0)
    {
        prompt = "> TERMINAL_LINK : PRESS [E] <";
    }

    const float cx = windowWidth * 0.5f;
    const float cy = windowHeight * 0.5f;
    const float offset = 14.0f;
    const float arm = 10.0f;

    glLineWidth(1.0f);
    glBegin(GL_LINES);
    glColor3f(1.0f, 1.0f, 1.0f);

    glVertex2f(cx - offset - arm, cy + offset);
    glVertex2f(cx - offset, cy + offset);
    glVertex2f(cx - offset - arm, cy + offset);
    glVertex2f(cx - offset - arm, cy + offset - arm);

    glVertex2f(cx + offset, cy + offset);
    glVertex2f(cx + offset + arm, cy + offset);
    glVertex2f(cx + offset + arm, cy + offset);
    glVertex2f(cx + offset + arm, cy + offset - arm);

    glVertex2f(cx - offset - arm, cy - offset);
    glVertex2f(cx - offset, cy - offset);
    glVertex2f(cx - offset - arm, cy - offset);
    glVertex2f(cx - offset - arm, cy - offset + arm);

    glVertex2f(cx + offset, cy - offset);
    glVertex2f(cx + offset + arm, cy - offset);
    glVertex2f(cx + offset + arm, cy - offset);
    glVertex2f(cx + offset + arm, cy - offset + arm);

    glColor3f(0.8f, 0.1f, 0.1f);
    glVertex2f(cx - 6.0f, cy);
    glVertex2f(cx + 6.0f, cy);
    glEnd();

    glColor3f(1.0f, 1.0f, 1.0f);
    renderText(centeredTextX(GLUT_BITMAP_8_BY_13, prompt), cy - 42.0f, GLUT_BITMAP_8_BY_13, prompt);

    endOrthoTextPass();
}

void drawWallLamps()
{
    glDisable(GL_LIGHTING);
    float pulse = 0.7f + 0.3f * std::sin(gState.timeValue * 6.0f);
    float r = 0.8f;
    float g = 0.1f;
    float b = 0.1f;

    glPushMatrix();
    glTranslatef(-9.0f, 4.0f, -9.0f);
    drawBoxPolygons(0.6f, 0.2f, 0.6f, r * pulse, g * pulse, b * pulse);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(9.0f, 4.0f, 9.0f);
    drawBoxPolygons(0.6f, 0.2f, 0.6f, r * pulse, g * pulse, b * pulse);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-9.0f, 4.0f, 9.0f);
    drawBoxPolygons(0.6f, 0.2f, 0.6f, r * pulse, g * pulse, b * pulse);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(9.0f, 4.0f, -9.0f);
    drawBoxPolygons(0.6f, 0.2f, 0.6f, r * pulse, g * pulse, b * pulse);
    glPopMatrix();

    glEnable(GL_LIGHTING);
}

void display()
{
    switch (gameState)
    {
    case STATE_MENU:
        renderMenuScreen();
        break;

    case STATE_GAME_OVER:
        renderGameOverScreen();
        break;

    case STATE_WIN:
        renderWinScreen();
        break;

    case STATE_PLAYING:
    default:
        configureDynamicAtmosphere();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        setupCamera();
        configureLight();

        drawRoomShell();
        drawMapCollisionCubes();
        drawWallLamps();
        drawPedestal();
        drawPortalOnBackWall();
        drawRuneOnPedestal();
        drawFloatingArtifact();
        drawCodeFragments();
        drawReadableTerminal();
        drawGuardian();
        renderPickupPrompt();
        break;
    }

    glutSwapBuffers();
}

void reshape(int width, int height)
{
    if (height == 0)
    {
        height = 1;
    }

    windowWidth = width;
    windowHeight = height;

    float aspect = static_cast<float>(width) / static_cast<float>(height);
    glViewport(0, 0, width, height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(66.0f, aspect, 0.1f, 130.0f);

    glMatrixMode(GL_MODELVIEW);
}

void updateScene()
{
    if (gameState != STATE_PLAYING)
    {
        glutPostRedisplay();
        return;
    }

    const int now = glutGet(GLUT_ELAPSED_TIME);
    float dt = (now - gState.lastTicks) * 0.001f;
    if (dt < 0.0f)
    {
        dt = 0.0f;
    }
    if (dt > 0.1f)
    {
        dt = 0.1f;
    }
    gState.lastTicks = now;

    gState.timeValue += dt;

    if (keyHeld[static_cast<unsigned char>('q')])
    {
        yaw -= PLAYER_TURN_SPEED * dt;
    }
    if (keyHeld[static_cast<unsigned char>('r')])
    {
        yaw += PLAYER_TURN_SPEED * dt;
    }

    const float twoPi = 6.2831853f;
    if (yaw > twoPi)
    {
        yaw -= twoPi;
    }
    else if (yaw < -twoPi)
    {
        yaw += twoPi;
    }

    const int forwardAxis =
        ((keyHeld[static_cast<unsigned char>('w')] || keyHeld[static_cast<unsigned char>('W')]) ? 1 : 0) -
        ((keyHeld[static_cast<unsigned char>('s')] || keyHeld[static_cast<unsigned char>('S')]) ? 1 : 0);
    const int strafeAxis =
        ((keyHeld[static_cast<unsigned char>('d')] || keyHeld[static_cast<unsigned char>('D')]) ? 1 : 0) -
        ((keyHeld[static_cast<unsigned char>('a')] || keyHeld[static_cast<unsigned char>('A')]) ? 1 : 0);

    const bool hasMovementIntent = (forwardAxis != 0) || (strafeAxis != 0);
    const bool sprintRequested = (forwardAxis > 0) && keyHeld[static_cast<unsigned char>('W')] && (gState.stamina > 0.01f);
    const float speedMultiplier = sprintRequested ? PLAYER_SPRINT_MULT : 1.0f;
    const float desiredSpeed = PLAYER_WALK_SPEED * speedMultiplier;

    bool movedThisFrame = false;
    if (hasMovementIntent)
    {
        float wishX = std::cos(yaw) * static_cast<float>(forwardAxis) + std::cos(yaw + 1.5708f) * static_cast<float>(strafeAxis);
        float wishZ = std::sin(yaw) * static_cast<float>(forwardAxis) + std::sin(yaw + 1.5708f) * static_cast<float>(strafeAxis);
        normalizeXZ(wishX, wishZ);

        const float stepX = wishX * desiredSpeed * dt;
        const float stepZ = wishZ * desiredSpeed * dt;

        if (canMoveTo(gState.camX + stepX, gState.camZ + stepZ))
        {
            gState.camX += stepX;
            gState.camZ += stepZ;
            movedThisFrame = true;
        }
        else
        {
            if (canMoveTo(gState.camX + stepX, gState.camZ))
            {
                gState.camX += stepX;
                movedThisFrame = true;
            }
            if (canMoveTo(gState.camX, gState.camZ + stepZ))
            {
                gState.camZ += stepZ;
                movedThisFrame = true;
            }
        }

        clampCameraToRoom();
    }

    if (sprintRequested && movedThisFrame)
    {
        gState.stamina -= STAMINA_DRAIN_RATE * dt;
    }
    else
    {
        gState.stamina += STAMINA_RECOVER_RATE * dt;
    }
    gState.stamina = clampf(gState.stamina, 0.0f, STAMINA_MAX);

    if (gState.stamina < STAMINA_PANIC_THRESHOLD)
    {
        const float panic = (STAMINA_PANIC_THRESHOLD - gState.stamina) / STAMINA_PANIC_THRESHOLD;
        gState.flashlightCutoff = FLASHLIGHT_BASE_CUTOFF - (FLASHLIGHT_BASE_CUTOFF - FLASHLIGHT_PANIC_CUTOFF) * panic;
        gState.panicJitterX = randomSignedUnit() * PANIC_JITTER_X_MAX * panic;
        gState.panicJitterY = randomSignedUnit() * PANIC_JITTER_Y_MAX * panic;
    }
    else
    {
        gState.flashlightCutoff = FLASHLIGHT_BASE_CUTOFF;
        gState.panicJitterX = 0.0f;
        gState.panicJitterY = 0.0f;
    }

    if (movedThisFrame)
    {
        const float bobFreq = sprintRequested ? HEAD_BOB_FREQ_SPRINT : HEAD_BOB_FREQ_WALK;
        const float bobAmp = sprintRequested ? HEAD_BOB_AMP_SPRINT : HEAD_BOB_AMP_WALK;
        const float rollAmp = sprintRequested ? HEAD_ROLL_AMP_SPRINT : HEAD_ROLL_AMP_WALK;

        gState.camBobOffset = std::sin(gState.timeValue * bobFreq) * bobAmp;
        gState.camRollOffset = std::cos(gState.timeValue * bobFreq * 0.5f) * rollAmp;
    }
    else
    {
        const float bobDamp = clampf(1.0f - (8.0f * dt), 0.0f, 1.0f);
        const float rollDamp = clampf(1.0f - (10.0f * dt), 0.0f, 1.0f);
        gState.camBobOffset *= bobDamp;
        gState.camRollOffset *= rollDamp;
    }
    gState.camY = gState.camBaseY + gState.camBobOffset;

    gState.headAngle += gState.headDir * 34.0f * dt;
    if (gState.headAngle > 30.0f)
    {
        gState.headAngle = 30.0f;
        gState.headDir = -1.0f;
    }
    if (gState.headAngle < -30.0f)
    {
        gState.headAngle = -30.0f;
        gState.headDir = 1.0f;
    }

    gState.portalAngle += gState.portalSpinSpeed * dt;
    if (gState.portalAngle >= 360.0f)
    {
        gState.portalAngle -= 360.0f;
    }

    if (gState.phase == PHASE_PLAYING)
    {
        canInteract = false;
        targetedInteractableIndex = -1;
        updateDoorLockState();

        const float adminRoamStep = ADMIN_ROAM_STEP * getCurrentLevel().adminSpeedMult;
        const float adminHuntStep = ADMIN_HUNT_STEP * getCurrentLevel().adminSpeedMult;
        const bool playerDetected = canAdminDetectPlayer();
        const int playerGridX = worldToGrid(gState.camX);
        const int playerGridZ = worldToGrid(gState.camZ);
        const int adminGridX = worldToGrid(gState.guardianX);
        const int adminGridZ = worldToGrid(gState.guardianZ);

        adminChasing = adminForcedChase || playerDetected;

        if (adminChasing)
        {
            const bool chaseGoalMoved =
                (playerGridX != adminPathGoalX) ||
                (playerGridZ != adminPathGoalZ) ||
                (adminPathLevelIndex != currentLevelIndex);

            if (chaseGoalMoved || adminPath.empty())
            {
                if (findAdminPath(adminGridX, adminGridZ, playerGridX, playerGridZ))
                {
                    adminPathGoalX = playerGridX;
                    adminPathGoalZ = playerGridZ;
                    adminPathLevelIndex = currentLevelIndex;
                    adminHasWaypoint = true;
                    adminWaypointX = gridToWorld(playerGridX);
                    adminWaypointZ = gridToWorld(playerGridZ);
                }
                else
                {
                    adminPath.clear();
                }
            }

            followAdminPath(adminHuntStep);

            if (adminForcedChase && !playerDetected && adminPath.empty())
            {
                adminForcedChase = false;
                adminHasWaypoint = false;
            }
        }
        else
        {
            adminPathGoalX = -1;
            adminPathGoalZ = -1;
            adminPathLevelIndex = -1;

            if (!adminHasWaypoint || adminPath.empty() || adminStuckFrames > ADMIN_STUCK_FRAME_LIMIT)
            {
                chooseRandomAdminWaypoint();
            }

            followAdminPath(adminRoamStep);

            if (adminPath.empty())
            {
                adminHasWaypoint = false;
            }
        }

        gState.guardianX = clampf(gState.guardianX, -ROOM_HALF + 1.2f, ROOM_HALF - 1.2f);
        gState.guardianZ = clampf(gState.guardianZ, -ROOM_HALF + 1.2f, ROOM_HALF - 1.2f);

        const float distAfterMove = std::sqrt(
                                        std::pow(gState.camX - gState.guardianX, 2.0f) +
                                        std::pow(gState.camZ - gState.guardianZ, 2.0f)
                                    );

        if (distAfterMove <= ADMIN_HIT_RADIUS)
        {
            gameState = STATE_GAME_OVER;
            triggerGameOver();
        }
        else
        {
            updateInteractionTarget();

            if (doorUnlocked)
            {
                const float distToPortal = std::sqrt(
                                               std::pow(gState.camX - 0.0f, 2.0f) +
                                               std::pow(gState.camZ - (-18.0f), 2.0f)
                                           );
                if (distToPortal < 2.5f)
                {
                    if (currentLevelIndex + 1 < static_cast<int>(levels.size()))
                    {
                        const int nextLevel = currentLevelIndex + 1;
                        std::printf("LEVEL %d CLEARED. Descending into Level %d.\n", currentLevelIndex + 1, nextLevel + 1);
                        loadLevel(nextLevel);
                        gState.lastTicks = now;
                        glutPostRedisplay();
                        return;
                    }

                    gameState = STATE_WIN;
                    triggerWin();
                }
            }
        }
    }

    glutPostRedisplay();
}

void keyboard(unsigned char key, int, int)
{
    if (key == 27)
    {
        std::exit(0);
    }

    if (gameState == STATE_MENU && key == 13)
    {
        gameState = STATE_PLAYING;
        loadLevel(0);
        for (int i = 0; i < 256; ++i)
        {
            keyHeld[i] = false;
        }
        gState.lastTicks = glutGet(GLUT_ELAPSED_TIME);
        glutPostRedisplay();
        return;
    }

    if (gameState != STATE_PLAYING)
    {
        glutPostRedisplay();
        return;
    }

    if ((key == 'e' || key == 'E') && canInteract && targetedInteractableIndex >= 0 && targetedInteractableIndex < interactableCount)
    {
        Interactable& item = interactables[targetedInteractableIndex];

        if ((item.typeFlags & TYPE_PICKUP) != 0)
        {
            if (item.active)
            {
                if (item.sequenceOrder > 0 && item.sequenceOrder != currentSequenceTarget)
                {
                    gState.stamina = 0.0f;
                    gState.flashlightCutoff = FLASHLIGHT_PANIC_CUTOFF;
                    gState.panicJitterX = randomSignedUnit() * PANIC_JITTER_X_MAX;
                    gState.panicJitterY = randomSignedUnit() * PANIC_JITTER_Y_MAX;
                    adminChasing = true;
                    adminForcedChase = true;
                    adminHasWaypoint = true;
                    adminWaypointX = gState.camX;
                    adminWaypointZ = gState.camZ;
                    adminPath.clear();
                    adminPathGoalX = -1;
                    adminPathGoalZ = -1;
                    adminPathLevelIndex = -1;
                    std::printf("SEQUENCE ERROR: Fragment order violated. The Admin heard you.\n");
                }
                else
                {
                    item.active = false;
                    ++score;
                    if (item.sequenceOrder > 0)
                    {
                        ++currentSequenceTarget;
                    }
                    std::printf("Pickup recovered: %d/%d fragments.\n", score, getCurrentLevel().requiredScore);
                }
            }
        }
        else if ((item.typeFlags & TYPE_READABLE) != 0)
        {
            readableVisited = true;
            std::printf("LOG: Core routing rerouted through dual lock circuit.\n");
        }

        updateDoorLockState();
        updateInteractionTarget();

        glutPostRedisplay();
        return;
    }

    switch (key)
    {
    case 'w':
        keyHeld[static_cast<unsigned char>('w')] = true;
        keyHeld[static_cast<unsigned char>('W')] = false;
        break;
    case 'W':
        keyHeld[static_cast<unsigned char>('w')] = true;
        keyHeld[static_cast<unsigned char>('W')] = true;
        break;
    case 'a':
    case 'A':
        keyHeld[static_cast<unsigned char>('a')] = true;
        break;
    case 's':
    case 'S':
        keyHeld[static_cast<unsigned char>('s')] = true;
        break;
    case 'd':
    case 'D':
        keyHeld[static_cast<unsigned char>('d')] = true;
        break;
    case 'q':
    case 'Q':
        keyHeld[static_cast<unsigned char>('q')] = true;
        break;
    case 'r':
    case 'R':
        keyHeld[static_cast<unsigned char>('r')] = true;
        break;
    default:
        break;
    }
}

void keyboardUp(unsigned char key, int, int)
{
    switch (key)
    {
    case 'w':
    case 'W':
        keyHeld[static_cast<unsigned char>('w')] = false;
        keyHeld[static_cast<unsigned char>('W')] = false;
        break;
    case 'a':
    case 'A':
        keyHeld[static_cast<unsigned char>('a')] = false;
        break;
    case 's':
    case 'S':
        keyHeld[static_cast<unsigned char>('s')] = false;
        break;
    case 'd':
    case 'D':
        keyHeld[static_cast<unsigned char>('d')] = false;
        break;
    case 'q':
    case 'Q':
        keyHeld[static_cast<unsigned char>('q')] = false;
        break;
    case 'r':
    case 'R':
        keyHeld[static_cast<unsigned char>('r')] = false;
        break;
    default:
        break;
    }
}

void specialKeys(int key, int, int)
{
    if (gameState != STATE_PLAYING)
    {
        return;
    }

    switch (key)
    {
    case GLUT_KEY_UP:
        if (canMoveTo(gState.camX, gState.camZ - gState.moveStep))
        {
            gState.camZ -= gState.moveStep;
        }
        break;
    case GLUT_KEY_DOWN:
        if (canMoveTo(gState.camX, gState.camZ + gState.moveStep))
        {
            gState.camZ += gState.moveStep;
        }
        break;
    case GLUT_KEY_LEFT:
        if (canMoveTo(gState.camX - gState.moveStep, gState.camZ))
        {
            gState.camX -= gState.moveStep;
        }
        break;
    case GLUT_KEY_RIGHT:
        if (canMoveTo(gState.camX + gState.moveStep, gState.camZ))
        {
            gState.camX += gState.moveStep;
        }
        break;
    default:
        break;
    }

    clampCameraToRoom();
    updateInteractionTarget();
    glutPostRedisplay();
}

void initialize()
{
    gameState = STATE_MENU;
    loadLevel(0);
    for (int i = 0; i < 256; ++i)
    {
        keyHeld[i] = false;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);
    glEnable(GL_LIGHT2);
    glEnable(GL_LIGHT3);
    glEnable(GL_LIGHT4);
    glEnable(GL_LIGHT5);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    glEnable(GL_NORMALIZE);
    glShadeModel(GL_SMOOTH);

    GLfloat globalAmbient[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmbient);

    GLfloat materialSpec[] = { 0.24f, 0.22f, 0.26f, 1.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, materialSpec);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 22.0f);

    glEnable(GL_FOG);
    glFogi(GL_FOG_MODE, GL_EXP2);
    glFogf(GL_FOG_DENSITY, getCurrentLevel().fogDensity);
    glHint(GL_FOG_HINT, GL_NICEST);

    initializeMeshAssets();

    glDisable(GL_CULL_FACE);
    gState.lastTicks = glutGet(GLUT_ELAPSED_TIME);
}

int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(1280, 720);
    glutCreateWindow("The Glitched Server");

    initialize();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutIdleFunc(updateScene);
    glutKeyboardFunc(keyboard);
    glutKeyboardUpFunc(keyboardUp);
    glutSpecialFunc(specialKeys);

    glutMainLoop();
    return 0;
}
