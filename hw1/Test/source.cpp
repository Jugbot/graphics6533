#include <stdio.h>
#include <math.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <tuple>

#ifdef __APPLE__  // include Mac OS X verions of headers
#include <GLUT/glut.h>
#else // non-Mac OS X operating systems
#include <GL/glut.h>
#endif

#define XOFF          50
#define YOFF          50
#define WINDOW_WIDTH  600
#define WINDOW_HEIGHT 600


void display(void);
void myinit(void);
void draw_circle(int, int, int);
void file_in(void);
void keyboard(unsigned char, int, int);
void setView(bool);

std::vector<std::tuple<int, int, int>> positions;
int maxw = WINDOW_WIDTH / 2;
int maxh = WINDOW_HEIGHT / 2;

bool c = false;
bool d = false;
bool e = false;
int circle_input[3] = { 0,0,0 };

/*-----------------
The main function
------------------*/
int main(int argc, char **argv)
{
    glutInit(&argc, argv);

    /* Use both double buffering and Z buffer */
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);

    glutInitWindowPosition(XOFF, YOFF);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutCreateWindow("CS6533/CS4533 Assignment 1");
    glutDisplayFunc(display);
	glutKeyboardFunc(keyboard);

    /* Function call to handle file input here */
    file_in();

    myinit();
    glutMainLoop();

    return 0;
}

/*---------------------------
draw_circle(center x, center y, radius): draw Bresenham circle
-----------------------------*/
void draw_circle(int center_x, int center_y, int radius) {
	glPushMatrix();
	glTranslatef(center_x, center_y, 0);
	glBegin(GL_POINTS);
	int x = 0;
	int y = radius;
	int det = 1 - radius;
	// bottom 1/8 counter-clockwise curve
	while (x <= y) {
		// add a point for each 1/8th portion
		// by reflecting and adding to the center coordinates
		glVertex2i(+x, +y);
		glVertex2i(-y, -x);
		glVertex2i(-y, +x);
		glVertex2i(+x, -y);
		glVertex2i(-x, -y);
		glVertex2i(+y, +x);
		glVertex2i(+y, -x);
		glVertex2i(-x, +y);
		// calculate next y
		if (det >= 0) {
			det += 2 * (x - y) + 5;
			y--;
		}
		else {
			det += 2 * x + 3;
		}
		x++;
	}
	glEnd();
	glPopMatrix();
}

/*----------
file_in(): file input function. Modify here.
------------*/
void file_in(void)
{
	std::ifstream fs;
	fs.open("input_circles.txt");
	if (fs.is_open()) {
		int num;
		fs >> num;
		int x; int y; int r;
		while (fs >> x >> y >> r) {
			int maxx = std::abs(x) + r;
			int maxy = std::abs(y) + r;
			std::cout << maxx << "," << maxy << "\n";
			if (maxx > maxw)
				maxw = maxx;
			if (maxy > maxh)
				maxh = maxy;

			positions.emplace_back(x, y, r);
		}
	}
	else {
		std::cout << "no file read\n";
	}
	// Handle rectangular window sizes
	if (maxw / (float)maxh > WINDOW_WIDTH / (float)WINDOW_HEIGHT)
		maxh = WINDOW_HEIGHT / (float)WINDOW_WIDTH * maxw;
	else
		maxw = WINDOW_WIDTH / (float)WINDOW_HEIGHT * maxh;

	fs.close();
}

/*---------------------------------------------------------------------
keyboard(key, mousex, mousey): This function is called for key press events.
---------------------------------------------------------------------*/
void keyboard(unsigned char key, int x, int y) {
	//Prompt three numbers if letter c is selected
	static bool n_flag = 0;
	static int ind = 0;
	if (c && ind < 3) {
		if ('0' <= key && key <= '9') {
			circle_input[ind] *= 10;
			int num = key - '0';
			if (n_flag)
				circle_input[ind] -= num;
			else
				circle_input[ind] += num;
			std::cout << num;
		} 
		else if (key == '-' && circle_input[ind] == 0 && ind < 2) {
			n_flag = true;
			std::cout << '-';
		}
		else {
			ind++;
			n_flag = false;
			if (ind == 3)
				std::cout << std::endl;
			else
				std::cout << ',';

		}
	}
	//Select problem
	else {
		switch (key) {
		case 'c':
			ind = 0;
			circle_input[0] = circle_input[1] = circle_input[2] = 0;
			c = true;
			d = e = false;
			setView(0);
			std::cout << "Problem letter c \n";
			std::cout << "Please enter a comma separated list of numbers for circle x,y,radius (no spaces) \n";
			break;
		case 'd':
			d = true;
			c = e = false;
			setView(1);
			std::cout << "Problem letter d \n";
			break;
		case 'e':
			e = true;
			d = c = false;
			setView(1);
			std::cout << "Problem letter e \n";
			break;
		default:
			std::cout << "Enter problem letter c, d, or e \n";
			break;
		}
	}
	glutPostRedisplay();
}

/*---------------------------------------------------------------------
display(): This function is called once for _every_ frame. 
---------------------------------------------------------------------*/
void display(void)
{
	static unsigned int frame = 0;
	static const int K = 150;
	static const int FPS = 30;
	static const int FR = 1000 / FPS;

	frame++;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glColor3f(1.0, 0.84, 0);              /* draw in golden yellow */
    glPointSize(1.0);                     /* size of each point */

	if (c) {							  /* manually draw circle */
		draw_circle(circle_input[0], circle_input[1], circle_input[2]);
	}
	if (d) {							  /* draw circles from file */
		for (auto const& cpos : positions) { 
			draw_circle(std::get<0>(cpos), std::get<1>(cpos), std::get<2>(cpos));
		}
	}
	if (e) {							  /* animate circles from file */
		for (auto const& cpos : positions) {  
			draw_circle(std::get<0>(cpos), std::get<1>(cpos),
				(frame % K) / (float)K * std::get<2>(cpos));
		}
	}
    glFlush();                            /* render graphics */

    glutSwapBuffers();                    /* swap buffers */

	Sleep(FR);							  /* wait */
	glutPostRedisplay();				  /* display again */
}


/*---------------------------------------------------------------------
myinit(is_normalized): Set view matrix
normalized: if true creates view that containes all points
---------------------------------------------------------------------*/
void setView(bool normalized) {
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	if (normalized)
		gluOrtho2D(-maxw, maxw, -maxh, maxh);
	else
		gluOrtho2D(0.0, WINDOW_WIDTH, 0.0, WINDOW_HEIGHT);
	glMatrixMode(GL_MODELVIEW);
}

/*---------------------------------------------------------------------
myinit(): Set up attributes and viewing
---------------------------------------------------------------------*/
void myinit()
{
    glClearColor(0.0, 0.0, 0.92, 0.0);    /* blue background*/

    /* set up viewing */
	setView(0);
}
