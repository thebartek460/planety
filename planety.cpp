#include <GL/freeglut.h>
#include <cmath>
#include <cstdio>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

//KAMERA 
float camYaw   = -30.0f;
float camPitch = 20.0f;
float camDist  = 40.0f;
float camX = 0, camY = 0, camZ = 0; 
bool cameraOnEarth = false;

//MYSZ 
int lastMouseX = -1;
int lastMouseY = -1;
bool mouseLeftDown = false;
float timeSim = 0.0f;

//TEKSTURY 
GLuint texSun, texMercury, texVenus, texEarth, texMars;
GLuint texJupiter, texSaturn, texUranus, texNeptune, texMoon;
GLuint texSaturnRing;

//STRUKTURA PLANETY 
struct Planet {
    float radius;
    float orbitRadius;
    float orbitSpeed;
    float spinSpeed;
    float tilt;
    float eccentricity;
    GLuint texture;
};

//PLANETY 
Planet mercury = {0.25f, 4.0f, 4.15f, 6.0f, 0.01f, 0.205f, 0};
Planet venus   = {0.6f, 6.0f, 1.62f, -2.0f, 177.0f, 0.006f, 0};
Planet earth   = {0.65f, 8.0f, 1.0f, 5.0f, 23.5f, 0.017f, 0};
Planet mars    = {0.35f, 10.0f, 0.53f, 4.0f, 25.0f, 0.093f, 0};
Planet jupiter = {1.4f, 13.5f, 0.083f, 8.0f, 3.0f, 0.049f, 0};
Planet saturn  = {1.2f, 17.0f, 0.033f, 7.0f, 27.0f, 0.056f, 0};
Planet uranus  = {0.9f, 20.5f, 0.012f, 6.0f, 98.0f, 0.047f, 0};
Planet neptune = {0.85f,24.0f, 0.006f, 5.0f, 30.0f, 0.009f, 0};
Planet moon    = {0.18f,1.5f,5.0f, 0.0f, 6.7f, 0.055f, 0};

//FUNKCJA ŁADUJĄCA TEKSTURY 
GLuint loadTexture(const char* filename) {
    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(filename, &width, &height, &channels, 0);
    if (!data) {
        printf("Nie mozna otworzyc pliku %s\n", filename);
        return 0;
    }

    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;
    gluBuild2DMipmaps(GL_TEXTURE_2D, (channels == 4) ? 4 : 3,
                      width, height, format, GL_UNSIGNED_BYTE, data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    stbi_image_free(data);
    return texID;
}

//RYSOWANIE SFER
void drawTexturedSphere(float radius, GLuint tex) {
    GLUquadric* q = gluNewQuadric();
    gluQuadricTexture(q, GL_TRUE);
    glBindTexture(GL_TEXTURE_2D, tex);
    gluSphere(q, radius, 32, 32);
    gluDeleteQuadric(q);
}

//RYSOWANIE PLANETY 
void drawPlanet(const Planet& p) {
    glPushMatrix();
    float theta = timeSim * p.orbitSpeed * M_PI / 180.0f;
    float a = p.orbitRadius;
    float e = p.eccentricity;
    float r = (a * (1 - e * e)) / (1 + e * cos(theta));
    float x = r * cos(theta);
    float z = r * sin(theta);

    glTranslatef(x, 0.0f, z);
    glRotatef(p.tilt, 0, 0, 1);
    glRotatef(timeSim * p.spinSpeed, 0, 1, 0);

    drawTexturedSphere(p.radius, p.texture);
    glPopMatrix();
}

void drawSaturnRings(float innerRadius, float outerRadius, GLuint tex) {
    glPushMatrix();
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glBegin(GL_TRIANGLE_STRIP);
    for (int i = 0; i <= 360; i += 10) {
        float angle = i * M_PI / 180.0f;
        float x = cos(angle);
        float z = sin(angle);

        glNormal3f(0.0f, 1.0f, 0.0f); 
        glTexCoord2f(0.0f, (float)i / 360.0f); 
        glVertex3f(x * innerRadius, 0.0f, z * innerRadius);

        glTexCoord2f(1.0f, (float)i / 360.0f);
        glVertex3f(x * outerRadius, 0.0f, z * outerRadius);
    }
    glEnd();
    glPopMatrix();
}

//RYSOWANIE ORBIT 
void drawOrbit(const Planet& p) {
    glDisable(GL_LIGHTING);
    glColor3f(0.5f, 0.5f, 0.5f);

    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < 360; i++) {
        float theta = i * M_PI / 180.0f;
        float a = p.orbitRadius;
        float e = p.eccentricity;
        float r = (a * (1 - e*e)) / (1 + e * cos(theta));
        float x = r * cos(theta);
        float z = r * sin(theta);
        glVertex3f(x, 0.0f, z);
    }
    glEnd();
    glEnable(GL_LIGHTING);
}

//INICJALIZACJA PROGRAMU
void init() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_TEXTURE_2D);

    //Tekstury
    texSun     = loadTexture("sun.jpg");
    texMercury = loadTexture("mercury.jpg");
    texVenus   = loadTexture("venus.jpg");
    texEarth   = loadTexture("earth.jpg");
    texMars    = loadTexture("mars.jpg");
    texJupiter = loadTexture("jupiter.jpg");
    texSaturn  = loadTexture("saturn.jpg");
    texUranus  = loadTexture("uranus.jpg");
    texNeptune = loadTexture("neptune.jpg");
    texMoon    = loadTexture("moon.jpg");
    texSaturnRing = loadTexture("saturn_ring.jpg");

    mercury.texture = texMercury;
    venus.texture   = texVenus;
    earth.texture   = texEarth;
    mars.texture    = texMars;
    jupiter.texture = texJupiter;
    saturn.texture  = texSaturn;
    uranus.texture  = texUranus;
    neptune.texture = texNeptune;
    moon.texture    = texMoon;

    GLfloat lightPos[]      = {0, 0, 0, 1};
    GLfloat lightDiffuse[]  = {1, 1, 1, 1};
    GLfloat lightSpecular[] = {1, 1, 1, 1};
    GLfloat noAmbient[]     = {0, 0, 0, 1};

    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  lightDiffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpecular);
    glLightfv(GL_LIGHT0, GL_AMBIENT,  noAmbient);

    glEnable(GL_COLOR_MATERIAL);

    printf("Sterowanie Programem:\n");
    printf("  = / -    : Przyblizanie / oddalanie kamery\n");
    printf("  W / S    : Chod kamery do przodu / do tylu\n");
    printf("  A / D    : Chod kamery w lewo / w prawo\n");
    printf("  Strzalki : Obrot kamery\n");
    printf("  Lewy przycisk myszy + ruch : Obrot kamery myszka\n");
    printf("  C        : Przelaczanie kamery na Ziemi / wolna\n");
    printf("  ESC      : Wyjscie z programu\n");
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    float thetaEarth = timeSim * earth.orbitSpeed * M_PI / 180.0f;
    float rEarth = (earth.orbitRadius * (1 - earth.eccentricity*earth.eccentricity)) / 
                   (1 + earth.eccentricity * cos(thetaEarth));
    float xEarth = rEarth * cos(thetaEarth);
    float zEarth = rEarth * sin(thetaEarth);

    if (cameraOnEarth) {
        float camHeight = earth.radius + 0.2f;
        gluLookAt(
            xEarth, camHeight, zEarth,
            xEarth + sin(timeSim * moon.orbitSpeed * M_PI/180.0f),
            camHeight,
            zEarth - cos(timeSim * moon.orbitSpeed * M_PI/180.0f),
            0,1,0
        );
    } else {
        glTranslatef(0, 0, -camDist);
        glRotatef(camPitch, 1, 0, 0);
        glRotatef(camYaw, 0, 1, 0);
        glTranslatef(-camX, -camY, -camZ);
    }

    GLfloat lightPos[] = {0, 0, 0, 1};
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

    //Słońce
    glPushMatrix();
        glDisable(GL_LIGHTING);
        drawTexturedSphere(2.0f, texSun);
        glEnable(GL_LIGHTING);
    glPopMatrix();

    //OrbitY
    drawOrbit(mercury); drawOrbit(venus); drawOrbit(earth); drawOrbit(mars);
    drawOrbit(jupiter); drawOrbit(saturn); drawOrbit(uranus); drawOrbit(neptune);
    //Planety
    drawPlanet(mercury); drawPlanet(venus); drawPlanet(earth); drawPlanet(mars);
    drawPlanet(jupiter);
    //Saturn z pierscieniami
    glPushMatrix();
        float thetaSat = timeSim * saturn.orbitSpeed * M_PI / 180.0f;
        float rSat = (saturn.orbitRadius * (1 - saturn.eccentricity*saturn.eccentricity)) / 
                     (1 + saturn.eccentricity * cos(thetaSat));
        glTranslatef(rSat * cos(thetaSat), 0.0f, rSat * sin(thetaSat));
        glRotatef(saturn.tilt, 0, 0, 1); 
        glPushMatrix();
            glRotatef(timeSim * saturn.spinSpeed, 0, 1, 0);
            drawTexturedSphere(saturn.radius, saturn.texture);
        glPopMatrix();

        glDisable(GL_CULL_FACE); 
        drawSaturnRings(saturn.radius + 0.2f, saturn.radius + 1.2f, texSaturnRing);
        glEnable(GL_CULL_FACE);
    glPopMatrix();

    drawPlanet(uranus); drawPlanet(neptune);
//KSIĘŻYC
    glPushMatrix();
        glTranslatef(xEarth, 0, zEarth);
        float thetaMoon = timeSim * moon.orbitSpeed * M_PI / 180.0f;
        float rMoon = moon.orbitRadius;
        float xMoon = rMoon * cos(thetaMoon);
        float zMoon = rMoon * sin(thetaMoon);
        glTranslatef(xMoon, 0, zMoon);
        glRotatef(timeSim * moon.spinSpeed, 0, 1, 0);
        drawTexturedSphere(moon.radius, moon.texture);
    glPopMatrix();

    glutSwapBuffers();
}

//TIMER 
void timer(int) {
    timeSim += 0.03f;
    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);
}

//STEROWANIE 
void specialKeys(int key,int,int){
    if(key==GLUT_KEY_LEFT) camYaw-=5;
    if(key==GLUT_KEY_RIGHT) camYaw+=5;
    if(key==GLUT_KEY_UP) camPitch-=5;
    if(key==GLUT_KEY_DOWN) camPitch+=5;
}

void keyboard(unsigned char key,int,int){
    if(key==27) exit(0);
    if(key=='=') camDist-=1.0f;
    if(key=='-') camDist+=1.0f;
    if(key=='w') camZ+=1.0f;
    if(key=='s') camZ-=1.0f;
    if(key=='a') camX-=1.0f;
    if(key=='d') camX+=1.0f;
    if(key=='c') cameraOnEarth = !cameraOnEarth;
}

//MYSZ 
void mouseMotion(int x,int y){
    if(mouseLeftDown){
        if(lastMouseX>=0 && lastMouseY>=0){
            camYaw+=(x-lastMouseX)*0.3f;
            camPitch+=(y-lastMouseY)*0.3f;
        }
        lastMouseX=x; lastMouseY=y;
    }
}

void mouseButton(int button,int state,int x,int y){
    if(button==GLUT_LEFT_BUTTON){
        mouseLeftDown=(state==GLUT_DOWN);
        lastMouseX=x; lastMouseY=y;
    }
}

void reshape(int w,int h){
    if(h==0) h=1;
    glViewport(0,0,w,h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45,(float)w/h,0.1,200);
    glMatrixMode(GL_MODELVIEW);
}

//MAIN 
int main(int argc,char** argv){
    glutInit(&argc,argv);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB|GLUT_DEPTH);
    glutInitWindowSize(900,700);
    glutCreateWindow("Uklad Sloneczny - z Ksiezycem i Sterowaniem");

    init();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutSpecialFunc(specialKeys);
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouseButton);
    glutMotionFunc(mouseMotion);
    glutTimerFunc(0,timer,0);

    glutMainLoop();
    return 0;
}
