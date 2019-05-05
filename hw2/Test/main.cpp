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

typedef Angel::vec4  color4;
typedef Angel::vec3  point3;

// Utility type for passing VBOs to render
struct ObjBuffer {
	GLuint id;
	int size;
	vec4 ambient, diffuse, specular;
	float shininess;
};

GLuint Angel::InitShader(const char* vShaderFile, const char* fShaderFile);

GLuint program;       /* shader program object id */

ObjBuffer floor_buf;  /* vertex buffer object id for floor */
ObjBuffer sphere;
ObjBuffer sphere_shadow;
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
int lightingFlag = 1;
int shadingFlag = 1;
int sourceFlag = 0;

enum menuOptions { MENU_QUIT, MENU_VIEW_DEFAULT, 
	MENU_ENABLE_WIREFRAME,
	MENU_SHADOW_ON, MENU_SHADOW_OFF, 
	MENU_LIGHTING_ON, MENU_LIGHTING_OFF,
	MENU_SHADING_SMOOTH, MENU_SHADING_FLAT,
	MENU_SOURCE_POINT, MENU_SOURCE_SPOT
};

void registerObj(vec3* buf_points, vec4* buf_colors, vec3* buf_normals, GLuint& buf_id, int size) {
	glGenBuffers(1, &buf_id);
	glBindBuffer(GL_ARRAY_BUFFER, buf_id);
	glBufferData(GL_ARRAY_BUFFER, size  * (sizeof(vec3) + sizeof(vec4) + sizeof(vec3)), NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, size * sizeof(vec3), buf_points);
	glBufferSubData(GL_ARRAY_BUFFER, size * sizeof(vec3), size * sizeof(vec4), buf_colors);
	glBufferSubData(GL_ARRAY_BUFFER, size * sizeof(vec3) + size * sizeof(vec4), size * sizeof(vec3), buf_normals);
}

ObjBuffer makeAxis() {
	point3 points[6] = {
		{0.0f,0.0f,0.0f}, {1.0f,0.0f,0.0f},
		{0.0f,0.0f,0.0f}, {0.0f,1.0f,0.0f},
		{0.0f,0.0f,0.0f}, {0.0f,0.0f,1.0f}
	};
	color4 colors[6] = {
		{1.0f,0.0f,0.0f,1.0f}, {1.0f,0.0f,0.0f,1.0f},
		{1.0f,0.0f,1.0f,1.0f}, {1.0f,0.0f,1.0f,1.0f},
		{0.0f,0.0f,1.0f,1.0f}, {0.0f,0.0f,1.0f,1.0f},
	};
	GLuint id;
	registerObj(points, colors, points, id, 6);
	ObjBuffer obj = { id, 6 };
	obj.ambient = { 1.0f,0.0f,0.0f };
	return obj;
}

ObjBuffer makePlane(point3 p1, point3 p2, point3 p3, point3 p4) {
	color4 c { 0.0, 1.0, 0.0, 1.0 };
	vec3 n = normalize(cross(p1-p2,p1-p3));
	point3 floor_points[6] = { p1,p2,p3,p3,p4,p1 };
	vec3 floor_normals[6] = { n,n,n,n,n,n };
	color4 floor_colors[6] = { c,c,c,c,c,c };
	GLuint id;
	registerObj(floor_points, floor_colors, floor_normals, id, 6);
	return { id, 6, {0.2, 0.2, 0.2, 1.0}, {0.0, 1.0, 0.0, 1.0}, {0.0, 0.0, 0.0, 1.0} };
}

ObjBuffer read_obj(const char* file, color4 color)
{
	std::ifstream fs;
	fs.open(file);
	ObjBuffer obj;
	if (fs.is_open()) {
		int total;
		fs >> total;
		total *= 3;
		point3 *obj_array = new point3[total];
		vec3 *obj_normals = new vec3[total];
		color4 *obj_colors = new color4[total];
		int verts;
		int i = 0;
		while (fs >> verts) {
			vec3 p1;
			fs >> p1;
			vec3 p2;
			fs >> p2;
			vec3 p3;
			fs >> p3;
			vec3 n = normalize(cross(p2-p1, p3 - p1));
			obj_normals[i] = n;
			obj_colors[i] = color;
			obj_array[i++] = p1;
			obj_normals[i] = n;
			obj_colors[i] = color;
			obj_array[i++] = p2;
			obj_normals[i] = n;
			obj_colors[i] = color;
			obj_array[i++] = p3;
		}
		GLuint buf_id;
		registerObj(obj_array, obj_colors, obj_normals, buf_id, total);
		obj = { buf_id, total };
		delete[] obj_array;
		delete[] obj_normals;
		delete[] obj_colors;
	}
	else {
		std::cout << "no file read\n";
	}
	fs.close();
	obj.ambient = { 0.2, 0.2, 0.2, 1.0 };
	obj.specular = { 1.0, 0.84, 0.0, 1.0 };
	obj.diffuse = { 1.0, 0.84, 0.0, 1.0 };
	obj.shininess = 125.f;
	return obj;
}
//----------------------------------------------------------------------------
// OpenGL initialization
void init()
{
	std::cout << "Enter an object file: \n";
	std::string file;
	std::getline(std::cin, file);
	if (file.size() == 0) file = "sphere.1024";
	sphere = read_obj(file.c_str(), { 1.0, 0.84, 0.0, 1.0});
	sphere_shadow = read_obj(file.c_str(), { 0.25, 0.25, 0.25, 0.65 });

	floor_buf = makePlane(
		{ 5.0f,0.0f,8.0f },
		{ 5.0f,0.0f,-4.0f },
		{ -5.0f,0.0f,-4.0f },
		{ -5.0f,0.0f,8.0f });

	axis = makeAxis();

 // Load shaders and create a shader program (to be used in display())
    program = InitShader("vshader42.glsl", "fshader42.glsl");
    glEnable( GL_DEPTH_TEST );
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor( 0.529f, 0.807f, 0.92f, 0.0);
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
	glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0,
		BUFFER_OFFSET(sizeof(point3) * obj.size));
	GLuint vNormal = glGetAttribLocation(program, "vNormal");
	glEnableVertexAttribArray(vNormal);
	glVertexAttribPointer(vNormal, 3, GL_FLOAT, GL_FALSE, 0,
		BUFFER_OFFSET((sizeof(point3) + sizeof(color4)) * obj.size));
	glUniform4f(glGetUniformLocation(program, "ambient"), obj.ambient.x, obj.ambient.y, obj.ambient.z, obj.ambient.w);
	glUniform4f(glGetUniformLocation(program, "diffuse"), obj.diffuse.x, obj.diffuse.y, obj.diffuse.z, obj.diffuse.w);
	glUniform4f(glGetUniformLocation(program, "specular"), obj.specular.x, obj.specular.y, obj.specular.z, obj.specular.w);
	glUniform1f(glGetUniformLocation(program, "shininess"), obj.shininess);

    /* Draw a sequence of geometric objs (triangles) from the vertex buffer
       (using the attributes specified in each enabled vertex attribute array) */
    glDrawArrays(mode, 0, obj.size);

    /*--- Disable each vertex attribute array being enabled ---*/
    glDisableVertexAttribArray(vPosition);
	glDisableVertexAttribArray(vColor);
	glDisableVertexAttribArray(vNormal);
}
//
//mat4 shadowMatrix2() {
//	vec4 normal(0.f, 1.f, 0.f, 0.f);
//
//	return dot(light_source, normal) * mat4(1.f) - vmult(normal, light_source);
//	/* 
//		p=l-(d+n*l)/(n*(v-l)) (v-l)	
//	*/
//}

mat4 shadowMatrix() {
	return mat4(vec4(light_source[1],-light_source[0],0.f,0.f),
				vec4(0.f),
				vec4(0.f,-light_source[2],light_source[1], 0.f),
				vec4(0.f,-1.f,0.f,light_source[1])
				);
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
	GLuint  camera;  // look-at matrix uniform shader variable location

    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    glUseProgram(program); // Use the shader program

    model_view = glGetUniformLocation(program, "model_view" );
	projection = glGetUniformLocation(program, "projection");
	camera = glGetUniformLocation(program, "camera");
/*---  Set up and pass on Projection matrix to the shader ---*/
    mat4  p = Perspective(fovy, aspect, zNear, zFar);
    glUniformMatrix4fv(projection, 1, GL_TRUE, p); // GL_TRUE: matrix is row-major

/*---  Set up and pass on Model-View matrix to the shader ---*/
    // eye is a global variable of vec4 set to init_eye and updated by keyboard()
	mat4  mv;
	vec4    at(0.0, 0.0, 0.0, 1.0);
    vec4    up(0.0, 1.0, 0.0, 0.0);
	mat4 la = LookAt(eye, at, up);
	glUniformMatrix4fv(camera, 1, GL_TRUE, la); // GL_TRUE: matrix is row-major
	glUniform4f(glGetUniformLocation(program, "point_light"), light_source.x, light_source.y, light_source.z, light_source.w);
	glUniform1i(glGetUniformLocation(program, "f_lighting"), lightingFlag);
	glUniform1i(glGetUniformLocation(program, "f_spotlight"), sourceFlag);


	/*----- Set Up the Model-View matrix for the sphere -----*/
	mv = ballMatrix;
	glUniformMatrix4fv(model_view, 1, GL_TRUE, mv); // GL_TRUE: matrix is row-major
	if (sphereFlag != 1) // Filled sphere
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	else {            // Wireframe sphere
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glUniform1i(glGetUniformLocation(program, "f_lighting"), GL_FALSE);
	}
	glUniform1i(glGetUniformLocation(program, "f_shading"), shadingFlag);
	drawObj(sphere, GL_TRIANGLES);  // draw the sphere
	glUniform1i(glGetUniformLocation(program, "f_lighting"), lightingFlag);
	glUniform1i(glGetUniformLocation(program, "f_shading"), GL_FALSE);


	/*----- Set up the Mode-View matrix for the floor -----*/
	mv = mat4(1.f);

    glUniformMatrix4fv(model_view, 1, GL_TRUE, mv); // GL_TRUE: matrix is row-major
    if (floorFlag == 1) // Filled floor
       glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    else              // Wireframe floor
       glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glDepthMask(GL_FALSE);
	drawObj(floor_buf, GL_TRIANGLES);  // draw the floor
	if (shadowFlag && eye[1] > 0.f) {
		glEnable(GL_BLEND);
		mv = shadowMatrix() * ballMatrix;
		glUniformMatrix4fv(model_view, 1, GL_TRUE, mv); 
		if (sphereFlag != 1) // Filled sphere
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		else				 // Wireframe sphere
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glUniform1i(glGetUniformLocation(program, "f_lighting"), GL_FALSE);
		drawObj(sphere_shadow, GL_TRIANGLES);
		glUniform1i(glGetUniformLocation(program, "f_lighting"), lightingFlag);
		glDisable(GL_BLEND);
	}
	glDepthMask(GL_TRUE);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	drawObj(floor_buf, GL_TRIANGLES);  // draw the floor
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

	/*----- Set up the Mode-View matrix for the axis -----*/
	mv = Scale(10.0f);
	glUniformMatrix4fv(model_view, 1, GL_TRUE, mv); // GL_TRUE: matrix is row-major
	glUniform1i(glGetUniformLocation(program, "f_lighting"), GL_FALSE);
	drawObj(axis, GL_LINES);
	glUniform1i(glGetUniformLocation(program, "f_lighting"), lightingFlag);

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
	case MENU_LIGHTING_OFF:
		lightingFlag = 0;
		break;
	case MENU_LIGHTING_ON:
		lightingFlag = 1;
		break;
	case MENU_SHADING_SMOOTH:
		shadingFlag = 1;
		break;
	case MENU_SHADING_FLAT:
		shadingFlag = 0;
		break;
	case MENU_SOURCE_SPOT:
		sourceFlag = 1;
		break;
	case MENU_SOURCE_POINT:
		sourceFlag = 0;
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

	int shadowMenuId = glutCreateMenu(menu);
	glutAddMenuEntry("Yes", MENU_SHADOW_ON);
	glutAddMenuEntry("No", MENU_SHADOW_OFF);
	int lightingMenuId = glutCreateMenu(menu);
	glutAddMenuEntry("Yes", MENU_LIGHTING_ON);
	glutAddMenuEntry("No", MENU_LIGHTING_OFF);
	int shadingMenuId = glutCreateMenu(menu);
	glutAddMenuEntry("Smooth", MENU_SHADING_SMOOTH);
	glutAddMenuEntry("Flat", MENU_SHADING_FLAT);
	int sourceMenuId = glutCreateMenu(menu);
	glutAddMenuEntry("Spotlight", MENU_SOURCE_SPOT);
	glutAddMenuEntry("Point", MENU_SOURCE_POINT);
	glutCreateMenu(menu);
	glutAddMenuEntry("Default View Point", MENU_VIEW_DEFAULT);
	glutAddMenuEntry("Quit", MENU_QUIT);
	glutAddMenuEntry("Wireframe", MENU_ENABLE_WIREFRAME);
	glutAddSubMenu("Shading", shadingMenuId);
	glutAddSubMenu("Shadow", shadowMenuId);
	glutAddSubMenu("Lighting", lightingMenuId);
	glutAddSubMenu("Lighting Source", sourceMenuId);
	glutAttachMenu(GLUT_LEFT_BUTTON);


    init();
    glutMainLoop();
    return 0;
}
