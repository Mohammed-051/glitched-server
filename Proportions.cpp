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
dowHeight - 34.0f, GLUT_BITMAP_8_BY_13, fragmentStatusText());

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
