#include <GL/glut.h>
#include <cmath>

#include "globals.h"

namespace {
constexpr int kMapSize = 20;
constexpr int kStatePlaying = 2;
constexpr int kStateGameOver = 3;
constexpr int kFragmentGoal = 3;
constexpr float kKillDistance = 1.2f;
constexpr float kRoamStep = 0.02f;
constexpr float kHuntStep = 0.03f;

bool isWall(float x, float z) {
	const int gx = static_cast<int>(x);
	const int gz = static_cast<int>(z);
	if (gx < 0 || gz < 0 || gx >= kMapSize || gz >= kMapSize) {
		return true;
	}
	return map[gx][gz] == 1;
}

void moveAdminWithCollision(float dx, float dz) {
	const float nextX = adminX + dx;
	const float nextZ = adminZ + dz;

	if (!isWall(nextX, nextZ)) {
		adminX = nextX;
		adminZ = nextZ;
		return;
	}

	bool moved = false;
	if (!isWall(adminX + dx, adminZ)) {
		adminX += dx;
		moved = true;
	}
	if (!isWall(adminX, adminZ + dz)) {
		adminZ += dz;
		moved = true;
	}

	if (!moved) {
		adminDirX = -adminDirX;
		adminDirZ = -adminDirZ;
	}
}
}

void drawAdmin() {
	const float t = glutGet(GLUT_ELAPSED_TIME) * 0.001f;
	const float pulse = 1.0f + 0.1f * std::sin(t * 5.0f);
	const float headSpin = t * 140.0f;

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
	if (gameState != kStatePlaying) {
		return;
	}

	if (score < kFragmentGoal) {
		const float nextAdminX = adminX + adminDirX;
		const float nextAdminZ = adminZ + adminDirZ;

		if (isWall(nextAdminX, nextAdminZ)) {
			if (adminDirX != 0.0f) {
				adminDirX = 0.0f;
				adminDirZ = (adminDirZ >= 0.0f) ? kRoamStep : -kRoamStep;
				if (adminDirZ == 0.0f) {
					adminDirZ = kRoamStep;
				}
			} else {
				adminDirZ = 0.0f;
				adminDirX = (adminDirX >= 0.0f) ? -kRoamStep : kRoamStep;
				if (adminDirX == 0.0f) {
					adminDirX = kRoamStep;
				}
			}
		}

		moveAdminWithCollision(adminDirX, adminDirZ);
	} else {
		const float dx = playerX - adminX;
		const float dz = playerZ - adminZ;
		const float distToPlayer = std::sqrt(std::pow(dx, 2.0f) + std::pow(dz, 2.0f));

		if (distToPlayer > 0.0001f) {
			const float chaseX = (dx / distToPlayer) * kHuntStep;
			const float chaseZ = (dz / distToPlayer) * kHuntStep;
			moveAdminWithCollision(chaseX, chaseZ);
		}
	}

	const float killDistance = std::sqrt(
		std::pow(playerX - adminX, 2.0f) +
		std::pow(playerZ - adminZ, 2.0f)
	);

	if (killDistance <= kKillDistance) {
		gameState = kStateGameOver;
	}
}
