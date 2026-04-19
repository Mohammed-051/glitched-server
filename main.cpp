#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#ifndef __has_include
#define __has_include(x) 0
#endif

#if __has_include(<GL/glut.h>)
#include <GL/glut.h>
#elif __has_include(<GL/freeglut.h>)
#include <GL/freeglut.h>
#else
#error "No GLUT header found. Install freeglut/glut development headers and libraries."
#endif
#endif
#include <cmath>
#include <iostream>

struct Fragment {
	float x;
	float z;
	bool active;
};

const int STATE_MENU = 1;
const int STATE_PLAYING = 2;
const int STATE_GAME_OVER = 3;
const int STATE_WIN = 4;

const int MAP_SIZE = 20;
const int FRAGMENT_COUNT = 3;

const float PLAYER_SPEED = 0.2f;
const float TURN_SPEED = 0.1f;
const float HALF_PI = 1.5707963f;
const float PICKUP_DISTANCE = 1.45f;
const float KILL_DISTANCE = 1.2f;
const float ROAM_STEP = 0.02f;
const float HUNT_STEP = 0.03f;
const float DOOR_X = 18.0f;
const float DOOR_Z = 18.0f;

int gameState = STATE_MENU;

int map[MAP_SIZE][MAP_SIZE] = {
	{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
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

float playerX = 1.5f;
float playerY = 2.0f;
float playerZ = 14.5f;
float yaw = 0.0f;

float adminX = 10.5f;
float adminZ = 3.5f;
float adminDirX = ROAM_STEP;
float adminDirZ = 0.0f;

int score = 0;

Fragment fragments[FRAGMENT_COUNT] = {
	{ 3.5f, 3.5f, true },
	{ 9.5f, 12.5f, true },
	{ 15.5f, 6.5f, true }
};

int windowWidth = 1280;
int windowHeight = 720;
int lastTicks = 0;
float animTime = 0.0f;
float fragmentSpin = 0.0f;
float fragmentBob[FRAGMENT_COUNT] = { 0.0f, 0.0f, 0.0f };

bool isWall(float x, float z);
void resetGameWorld();
void setupProjection(int width, int height);
int collectedFragments();
bool findNearbyFragment(int& outIndex);
bool isPickupNearby();
const char* fragmentStatusText();

void drawMidpointCircle(float cx, float cy, float cz, int radius, float step);
void drawBresenhamLine(float x0, float z0, float x1, float z1, float y);
void drawDDALine(float x0, float z0, float x1, float z1, float y);

void drawAdmin();
void updateAdminAI();

void renderText(float x, float y, void* font, const char* text);
void renderMenu();
void renderGameOver();
void renderWinScreen();
void renderHUD();

void display();

void keyboard(unsigned char key, int x, int y);
void idle();
void reshape(int width, int height);

void plotCircleOctants(float cx, float cy, float cz, int x, int y, float step) {
	glVertex3f(cx + x * step, cy + y * step, cz);
	glVertex3f(cx - x * step, cy + y * step, cz);
	glVertex3f(cx + x * step, cy - y * step, cz);
	glVertex3f(cx - x * step, cy - y * step, cz);
	glVertex3f(cx + y * step, cy + x * step, cz);
	glVertex3f(cx - y * step, cy + x * step, cz);
	glVertex3f(cx + y * step, cy - x * step, cz);
	glVertex3f(cx - y * step, cy - x * step, cz);
}

void drawMidpointCircle(float cx, float cy, float cz, int radius, float step) {
	int x = 0;
	int y = radius;
	int decision = 1 - radius;

	glBegin(GL_POINTS);
	while (x <= y) {
		plotCircleOctants(cx, cy, cz, x, y, step);
		if (decision < 0) {
			decision += (2 * x) + 3;
		} else {
			decision += (2 * (x - y)) + 5;
			--y;
		}
		++x;
	}
	glEnd();
}

void drawBresenhamLine(float x0, float z0, float x1, float z1, float y) {
	const float scale = 100.0f;
	int ix0 = static_cast<int>(std::round(x0 * scale));
	int iz0 = static_cast<int>(std::round(z0 * scale));
	int ix1 = static_cast<int>(std::round(x1 * scale));
	int iz1 = static_cast<int>(std::round(z1 * scale));

	int dx = static_cast<int>(std::fabs(static_cast<float>(ix1 - ix0)));
	int dz = static_cast<int>(std::fabs(static_cast<float>(iz1 - iz0)));
	int sx = (ix0 < ix1) ? 1 : -1;
	int sz = (iz0 < iz1) ? 1 : -1;
	int err = dx - dz;

	glBegin(GL_POINTS);
	while (true) {
		glVertex3f(ix0 / scale, y, iz0 / scale);

		if (ix0 == ix1 && iz0 == iz1) {
			break;
		}

		int e2 = err * 2;
		if (e2 > -dz) {
			err -= dz;
			ix0 += sx;
		}
		if (e2 < dx) {
			err += dx;
			iz0 += sz;
		}
	}
	glEnd();
}

void drawDDALine(float x0, float z0, float x1, float z1, float y) {
	float dx = x1 - x0;
	float dz = z1 - z0;
	float maxDelta = std::fabs(dx) > std::fabs(dz) ? std::fabs(dx) : std::fabs(dz);

	int steps = static_cast<int>(maxDelta * 120.0f);
	if (steps < 1) {
		steps = 1;
	}

	float xInc = dx / static_cast<float>(steps);
	float zInc = dz / static_cast<float>(steps);
	float x = x0;
	float z = z0;

	glBegin(GL_POINTS);
	for (int i = 0; i <= steps; ++i) {
		glVertex3f(x, y, z);
		x += xInc;
		z += zInc;
	}
	glEnd();
}

bool isWall(float x, float z) {
	int gx = static_cast<int>(x);
	int gz = static_cast<int>(z);
	if (gx < 0 || gz < 0 || gx >= MAP_SIZE || gz >= MAP_SIZE) {
		return true;
	}
	return map[gx][gz] == 1;
}

void drawAdmin() {
	float t = glutGet(GLUT_ELAPSED_TIME) * 0.001f;
	float pulse = 1.0f + 0.1f * std::sin(t * 5.0f);
	float headSpin = t * 140.0f;

	glPushMatrix();
	glTranslatef(adminX, 0.8f, adminZ);

	glPushMatrix();
	glScalef(1.0f, pulse, 1.0f);
	glColor3f(0.85f, 0.12f, 0.12f);
	glutSolidCube(1.2f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(0.0f, 0.95f, 0.0f);
	glRotatef(headSpin, 0.0f, 1.0f, 0.0f);
	glColor3f(0.95f, 0.22f, 0.22f);
	glutSolidCube(0.62f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(-0.75f, 0.1f, 0.0f);
	glColor3f(0.72f, 0.08f, 0.08f);
	glutSolidCube(0.32f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(0.75f, 0.1f, 0.0f);
	glColor3f(0.72f, 0.08f, 0.08f);
	glutSolidCube(0.32f);
	glPopMatrix();

	glPopMatrix();
}

void updateAdminAI() {
	if (gameState != STATE_PLAYING) {
		return;
	}

	if (score < FRAGMENT_COUNT) {
		float nextX = adminX + adminDirX;
		float nextZ = adminZ + adminDirZ;

		if (isWall(nextX, nextZ)) {
			adminDirX = -adminDirX;
			adminDirZ = -adminDirZ;

			nextX = adminX + adminDirX;
			nextZ = adminZ + adminDirZ;
			if (isWall(nextX, nextZ)) {
				float previousDirX = adminDirX;
				adminDirX = 0.0f;
				adminDirZ = previousDirX >= 0.0f ? ROAM_STEP : -ROAM_STEP;
				nextX = adminX + adminDirX;
				nextZ = adminZ + adminDirZ;
				if (isWall(nextX, nextZ)) {
					adminDirZ = -adminDirZ;
					nextX = adminX;
					nextZ = adminZ + adminDirZ;
				}
			}
		}

		if (!isWall(nextX, nextZ)) {
			adminX = nextX;
			adminZ = nextZ;
		}
	} else {
		float dx = playerX - adminX;
		float dz = playerZ - adminZ;
		float dist = std::sqrt(dx * dx + dz * dz);

		if (dist > 0.0001f) {
			float stepX = (dx / dist) * HUNT_STEP;
			float stepZ = (dz / dist) * HUNT_STEP;
			float nextX = adminX + stepX;
			float nextZ = adminZ + stepZ;

			if (!isWall(nextX, nextZ)) {
				adminX = nextX;
				adminZ = nextZ;
			} else {
				bool moved = false;
				if (!isWall(adminX + stepX, adminZ)) {
					adminX += stepX;
					moved = true;
				}
				if (!isWall(adminX, adminZ + stepZ)) {
					adminZ += stepZ;
					moved = true;
				}
				if (!moved) {
					adminDirX = -adminDirX;
					adminDirZ = -adminDirZ;
				}
			}
		}
	}

	float playerDx = playerX - adminX;
	float playerDz = playerZ - adminZ;
	float killDistance = std::sqrt(playerDx * playerDx + playerDz * playerDz);
	if (killDistance <= KILL_DISTANCE) {
		gameState = STATE_GAME_OVER;
	}
}

void beginOrthoPass(int width, int height) {
	glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_LINE_BIT);
	glDisable(GL_LIGHTING);
	glDisable(GL_FOG);
	glDisable(GL_DEPTH_TEST);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0.0, static_cast<double>(width), 0.0, static_cast<double>(height), -1.0, 1.0);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
}

void endOrthoPass() {
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	glPopAttrib();
	glMatrixMode(GL_MODELVIEW);
}

float textWidth(void* font, const char* text) {
	if (font == 0 || text == 0) {
		return 0.0f;
	}

	int width = 0;
	for (const char* c = text; *c != '\0'; ++c) {
		width += glutBitmapWidth(font, *c);
	}
	return static_cast<float>(width);
}

void drawCenteredText(float y, void* font, const char* text, float r, float g, float b) {
	glColor3f(r, g, b);
	float x = (windowWidth - textWidth(font, text)) * 0.5f;
	renderText(x, y, font, text);
}

int collectedFragments() {
	int collected = 0;
	for (int i = 0; i < FRAGMENT_COUNT; ++i) {
		if (!fragments[i].active) {
			++collected;
		}
	}
	return collected;
}

bool findNearbyFragment(int& outIndex) {
	outIndex = -1;
	float bestDistance = PICKUP_DISTANCE;

	for (int i = 0; i < FRAGMENT_COUNT; ++i) {
		if (!fragments[i].active) {
			continue;
		}

		float dx = playerX - fragments[i].x;
		float dz = playerZ - fragments[i].z;
		float dist = std::sqrt(dx * dx + dz * dz);
		if (dist < bestDistance) {
			bestDistance = dist;
			outIndex = i;
		}
	}

	return outIndex >= 0;
}

bool isPickupNearby() {
	int index = -1;
	return findNearbyFragment(index);
}

const char* fragmentStatusText() {
	int collected = collectedFragments();
	if (collected <= 0) {
		return "FRAGMENTS: 0/3";
	}
	if (collected == 1) {
		return "FRAGMENTS: 1/3";
	}
	if (collected == 2) {
		return "FRAGMENTS: 2/3";
	}
	return "FRAGMENTS: 3/3";
}

void renderText(float x, float y, void* font, const char* text) {
	if (font == 0 || text == 0) {
		return;
	}

	glRasterPos2f(x, y);
	for (const char* c = text; *c != '\0'; ++c) {
		glutBitmapCharacter(font, *c);
	}
}

void renderMenu() {
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	beginOrthoPass(windowWidth, windowHeight);

	drawCenteredText(windowHeight * 0.68f, GLUT_BITMAP_HELVETICA_18, "THE GLITCHED SERVER", 1.0f, 1.0f, 1.0f);
	drawCenteredText(windowHeight * 0.58f, GLUT_BITMAP_HELVETICA_12, "WASD Move  |  Q/R Rotate  |  E Pickup", 0.75f, 0.75f, 0.75f);
	drawCenteredText(windowHeight * 0.50f, GLUT_BITMAP_HELVETICA_12, "Collect 3 fragments, then escape through the server door", 0.75f, 0.75f, 0.75f);
	drawCenteredText(windowHeight * 0.38f, GLUT_BITMAP_HELVETICA_12, "Press [ENTER] to Initialize Connection", 1.0f, 1.0f, 1.0f);

	endOrthoPass();
}

void renderGameOver() {
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	beginOrthoPass(windowWidth, windowHeight);

	drawCenteredText(windowHeight * 0.54f, GLUT_BITMAP_HELVETICA_18, "SYSTEM FAILURE - GAME OVER", 0.95f, 0.18f, 0.18f);
	drawCenteredText(windowHeight * 0.46f, GLUT_BITMAP_HELVETICA_12, "Press [ENTER] for Menu", 1.0f, 1.0f, 1.0f);

	endOrthoPass();
}

void renderWinScreen() {
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	beginOrthoPass(windowWidth, windowHeight);

	drawCenteredText(windowHeight * 0.54f, GLUT_BITMAP_HELVETICA_18, "LEVEL PASSED", 1.0f, 1.0f, 1.0f);
	drawCenteredText(windowHeight * 0.46f, GLUT_BITMAP_HELVETICA_12, "SYSTEM RESTORED", 1.0f, 1.0f, 1.0f);
	drawCenteredText(windowHeight * 0.38f, GLUT_BITMAP_HELVETICA_12, "Press [ENTER] for Menu", 0.82f, 0.82f, 0.82f);

	endOrthoPass();
}

void renderHUD() {
	beginOrthoPass(windowWidth, windowHeight);

	float cx = windowWidth * 0.5f;
	float cy = windowHeight * 0.5f;
	float radius = 14.0f;

	glColor3f(0.82f, 0.92f, 1.0f);
	glBegin(GL_LINE_LOOP);
	for (int i = 0; i < 40; ++i) {
		float t = (6.2831853f * i) / 40.0f;
		glVertex2f(cx + std::cos(t) * radius, cy + std::sin(t) * radius);
	}
	glEnd();

	glBegin(GL_LINES);
	for (int i = 0; i < 4; ++i) {
		float t = (6.2831853f * i) / 4.0f;
		float x0 = cx + std::cos(t) * (radius + 3.0f);
		float y0 = cy + std::sin(t) * (radius + 3.0f);
		float x1 = cx + std::cos(t) * (radius + 11.0f);
		float y1 = cy + std::sin(t) * (radius + 11.0f);
		glVertex2f(x0, y0);
		glVertex2f(x1, y1);
	}
	glEnd();

	float radarCx = windowWidth - 120.0f;
	float radarCy = windowHeight - 110.0f;
	int radarRadius = 52;

	glPointSize(2.0f);
	glColor3f(0.34f, 0.92f, 1.0f);
	drawMidpointCircle(radarCx, radarCy, 0.0f, radarRadius, 1.0f);

	float t = glutGet(GLUT_ELAPSED_TIME) * 0.003f;
	float lineX = radarCx + std::cos(t) * (radarRadius - 4.0f);
	float lineY = radarCy + std::sin(t) * (radarRadius - 4.0f);

	glPushMatrix();
	glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
	glColor3f(0.78f, 0.98f, 1.0f);
	drawBresenhamLine(radarCx, radarCy, lineX, lineY, 0.0f);
	glPopMatrix();

	float meterStartX = 40.0f;
	float meterY = 42.0f;
	float meterEndX = 240.0f;
	float progress = static_cast<float>(collectedFragments()) / static_cast<float>(FRAGMENT_COUNT);
	float fillX = meterStartX + (meterEndX - meterStartX) * progress;

	glPushMatrix();
	glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
	glColor3f(0.24f, 0.24f, 0.24f);
	drawDDALine(meterStartX, meterY, meterEndX, meterY, 0.0f);
	glColor3f(0.92f, 0.88f, 0.22f);
	drawDDALine(meterStartX, meterY, fillX, meterY, 0.0f);
	glPopMatrix();

	glColor3f(1.0f, 1.0f, 1.0f);
	renderText(28.0f, windowHeight - 34.0f, GLUT_BITMAP_8_BY_13, fragmentStatusText());

	if (isPickupNearby()) {
		drawCenteredText(34.0f, GLUT_BITMAP_HELVETICA_12, "Press [E] to pickup", 1.0f, 1.0f, 1.0f);
	}

	endOrthoPass();
}

void drawFloorTile(int x, int z) {
	glColor3f(0.08f, 0.08f, 0.09f);
	glBegin(GL_QUADS);
	glVertex3f(static_cast<float>(x), 0.0f, static_cast<float>(z));
	glVertex3f(static_cast<float>(x + 1), 0.0f, static_cast<float>(z));
	glVertex3f(static_cast<float>(x + 1), 0.0f, static_cast<float>(z + 1));
	glVertex3f(static_cast<float>(x), 0.0f, static_cast<float>(z + 1));
	glEnd();
}

void drawWallCube(int x, int z) {
	glPushMatrix();
	glTranslatef(static_cast<float>(x) + 0.5f, 0.55f, static_cast<float>(z) + 0.5f);
	glColor3f(0.16f, 0.16f, 0.18f);
	glutSolidCube(1.0f);
	glPopMatrix();
}

void drawVoxelMap() {
	for (int x = 0; x < MAP_SIZE; ++x) {
		for (int z = 0; z < MAP_SIZE; ++z) {
			drawFloorTile(x, z);
			if (map[x][z] == 1) {
				drawWallCube(x, z);
			}
		}
	}
}

void drawEscapeDoor() {
	if (score < FRAGMENT_COUNT) {
		return;
	}

	glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_LINE_BIT);
	glDisable(GL_LIGHTING);
	glLineWidth(2.0f);

	float pulse = 1.0f + 0.12f * std::sin(animTime * 6.0f);
	glColor3f(0.72f, 0.98f, 1.0f);

	glPushMatrix();
	glTranslatef(DOOR_X, 0.95f, DOOR_Z);
	glScalef(pulse, pulse, pulse);
	glutWireCube(1.25f);
	glPopMatrix();

	glPopAttrib();
}

void drawActiveFragments() {
	for (int i = 0; i < FRAGMENT_COUNT; ++i) {
		if (!fragments[i].active) {
			continue;
		}

		glPushMatrix();
		glTranslatef(fragments[i].x, 0.72f + fragmentBob[i], fragments[i].z);
		glRotatef(fragmentSpin + i * 45.0f, 0.0f, 1.0f, 0.0f);
		glColor3f(0.95f, 0.86f, 0.20f);
		glutSolidCube(0.45f);
		glPopMatrix();
	}
}

void display() {
	switch (gameState) {
		case STATE_MENU:
			renderMenu();
			break;

		case STATE_GAME_OVER:
			renderGameOver();
			break;

		case STATE_WIN:
			renderWinScreen();
			break;

		case STATE_PLAYING:
		default:
			glClearColor(0.02f, 0.02f, 0.03f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			{
				float lookX = playerX + std::cos(yaw);
				float lookZ = playerZ + std::sin(yaw);
				gluLookAt(
					playerX, playerY, playerZ,
					lookX, playerY, lookZ,
					0.0f, 1.0f, 0.0f
				);
			}

			drawVoxelMap();
			drawEscapeDoor();
			drawAdmin();
			drawActiveFragments();
			renderHUD();
			break;
	}

	glutSwapBuffers();
}

void resetGameWorld() {
	score = 0;

	playerX = 1.5f;
	playerY = 2.0f;
	playerZ = 14.5f;
	yaw = 0.0f;

	adminX = 10.5f;
	adminZ = 3.5f;
	adminDirX = ROAM_STEP;
	adminDirZ = 0.0f;

	fragments[0] = { 3.5f, 3.5f, true };
	fragments[1] = { 9.5f, 12.5f, true };
	fragments[2] = { 15.5f, 6.5f, true };

	animTime = 0.0f;
	fragmentSpin = 0.0f;
	for (int i = 0; i < FRAGMENT_COUNT; ++i) {
		fragmentBob[i] = 0.0f;
	}
}

void setupProjection(int width, int height) {
	if (height == 0) {
		height = 1;
	}

	windowWidth = width;
	windowHeight = height;

	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(66.0, static_cast<double>(width) / static_cast<double>(height), 0.1, 200.0);
	glMatrixMode(GL_MODELVIEW);
}

void keyboard(unsigned char key, int, int) {
	if (gameState == STATE_MENU) {
		if (key == 13) {
			resetGameWorld();
			gameState = STATE_PLAYING;
			lastTicks = glutGet(GLUT_ELAPSED_TIME);
		}
		glutPostRedisplay();
		return;
	}

	if (gameState == STATE_GAME_OVER || gameState == STATE_WIN) {
		if (key == 13) {
			gameState = STATE_MENU;
		}
		glutPostRedisplay();
		return;
	}

	if (gameState != STATE_PLAYING) {
		glutPostRedisplay();
		return;
	}

	if (key == 'q' || key == 'Q') {
		yaw -= TURN_SPEED;
		glutPostRedisplay();
		return;
	}

	if (key == 'r' || key == 'R') {
		yaw += TURN_SPEED;
		glutPostRedisplay();
		return;
	}

	if (key == 'e' || key == 'E') {
		int fragmentIndex = -1;
		if (findNearbyFragment(fragmentIndex)) {
			fragments[fragmentIndex].active = false;
			++score;
		}
		glutPostRedisplay();
		return;
	}

	float nextX = playerX;
	float nextZ = playerZ;
	bool moving = false;

	if (key == 'w' || key == 'W') {
		nextX = playerX + std::cos(yaw) * PLAYER_SPEED;
		nextZ = playerZ + std::sin(yaw) * PLAYER_SPEED;
		moving = true;
	} else if (key == 's' || key == 'S') {
		nextX = playerX - std::cos(yaw) * PLAYER_SPEED;
		nextZ = playerZ - std::sin(yaw) * PLAYER_SPEED;
		moving = true;
	} else if (key == 'a' || key == 'A') {
		nextX = playerX + std::cos(yaw - HALF_PI) * PLAYER_SPEED;
		nextZ = playerZ + std::sin(yaw - HALF_PI) * PLAYER_SPEED;
		moving = true;
	} else if (key == 'd' || key == 'D') {
		nextX = playerX + std::cos(yaw + HALF_PI) * PLAYER_SPEED;
		nextZ = playerZ + std::sin(yaw + HALF_PI) * PLAYER_SPEED;
		moving = true;
	}

	if (moving && !isWall(nextX, nextZ)) {
		playerX = nextX;
		playerZ = nextZ;
	}

	glutPostRedisplay();
}

void idle() {
	if (gameState == STATE_PLAYING) {
		int now = glutGet(GLUT_ELAPSED_TIME);
		float dt = (now - lastTicks) * 0.001f;
		if (dt < 0.0f) {
			dt = 0.0f;
		}
		if (dt > 0.1f) {
			dt = 0.1f;
		}
		lastTicks = now;

		animTime += dt;
		fragmentSpin += 120.0f * dt;
		if (fragmentSpin >= 360.0f) {
			fragmentSpin -= 360.0f;
		}

		for (int i = 0; i < FRAGMENT_COUNT; ++i) {
			fragmentBob[i] = 0.18f * std::sin(animTime * 3.4f + static_cast<float>(i));
		}

		updateAdminAI();

		if (score >= FRAGMENT_COUNT) {
			float dx = playerX - DOOR_X;
			float dz = playerZ - DOOR_Z;
			float doorDistance = std::sqrt(dx * dx + dz * dz);
			if (doorDistance < 1.0f) {
				gameState = STATE_WIN;
			}
		}
	}

	glutPostRedisplay();
}

void reshape(int width, int height) {
	setupProjection(width, height);
}

int main(int argc, char** argv) {
	std::cout << "Launching The Glitched Server..." << std::endl;

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize(windowWidth, windowHeight);
	glutCreateWindow("The Glitched Server");

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_LIGHTING);
	glDisable(GL_FOG);

	resetGameWorld();
	gameState = STATE_MENU;
	lastTicks = glutGet(GLUT_ELAPSED_TIME);
	setupProjection(windowWidth, windowHeight);

	glutDisplayFunc(display);
	glutKeyboardFunc(keyboard);
	glutIdleFunc(idle);
	glutReshapeFunc(reshape);

	glutMainLoop();
	return 0;
}
