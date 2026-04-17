#include <GL/glut.h>

void initializeScene() {
    glClearColor(0.04f, 0.04f, 0.04f, 1.0f);
    glEnable(GL_DEPTH_TEST);
}

void renderFrame() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glutSwapBuffers();
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(960, 640);
    glutInitWindowPosition(120, 80);
    glutCreateWindow("Escape Room - CG Project");

    initializeScene();
    glutDisplayFunc(renderFrame);
    glutMainLoop();

    return 0;
}
