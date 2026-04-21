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

// Helper: draw a tapered cylinder segment along Y using a quad strip
// Used for limbs (arms, fingers/claws)
void drawTaperedLimb(float baseR, float topR, float height, int slices) {
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= slices; ++i) {
        const float angle = (2.0f * 3.14159265f * i) / slices;
        const float cx = std::cos(angle);
        const float cz = std::sin(angle);
        glNormal3f(cx, 0.0f, cz);
        glVertex3f(cx * baseR, 0.0f, cz * baseR);
        glVertex3f(cx * topR,  height, cz * topR);
    }
    glEnd();
}

// Draw one claw finger: a bent tapered cone ending in a sharp curved tip
void drawClaw(float angle, float reach) {
    glPushMatrix();
    glRotatef(angle, 0.0f, 1.0f, 0.0f);  // spread fingers
    glRotatef(-30.0f, 1.0f, 0.0f, 0.0f); // slight downward curl

    // Knuckle segment
    glColor3f(0.72f, 0.65f, 0.52f);
    drawTaperedLimb(0.045f, 0.032f, 0.18f, 8);

    glTranslatef(0.0f, 0.18f, 0.0f);
    glRotatef(-40.0f, 1.0f, 0.0f, 0.0f); // curl inward

    // Nail/claw tip — dark, sharp
    glColor3f(0.15f, 0.12f, 0.10f);
    drawTaperedLimb(0.030f, 0.004f, reach, 6);

    glPopMatrix();
}

} // namespace

// ---------------------------------------------------------------------------
// drawAdmin  —  Nun + Granny horror character
//
// Coordinate space (local, before the outer glTranslatef):
//   Y=0   : floor level
//   Y=0.5 : waist
//   Y=1.0 : shoulders / top of robe
//   Y=1.3 : neck
//   Y=1.55: centre of head
//   Y=1.9 : top of veil
//
// The figure is intentionally hunched: the body matrix applies a small
// forward lean so the creature looks like it's lurching toward the player.
// ---------------------------------------------------------------------------
void drawAdmin() {
    const float t       = glutGet(GLUT_ELAPSED_TIME) * 0.001f;
    const float breathe = 1.0f + 0.03f * std::sin(t * 1.8f);   // slow breathing
    const float sway    = 4.0f  * std::sin(t * 1.2f);           // side-sway (degrees)
    const float eyeGlow = 0.55f + 0.45f * std::sin(t * 3.5f);  // pulsing eye glow

    glPushMatrix();
    glTranslatef(adminX, 0.0f, adminZ);
    glRotatef(sway, 0.0f, 0.0f, 1.0f); // gentle sway

    // -----------------------------------------------------------------------
    // ROBE  —  wide black bell shape, slightly hunched forward
    // -----------------------------------------------------------------------
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, 0.08f);  // lean forward
    glRotatef(-8.0f, 1.0f, 0.0f, 0.0f);

    // Lower robe: wide flared skirt
    glColor3f(0.06f, 0.06f, 0.06f);
    glPushMatrix();
    glScalef(1.0f, breathe, 1.0f);
    // Skirt base (wide, flat-bottomed cylinder)
    glPushMatrix();
    glScalef(0.85f, 1.0f, 0.72f);
    glutSolidCone(0.72f, 0.9f, 16, 4);
    glPopMatrix();
    glPopMatrix();

    // Upper robe / torso (narrower)
    glPushMatrix();
    glTranslatef(0.0f, 0.82f, 0.0f);
    glScalef(0.72f * breathe, 1.0f, 0.60f * breathe);
    glutSolidCube(0.52f);
    glPopMatrix();

    // Robe shadow underside (near floor, semi-transparent dark oval)
    glColor3f(0.02f, 0.02f, 0.02f);
    glPushMatrix();
    glTranslatef(0.0f, 0.02f, 0.0f);
    glScalef(1.0f, 0.08f, 0.82f);
    glutSolidSphere(0.72f, 12, 4);
    glPopMatrix();

    glPopMatrix(); // end robe lean

    // -----------------------------------------------------------------------
    // NUN COLLAR  —  stiff white bib around the neck base
    // -----------------------------------------------------------------------
    glPushMatrix();
    glTranslatef(0.0f, 1.08f, 0.04f);
    glColor3f(0.88f, 0.87f, 0.83f);
    glScalef(1.0f, 0.18f, 0.85f);
    glutSolidSphere(0.30f, 12, 6);
    glPopMatrix();

    // -----------------------------------------------------------------------
    // CRUCIFIX on chest  —  dark wood cross
    // -----------------------------------------------------------------------
    glPushMatrix();
    glTranslatef(0.0f, 0.85f, 0.34f);
    glColor3f(0.30f, 0.20f, 0.10f);
    // Vertical beam
    glPushMatrix();
    glScalef(0.05f, 0.26f, 0.05f);
    glutSolidCube(1.0f);
    glPopMatrix();
    // Horizontal beam
    glPushMatrix();
    glScalef(0.18f, 0.05f, 0.05f);
    glutSolidCube(1.0f);
    glPopMatrix();
    glPopMatrix();

    // -----------------------------------------------------------------------
    // NECK  —  thin, wrinkled
    // -----------------------------------------------------------------------
    glPushMatrix();
    glTranslatef(0.0f, 1.12f, 0.0f);
    glColor3f(0.72f, 0.64f, 0.52f);
    glScalef(0.13f, 0.22f, 0.13f);
    glutSolidCylinder(1.0, 1.0, 8, 2);
    glPopMatrix();

    // -----------------------------------------------------------------------
    // HEAD  —  gaunt elongated skull
    // -----------------------------------------------------------------------
    glPushMatrix();
    glTranslatef(0.0f, 1.55f, -0.02f);
    glRotatef(-6.0f, 1.0f, 0.0f, 0.0f); // slightly tilted forward (lurching look)

    // Skull base
    glColor3f(0.71f, 0.62f, 0.50f);
    glPushMatrix();
    glScalef(0.78f, 1.0f, 0.82f);
    glutSolidSphere(0.28f, 14, 10);
    glPopMatrix();

    // Forehead ridge (slightly protruding)
    glColor3f(0.68f, 0.60f, 0.48f);
    glPushMatrix();
    glTranslatef(0.0f, 0.10f, 0.18f);
    glScalef(0.55f, 0.22f, 0.28f);
    glutSolidSphere(0.28f, 10, 6);
    glPopMatrix();

    // Gaunt cheekbones
    glColor3f(0.65f, 0.56f, 0.44f);
    for (int side = -1; side <= 1; side += 2) {
        glPushMatrix();
        glTranslatef(side * 0.20f, -0.04f, 0.14f);
        glScalef(0.35f, 0.22f, 0.40f);
        glutSolidSphere(0.18f, 8, 5);
        glPopMatrix();
    }

    // Sunken EYE SOCKETS
    for (int side = -1; side <= 1; side += 2) {
        glPushMatrix();
        glTranslatef(side * 0.10f, 0.06f, 0.22f);

        // Deep socket (dark cavity)
        glColor3f(0.06f, 0.04f, 0.02f);
        glutSolidSphere(0.075f, 8, 6);

        // Glowing iris — orange-red pulse
        glColor3f(eyeGlow, eyeGlow * 0.22f, 0.0f);
        glTranslatef(0.0f, 0.0f, 0.04f);
        glutSolidSphere(0.040f, 8, 6);

        // Bright pupil centre
        glColor3f(1.0f, 0.85f * eyeGlow, 0.0f);
        glTranslatef(0.0f, 0.0f, 0.025f);
        glutSolidSphere(0.018f, 6, 4);

        glPopMatrix();
    }

    // Hooked NOSE
    glColor3f(0.66f, 0.56f, 0.44f);
    glPushMatrix();
    glTranslatef(0.0f, -0.04f, 0.25f);
    glScalef(0.32f, 0.38f, 0.55f);
    glutSolidSphere(0.18f, 8, 6);
    glPopMatrix();
    // Nose hook tip
    glPushMatrix();
    glTranslatef(0.0f, -0.10f, 0.29f);
    glScalef(0.22f, 0.28f, 0.38f);
    glutSolidSphere(0.14f, 6, 4);
    glPopMatrix();

    // Sunken MOUTH — grimace
    glColor3f(0.28f, 0.15f, 0.12f);
    glPushMatrix();
    glTranslatef(0.0f, -0.14f, 0.22f);
    glScalef(0.65f, 0.18f, 0.30f);
    glutSolidSphere(0.14f, 8, 4);
    glPopMatrix();

    // TEETH — crooked, yellowed
    glColor3f(0.82f, 0.80f, 0.62f);
    for (int i = -1; i <= 1; ++i) {
        glPushMatrix();
        glTranslatef(i * 0.072f, -0.14f, 0.26f);
        glScalef(0.35f, 0.5f, 0.5f);
        glutSolidCube(0.065f);
        glPopMatrix();
    }

    // WRINKLE lines — rendered as thin dark ridges on the forehead and cheeks
    glColor3f(0.55f, 0.46f, 0.36f);
    for (int w = 0; w < 3; ++w) {
        glPushMatrix();
        glTranslatef(0.0f, 0.06f + w * 0.055f, 0.26f);
        glScalef(1.0f - w * 0.15f, 0.06f, 0.12f);
        glutSolidSphere(0.10f, 6, 2);
        glPopMatrix();
    }

    // -----------------------------------------------------------------------
    // VEIL  —  black rectangular fabric draping over the head
    // -----------------------------------------------------------------------
    glColor3f(0.04f, 0.04f, 0.04f);
    // Top of veil (flat cap)
    glPushMatrix();
    glTranslatef(0.0f, 0.32f, -0.04f);
    glScalef(0.88f, 0.14f, 0.80f);
    glutSolidCube(0.38f);
    glPopMatrix();
    // Back drape
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, -0.24f);
    glScalef(0.82f, 0.82f, 0.12f);
    glutSolidCube(0.46f);
    glPopMatrix();
    // Side drapes
    for (int side = -1; side <= 1; side += 2) {
        glPushMatrix();
        glTranslatef(side * 0.24f, -0.10f, 0.0f);
        glScalef(0.10f, 0.75f, 0.62f);
        glutSolidCube(0.38f);
        glPopMatrix();
    }

    // WISPY GREY HAIR strands peeking from veil edges
    glColor3f(0.72f, 0.72f, 0.70f);
    for (int s = -1; s <= 1; s += 2) {
        for (int h = 0; h < 3; ++h) {
            glPushMatrix();
            glTranslatef(s * (0.27f + h * 0.03f), -0.15f + h * 0.08f, 0.08f);
            glScalef(0.06f, 0.22f, 0.06f);
            glutSolidCylinder(1.0, 1.0, 4, 2);
            glPopMatrix();
        }
    }

    glPopMatrix(); // end head

    // -----------------------------------------------------------------------
    // LEFT ARM  —  reaching outward, bent at elbow (threatening pose)
    // -----------------------------------------------------------------------
    glPushMatrix();
    glTranslatef(-0.36f, 1.0f, 0.0f);
    glRotatef(30.0f, 0.0f, 0.0f, 1.0f);  // outward
    glRotatef(20.0f, 0.0f, 1.0f, 0.0f);  // slightly forward

    glColor3f(0.06f, 0.06f, 0.06f);
    // Upper arm
    glPushMatrix();
    glScalef(0.12f, 0.36f, 0.12f);
    glutSolidCylinder(1.0, 1.0, 8, 2);
    glPopMatrix();

    glTranslatef(0.0f, 0.36f, 0.0f);
    glRotatef(-55.0f, 1.0f, 0.0f, 0.0f); // elbow bend down

    // Forearm
    glPushMatrix();
    glScalef(0.10f, 0.32f, 0.10f);
    glutSolidCylinder(1.0, 1.0, 8, 2);
    glPopMatrix();

    // Bony wrist / hand
    glTranslatef(0.0f, 0.32f, 0.0f);
    glColor3f(0.72f, 0.64f, 0.52f);
    glPushMatrix();
    glScalef(0.85f, 0.55f, 0.65f);
    glutSolidSphere(0.11f, 8, 6);
    glPopMatrix();

    // Four clawed fingers
    drawClaw(-30.0f, 0.16f);
    drawClaw(-10.0f, 0.19f);
    drawClaw( 10.0f, 0.19f);
    drawClaw( 30.0f, 0.15f);

    glPopMatrix(); // end left arm

    // -----------------------------------------------------------------------
    // RIGHT ARM  —  tucked lower, holding the cane
    // -----------------------------------------------------------------------
    glPushMatrix();
    glTranslatef(0.36f, 0.92f, 0.0f);
    glRotatef(-18.0f, 0.0f, 0.0f, 1.0f); // slightly inward
    glRotatef(10.0f, 0.0f, 1.0f, 0.0f);

    glColor3f(0.06f, 0.06f, 0.06f);
    // Upper arm
    glPushMatrix();
    glScalef(0.12f, 0.34f, 0.12f);
    glutSolidCylinder(1.0, 1.0, 8, 2);
    glPopMatrix();

    glTranslatef(0.0f, 0.34f, 0.0f);
    glRotatef(-40.0f, 1.0f, 0.0f, 0.0f);

    // Forearm
    glPushMatrix();
    glScalef(0.10f, 0.30f, 0.10f);
    glutSolidCylinder(1.0, 1.0, 8, 2);
    glPopMatrix();

    glTranslatef(0.0f, 0.30f, 0.0f);
    glColor3f(0.72f, 0.64f, 0.52f);
    glPushMatrix();
    glScalef(0.85f, 0.55f, 0.65f);
    glutSolidSphere(0.11f, 8, 6);
    glPopMatrix();

    // Clawed grip (3 fingers wrapped around cane)
    drawClaw(-20.0f, 0.14f);
    drawClaw(  0.0f, 0.16f);
    drawClaw( 20.0f, 0.14f);

    glPopMatrix(); // end right arm

    // -----------------------------------------------------------------------
    // CANE  —  gnarled dark wood walking stick (Granny element)
    // Drawn in world-space, extending downward from right hand position.
    // -----------------------------------------------------------------------
    glPushMatrix();
    glTranslatef(adminX + 0.38f, 0.0f, adminZ + 0.1f); // near right hand
    glColor3f(0.28f, 0.18f, 0.08f);
    // Shaft
    glPushMatrix();
    glScalef(0.06f, 1.0f, 0.06f);
    glutSolidCylinder(1.0, 1.0, 6, 2);
    glPopMatrix();
    // Curved crook at top
    for (int seg = 0; seg < 6; ++seg) {
        glPushMatrix();
        const float a  = seg * 15.0f * 3.14159265f / 180.0f;
        const float r  = 0.10f;
        glTranslatef(r * std::sin(a), 1.0f + r * (1.0f - std::cos(a)), 0.0f);
        glScalef(0.055f, 0.055f, 0.055f);
        glutSolidSphere(1.0f, 5, 4);
        glPopMatrix();
    }
    glPopMatrix();

    glPopMatrix(); // end figure transform
}

// ---------------------------------------------------------------------------
// updateAdminAI — unchanged from original
// ---------------------------------------------------------------------------
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
