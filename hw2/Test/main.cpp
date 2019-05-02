/************************************************************
 * Handout: rotate-sphere-new.cpp (A Sample Code for Shader-Based OpenGL ---
                                 for OpenGL version 3.1 and later)
 * Originally from Ed Angel's textbook "Interactive Computer Graphics" 6th Ed
              sample code "example3.cpp" of Chapter 4.
 * Moodified by Yi-Jen Chiang to include the use of a general rotation function
   Rotate(angle, x, y, z), where the vector (x, y, z) can have length != 1.0,
   and also to include the use of the function NormalMatrix(mv) to return the
   normal matrix (mat3) of a given model-view matrix mv (mat4).

   (The functions Rotate() and NormalMatrix() are added to the file "mat-yjc-new.h"
   by Yi-Jen Chiang, where a new and correct transpose function "transpose1()" and
   other related functions such as inverse(m) for the inverse of 3x3 matrix m are
   also added; see the file "mat-yjc-new.h".)

 * Extensively modified by Yi-Jen Chiang for the program structure and user
   interactions. See the function keyboard() for the keyboard actions.
   Also extensively re-structured by Yi-Jen Chiang to create and use the new
   function drawObj() so that it is easier to draw multiple objects. Now a floor
   and a rotating sphere are drawn.

** Perspective view of a color sphere using LookAt() and Perspective()

** Colors are assigned to each vertex and then the rasterizer interpolates
   those colors across the triangles.
**************************************************************/
#include "Angel-yjc.h"
#include <string>

typedef Angel::vec3  color3;
typedef Angel::vec3  point3;

// Utility type for passing VBOs to render
struct ObjBuffer {
	GLuint id;
	int size;
	ObjBuffer(GLuint id = 0, int size = 0) : id(id), size(size) {}
};

GLuint Angel::InitShader(const char* vShaderFile, const char* fShaderFile);

GLuint program;       /* shader program object id */

ObjBuffer floor_buf;  /* vertex buffer object id for floor */
ObjBuffer sphere;
ObjBuffer axis;

mat4 ballMatrix = mat4(1.0f);

// Projection transformation parameters
GLfloat  fovy = 45.0;  // Field-of-view in Y direction angle (in degrees)
GLfloat  aspect;       // Viewport aspect ratio
GLfloat  zNear = 0.05f, zFar = 30.0;

GLfloat angleCounter = 0.0; // rotation angle in degrees
vec4 init_eye(7.0, 3.0, -10.0, 1.0); // initial viewer position
vec4 eye = init_eye;               // current viewer position

//vec4 light_source(0.f, 3.f, 0.f, 0.f);
vec4 light_source(-14.f, 12.f, -3.f, 1.f);
vec4 shadow_color(.25f, .25f, .25f, .65f);

int animationFlag = 1; // 1: animation; 0: non-animation. Toggled by key 'a' or 'A'
int rollFlag = 1;		// 1: animation; 0: non-animation. Toggled by right mouse button down
int sphereFlag = 1;   // 1: solid sphere; 0: wireframe sphere. Toggled by key 'c' or 'C'
int floorFlag = 1;  // 1: solid floor; 0: wireframe floor. Toggled by key 'f' or 'F'
int shadowFlag = 1;

enum menuOptions { MENU_QUIT, MENU_VIEW_DEFAULT, 
	MENU_ENABLE_WIREFRAME,
	MENU_SHADOW_ON, MENU_SHADOW_OFF, 
	MENU_LIGHTING_ON, MENU_LIGHTING_OFF,
	MENU_SHADING_SMOOTH, MENU_SHADING_FLAT
};

void registerObj(vec3* buf_points, vec3* buf_colors, GLuint& buf_id, int size) {
	size *= sizeof(vec3);
	glGenBuffers(1, &buf_id);
	glBindBuffer(GL_ARRAY_BUFFER, buf_id);
	glBufferData(GL_ARRAY_BUFFER, size * 2, NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, size, buf_points);
	glBufferSubData(GL_ARRAY_BUFFER, size, size, buf_colors);
}

ObjBuffer makeAxis() {
	point3 points[6] = {
		{0.0f,0.0f,0.0f}, {1.0f,0.0f,0.0f},
		{0.0f,0.0f,0.0f}, {0.0f,1.0f,0.0f},
		{0.0f,0.0f,0.0f}, {0.0f,0.0f,1.0f}
	};
	color3 colors[6] = {
		{1.0f,0.0f,0.0f}, {1.0f,0.0f,0.0f},
		{1.0f,0.0f,01.0f}, {1.0f,0.0f,1.0f},
		{0.0f,0.0f,1.0f}, {0.0f,0.0f,1.0f},
	};
	GLuint id;
	registerObj(points, colors, id, 6);
	return { id, 6 };
}

ObjBuffer makePlane(point3 a, point3 b, point3 c, point3 d, color3 e) {
	point3 floor_points[6] = { a,b,c,c,d,a };
	color3 floor_colors[6] = { e,e,e,e,e,e };
	GLuint id;
	registerObj(floor_points, floor_colors, id, 6);
	return { id, 6 };
}

ObjBuffer read_obj(const char* file)
{
	std::ifstream fs;
	fs.open(file);
	ObjBuffer obj = { 0, 0 };
	if (fs.is_open()) {
		int total;
		fs >> total;
		total *= 3;
		point3 *obj_array = new point3[total];
		color3 *obj_colors = new color3[total];
		int verts;
		int i = 0;
		while (fs >> verts) {
			//std::cout << verts << std::endl;
			for (int j = 0; j < verts; j++) {
				vec3 p;
				fs >> p;
				//std::cout << p << std::endl;
				obj_array[i] = p;
				//obj_colors[i] = p; // RAINBOW
				obj_colors[i] = { 1.0f, 0.84f, 0.0f };
				i++;
			}
		}
		GLuint buf_id;
		registerObj(obj_array, obj_colors, buf_id, total);
		obj = { buf_id, total };
		delete[] obj_array;
		delete[] obj_colors;
	}
	else {
		std::cout << "no file read\n";
	}
	fs.close();
	return obj;
}
//----------------------------------------------------------------------------
// OpenGL initialization
void init()
{
	std::cout << "Enter an object file: \n";
	std::string file;
	std::getline(std::cin, file);
	if (file.size() == 0) file = "sphere.128";
	sphere = read_obj(file.c_str());

	floor_buf = makePlane(
		{ 5.0f,0.0f,8.0f },
		{ 5.0f,0.0f,-4.0f },
		{ -5.0f,0.0f,-4.0f },
		{ -5.0f,0.0f,8.0f },
		{ 0.0f,1.0f,0.0f });

	axis = makeAxis();

 // Load shaders and create a shader program (to be used in display())
    program = InitShader("vshader42.glsl", "fshader42.glsl");
    glEnable( GL_DEPTH_TEST );
    glClearColor( 0.529f, 0.807f, 0.92f, 0.0);
	glColor4f(shadow_color[0], shadow_color[1], shadow_color[2],shadow_color[3]);
    glLineWidth(2.0);
}
//----------------------------------------------------------------------------
// drawObj(buffer, num_vertices):
//   draw the object that is associated with the vertex buffer object "buffer"
//   and has "num_vertices" vertices.
//
void drawObj(ObjBuffer obj, unsigned int mode)
{
    //--- Activate the vertex buffer object to be drawn ---//
    glBindBuffer(GL_ARRAY_BUFFER, obj.id);

    /*----- Set up vertex attribute arrays for each vertex attribute -----*/
    GLuint vPosition = glGetAttribLocation(program, "vPosition");
    glEnableVertexAttribArray(vPosition);
    glVertexAttribPointer(vPosition, 3, GL_FLOAT, GL_FALSE, 0, 
			  BUFFER_OFFSET(0));

    GLuint vColor = glGetAttribLocation(program, "vColor"); 
    glEnableVertexAttribArray(vColor);
    glVertexAttribPointer(vColor, 3, GL_FLOAT, GL_FALSE, 0, 
			  BUFFER_OFFSET(sizeof(point3) * obj.size) ); 
      // the offset is the (total) size of the previous vertex attribute array(s)

    /* Draw a sequence of geometric objs (triangles) from the vertex buffer
       (using the attributes specified in each enabled vertex attribute array) */
    glDrawArrays(mode, 0, obj.size);

    /*--- Disable each vertex attribute array being enabled ---*/
    glDisableVertexAttribArray(vPosition);
    glDisableVertexAttribArray(vColor);
}
//vec4 * transpose(vec4)
mat4 vmult(vec4 v1, vec4 v2) {
	return mat4(
		v1[0] * v2[0],
		v1[0] * v2[1],
		v1[0] * v2[2],
		v1[0] * v2[3],

		v1[1] * v2[0],
		v1[1] * v2[1],
		v1[1] * v2[2],
		v1[1] * v2[3],

		v1[2] * v2[0],
		v1[2] * v2[1],
		v1[2] * v2[2],
		v1[2] * v2[3],

		v1[3] * v2[0],
		v1[3] * v2[1],
		v1[3] * v2[2],
		v1[3] * v2[3]);
}

mat4 shadowMatrix2() {
	vec4 normal(0.f, 1.f, 0.f, 0.f);

	return dot(light_source, normal) * mat4(1.f) - vmult(normal, light_source);
	//return dot(normal, light_source)*mat4(1.f) -
	/* 
		p=l-(d+n*l)/(n*(v-l)) (v-l)	
	*/
}

mat4 shadowMatrix() {
	return mat4(vec4(light_source[1],-light_source[0],0.f,0.f),
				vec4(0.f),
				vec4(0.f,-light_source[2],light_source[1], 0.f),
				vec4(0.f,-1.f,0.f,light_source[1])
				);
}

void drawShadow(ObjBuffer obj, unsigned int mode)
{

	glBindBuffer(GL_ARRAY_BUFFER, obj.id);
	/*----- Set up vertex attribute arrays for each vertex attribute -----*/
	GLuint vPosition = glGetAttribLocation(program, "vPosition");
	glEnableVertexAttribArray(vPosition);
	glVertexAttribPointer(vPosition, 3, GL_FLOAT, GL_FALSE, 0,
		BUFFER_OFFSET(0));
/*
	GLuint vColor = glGetAttribLocation(program, "vColor");
	glEnableVertexAttribArray(vColor);
	glVertexAttribPointer(vColor, 3, GL_FLOAT, GL_FALSE, 0,
		BUFFER_OFFSET(sizeof(point3) * obj.size));*/
	// the offset is the (total) size of the previous vertex attribute array(s)

  /* Draw a sequence of geometric objs (triangles) from the vertex buffer
	 (using the attributes specified in each enabled vertex attribute array) */
	glDrawArrays(mode, 0, obj.size);

	/*--- Disable each vertex attribute array being enabled ---*/
	glDisableVertexAttribArray(vPosition);
	//glDisableVertexAttribArray(vColor);
}


//Rotate in the direction of delta by angle (radians)
mat4 Roll(const vec3& delta, float angle, float radius) {
	vec3 rotvec = cross(vec3(0, 1, 0),delta);
	return  Rotate(angle*180.f/M_PI, rotvec.x, rotvec.y, rotvec.z);
}
//Handles rolling animation to arbitrary points
mat4 RollAnimation(float angle, float radius = 1.0f) {
	static vec3 keyframes[] = {
		vec3(-4, 1, 4), 
		vec3(-1, 1, -4),
		vec3(3, 1, 5 ), };
	static int size = sizeof(keyframes) / sizeof(vec3);
	static int step = 0;
	static float lastAngle = 0.0f;
	static mat4 savedRot = mat4(1.0f);
	// end static
	float angleDelta = angle - lastAngle;
	vec3 a = keyframes[step%size];
	vec3 b = keyframes[(step+1)%size];
	vec3 distanceDelta = b-a;
	float distance = length(distanceDelta);
	float progress = angleDelta * radius / distance;
	if (progress > 1.0f) {
		savedRot = Roll(distanceDelta, distance/radius, radius) * savedRot;
		lastAngle += distance / radius;
		step += 1;
		return RollAnimation(angle, radius);
	}
	mat4 currentRot = Roll(distanceDelta, angleDelta, radius);
	return Translate(a + (distanceDelta * progress)) * currentRot * savedRot;
}

//----------------------------------------------------------------------------
void display( void )
{
  GLuint  model_view;  // model-view matrix uniform shader variable location
  GLuint  projection;  // projection matrix uniform shader variable location

    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    glUseProgram(program); // Use the shader program

    model_view = glGetUniformLocation(program, "model_view" );
    projection = glGetUniformLocation(program, "projection" );

/*---  Set up and pass on Projection matrix to the shader ---*/
    mat4  p = Perspective(fovy, aspect, zNear, zFar);
    glUniformMatrix4fv(projection, 1, GL_TRUE, p); // GL_TRUE: matrix is row-major

/*---  Set up and pass on Model-View matrix to the shader ---*/
    // eye is a global variable of vec4 set to init_eye and updated by keyboard()
	vec4    at(0.0, 0.0, 0.0, 1.0);
    vec4    up(0.0, 1.0, 0.0, 0.0);

	mat4 la = LookAt(eye, at, up);
	mat4  mv;


/*----- Set Up the Model-View matrix for the sphere -----*/
mv = la * ballMatrix;
glUniformMatrix4fv(model_view, 1, GL_TRUE, mv); // GL_TRUE: matrix is row-major
if (sphereFlag != 1) // Filled sphere
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
else              // Wireframe sphere
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
drawObj(sphere, GL_TRIANGLES);  // draw the sphere


/*----- Set up the Mode-View matrix for the floor -----*/
	mv = la;

    glUniformMatrix4fv(model_view, 1, GL_TRUE, mv); // GL_TRUE: matrix is row-major
    if (floorFlag == 1) // Filled floor
       glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    else              // Wireframe floor
       glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glDepthMask(GL_FALSE);
	drawObj(floor_buf, GL_TRIANGLES);  // draw the floor
	glDepthMask(GL_TRUE);
	if (shadowFlag) {
		mv = la * shadowMatrix() * ballMatrix;
		glUniformMatrix4fv(model_view, 1, GL_TRUE, mv);
		drawShadow(sphere, GL_TRIANGLES);
	}
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	drawObj(floor_buf, GL_TRIANGLES);  // draw the floor
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

/*----- Set up the Mode-View matrix for the axis -----*/
	mv = la * Scale(10.0f);
	glUniformMatrix4fv(model_view, 1, GL_TRUE, mv); // GL_TRUE: matrix is row-major
	drawObj(axis, GL_LINES);

    glutSwapBuffers();
}
//---------------------------------------------------------------------------
void idle (void)
{
	if (rollFlag)
		angleCounter+=0.001f; //roll speed
	ballMatrix = RollAnimation(angleCounter);
    glutPostRedisplay();
}
//----------------------------------------------------------------------------
void mouse(int button, int state, int x, int y) {
	if (button == GLUT_RIGHT_BUTTON) {
		if (state == GLUT_DOWN) {
			rollFlag = 1 - rollFlag; //roll sphere toggle
		}
	}
}
//----------------------------------------------------------------------------
void keyboard(unsigned char key, int x, int y)
{
    switch(key) {
	case 033: // Escape Key
	case 'q': case 'Q':
	    exit( EXIT_SUCCESS );
	    break;

        case 'X': eye[0] += 1.0; break;
	case 'x': eye[0] -= 1.0; break;
        case 'Y': eye[1] += 1.0; break;
	case 'y': eye[1] -= 1.0; break;
        case 'Z': eye[2] += 1.0; break;
	case 'z': eye[2] -= 1.0; break;

        case 'b': case 'B': // Toggle between animation and non-animation
			glutIdleFunc(idle);
            break;
	   
        case 'c': case 'C': // Toggle between filled and wireframe sphere
	    sphereFlag = 1 -  sphereFlag;   
            break;

        case 'f': case 'F': // Toggle between filled and wireframe floor
	    floorFlag = 1 -  floorFlag; 
            break;

	case ' ':  // reset to initial viewer/eye position
	    eye = init_eye;
	    break;
    }
    glutPostRedisplay();
}
//----------------------------------------------------------------------------
void menu(int choice) {
	switch (choice) {
	case MENU_QUIT:
		exit(EXIT_SUCCESS);
	case MENU_VIEW_DEFAULT:
		rollFlag = 1; //roll ball
		eye = init_eye; //reset view
		break;
	case MENU_SHADOW_ON:
		shadowFlag = 1;
		break;
	case MENU_SHADOW_OFF:
		shadowFlag = 0;
		break;
	case MENU_ENABLE_WIREFRAME:
		sphereFlag = 1 - sphereFlag;
		break;
	}
}
//----------------------------------------------------------------------------
void reshape(int width, int height)
{
    glViewport(0, 0, width, height);
    aspect = (GLfloat) width  / (GLfloat) height;
    glutPostRedisplay();
}
//----------------------------------------------------------------------------
int main( int argc, char **argv )
{
    glutInit(&argc, argv);
#ifdef __APPLE__ // Enable core profile of OpenGL 3.2 on macOS.
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH | GLUT_3_2_CORE_PROFILE);
#else
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
#endif
    glutInitWindowSize(512, 512);
    glutCreateWindow("Color Cube");

#ifdef __APPLE__ // on macOS
    // Core profile requires to create a Vertex Array Object (VAO).
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
#else           // on Linux or Windows, we still need glew
    /* Call glewInit() and error checking */
    int err = glewInit();
    if (GLEW_OK != err)
    { 
        printf("Error: glewInit failed: %s\n", (char*) glewGetErrorString(err)); 
        exit(1);
    }
#endif

    // Get info of GPU and supported OpenGL version
    printf("Renderer: %s\n", glGetString(GL_RENDERER));
    printf("OpenGL version supported %s\n", glGetString(GL_VERSION));

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
	ballMatrix = RollAnimation(0.f); //calculate transformations once
    //glutIdleFunc(idle);
    glutKeyboardFunc(keyboard);
	glutMouseFunc(mouse);

	int lightingMenuId = glutCreateMenu(menu);
	glutAddMenuEntry("Yes", MENU_SHADOW_ON);
	glutAddMenuEntry("No", MENU_SHADOW_OFF);
	int shadowMenuId = glutCreateMenu(menu);
	glutAddMenuEntry("Yes", MENU_LIGHTING_ON);
	glutAddMenuEntry("No", MENU_LIGHTING_OFF);
	int shadingMenuId = glutCreateMenu(menu);
	glutAddMenuEntry("Yes", MENU_SHADING_SMOOTH);
	glutAddMenuEntry("No", MENU_SHADING_FLAT);
	glutCreateMenu(menu);
	glutAddMenuEntry("Default View Point", MENU_VIEW_DEFAULT);
	glutAddMenuEntry("Quit", MENU_QUIT);
	glutAddMenuEntry("Wireframe", MENU_ENABLE_WIREFRAME);
	glutAddMenuEntry("Shading", shadingMenuId);
	glutAddSubMenu("Shadow", shadowMenuId);
	glutAddSubMenu("Lighting", lightingMenuId);
	glutAttachMenu(GLUT_LEFT_BUTTON);


    init();
    glutMainLoop();
    return 0;
}
