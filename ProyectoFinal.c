#include <GL/glew.h>
#include <GL/freeglut.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "Utils.h"
#include "Transforms.h"
#include "Vec3.h"
#include "Vec4.h"
#include "Sphere.h"

#define toRadians(deg) deg * M_PI / 180.0
#define max(a, b) (a > b? a : b)
#define min(a, b) (a < b? a : b)
#define MAX_SATELITES 100
#define ROOM_SIZE 10000
#define TEXTURES_N 32

typedef enum { IDLE, LEFT, RIGHT, UP, DOWN, FRONT, BACK } MOTION_TYPE;

typedef float vec2[2];
typedef float vec3[3];

static Mat4   modelMatrix, projectionMatrix, viewMatrix;
static GLuint programId1, vertexPositionLoc, vertexNormalLoc, modelMatrixLoc,  projectionMatrixLoc,  viewMatrixLoc;
static GLuint  vertexPositionLoc2, modelColorLoc2,  modelMatrixLoc2, projectionMatrixLoc2, viewMatrixLoc2;
static GLuint ambientLightLoc, materialALoc, materialDLoc;
static GLuint materialSLoc, cameraPositionLoc, vertexTexcoordLoc;
static GLuint windowInfoLoc, windowInfoLoc2;

static bool highlightTexture = true;

GLuint cubeVA, roomVA, rhombusVA, rhombusBuffer[3];
static GLuint lightsBufferId;

static MOTION_TYPE motionType      			= 0;
//static MOTION_TYPE rotationMotionType 	= 0;

static vec3 ambientLight  = {1.0, 1.0, 1.0};
static vec3 materialA     = {0.8, 0.8, 0.8};
static vec3 materialD     = {0.6, 0.6, 0.6};
static vec3 materialS     = {0.6, 0.6, 0.6};
static vec2 windowInfo	  = { 800, 800};

static float rotationSpeed = 0.1;
static float cameraSpeed     = 1;
static vec3 cameraPos = {0.0, 0.0, 0.0};
static float cameraAngleX, cameraAngleY, cameraAngleZ  = 0;
static vec3 pointPos = {0.0, 0.0, 1.0};

static bool firstTime = true;
static int prevMouseX = 0;
static int prevMouseY = 0;

static float objectX     = 0;
static float objectY     = 0;
static float objectZ     = 0;
static float objectSpeed = 0.0;
vec3 vectorDir = {0.6, 0.5, -0.4};

static const int ROOM_WIDTH  = ROOM_SIZE;
static const int ROOM_HEIGHT = ROOM_SIZE;
static const int ROOM_DEPTH  = ROOM_SIZE;

static int currentPlanetIndex = 2;
static int currentSatelites = 0;
static int selected = 0;
static int lastSelected = 0;
static bool gravity = false;

//                          Color    		subcutoff, 	 Position  		Exponent 	Direction  	Cos(cutoff)
static float lights[]   = { 1, 0, 0,  0.8660,   -2, 0, 0,  128,	 -1, 0,  0,   0.5,		// Luz Roja
        0, 1, 0,  0.9659,    0, 0, 0,  128,   0, 0, -1,   0.866, 	// Luz Verde
        0, 0, 1,  0.9238,    2, 0, 0,  128,   1, 0,  0,	  0.7071 	// Luz Verde
};

typedef struct {
	Vec3 position;
	Vec3 color; 	//TO DO: textureID
	Vec3 dimensions;
	Mat4 matrix;
	int fatherSateliteN;
	int thisSatelite;
	double d;
	bool free;
	//bool hasChildren;
	int texture;
} Planet;

Planet satelitesArray[MAX_SATELITES];

Sphere latierra[MAX_SATELITES];

static void checkNewSelection()
{
	printf("Selected: %d, Last Selected: %d\n", selected, lastSelected);
	if(lastSelected != selected)
	{
		highlightTexture = true;
		lastSelected = selected;
	}
}

static void changePlanetIndex() {
    currentPlanetIndex++;
    if(currentPlanetIndex == TEXTURES_N-1) currentPlanetIndex = 2;
    satelitesArray[selected].texture = currentPlanetIndex;

//    switch(currentPlanetIndex)
//    {
//		case 0: printf("Sun selected\n"); break;
//		case 1: printf("Mercury selected\n"); break;
//		case 2: printf("Venus selected\n"); break;
//		case 3: printf("Terra selected\n"); break;
//		case 4: printf("Mars selected\n"); break;
//		case 5: printf("Jupiter selected\n"); break;
//		case 6: printf("Saturn selected\n"); break;
//		case 7: printf("Sun selected\n"); break;
//		case 8: printf("Sun selected\n"); break;
//		case 9: printf("Sun selected\n"); break;
//		case 10: printf("Sun selected\n"); break;
//		case 11: printf("Sun selected\n"); break;
//    }
}

static bool intersectRaySatelite(Vec3 ray_origin, Vec3 ray_direction, Planet Planet) {
	// ************************
	printf("running");
	//	Calcular límites (x1, y1, z1), (x2, y2, z2) de la caja
	float x1 = Planet.position.x - Planet.dimensions.x / 2;
	float x2 = Planet.position.x + Planet.dimensions.x / 2;
	float y1 = Planet.position.y - Planet.dimensions.y / 2;
	float y2 = Planet.position.y + Planet.dimensions.y / 2;
	float z1 = Planet.position.z - Planet.dimensions.z / 2;
	float z2 = Planet.position.z + Planet.dimensions.z / 2;

	// Calcular momento de cruce del rayo con cada límite de la caja
	float tx1 = (x1 - ray_origin.x) / ray_direction.x;					// tx = [ 7, 15]
	float tx2 = (x2 - ray_origin.x) / ray_direction.x;					// ty = [10, 14]
	float ty1 = (y1 - ray_origin.y) / ray_direction.y;					// tz = [12, 19]
	float ty2 = (y2 - ray_origin.y) / ray_direction.y;
	float tz1 = (z1 - ray_origin.z) / ray_direction.z;
	float tz2 = (z2 - ray_origin.z) / ray_direction.z;

	// Determinar si el rayo no pasa por la caja en (x,y)
	float maxTx = max(tx1, tx2);
	float maxTy = max(ty1, ty2);
	if(maxTx < 0 || maxTy < 0) return false;
	float minTx = min(tx1, tx2);
	float minTy = min(ty1, ty2);
	if(maxTx < minTy || maxTy < minTx) return false;

	// Determinar si el rayo no pasa por la caja en (x,y,z)
	float maxTz = max(tz1, tz2);
	if(maxTz < 0) return false;
	float minTz = min(tz1, tz2);
	if(maxTz < max(minTx, minTy) || minTz > min(maxTx, maxTy)) return false;

	return true;
}

static void mouseFunc(int button, int state, int mx, int my) {
	checkNewSelection();
	if(state != GLUT_UP) return;
	// ************************
	// Obtener coordenadas de dispositivo normalizado
	float nx =       2.0 * mx / glutGet(GLUT_SCREEN_WIDTH)  - 1;
	float ny = -1 * (2.0 * my / glutGet(GLUT_SCREEN_HEIGHT) - 1);
//	printf("Normalizado: %.2f, %.2f\n", nx, ny);
	Vec4 rayN = { nx, ny, -1, 1 };

	// (desproyectar) Obtener coordenadas en el sistema de la cámara
	Mat4 invProjectionMatrix;
	inverse(projectionMatrix, &invProjectionMatrix);
	Vec4 rayV;
 	multiply(invProjectionMatrix, rayN, &rayV);
//	printf("Vista: %.2f, %.2f\n", rayV.x, rayV.y);

	//	Obtener coordenadas en el sistema del mundo
	rayV.z = -1;
	rayV.w =  0;
	Mat4 invViewMatrix;
	inverse(viewMatrix, &invViewMatrix);
	Vec4 rayM;
	multiply(invViewMatrix, rayV, &rayM);
	rayM.w = 0;
	vec4_normalize(&rayM);
	//printf("Mundo: %.2f, %.2f, %.2f\n", rayM.x, rayM.y, rayM.z);

	// Obtener la caja más cercana con quien hubo colisión
	Vec3 ray_origin = { cameraPos[0], cameraPos[1], cameraPos[2] };
	Vec3 ray_direction = { rayM.x, rayM.y, rayM.z };
	int min_index = -1;
	float min_distance = 0;
	for(int i = 0; i < currentSatelites; i ++) {
		Planet Planet = satelitesArray[i];
		if(Planet.free) continue;
		if(!intersectRaySatelite(ray_origin, ray_direction, Planet)) continue;
		Vec3 originToSatelite = {
							Planet.position.x - ray_origin.x,
				            Planet.position.y - ray_origin.y,
							Planet.position.z - ray_origin.z};
		float distance = vec3_magnitude(&originToSatelite);
		if(min_index < 0 || distance < min_distance) {
			min_distance = distance;
			min_index = i;
		}
	}

	if(min_index >= 0)
	{
		selected = min_index;
		glutPostRedisplay();
	}
}

static void initTexture(const char* filename, GLuint textureId) {
	unsigned char* data;
	unsigned int width, height;
	glBindTexture(GL_TEXTURE_2D, textureId);
	loadBMP(filename, &data, &width, &height);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

static GLuint textures[TEXTURES_N];
static void initTextures()
{
	glGenTextures(TEXTURES_N, textures);
	initTexture("textures/Pewds.bmp",    textures[0]);
	initTexture("textures/Space.bmp",    textures[1]);
	initTexture("textures/Star.bmp",   textures[2]);
	initTexture("textures/RedStar.bmp",   textures[3]);
	initTexture("textures/BlueGiant.bmp",   textures[4]);
	initTexture("textures/RedDwarf.bmp",   textures[5]);
	initTexture("textures/Mercury.bmp",  textures[6]);
	initTexture("textures/Venus.bmp",    textures[7]);
	initTexture("textures/Earth.bmp",   textures[8]);
	initTexture("textures/Moon.bmp",   textures[9]);
	initTexture("textures/Mars.bmp",   textures[10]);
	initTexture("textures/Jupiter.bmp",   textures[11]);
	initTexture("textures/Saturn.bmp",   textures[12]);
	initTexture("textures/Uranus.bmp",   textures[13]);
	initTexture("textures/Neptune.bmp",   textures[14]);
	initTexture("textures/Fumalonx.bmp",   textures[15]);
	initTexture("textures/Kimchi.bmp",   textures[16]);
	initTexture("textures/Lemus.bmp",   textures[17]);
	initTexture("textures/Mazacuat.bmp",   textures[18]);
	initTexture("textures/Monterrey.bmp",   textures[19]);
	initTexture("textures/NewAruba.bmp",   textures[20]);
	initTexture("textures/Omni.bmp",   textures[21]);
	initTexture("textures/Salusa.bmp",   textures[22]);
	initTexture("textures/Sequoia.bmp",   textures[23]);
	initTexture("textures/Serendip.bmp",   textures[24]);
	initTexture("textures/Shiva.bmp",   textures[25]);
	initTexture("textures/Shrek.bmp",   textures[26]);
	initTexture("textures/Skaoi.bmp",   textures[27]);
	initTexture("textures/Sriracha.bmp",   textures[28]);
	initTexture("textures/Vulkan.bmp",   textures[29]);
	initTexture("textures/XinWuu.bmp",   textures[30]);
}

static void initPlanets()
{
	vec3 c = { 1.0, 0.0, 0.0 };
	for(int i = 0; i < MAX_SATELITES; i++) { latierra[i] = createSphere(5, 50, 50, c); }
}

static void bindPlanets()
{
	for(int i = 0; i < MAX_SATELITES; i++) { sphereBind(latierra[i], vertexPositionLoc, vertexTexcoordLoc, vertexNormalLoc); }
}

static void initShaders() {
	initPlanets();
    windowInfo[0] = glutGet(GLUT_SCREEN_WIDTH);
    windowInfo[1] = glutGet(GLUT_SCREEN_HEIGHT);

	GLuint vShader = compileShader("shaders/phong.vsh", GL_VERTEX_SHADER);
	if(!shaderCompiled(vShader)) return;
	GLuint fShader = compileShader("shaders/phong.fsh", GL_FRAGMENT_SHADER);
	if(!shaderCompiled(fShader)) return;
	programId1 = glCreateProgram();
	glAttachShader(programId1, vShader);
	glAttachShader(programId1, fShader);
	glLinkProgram(programId1);

	vertexPositionLoc   = glGetAttribLocation(programId1, "vertexPosition");
	vertexTexcoordLoc   = glGetAttribLocation(programId1, "vertexTexcoord");
	vertexNormalLoc     = glGetAttribLocation(programId1, "vertexNormal");
	modelMatrixLoc      = glGetUniformLocation(programId1, "modelMatrix");
	viewMatrixLoc       = glGetUniformLocation(programId1, "viewMatrix");
	projectionMatrixLoc = glGetUniformLocation(programId1, "projMatrix");
	ambientLightLoc     = glGetUniformLocation(programId1, "ambientLight");
	materialALoc        = glGetUniformLocation(programId1, "materialA");
	materialDLoc        = glGetUniformLocation(programId1, "materialD");
	materialSLoc        = glGetUniformLocation(programId1, "materialS");
	materialSLoc        = glGetUniformLocation(programId1, "materialS");
	windowInfoLoc		= glGetUniformLocation(programId1, "windowInfo");
	cameraPositionLoc   = glGetUniformLocation(programId1, "cameraPosition");
	bindPlanets();

//	vShader = compileShader("shaders/position_mvp.vsh", GL_VERTEX_SHADER);
//	if(!shaderCompiled(vShader)) return;
//	fShader = compileShader("shaders/modelColor.fsh", GL_FRAGMENT_SHADER);
//	if(!shaderCompiled(fShader)) return;
//	programId2 = glCreateProgram();
//	glAttachShader(programId2, vShader);
//	glAttachShader(programId2, fShader);
//	glLinkProgram(programId2);
//
//	vertexPositionLoc2   = glGetAttribLocation(programId2, "vertexPosition");
//	windowInfoLoc2	 	 = glGetUniformLocation(programId2, "windowInfo");
//	modelMatrixLoc2      = glGetUniformLocation(programId2, "modelMatrix");
//	viewMatrixLoc2       = glGetUniformLocation(programId2, "viewMatrix");
//	projectionMatrixLoc2 = glGetUniformLocation(programId2, "projectionMatrix");
//	modelColorLoc2       = glGetUniformLocation(programId2, "modelColor");
}

static void initLights() {
	glUseProgram(programId1);
	glUniform3fv(ambientLightLoc,  1, ambientLight);

	glUniform3fv(materialALoc,  1, materialA);
	glUniform3fv(materialDLoc,  1, materialD);
	glUniform3fv(materialSLoc,  1, materialS);
	glUniform2fv(windowInfoLoc,	1, windowInfo);

	glGenBuffers(1, &lightsBufferId);
	glBindBuffer(GL_UNIFORM_BUFFER, lightsBufferId);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(lights), lights, GL_DYNAMIC_DRAW);

	GLuint uniformBlockIndex = glGetUniformBlockIndex(programId1, "LightBlock");
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, lightsBufferId);
	glUniformBlockBinding(programId1, uniformBlockIndex, 0);
}

static int getMaxIndex()
{
	for(int i = MAX_SATELITES - 1; i >= 0; i--)
	{
		if(satelitesArray[i].free == false) { return i + 1; }
	}
	return 0;
}

static void initializeSatelites()
{
	for(int i = 0; i < MAX_SATELITES; i++) { satelitesArray[i].free = true; }
}

static void createSatelite()
{
	int freeSpace = 0;
	for(int i = 0; i < sizeof(satelitesArray) / sizeof(Planet); i++)
	{
		if(satelitesArray[i].free == true){ freeSpace = i; break;}
	}
	satelitesArray[freeSpace].matrix = modelMatrix;
	satelitesArray[freeSpace].fatherSateliteN = selected;
	satelitesArray[freeSpace].position.x = cameraPos[0] - satelitesArray[satelitesArray[freeSpace].fatherSateliteN].position.x;
	satelitesArray[freeSpace].position.y = 0.0;
	satelitesArray[freeSpace].position.z = cameraPos[2] - satelitesArray[satelitesArray[freeSpace].fatherSateliteN].position.z;
	satelitesArray[freeSpace].dimensions.x = 1;
	satelitesArray[freeSpace].dimensions.y = 1;
	satelitesArray[freeSpace].dimensions.z = 1;
	satelitesArray[freeSpace].thisSatelite = freeSpace;
	satelitesArray[freeSpace].d = sqrt(pow(cameraPos[0], 2) + pow(cameraPos[1], 2));
	satelitesArray[freeSpace].free = false;
	satelitesArray[freeSpace].texture = 2;

//	if(currentSatelites <= 0) {satelitesArray[selected].hasChildren = true; }
//	else{ satelitesArray[selected].hasChildren = false; }
//	if(selected != 0) {satelitesArray[selected].hasChildren = true; }

	currentSatelites = getMaxIndex();
}

static void killChildren(int father);
static void removeSatelite(int n)
{
	if(n == 0) { return ; }
	killChildren(n);
	satelitesArray[n].free = true;
	if(n == selected) { selected = 0; }
	currentSatelites = getMaxIndex();

	printf("removing satelite");
}

static void killChildren(int father)
{
	for(int i = 0; i < currentSatelites; i++)
	{
		if(i == father) { continue; }
		if(satelitesArray[i].fatherSateliteN == father) { removeSatelite(i); }
	}
}

static void increaseSateliteSize()
{
	satelitesArray[selected].dimensions.x++;
	satelitesArray[selected].dimensions.y++;
	satelitesArray[selected].dimensions.z++;
}

static void decreaseSateliteSize()
{
	if(satelitesArray[selected].dimensions.x > 1)
	{
		satelitesArray[selected].dimensions.x--;
		satelitesArray[selected].dimensions.y--;
		satelitesArray[selected].dimensions.z--;
	}
}

static void cleanseThisMortalWorld()
{
	for(int i = 0; i < currentSatelites; i++)
	{
		if(satelitesArray[satelitesArray[i].fatherSateliteN].free == true) { removeSatelite(i); }
	}
}

static void gravityIsAThingNow() { gravity = true; }

static void initSpace() {
	float w1 = -ROOM_WIDTH  / 2, w2 = ROOM_WIDTH  / 2;
	float h1 = -ROOM_HEIGHT / 2, h2 = ROOM_HEIGHT / 2;
	float d1 = -ROOM_DEPTH  / 2, d2 = ROOM_DEPTH  / 2;

	float positions[] = {w1, h2, d1, w1, h1, d1, w2, h1, d1,   w2, h1, d1, w2, h2, d1, w1, h2, d1,  // Frente
			             w2, h2, d2, w2, h1, d2, w1, h1, d2,   w1, h1, d2, w1, h2, d2, w2, h2, d2,  // Atrás
			             w1, h2, d2, w1, h1, d2, w1, h1, d1,   w1, h1, d1, w1, h2, d1, w1, h2, d2,  // Izquierda
			             w2, h2, d1, w2, h1, d1, w2, h1, d2,   w2, h1, d2, w2, h2, d2, w2, h2, d1,  // Derecha
			             w1, h1, d1, w1, h1, d2, w2, h1, d2,   w2, h1, d2, w2, h1, d1, w1, h1, d1,  // Abajo
						 w1, h2, d2, w1, h2, d1, w2, h2, d1,   w2, h2, d1, w2, h2, d2, w1, h2, d2   // Arriba
	};

	float normals[] = { 0,  0,  1,  0,  0,  1,  0,  0,  1,  0,  0,  1,  0,  0,  1,  0,  0,  1,  // Frente
						0,  0, -1,  0,  0, -1,  0,  0, -1,  0,  0, -1,  0,  0, -1,  0,  0, -1,  // Atrás
					    1,  0,  0,  1,  0,  0,  1,  0,  0,  1,  0,  0,  1,  0,  0,  1,  0,  0,  // Izquierda
					   -1,  0,  0, -1,  0,  0, -1,  0,  0, -1,  0,  0, -1,  0,  0, -1,  0,  0,  // Derecha
					    0,  1,  0,  0,  1,  0,  0,  1,  0,  0,  1,  0,  0,  1,  0,  0,  1,  0,  // Abajo
					    0, -1,  0,  0, -1,  0,  0, -1,  0,  0, -1,  0,  0, -1,  0,  0, -1,  0,  // Arriba
	};

	float wh = (float) ROOM_WIDTH / ROOM_HEIGHT;
	float dh = (float) ROOM_DEPTH / ROOM_HEIGHT;
	float wd = (float) ROOM_WIDTH  / ROOM_DEPTH;


	float texcoords[] = {           0, 2,       0, 0,    2 * wh, 0,   2 * wh, 0,   2 * wh, 2,       0, 2,
					           2 * wh, 2,  2 * wh, 0,         0, 0,        0, 0,        0, 2,  2 * wh, 2,

							        0, 2,       0, 0,    2 * dh, 0,   2 * dh, 0,   2 * dh, 2,       0, 2,
							   2 * dh, 2,  2 * dh, 0,         0, 0,        0, 0,        0, 2,  2 * dh, 2,

							 	    0, 0,    0, 2*dh,   2*dh*wd, 2*dh,  2*dh*wd, 2*dh,  2*dh*wd, 0,  0, 0,
									0, 0,    0, 2*dh,   2*dh*wd, 2*dh,  2*dh*wd, 2*dh,  2*dh*wd, 0,  0, 0
	};

	glUseProgram(programId1);
	glGenVertexArrays(1, &roomVA);
	glBindVertexArray(roomVA);
	GLuint buffers[3];
	glGenBuffers(3, buffers);

	glBindBuffer(GL_ARRAY_BUFFER, buffers[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(positions), positions, GL_STATIC_DRAW);
	glVertexAttribPointer(vertexPositionLoc, 3, GL_FLOAT, 0, 0, 0);
	glEnableVertexAttribArray(vertexPositionLoc);

	glBindBuffer(GL_ARRAY_BUFFER, buffers[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(normals), normals, GL_STATIC_DRAW);
	glVertexAttribPointer(vertexNormalLoc, 3, GL_FLOAT, 0, 0, 0);
	glEnableVertexAttribArray(vertexNormalLoc);

	glBindBuffer(GL_ARRAY_BUFFER, buffers[2]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(texcoords), texcoords, GL_STATIC_DRAW);
	glVertexAttribPointer(vertexTexcoordLoc, 2, GL_FLOAT, 0, 0, 0);
	glEnableVertexAttribArray(vertexTexcoordLoc);
}

static float dot(vec3 u, vec3 v) { return (u[0] * v[0]) + (u[1] * v[1]) + (u[2] * v[2]); }

static void moveForward() {
	float radiansYAW = M_PI * cameraAngleY / 180;
	float radiansPITCH = M_PI * cameraAngleX / 180;

	cameraPos[0] += cameraSpeed * sin(radiansYAW) * cos(radiansPITCH);
	cameraPos[1] -= cameraSpeed * sin(radiansPITCH);
	cameraPos[2] -= cameraSpeed * cos(radiansYAW) * cos(radiansPITCH);

	pointPos[0] += cameraSpeed * sin(radiansYAW) * cos(radiansPITCH);
	pointPos[1] -= cameraSpeed * sin(radiansPITCH);
	pointPos[2] -= cameraSpeed * cos(radiansYAW) * cos(radiansPITCH);
}

static void moveBackward() {
	float radiansYAW = M_PI * cameraAngleY / 180;
	float radiansPITCH = M_PI * cameraAngleX / 180;

	cameraPos[0] -= cameraSpeed * sin(radiansYAW) * cos(radiansPITCH);
	cameraPos[1] += cameraSpeed * sin(radiansPITCH);
	cameraPos[2] += cameraSpeed * cos(radiansYAW) * cos(radiansPITCH);

	pointPos[0] -= cameraSpeed * sin(radiansYAW) * cos(radiansPITCH);
	pointPos[1] += cameraSpeed * sin(radiansPITCH);
	pointPos[2] += cameraSpeed * cos(radiansYAW) * cos(radiansPITCH);
}

static void rotateLeft(float m) { cameraAngleY -= m * rotationSpeed; }
static void rotateRight(float m) { cameraAngleY += m * rotationSpeed; }
static void rotateUp(float m) { cameraAngleX -= m * rotationSpeed; }
static void rotateDown(float m) { cameraAngleX += m * rotationSpeed; }

static bool checkLimits(float w1, float w2, float h1, float h2, float d1, float d2) {
	if(( cameraPos[0] - rotationSpeed > w1 && cameraPos[0] + cameraSpeed < w2)
	  && (cameraPos[1] - cameraSpeed > h1 && cameraPos[1] + cameraSpeed < h2)
	  && (cameraPos[2] - cameraSpeed > d1 && cameraPos[2] + cameraSpeed < d2) ){
		//printf("TRUE POS: %f, %f, %f\n", cameraPos[0], cameraPos[1], cameraPos[2]);
		//printf("w1:%f, w2:%f, h1: %f, h2: %f, d1: %f, d2: %f\n", w1,w2,h1,h2,d1,d2);
		return true;
	}
	else {
		if(cameraPos[0] - cameraSpeed < w1 || cameraPos[1] - cameraSpeed < h1 || cameraPos[2] - cameraSpeed < d1){
			cameraPos[0] += cameraSpeed;
			cameraPos[1] += cameraSpeed;
			cameraPos[2] += cameraSpeed;
		}
		if(cameraPos[0] + cameraSpeed > w1 || cameraPos[1] + cameraSpeed > h1 || cameraPos[2] + cameraSpeed > d1){
			cameraPos[0] -= cameraSpeed;
			cameraPos[1] -= cameraSpeed;
			cameraPos[2] -= cameraSpeed;
		}
		//printf("w1:%f, w2:%f, h1: %f, h2: %f, d1: %f, d2: %f\n", w1,w2,h1,h2,d1,d2);
		//printf("FALSE POS: %f, %f, %f\n", cameraPos[0], cameraPos[1], cameraPos[2]);
		return false;
	}
}

static double clamp(double n, double lower, double upper)
{
	if(n > upper) return upper;
	if(n < lower) return lower;
	return n;
}

static void displayFunc() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	float dist = 0.2;
	float w1 = -ROOM_WIDTH  / 2 + dist, w2 = ROOM_WIDTH  / 2 - dist;
	float h1 = -ROOM_HEIGHT / 2 + dist, h2 = ROOM_HEIGHT / 2 - dist;
	float d1 = -ROOM_DEPTH  / 2 + dist, d2 = ROOM_DEPTH  / 2 - dist;

/***********************************************************************************************************/

	objectX += vectorDir[0]*objectSpeed;
	objectY += vectorDir[1]*objectSpeed;
	objectZ += vectorDir[2]*objectSpeed;

	// LIMITE -> PARED DERECHA
	if(objectX > w2){
		float vectorMagnitude = sqrt(pow(vectorDir[0], 2)+pow(vectorDir[1], 2)+pow(vectorDir[2], 2));
		vec3 vectorL = {vectorDir[0]/vectorMagnitude,
						vectorDir[1]/vectorMagnitude,
						vectorDir[2]/vectorMagnitude};
		vec3 vectorN = { -1.0, 0.0, 0.0 };
		float dotResult = dot(vectorN, vectorL);
		vec3 vecR_Part = {	(2 * vectorN[0]) * (dotResult),
							(2 * vectorN[1]) * (dotResult),
							(2 * vectorN[2]) * (dotResult) };
		vec3 vectorR = { 	vectorL[0] - vecR_Part[0],
							vectorL[1] - vecR_Part[1],
							vectorL[2] - vecR_Part[2] };
		vectorMagnitude = sqrt(pow(vectorR[0], 2)+pow(vectorR[1], 2)+pow(vectorR[2], 2));
		vectorDir[0] = vectorR[0]/vectorMagnitude;
		vectorDir[1] = vectorR[1]/vectorMagnitude;
		vectorDir[2] = vectorR[2]/vectorMagnitude;
	// IZQUIERDA
	} else if(objectX < w1){
		float vectorMagnitude = sqrt(pow(vectorDir[0], 2)+pow(vectorDir[1], 2)+pow(vectorDir[2], 2));
		vec3 vectorL = {vectorDir[0]/vectorMagnitude, vectorDir[1]/vectorMagnitude, vectorDir[2]/vectorMagnitude};
		vec3 vectorN = { 1.0,  0.0,  0.0 };
		float dotResult = dot(vectorN, vectorL);
		vec3 vecR_Part = {	(2 * vectorN[0]) * (dotResult),
							(2 * vectorN[1]) * (dotResult),
							(2 * vectorN[2]) * (dotResult) };
		vec3 vectorR = { 	vectorL[0] - vecR_Part[0],
							vectorL[1] - vecR_Part[1],
							vectorL[2] - vecR_Part[2] };
		vectorMagnitude = sqrt(pow(vectorR[0], 2)+pow(vectorR[1], 2)+pow(vectorR[2], 2));
		vectorDir[0] = vectorR[0]/vectorMagnitude;
		vectorDir[1] = vectorR[1]/vectorMagnitude;
		vectorDir[2] = vectorR[2]/vectorMagnitude;
	}
	//  ABAJO
	if(objectY > h2){
		float vectorMagnitude = sqrt(pow(vectorDir[0], 2)+pow(vectorDir[1], 2)+pow(vectorDir[2], 2));
		vec3 vectorL = {vectorDir[0]/vectorMagnitude, vectorDir[1]/vectorMagnitude, vectorDir[2]/vectorMagnitude};
		vec3 vectorN = { 0.0,  -1.0,  0.0 };
		float dotResult = dot(vectorN, vectorL);
		vec3 vecR_Part = {	(2 * vectorN[0]) * (dotResult),
							(2 * vectorN[1]) * (dotResult),
							(2 * vectorN[2]) * (dotResult) };
		vec3 vectorR = { 	vectorL[0] - vecR_Part[0],
							vectorL[1] - vecR_Part[1],
							vectorL[2] - vecR_Part[2] };
		vectorMagnitude = sqrt(pow(vectorR[0], 2)+pow(vectorR[1], 2)+pow(vectorR[2], 2));
		vectorDir[0] = vectorR[0]/vectorMagnitude;
		vectorDir[1] = vectorR[1]/vectorMagnitude;
		vectorDir[2] = vectorR[2]/vectorMagnitude;
	// ARRIBA
	} else if(objectY < h1){
		float vectorMagnitude = sqrt(pow(vectorDir[0], 2)+pow(vectorDir[1], 2)+pow(vectorDir[2], 2));
		vec3 vectorL = {vectorDir[0]/vectorMagnitude, vectorDir[1]/vectorMagnitude, vectorDir[2]/vectorMagnitude};
		vec3 vectorN = { 0.0,  1.0,  0.0 };
		float dotResult = dot(vectorN, vectorL);
		vec3 vecR_Part = {	(2 * vectorN[0]) * (dotResult),
							(2 * vectorN[1]) * (dotResult),
							(2 * vectorN[2]) * (dotResult) };
		vec3 vectorR = { 	vectorL[0] - vecR_Part[0],
							vectorL[1] - vecR_Part[1],
							vectorL[2] - vecR_Part[2] };
		vectorMagnitude = sqrt(pow(vectorR[0], 2)+pow(vectorR[1], 2)+pow(vectorR[2], 2));
		vectorDir[0] = vectorR[0]/vectorMagnitude;
		vectorDir[1] = vectorR[1]/vectorMagnitude;
		vectorDir[2] = vectorR[2]/vectorMagnitude;
	}
	// ADELANTE
	if(objectZ > d2){
		float vectorMagnitude = sqrt(pow(vectorDir[0], 2)+pow(vectorDir[1], 2)+pow(vectorDir[2], 2));
		vec3 vectorL = {vectorDir[0]/vectorMagnitude, vectorDir[1]/vectorMagnitude, vectorDir[2]/vectorMagnitude};
		vec3 vectorN = { 0.0,  0.0,  1.0 };
		float dotResult = dot(vectorN, vectorL);
		vec3 vecR_Part = {	(2 * vectorN[0]) * (dotResult),
							(2 * vectorN[1]) * (dotResult),
							(2 * vectorN[2]) * (dotResult) };
		vec3 vectorR = { 	vectorL[0] - vecR_Part[0],
							vectorL[1] - vecR_Part[1],
							vectorL[2] - vecR_Part[2] };
		vectorMagnitude = sqrt(pow(vectorR[0], 2)+pow(vectorR[1], 2)+pow(vectorR[2], 2));
		vectorDir[0] = vectorR[0]/vectorMagnitude;
		vectorDir[1] = vectorR[1]/vectorMagnitude;
		vectorDir[2] = vectorR[2]/vectorMagnitude;
	// ATRAS
	} else if(objectZ < d1){
		float vectorMagnitude = sqrt(pow(vectorDir[0], 2)+pow(vectorDir[1], 2)+pow(vectorDir[2], 2));
		vec3 vectorL = {vectorDir[0]/vectorMagnitude, vectorDir[1]/vectorMagnitude, vectorDir[2]/vectorMagnitude};
		vec3 vectorN = { 0.0,  0.0,  -1.0 };
		float dotResult = dot(vectorN, vectorL);
		vec3 vecR_Part = {	(2 * vectorN[0]) * (dotResult),
							(2 * vectorN[1]) * (dotResult),
							(2 * vectorN[2]) * (dotResult) };
		vec3 vectorR = { 	vectorL[0] - vecR_Part[0],
							vectorL[1] - vecR_Part[1],
							vectorL[2] - vecR_Part[2] };
		vectorMagnitude = sqrt(pow(vectorR[0], 2)+pow(vectorR[1], 2)+pow(vectorR[2], 2));
		vectorDir[0] = vectorR[0]/vectorMagnitude;
		vectorDir[1] = vectorR[1]/vectorMagnitude;
		vectorDir[2] = vectorR[2]/vectorMagnitude;
	}

/****************************************************************************************************/
	switch(motionType) {
		case  LEFT  :   break;
		case  RIGHT :   break;
		case  FRONT :  if(checkLimits(w1 , w2, h1, h2, d1, d2)) moveForward(); break;
		case  BACK  :  if(checkLimits(w1, w2, h1, h2, d1, d2)) moveBackward(); break;
		case  UP    :  break;
		case  DOWN  :  break;
		case  IDLE  :  ;
	}


//	Envío de proyección, vista y posición de la cámara al programa 1 (cuarto, rombo)
	glUseProgram(programId1);
	glUniformMatrix4fv(projectionMatrixLoc, 1, true, projectionMatrix.values);

	mIdentity(&viewMatrix);
	rotateAll(&viewMatrix, -cameraAngleX, -cameraAngleY, -cameraAngleZ);
	translate(&viewMatrix, -cameraPos[0], -cameraPos[1], -cameraPos[2]);
	glUniformMatrix4fv(viewMatrixLoc, 1, true, viewMatrix.values);
	glUniform3f(cameraPositionLoc, cameraPos[0], cameraPos[1], cameraPos[2]);

//	Dibujar el espacio
	mIdentity(&modelMatrix);
	glUniformMatrix4fv(modelMatrixLoc, 1, true, modelMatrix.values);
	glBindVertexArray(roomVA);

	glBindTexture(GL_TEXTURE_2D, textures[1]);
	glDrawArrays(GL_TRIANGLES,  0, 24);
	glBindTexture(GL_TEXTURE_2D, textures[1]);
	glDrawArrays(GL_TRIANGLES, 24,  6);
	glBindTexture(GL_TEXTURE_2D, textures[1]);
	glDrawArrays(GL_TRIANGLES, 30,  6);

	//	Satelites
	static float globalAngle = 0;
	static float rangoTraslacion = 500.0;

	cleanseThisMortalWorld();
	for(int i = 0; i < currentSatelites; i++)
	{
		if(gravity == true) { globalAngle -= 0.02; }

		// ORBITS
		if(i == 0) // MAIN STAR
		{
			satelitesArray[i].matrix = modelMatrix;
			rotateY(&satelitesArray[i].matrix, globalAngle / 2.0 * clamp((rangoTraslacion - satelitesArray[i].d) / (rangoTraslacion / 5.0), 1.0, 5.0));
			translate(&satelitesArray[i].matrix, satelitesArray[i].position.x, satelitesArray[i].position.y, satelitesArray[i].position.z);
			pushMatrix(&satelitesArray[i].matrix);
			rotateY(&satelitesArray[i].matrix, globalAngle * 0.25);

			scale(&satelitesArray[i].matrix, satelitesArray[i].dimensions.x, satelitesArray[i].dimensions.y, satelitesArray[i].dimensions.z);
		}
		else
		{
			satelitesArray[i].matrix = satelitesArray[satelitesArray[i].fatherSateliteN].matrix;
			rotateY(&satelitesArray[i].matrix, globalAngle / 2.0 * clamp((rangoTraslacion - satelitesArray[i].d) / (rangoTraslacion / 5.0), 1.0, 5.0)); // Traslacion
			translate(&satelitesArray[i].matrix, satelitesArray[i].position.x, satelitesArray[i].position.y, satelitesArray[i].position.z);
			pushMatrix(&satelitesArray[i].matrix);
			rotateY(&satelitesArray[i].matrix, globalAngle * clamp((satelitesArray[i].dimensions.x), 1.0, 5.0)); // Rotacion

			scale(&satelitesArray[i].matrix, satelitesArray[i].dimensions.x, satelitesArray[i].dimensions.y, satelitesArray[i].dimensions.z);
		}
		glUniformMatrix4fv(modelMatrixLoc, 1, true, satelitesArray[i].matrix.values);
		popMatrix(&satelitesArray[i].matrix);

		// DRAW?
		glBindVertexArray(cubeVA);
		if(satelitesArray[i].free) continue;

		// DECIDE TEXTURE
		if(i == selected && gravity == false && highlightTexture == true) { sphereDraw(latierra[i], textures, 0); }
		else { sphereDraw(latierra[i], textures, satelitesArray[i].texture); }
	}
	glutSwapBuffers();
}

static void reshapeFunc(int w, int h) {
    if(h == 0) h = 1;
    glViewport(0, 0, w, h);
    float zFar = sqrt(pow(ROOM_WIDTH, 2) + pow(ROOM_DEPTH, 2)) + 500;
    float aspect = (float) w / h;
    setPerspective(&projectionMatrix, 80, aspect, -1, -zFar);
}

static void timerFunc(int id) {
	glutTimerFunc(10, timerFunc, id);
	glutPostRedisplay();
}

static void specialKeyReleasedFunc(int key,int x, int y) {
	motionType = IDLE;
}

static void keyReleasedFunc(unsigned char key,int x, int y) {
	motionType = IDLE;
}

static void specialKeyPressedFunc(int key, int x, int y) {
	switch(key) {
		case 100: motionType = LEFT;  break;
		case 102: motionType = RIGHT; break;
		case 101: motionType = FRONT; break;
		case 103: motionType = BACK;
	}
}

static void keyPressedFunc(unsigned char key, int x, int y) {
	switch(key) {
		case 'a':
		case 'A': motionType = LEFT; break;
		case 'd':
		case 'D': motionType = RIGHT; break;
		case 'w':
		case 'W': motionType = FRONT; break;
		case 's':
		case 'S': motionType = BACK; break;
		case 'r':
		case 'R': if(gravity == false) { removeSatelite(selected); } break;
		case 'f':
		case 'F': if(gravity == false) { createSatelite(); } break;
		case 't':
		case 'T': if(gravity == false) { increaseSateliteSize(); } break;
		case 'g':
		case 'G': if(gravity == false) { decreaseSateliteSize(); } break;
		case 'm':
		case 'M': if(gravity == false) { gravityIsAThingNow(); } break;
		case 'u':
		case 'U':
			if(gravity == false)
			{
				changePlanetIndex();
				highlightTexture = false;
			} break;
		case 27 : exit(0);
	}
 }

static void mouseMove(int x, int y)
{
	if (firstTime)
	{
		//is_first_time = false;
		// Check for zooming mode, but do not allow the model
		// jump off the screen when the mouse enters the window from an outside
		// region of screen. Thus, to activate the mouse, you need to click the
		// mouse to zoom the view AND move it a little:

		firstTime = false;
		//printf("first time?\n");
		prevMouseX = x;
		prevMouseY = y;
		return;
	}

	glutWarpPointer(windowInfo[0] / 2, windowInfo[1] / 2);
	float m = sqrt(pow(prevMouseX - x, 2) + pow(prevMouseY - y, 2));

	if(x < windowInfo[0] / 2) { rotateLeft(m); }
	if(x > windowInfo[0] / 2) { rotateRight(m); }
	if(y > windowInfo[1] / 2) { if(cameraAngleX <= 90) { rotateDown(m); } }
	if(y < windowInfo[1] / 2) { if(cameraAngleX >= -90) { rotateUp(m); } }

	//printf("%lf\n", cameraAngleX);
	prevMouseX = x;
	prevMouseY = y;
}
GLint m_viewport[4];

int main(int argc, char **argv)
{
	glutInit(&argc, argv);
	windowInfo[0] = glutGet(GLUT_SCREEN_WIDTH);
	windowInfo[1] = glutGet(GLUT_SCREEN_HEIGHT);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH);

    glutCreateWindow("S0LARI5");
    glutFullScreen();

    glutDisplayFunc(displayFunc);
    glutReshapeFunc(reshapeFunc);
    glutTimerFunc(10, timerFunc, 1);

    glutKeyboardFunc(keyPressedFunc);
    glutKeyboardUpFunc(keyReleasedFunc);
    glutSpecialFunc(specialKeyPressedFunc);
    glutSpecialUpFunc(specialKeyReleasedFunc);
    glutMouseFunc(mouseFunc);
    glewInit();

    glutPassiveMotionFunc(mouseMove);
	glutWarpPointer(windowInfo[0] / 2, windowInfo[1] / 2);

    glEnable(GL_DEPTH_TEST);
    glutSetCursor(GLUT_CURSOR_NONE);
    initializeSatelites();
    createSatelite();
    initTextures();
    initShaders();
    initLights();
    //initCube();
    initSpace();

    glClearColor(0.1, 0.1, 0.1, 1.0);
    glutMainLoop();

	return 0;
}
