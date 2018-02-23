// ==========================================================================
// Barebones OpenGL Core Profile Boilerplate
//    using the GLFW windowing system (http://www.glfw.org)
//
// Loosely based on
//  - Chris Wellons' example (https://github.com/skeeto/opengl-demo) and
//  - Camilla Berglund's example (http://www.glfw.org/docs/latest/quick.html)
//
// Author:  Sonny Chan, University of Calgary
// Co-Authors:
//			Jeremy Hart, University of Calgary
//			John Hall, University of Calgary
// Date:    December 2015
// ==========================================================================


#include <iostream>
#include <fstream>
#include <algorithm>
#include <string>
#include <iterator>
#include <glm-0.9.8.2/glm/glm.hpp>
#include <glm-0.9.8.2/glm/gtc/matrix_transform.hpp>
#include <glm-0.9.8.2/glm/gtc/type_ptr.hpp>


#include <glad/include/glad/glad.h>
#include <glfw/include/GLFW/glfw3.h>
#include <vector>

#include "texture.h"
#include "GlyphExtractor.h"

using namespace std;
using namespace glm;
// --------------------------------------------------------------------------

//global Variables
vector<vec2> vertexPoints;
vector<vec2> vertexLines;
vector<vec3> colours;
vector<vec3> coloursTwo;
vector<vec3> coloursThree;
GlyphExtractor extractor;
bool q1 = true;
bool q1p1= false;
bool q2 = false;
bool q3 = false;
bool borderReached = false;
float globalDegree = 3.0f; //default case is for quadratic bezier
float translation = 0.0f;
float speed = 0.02f;
float sum = 0.0f;
int click = 0;
string font = "Fonts/Lora-Italic.ttf";
// OpenGL utility and support function prototypes

void QueryGLVersion();
bool CheckGLErrors();

string LoadSource(const string &filename);
GLuint CompileShader(GLenum shaderType, const string &source);
GLuint LinkProgram(GLuint vertexShader, GLuint fragmentShader, GLuint tcsShader, GLuint tesShader);
GLuint LinkProgram2(GLuint vertexShader, GLuint fragmentShader);

// --------------------------------------------------------------------------
// Functions to set up OpenGL shader programs for rendering

// load, compile, and link shaders, returning true if successful
GLuint InitializeShaders()
{
	// load shader source from files
	string vertexSource = LoadSource("shaders/vertex.glsl");
	string fragmentSource = LoadSource("shaders/fragment.glsl");
	string tcsSource = LoadSource("shaders/tessControl.glsl");
	string tesSource = LoadSource("shaders/tessEval.glsl");
	if (vertexSource.empty() || fragmentSource.empty()) return 0;

	// compile shader source into shader objects
	GLuint vertex = CompileShader(GL_VERTEX_SHADER, vertexSource);
	GLuint fragment = CompileShader(GL_FRAGMENT_SHADER, fragmentSource);
	GLuint tcs = CompileShader(GL_TESS_CONTROL_SHADER, tcsSource);
	GLuint tes = CompileShader(GL_TESS_EVALUATION_SHADER, tesSource);

	// link shader program
	GLuint program = LinkProgram(vertex, fragment, tcs, tes);

	glDeleteShader(vertex);
	glDeleteShader(fragment);
	glDeleteShader(tcs);
	glDeleteShader(tes);

	if (CheckGLErrors())
		return 0;

	// check for OpenGL errors and return false if error occurred
	return program;
}

GLuint InitializeShaders2()
{
	// load shader source from files
	string vertexSource = LoadSource("shaders/vertex.glsl");
	string fragmentSource = LoadSource("shaders/fragment.glsl");
	if (vertexSource.empty() || fragmentSource.empty()) return false;

	// compile shader source into shader objects
	GLuint vertex = CompileShader(GL_VERTEX_SHADER, vertexSource);
	GLuint fragment = CompileShader(GL_FRAGMENT_SHADER, fragmentSource);

	// link shader program
	GLuint program = LinkProgram2(vertex, fragment);

	glDeleteShader(vertex);
	glDeleteShader(fragment);

	// check for OpenGL errors and return false if error occurred
	return program;
}
// --------------------------------------------------------------------------
// Functions to set up OpenGL buffers for storing geometry data

struct Geometry
{
	// OpenGL names for array buffer objects, vertex array object
	GLuint  vertexBuffer;
	GLuint  textureBuffer;
	GLuint  colourBuffer;
	GLuint  vertexArray;
	GLsizei elementCount;

	// initialize object names to zero (OpenGL reserved value)
	Geometry() : vertexBuffer(0), colourBuffer(0), vertexArray(0), elementCount(0)
	{}
};

Geometry MyGeometry;
Geometry MyGeometry2;

bool InitializeVAO(Geometry *geometry){

	const GLuint VERTEX_INDEX = 0;
	const GLuint COLOUR_INDEX = 1;

	//Generate Vertex Buffer Objects
	// create an array buffer object for storing our vertices
	glGenBuffers(1, &geometry->vertexBuffer);

	// create another one for storing our colours
	glGenBuffers(1, &geometry->colourBuffer);

	//Set up Vertex Array Object
	// create a vertex array object encapsulating all our vertex attributes
	glGenVertexArrays(1, &geometry->vertexArray);
	glBindVertexArray(geometry->vertexArray);

	// associate the position array with the vertex array object
	glBindBuffer(GL_ARRAY_BUFFER, geometry->vertexBuffer);
	glVertexAttribPointer(
		VERTEX_INDEX,		//Attribute index
		2, 					//# of components
		GL_FLOAT, 			//Type of component
		GL_FALSE, 			//Should be normalized?
		sizeof(vec2),		//Stride - can use 0 if tightly packed
		0);					//Offset to first
	glEnableVertexAttribArray(VERTEX_INDEX);

	// associate the colour array with the vertex array object
	glBindBuffer(GL_ARRAY_BUFFER, geometry->colourBuffer);
	glVertexAttribPointer(
		COLOUR_INDEX,		//Attribute index
		3, 					//# of components
		GL_FLOAT, 			//Type of component
		GL_FALSE, 			//Should be normalized?
		sizeof(vec3), 		//Stride - can use 0 if tightly packed
		0);					//Offset to first element
	glEnableVertexAttribArray(COLOUR_INDEX);

	// unbind our buffers, resetting to default state
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	return !CheckGLErrors();
}

// create buffers and fill with geometry data, returning true if successful
bool LoadGeometry(Geometry *geometry, vec2 *vertices, vec3 *colours, int elementCount)
{
	geometry->elementCount = elementCount;

	// create an array buffer object for storing our vertices
	glBindBuffer(GL_ARRAY_BUFFER, geometry->vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vec2)*geometry->elementCount, vertices, GL_STATIC_DRAW);

	// create another one for storing our colours
	glBindBuffer(GL_ARRAY_BUFFER, geometry->colourBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vec3)*geometry->elementCount, colours, GL_STATIC_DRAW);

	//Unbind buffer to reset to default state
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// check for OpenGL errors and return false if error occurred
	return !CheckGLErrors();
}

// deallocate geometry-related objects
void DestroyGeometry(Geometry *geometry)
{
	// unbind and destroy our vertex array object and associated buffers
	glBindVertexArray(0);
	glDeleteVertexArrays(1, &geometry->vertexArray);
	glDeleteBuffers(1, &geometry->vertexBuffer);
	glDeleteBuffers(1, &geometry->colourBuffer);
}

// --------------------------------------------------------------------------
// Rendering function that draws our scene to the frame buffer

void RenderScene(Geometry *geometry, GLuint program)
{

	// clear screen to a dark grey colour
	glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glPointSize(4);
/*
	LoadGeometry(&MyGeometry, vertexPoints.data(), coloursTwo.data(), vertexPoints.size());
	glUseProgram(program);
	glBindVertexArray(geometry->vertexArray);
	glDrawArrays(GL_POINTS, 0, geometry->elementCount);
*/

	// bind our shader program and the vertex array object containing our
	// scene geometry, then tell OpenGL to draw our geometry
	glUseProgram(program);

	glBindVertexArray(geometry->vertexArray);
	glDrawArrays(GL_PATCHES, 0, geometry->elementCount);

/*
	glUseProgram(program);
	glBindVertexArray(geometry->vertexArray);
	glDrawArrays(GL_PATCHES, 0, geometry->elementCount);
*/
	// reset state to default (no shader or geometry bound)
	glBindVertexArray(0);
	glUseProgram(0);

	// check for an report any OpenGL errors
	CheckGLErrors();
}

void RenderScene2(Geometry *geometry, GLuint program)
{

	glPointSize(4);

	// bind our shader program and the vertex array object containing our
	// scene geometry, then tell OpenGL to draw our geometry

	glUseProgram(program);
	glBindVertexArray(geometry->vertexArray);
	glDrawArrays(GL_POINTS, 0, geometry->elementCount);

	// reset state to default (no shader or geometry bound)
	glBindVertexArray(0);
	glUseProgram(0);

	// check for an report any OpenGL errors
	CheckGLErrors();
}

void RenderScene3(Geometry *geometry, GLuint program)
{


	glPointSize(4);


	// bind our shader program and the vertex array object containing our
	// scene geometry, then tell OpenGL to draw our geometry
	glUseProgram(program);
	glBindVertexArray(geometry->vertexArray);
	glDrawArrays(GL_LINES, 0, geometry->elementCount);

	// reset state to default (no shader or geometry bound)
	glBindVertexArray(0);
	glUseProgram(0);

	// check for an report any OpenGL errors
	CheckGLErrors();
}


void RenderSceneOther(Geometry *geometry, GLuint program)
{

	// clear screen to a dark grey colour
	glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glPointSize(4);

	// bind our shader program and the vertex array object containing our
	// scene geometry, then tell OpenGL to draw our geometry
	glUseProgram(program);

	glBindVertexArray(geometry->vertexArray);
	glDrawArrays(GL_PATCHES, 0, geometry->elementCount);


	// reset state to default (no shader or geometry bound)
	glBindVertexArray(0);
	glUseProgram(0);

	// check for an report any OpenGL errors
	CheckGLErrors();
}





// --------------------------------------------------------------------------
void generate_control_points(vector<vec2>* vertexPoints, vector<vec3>* colours, vector<vec3>* coloursTwo, vector<vec2>* vertexLines,
vector<vec3>* coloursThree)
{
	vertexPoints->clear();
	vertexLines->clear();
	colours->clear();
	coloursTwo->clear();
	coloursThree->clear();



	if(globalDegree == 3)
	{
		//setting up quadratic bezier curve control points
		float factor = 0.3;
		//curve 1
		vertexPoints->push_back(vec2(1.0*factor, 1.0*factor));
		vertexPoints->push_back(vec2(2.0*factor, -1.0*factor));
		vertexPoints->push_back(vec2(0.0*factor, -1.0*factor));

		//curve 2
		vertexPoints->push_back(vec2(0.0*factor, -1.0*factor));
		vertexPoints->push_back(vec2(-2.0*factor, -1.0*factor));
		vertexPoints->push_back(vec2(-1.0*factor, 1.0*factor));

		//curve 3
		vertexPoints->push_back(vec2(-1.0*factor, 1.0*factor));
		vertexPoints->push_back(vec2(0.0*factor, 1.0*factor));
		vertexPoints->push_back(vec2(1.0*factor, 1.0*factor));

		//curve 4
		vertexPoints->push_back(vec2(1.2*factor, 0.5*factor));
		vertexPoints->push_back(vec2(2.5*factor, 1.0*factor));
		vertexPoints->push_back(vec2(1.3*factor, -0.4*factor));

		//points to create polygon
		vertexLines->push_back(vec2(1.0*factor, 1.0*factor));
		vertexLines->push_back(vec2(2.0*factor, -1.0*factor));
		vertexLines->push_back(vec2(2.0*factor, -1.0*factor));
		vertexLines->push_back(vec2(0.0*factor, -1.0*factor));
		vertexLines->push_back(vec2(0.0*factor, -1.0*factor));
		vertexLines->push_back(vec2(1.0*factor, 1.0*factor));

		vertexLines->push_back(vec2(0.0*factor, -1.0*factor));
		vertexLines->push_back(vec2(-2.0*factor, -1.0*factor));
		vertexLines->push_back(vec2(-2.0*factor, -1.0*factor));
		vertexLines->push_back(vec2(-1.0*factor, 1.0*factor));
		vertexLines->push_back(vec2(-1.0*factor, 1.0*factor));
		vertexLines->push_back(vec2(0.0*factor, -1.0*factor));

		vertexLines->push_back(vec2(-1.0*factor, 1.0*factor));
		vertexLines->push_back(vec2(0.0*factor, 1.0*factor));
		vertexLines->push_back(vec2(0.0*factor, 1.0*factor));
		vertexLines->push_back(vec2(1.0*factor, 1.0*factor));
		vertexLines->push_back(vec2(1.0*factor, 1.0*factor));
		vertexLines->push_back(vec2(-1.0*factor, 1.0*factor));

		vertexLines->push_back(vec2(1.2*factor, 0.5*factor));
		vertexLines->push_back(vec2(2.5*factor, 1.0*factor));
		vertexLines->push_back(vec2(2.5*factor, 1.0*factor));
		vertexLines->push_back(vec2(1.3*factor, -0.4*factor));
		vertexLines->push_back(vec2(1.3*factor, -0.4*factor));
		vertexLines->push_back(vec2(1.2*factor, 0.5*factor));

		//color lines
		coloursThree->push_back(vec3(0.0f,0.0,1.0));
		coloursThree->push_back(vec3(0.0,0.0,1.0f));
		coloursThree->push_back(vec3(0.0f,0.0,1.0));
		coloursThree->push_back(vec3(0.0,0.0,1.0f));
		coloursThree->push_back(vec3(0.0f,0.0,1.0));
		coloursThree->push_back(vec3(0.0,0.0,1.0f));
		coloursThree->push_back(vec3(0.0f,0.0,1.0));
		coloursThree->push_back(vec3(0.0,0.0,1.0f));
		coloursThree->push_back(vec3(0.0f,0.0,1.0));
		coloursThree->push_back(vec3(0.0,0.0,1.0f));
		coloursThree->push_back(vec3(0.0f,0.0,1.0));
		coloursThree->push_back(vec3(0.0,0.0,1.0f));
		coloursThree->push_back(vec3(0.0f,0.0,1.0));
		coloursThree->push_back(vec3(0.0,0.0,1.0f));
		coloursThree->push_back(vec3(0.0f,0.0,1.0));
		coloursThree->push_back(vec3(0.0,0.0,1.0f));
		coloursThree->push_back(vec3(0.0f,0.0,1.0));
		coloursThree->push_back(vec3(0.0,0.0,1.0f));
		coloursThree->push_back(vec3(0.0f,0.0,1.0));
		coloursThree->push_back(vec3(0.0,0.0,1.0f));
		coloursThree->push_back(vec3(0.0f,0.0,1.0));
		coloursThree->push_back(vec3(0.0,0.0,1.0f));
		coloursThree->push_back(vec3(0.0f,0.0,1.0));
		coloursThree->push_back(vec3(0.0,0.0,1.0f));






		colours->push_back(vec3(1.0f,0.0,0.0));
		colours->push_back(vec3(0.0,0.0,1.0f));
		colours->push_back(vec3(1.0f,0.0,0.0));

		coloursTwo->push_back(vec3(1.0f,1.0,1.0));
		coloursTwo->push_back(vec3(0.0,1.0,0.0f));
		coloursTwo->push_back(vec3(1.0f,1.0,1.0));

		colours->push_back(vec3(1.0f,0.0,0.0));
		colours->push_back(vec3(0.0,0.0,1.0f));
		colours->push_back(vec3(1.0f,0.0,0.0));

		coloursTwo->push_back(vec3(1.0f,1.0,1.0));
		coloursTwo->push_back(vec3(0.0,1.0,0.0f));
		coloursTwo->push_back(vec3(1.0f,1.0,1.0));

		colours->push_back(vec3(1.0f,0.0,0.0));
		colours->push_back(vec3(0.0,0.0,1.0f));
		colours->push_back(vec3(1.0f,0.0,0.0));

		coloursTwo->push_back(vec3(1.0f,1.0,1.0));
		coloursTwo->push_back(vec3(0.0,1.0,0.0f));
		coloursTwo->push_back(vec3(1.0f,1.0,1.0));

		colours->push_back(vec3(1.0f,0.0,0.0));
		colours->push_back(vec3(0.0,0.0,1.0f));
		colours->push_back(vec3(1.0f,0.0,0.0));

		coloursTwo->push_back(vec3(1.0f,1.0,1.0));
		coloursTwo->push_back(vec3(0.0,1.0,0.0f));
		coloursTwo->push_back(vec3(1.0f,1.0,1.0));


	}

	else if(globalDegree == 4)
	{
		//setting up cubic bezier curve control points
		float factor = 0.12;
		float shift = 0.5;
		float shifty = 0.2;
		//curve 1
		vertexPoints->push_back(vec2(1.0*factor-shift, 1.0*factor-shifty));
		vertexPoints->push_back(vec2(4.0*factor-shift, 0.0*factor-shifty));
		vertexPoints->push_back(vec2(6.0*factor-shift, 2.0*factor-shifty));
		vertexPoints->push_back(vec2(9.0*factor-shift, 1.0*factor-shifty));

		//curve 2
		vertexPoints->push_back(vec2(8.0*factor-shift, 2.0*factor-shifty));
		vertexPoints->push_back(vec2(0.0*factor-shift, 8.0*factor-shifty));
		vertexPoints->push_back(vec2(0.0*factor-shift, -2.0*factor-shifty));
		vertexPoints->push_back(vec2(8.0*factor-shift, 4.0*factor-shifty));

		//curve 3
		vertexPoints->push_back(vec2(5.0*factor-shift, 3.0*factor-shifty));
		vertexPoints->push_back(vec2(3.0*factor-shift, 2.0*factor-shifty));
		vertexPoints->push_back(vec2(3.0*factor-shift, 3.0*factor-shifty));
		vertexPoints->push_back(vec2(5.0*factor-shift, 2.0*factor-shifty));

		//curve 4
		vertexPoints->push_back(vec2(3.0*factor-shift, 2.2*factor-shifty));
		vertexPoints->push_back(vec2(3.5*factor-shift, 2.7*factor-shifty));
		vertexPoints->push_back(vec2(3.5*factor-shift, 3.3*factor-shifty));
		vertexPoints->push_back(vec2(3.0*factor-shift, 3.8*factor-shifty));

		//curve 5
		vertexPoints->push_back(vec2(2.8*factor-shift, 3.5*factor-shifty));
		vertexPoints->push_back(vec2(2.4*factor-shift, 3.8*factor-shifty));
		vertexPoints->push_back(vec2(2.4*factor-shift, 3.2*factor-shifty));
		vertexPoints->push_back(vec2(2.8*factor-shift, 3.5*factor-shifty));

		//points for conrol polygon
		vertexLines->push_back(vec2(1.0*factor-shift, 1.0*factor-shifty));
		vertexLines->push_back(vec2(4.0*factor-shift, 0.0*factor-shifty));
		vertexLines->push_back(vec2(4.0*factor-shift, 0.0*factor-shifty));
		vertexLines->push_back(vec2(6.0*factor-shift, 2.0*factor-shifty));
		vertexLines->push_back(vec2(6.0*factor-shift, 2.0*factor-shifty));
		vertexLines->push_back(vec2(9.0*factor-shift, 1.0*factor-shifty));
		vertexLines->push_back(vec2(9.0*factor-shift, 1.0*factor-shifty));
		vertexLines->push_back(vec2(1.0*factor-shift, 1.0*factor-shifty));

		vertexLines->push_back(vec2(8.0*factor-shift, 2.0*factor-shifty));
		vertexLines->push_back(vec2(0.0*factor-shift, 8.0*factor-shifty));
		vertexLines->push_back(vec2(0.0*factor-shift, 8.0*factor-shifty));
		vertexLines->push_back(vec2(0.0*factor-shift, -2.0*factor-shifty));
		vertexLines->push_back(vec2(0.0*factor-shift, -2.0*factor-shifty));
		vertexLines->push_back(vec2(8.0*factor-shift, 4.0*factor-shifty));
		vertexLines->push_back(vec2(8.0*factor-shift, 4.0*factor-shifty));
		vertexLines->push_back(vec2(8.0*factor-shift, 2.0*factor-shifty));

		vertexLines->push_back(vec2(5.0*factor-shift, 3.0*factor-shifty));
		vertexLines->push_back(vec2(3.0*factor-shift, 2.0*factor-shifty));
		vertexLines->push_back(vec2(3.0*factor-shift, 2.0*factor-shifty));
		vertexLines->push_back(vec2(3.0*factor-shift, 3.0*factor-shifty));
		vertexLines->push_back(vec2(3.0*factor-shift, 3.0*factor-shifty));
		vertexLines->push_back(vec2(5.0*factor-shift, 2.0*factor-shifty));
		vertexLines->push_back(vec2(5.0*factor-shift, 2.0*factor-shifty));
		vertexLines->push_back(vec2(5.0*factor-shift, 3.0*factor-shifty));

		vertexLines->push_back(vec2(3.0*factor-shift, 2.2*factor-shifty));
		vertexLines->push_back(vec2(3.5*factor-shift, 2.7*factor-shifty));
		vertexLines->push_back(vec2(3.5*factor-shift, 2.7*factor-shifty));
		vertexLines->push_back(vec2(3.5*factor-shift, 3.3*factor-shifty));
		vertexLines->push_back(vec2(3.5*factor-shift, 3.3*factor-shifty));
		vertexLines->push_back(vec2(3.0*factor-shift, 3.8*factor-shifty));
		vertexLines->push_back(vec2(3.0*factor-shift, 3.8*factor-shifty));
		vertexLines->push_back(vec2(3.0*factor-shift, 2.2*factor-shifty));

		vertexLines->push_back(vec2(2.8*factor-shift, 3.5*factor-shifty));
		vertexLines->push_back(vec2(2.4*factor-shift, 3.8*factor-shifty));
		vertexLines->push_back(vec2(2.4*factor-shift, 3.8*factor-shifty));
		vertexLines->push_back(vec2(2.4*factor-shift, 3.2*factor-shifty));
		vertexLines->push_back(vec2(2.4*factor-shift, 3.2*factor-shifty));
		vertexLines->push_back(vec2(2.8*factor-shift, 3.5*factor-shifty));
		vertexLines->push_back(vec2(2.8*factor-shift, 3.5*factor-shifty));
		vertexLines->push_back(vec2(2.8*factor-shift, 3.5*factor-shifty));

		//color lines
		coloursThree->push_back(vec3(0.0f,0.0,1.0));
		coloursThree->push_back(vec3(0.0,0.0,1.0f));
		coloursThree->push_back(vec3(0.0f,0.0,1.0));
		coloursThree->push_back(vec3(0.0,0.0,1.0f));
		coloursThree->push_back(vec3(0.0f,0.0,1.0));
		coloursThree->push_back(vec3(0.0,0.0,1.0f));
		coloursThree->push_back(vec3(0.0f,0.0,1.0));
		coloursThree->push_back(vec3(0.0,0.0,1.0f));
		coloursThree->push_back(vec3(0.0f,0.0,1.0));
		coloursThree->push_back(vec3(0.0,0.0,1.0f));
		coloursThree->push_back(vec3(0.0f,0.0,1.0));
		coloursThree->push_back(vec3(0.0,0.0,1.0f));
		coloursThree->push_back(vec3(0.0f,0.0,1.0));
		coloursThree->push_back(vec3(0.0,0.0,1.0f));
		coloursThree->push_back(vec3(0.0f,0.0,1.0));
		coloursThree->push_back(vec3(0.0,0.0,1.0f));
		coloursThree->push_back(vec3(0.0f,0.0,1.0));
		coloursThree->push_back(vec3(0.0,0.0,1.0f));
		coloursThree->push_back(vec3(0.0f,0.0,1.0));
		coloursThree->push_back(vec3(0.0,0.0,1.0f));
		coloursThree->push_back(vec3(0.0f,0.0,1.0));
		coloursThree->push_back(vec3(0.0,0.0,1.0f));
		coloursThree->push_back(vec3(0.0f,0.0,1.0));
		coloursThree->push_back(vec3(0.0,0.0,1.0f));
		coloursThree->push_back(vec3(0.0f,0.0,1.0));
		coloursThree->push_back(vec3(0.0,0.0,1.0f));
		coloursThree->push_back(vec3(0.0f,0.0,1.0));
		coloursThree->push_back(vec3(0.0,0.0,1.0f));
		coloursThree->push_back(vec3(0.0f,0.0,1.0));
		coloursThree->push_back(vec3(0.0,0.0,1.0f));
		coloursThree->push_back(vec3(0.0f,0.0,1.0));
		coloursThree->push_back(vec3(0.0,0.0,1.0f));
		coloursThree->push_back(vec3(0.0f,0.0,1.0));
		coloursThree->push_back(vec3(0.0,0.0,1.0f));
		coloursThree->push_back(vec3(0.0f,0.0,1.0));
		coloursThree->push_back(vec3(0.0,0.0,1.0f));
		coloursThree->push_back(vec3(0.0f,0.0,1.0));
		coloursThree->push_back(vec3(0.0,0.0,1.0f));
		coloursThree->push_back(vec3(0.0f,0.0,1.0));
		coloursThree->push_back(vec3(0.0,0.0,1.0f));


		colours->push_back(vec3(1.0f,0.0,0.0));
		colours->push_back(vec3(0.0,0.0,1.0f));
		colours->push_back(vec3(1.0f,0.0,0.0));
		colours->push_back(vec3(1.0f,0.0,0.0));


		coloursTwo->push_back(vec3(1.0f,1.0,1.0));
		coloursTwo->push_back(vec3(0.0,1.0,0.0f));
		coloursTwo->push_back(vec3(0.0,1.0,0.0f));
		coloursTwo->push_back(vec3(1.0f,1.0,1.0));

		colours->push_back(vec3(1.0f,0.0,0.0));
		colours->push_back(vec3(0.0,0.0,1.0f));
		colours->push_back(vec3(1.0f,0.0,0.0));
		colours->push_back(vec3(1.0f,0.0,0.0));

		coloursTwo->push_back(vec3(1.0f,1.0,1.0));
		coloursTwo->push_back(vec3(0.0,1.0,0.0f));
		coloursTwo->push_back(vec3(0.0,1.0,0.0f));
		coloursTwo->push_back(vec3(1.0f,1.0,1.0));

		colours->push_back(vec3(1.0f,0.0,0.0));
		colours->push_back(vec3(0.0,0.0,1.0f));
		colours->push_back(vec3(1.0f,0.0,0.0));
		colours->push_back(vec3(1.0f,0.0,0.0));

		coloursTwo->push_back(vec3(1.0f,1.0,1.0));
		coloursTwo->push_back(vec3(0.0,1.0,0.0f));
		coloursTwo->push_back(vec3(0.0,1.0,0.0f));
		coloursTwo->push_back(vec3(1.0f,1.0,1.0));

		colours->push_back(vec3(1.0f,0.0,0.0));
		colours->push_back(vec3(0.0,0.0,1.0f));
		colours->push_back(vec3(1.0f,0.0,0.0));
		colours->push_back(vec3(1.0f,0.0,0.0));

		coloursTwo->push_back(vec3(1.0f,1.0,1.0));
		coloursTwo->push_back(vec3(0.0,1.0,0.0f));
		coloursTwo->push_back(vec3(0.0,1.0,0.0f));
		coloursTwo->push_back(vec3(1.0f,1.0,1.0));

		colours->push_back(vec3(1.0f,0.0,0.0));
		colours->push_back(vec3(0.0,0.0,1.0f));
		colours->push_back(vec3(1.0f,0.0,0.0));
		colours->push_back(vec3(1.0f,0.0,0.0));

		coloursTwo->push_back(vec3(1.0f,1.0,1.0));
		coloursTwo->push_back(vec3(0.0,1.0,0.0f));
		coloursTwo->push_back(vec3(0.0,1.0,0.0f));
		coloursTwo->push_back(vec3(1.0f,1.0,1.0));

	}
}

void generate_text(vector<vec2>* vertexPoints, vector<vec3>* colours, char character, string font, float shift, float clear)
{
	if (clear == 1.0f){
		vertexPoints->clear();
		colours->clear();
	}


	MyGlyph glyph = extractor.ExtractGlyph(character);
	float addFac = 1.5f - (clear * shift);
	float factor = 0.5f;
	for (int c_index = 0; c_index < glyph.contours.size(); c_index++)
	{
		MyContour contour = glyph.contours[c_index];

		for (int s_index = 0; s_index < contour.size(); s_index++)
		{
			MySegment segment = contour[s_index];

			for (int i = 0; i <= segment.degree; i++)
			{
				if(segment.degree == 1 && globalDegree == 4)
				{
					vertexPoints->push_back(vec2((segment.x[i] - addFac ) * factor, segment.y[i]* factor));
					vertexPoints->push_back(vec2((segment.x[i] - addFac ) * factor, segment.y[i]* factor));
					colours->push_back(vec3(1.0f,1.0f,1.0f));
					colours->push_back(vec3(1.0f,1.0f,1.0f));
				}

				else if(segment.degree == 1 && globalDegree == 3)
				{
					vertexPoints->push_back(vec2((segment.x[i] - addFac ) * factor, segment.y[i]* factor));
					colours->push_back(vec3(1.0f,1.0f,1.0f));
					if (i == 0)
					{
						vertexPoints->push_back(vec2((segment.x[i] - addFac) * factor, segment.y[i]* factor));
						colours->push_back(vec3(1.0f,1.0f,1.0f));
					}
				}
				else{
					vertexPoints->push_back(vec2((segment.x[i] - addFac ) * factor, segment.y[i]* factor));
					colours->push_back(vec3(1.0f,1.0f,1.0f));
				}
			}
		}
	}


}

void generate_text_scroll(vector<vec2>* vertexPoints, vector<vec3>* colours, char character, string font, float shift, float clear)
{
	if (clear == 1.0f ){
		vertexPoints->clear();
		colours->clear();
	}


	MyGlyph glyph = extractor.ExtractGlyph(character);
	float addFac = 1.5f - (clear * shift);
	float factor = 0.5f;
	for (int c_index = 0; c_index < glyph.contours.size(); c_index++)
	{
		MyContour contour = glyph.contours[c_index];

		for (int s_index = 0; s_index < contour.size(); s_index++)
		{
			MySegment segment = contour[s_index];

			for (int i = 0; i <= segment.degree; i++)
			{
				if(segment.degree == 1 && globalDegree == 4)
				{
					vertexPoints->push_back(vec2((segment.x[i] - addFac + translation) * factor, segment.y[i]* factor));
					vertexPoints->push_back(vec2((segment.x[i] - addFac + translation) * factor, segment.y[i]* factor));
					colours->push_back(vec3(1.0f,1.0f,1.0f));
					colours->push_back(vec3(1.0f,1.0f,1.0f));

				}

				else if(segment.degree == 1 && globalDegree == 3)
				{
					vertexPoints->push_back(vec2((segment.x[i] - addFac + translation) * factor, segment.y[i]* factor));
					colours->push_back(vec3(1.0f,1.0f,1.0f));

					if (i == 0)
					{
						vertexPoints->push_back(vec2((segment.x[i] - addFac + translation) * factor, segment.y[i]* factor));
						colours->push_back(vec3(1.0f,1.0f,1.0f));

					}
				}
				else{
					vertexPoints->push_back(vec2((segment.x[i] - addFac + translation) * factor, segment.y[i]* factor));
					colours->push_back(vec3(1.0f,1.0f,1.0f));

				}

			}
		}
	}



}


// GLFW callback functions

// reports GLFW errors
void ErrorCallback(int error, const char* description)
{
	cout << "GLFW ERROR " << error << ":" << endl;
	cout << description << endl;
}

// handles keyboard input events
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	else if(key == GLFW_KEY_1 && action == GLFW_PRESS)
	{
		q1 = true;
		q1p1 = false;
		q2 = false;
		q3 = false;
		globalDegree = 3;

	}

	else if(key == GLFW_KEY_2 && action == GLFW_PRESS)
	{
		q1 = true;
		q2 = false;
		q3 = false;
		globalDegree = 4;

	}




	else if(key == GLFW_KEY_3 && action == GLFW_PRESS)
	{
		vertexPoints.clear();
		colours.clear();
		q1 = false;
		q2 = true;
		q3 = false;
		globalDegree = 3; //since ttf files use quadratic bezier
		font = "Fonts/Lora-Italic.ttf";


	}

	else if(key == GLFW_KEY_4 && action == GLFW_PRESS)
	{
		vertexPoints.clear();
		colours.clear();
		q1 = false;
		q2 = true;
		q3 = false;
		globalDegree = 4; //since otf files use cubic bezier
		font = "Fonts/KaushanScript-Regular.otf";


	}

	else if(key == GLFW_KEY_5 && action == GLFW_PRESS)
	{
		vertexPoints.clear();
		colours.clear();
		q1 = false;
		q2 = true;
		q3 = false;
		globalDegree = 4;
		font = "Fonts/SourceSansPro-Black.otf"; //since otf files use cubic bezier

	}

	else if(key == GLFW_KEY_6 && action == GLFW_PRESS)
	{
		translation = 0.0f;
		q1 = false;
		q2 = false;
		q3 = true;
		globalDegree = 3;
		font = "Fonts/AlexBrush-Regular.ttf";


	}

	else if(key == GLFW_KEY_7 && action == GLFW_PRESS)
	{
		translation = 0.0f;
		q1 = false;
		q2 = false;
		q3 = true;
		globalDegree = 4;
		font = "Fonts/Inconsolata.otf";



	}

	else if(key == GLFW_KEY_8 && action == GLFW_PRESS)
	{
		translation = 0.0f;
		q1 = false;
		q2 = false;
		q3 = true;
		globalDegree = 3;
		font = "Fonts/AquilineTwo.ttf";


	}

	else if(key == GLFW_KEY_RIGHT && (action == GLFW_REPEAT ||  action == GLFW_PRESS))
	{

		q1 = false;
		q2 = false;
		q3 = true;
		translation-=1.0f * speed;
		if(translation < -26.4)
		{
			vertexPoints.clear();
			colours.clear();
			translation = 3.0f;
		}




	}



	else if(key == GLFW_KEY_UP && (action == GLFW_PRESS || action == GLFW_REPEAT) && speed < 0.2f)
	{
		speed+=0.02f;

	}

	else if(key == GLFW_KEY_DOWN && (action == GLFW_PRESS || action == GLFW_REPEAT) && speed > 0.02f)
	{
		speed-=0.02f;

	}


}

// ==========================================================================
// PROGRAM ENTRY POINT


int main(int argc, char *argv[])
{
	// initialize the GLFW windowing system
	if (!glfwInit()) {
		cout << "ERROR: GLFW failed to initialize, TERMINATING" << endl;
		return -1;
	}
	glfwSetErrorCallback(ErrorCallback);

	// attempt to create a window with an OpenGL 4.1 core profile context
	GLFWwindow *window = 0;
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	int width = 512, height = 512;
	window = glfwCreateWindow(width, height, "CPSC 453 OpenGL Boilerplate", 0, 0);
	if (!window) {
		cout << "Program failed to create GLFW window, TERMINATING" << endl;
		glfwTerminate();
		return -1;
	}

	// set keyboard callback function and make our context current (active)
	glfwSetKeyCallback(window, KeyCallback);
	glfwMakeContextCurrent(window);

	//Intialize GLAD
	if (!gladLoadGL())
	{
		cout << "GLAD init failed" << endl;
		return -1;
	}

	// query and print out information about our OpenGL environment
	QueryGLVersion();

	// call function to load and compile shader programs
	GLuint program = InitializeShaders();
	GLuint program2 = InitializeShaders2();

	if (program == 0) {
		cout << "Program could not initialize shaders, TERMINATING" << endl;
		return -1;
	}


	// call function to create and fill buffers with geometry data

	if (!InitializeVAO(&MyGeometry))
		cout << "Program failed to intialize geometry!" << endl;


	if (!InitializeVAO(&MyGeometry2))
			cout << "Program failed to intialize geometry!" << endl;

	if(!LoadGeometry(&MyGeometry, vertexPoints.data(), colours.data(), vertexPoints.size()))
		cout << "Failed to load geometry" << endl;



	// run an event-triggered main loop
	while (!glfwWindowShouldClose(window))
	{

		if(q1 == true)
		{
			//
			generate_control_points(&vertexPoints, &colours, &coloursTwo, &vertexLines, &coloursThree);
			LoadGeometry(&MyGeometry, vertexPoints.data(), colours.data(), vertexPoints.size());
			glUseProgram(program);
			glPatchParameteri(GL_PATCH_VERTICES, globalDegree);
			GLuint degree = glGetUniformLocation(program, "deg");
			glUniform1ui(degree, globalDegree);
			RenderScene(&MyGeometry, program);
			LoadGeometry(&MyGeometry2, vertexPoints.data(), coloursTwo.data(), vertexPoints.size());
			RenderScene2(&MyGeometry2, program2);
			LoadGeometry(&MyGeometry2, vertexLines.data(), coloursThree.data(), vertexLines.size());
			RenderScene3(&MyGeometry2,program2);

		}



		else if (q2 == true)
		{
			//
			extractor.LoadFontFile(font);
			generate_text(&vertexPoints, &colours, 'A', font, 0.0f,1.0f);
			generate_text(&vertexPoints, &colours, 'd', font, 0.5f,1.2f);
			generate_text(&vertexPoints, &colours, 'n', font, 1.0f,1.2f);
			generate_text(&vertexPoints, &colours, 'a', font, 1.5f,1.2f);
			generate_text(&vertexPoints, &colours, 'n', font, 2.0f,1.2f);
			LoadGeometry(&MyGeometry, vertexPoints.data(), colours.data(), vertexPoints.size());
			glUseProgram(program);
			glPatchParameteri(GL_PATCH_VERTICES, globalDegree);
			GLuint degree = glGetUniformLocation(program, "deg");
			glUniform1ui(degree, globalDegree);
			RenderSceneOther(&MyGeometry, program);

		}

		else if (q3 == true)
		{

			extractor.LoadFontFile(font);
			generate_text_scroll(&vertexPoints, &colours, 'T', font, 0.0f,1.0f);
			generate_text_scroll(&vertexPoints, &colours, 'h', font, 0.5f,1.2f);
			generate_text_scroll(&vertexPoints, &colours, 'e', font, 1.0f,1.2f);

			generate_text_scroll(&vertexPoints, &colours, ' ', font, 1.5f,1.2f);
			generate_text_scroll(&vertexPoints, &colours, 'q', font, 2.0f,1.2f);
			generate_text_scroll(&vertexPoints, &colours, 'u', font, 2.5f,1.2f);
			generate_text_scroll(&vertexPoints, &colours, 'i', font, 3.0f,1.2f);
			generate_text_scroll(&vertexPoints, &colours, 'c', font, 3.5f,1.2f);
			generate_text_scroll(&vertexPoints, &colours, 'k', font, 4.0f,1.2f);

			generate_text_scroll(&vertexPoints, &colours, ' ', font, 4.5f,1.2f);
			generate_text_scroll(&vertexPoints, &colours, 'b', font, 5.0f,1.2f);
		  generate_text_scroll(&vertexPoints, &colours, 'r', font, 5.5f,1.2f);
			generate_text_scroll(&vertexPoints, &colours, 'o', font, 6.0f,1.2f);
			generate_text_scroll(&vertexPoints, &colours, 'w', font, 6.5f,1.2f);
			generate_text_scroll(&vertexPoints, &colours, 'n', font, 7.0f,1.2f);

			generate_text_scroll(&vertexPoints, &colours, ' ', font, 7.5f,1.2f);
			generate_text_scroll(&vertexPoints, &colours, 'f', font, 8.0f,1.2f);
			generate_text_scroll(&vertexPoints, &colours, 'o', font, 8.5f,1.2f);
			generate_text_scroll(&vertexPoints, &colours, 'x', font, 9.0f,1.2f);

			generate_text_scroll(&vertexPoints, &colours, ' ', font, 9.5f,1.2f);
			generate_text_scroll(&vertexPoints, &colours, 'j', font, 10.0f,1.2f);
			generate_text_scroll(&vertexPoints, &colours, 'u', font, 10.5f,1.2f);
			generate_text_scroll(&vertexPoints, &colours, 'm', font, 11.0f,1.2f);
			generate_text_scroll(&vertexPoints, &colours, 'p', font, 11.5f,1.2f);
			generate_text_scroll(&vertexPoints, &colours, 's', font, 12.0f,1.2f);

			generate_text_scroll(&vertexPoints, &colours, ' ', font, 12.5f,1.2f);
			generate_text_scroll(&vertexPoints, &colours, 'o', font, 13.0f,1.2f);
			generate_text_scroll(&vertexPoints, &colours, 'v', font, 13.5f,1.2f);
			generate_text_scroll(&vertexPoints, &colours, 'e', font, 14.0f,1.2f);
			generate_text_scroll(&vertexPoints, &colours, 'r', font, 14.5f,1.2f);

			generate_text_scroll(&vertexPoints, &colours, ' ', font, 15.0f,1.2f);
		  generate_text_scroll(&vertexPoints, &colours, 't', font, 15.5f,1.2f);
			generate_text_scroll(&vertexPoints, &colours, 'h', font, 16.0f,1.2f);
			generate_text_scroll(&vertexPoints, &colours, 'e', font, 16.5f,1.2f);

			generate_text_scroll(&vertexPoints, &colours, ' ', font, 17.0f,1.2f);
			generate_text_scroll(&vertexPoints, &colours, 'l', font, 17.5f,1.2f);
			generate_text_scroll(&vertexPoints, &colours, 'a', font, 18.0f,1.2f);
			generate_text_scroll(&vertexPoints, &colours, 'z', font, 18.5f,1.2f);
			generate_text_scroll(&vertexPoints, &colours, 'y', font, 19.0f,1.2f);

			generate_text_scroll(&vertexPoints, &colours, ' ', font, 19.5f,1.2f);
			generate_text_scroll(&vertexPoints, &colours, 'd', font, 20.0f,1.2f);
			generate_text_scroll(&vertexPoints, &colours, 'o', font, 20.5f,1.2f);
			generate_text_scroll(&vertexPoints, &colours, 'g', font, 21.0f,1.2f);
			generate_text_scroll(&vertexPoints, &colours, '.', font, 21.5f,1.2f);

			LoadGeometry(&MyGeometry, vertexPoints.data(), colours.data(), vertexPoints.size());
			glUseProgram(program);
			glPatchParameteri(GL_PATCH_VERTICES, globalDegree);
			GLuint degree = glGetUniformLocation(program, "deg");
			glUniform1ui(degree, globalDegree);
			RenderSceneOther(&MyGeometry, program);



		}


		// call function to draw our scene


		glfwSwapBuffers(window);

		glfwPollEvents();
	}

	// clean up allocated resources before exit
	DestroyGeometry(&MyGeometry);
	DestroyGeometry(&MyGeometry2);
	glUseProgram(0);
	glDeleteProgram(program);
	glDeleteProgram(program2);
	glfwDestroyWindow(window);
	glfwTerminate();

	cout << "Goodbye!" << endl;
	return 0;
}

// ==========================================================================
// SUPPORT FUNCTION DEFINITIONS

// --------------------------------------------------------------------------
// OpenGL utility functions

void QueryGLVersion()
{
	// query opengl version and renderer information
	string version = reinterpret_cast<const char *>(glGetString(GL_VERSION));
	string glslver = reinterpret_cast<const char *>(glGetString(GL_SHADING_LANGUAGE_VERSION));
	string renderer = reinterpret_cast<const char *>(glGetString(GL_RENDERER));

	cout << "OpenGL [ " << version << " ] "
		<< "with GLSL [ " << glslver << " ] "
		<< "on renderer [ " << renderer << " ]" << endl;
}

bool CheckGLErrors()
{
	bool error = false;
	for (GLenum flag = glGetError(); flag != GL_NO_ERROR; flag = glGetError())
	{
		cout << "OpenGL ERROR:  ";
		switch (flag) {
		case GL_INVALID_ENUM:
			cout << "GL_INVALID_ENUM" << endl; break;
		case GL_INVALID_VALUE:
			cout << "GL_INVALID_VALUE" << endl; break;
		case GL_INVALID_OPERATION:
			cout << "GL_INVALID_OPERATION" << endl; break;
		case GL_INVALID_FRAMEBUFFER_OPERATION:
			cout << "GL_INVALID_FRAMEBUFFER_OPERATION" << endl; break;
		case GL_OUT_OF_MEMORY:
			cout << "GL_OUT_OF_MEMORY" << endl; break;
		default:
			cout << "[unknown error code]" << endl;
		}
		error = true;
	}
	return error;
}

// --------------------------------------------------------------------------
// OpenGL shader support functions

// reads a text file with the given name into a string
string LoadSource(const string &filename)
{
	string source;

	ifstream input(filename.c_str());
	if (input) {
		copy(istreambuf_iterator<char>(input),
			istreambuf_iterator<char>(),
			back_inserter(source));
		input.close();
	}
	else {
		cout << "ERROR: Could not load shader source from file "
			<< filename << endl;
	}

	return source;
}

// creates and returns a shader object compiled from the given source
GLuint CompileShader(GLenum shaderType, const string &source)
{
	// allocate shader object name
	GLuint shaderObject = glCreateShader(shaderType);

	// try compiling the source as a shader of the given type
	const GLchar *source_ptr = source.c_str();
	glShaderSource(shaderObject, 1, &source_ptr, 0);
	glCompileShader(shaderObject);

	// retrieve compile status
	GLint status;
	glGetShaderiv(shaderObject, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE)
	{
		GLint length;
		glGetShaderiv(shaderObject, GL_INFO_LOG_LENGTH, &length);
		string info(length, ' ');
		glGetShaderInfoLog(shaderObject, info.length(), &length, &info[0]);
		cout << "ERROR compiling shader:" << endl << endl;
		cout << source << endl;
		cout << info << endl;
	}

	return shaderObject;
}

// creates and returns a program object linked from vertex and fragment shaders
GLuint LinkProgram(GLuint vertexShader, GLuint fragmentShader, GLuint tcsShader, GLuint tesShader)
{
	// allocate program object name
	GLuint programObject = glCreateProgram();

	// attach provided shader objects to this program
	if (vertexShader)   glAttachShader(programObject, vertexShader);
	if (fragmentShader) glAttachShader(programObject, fragmentShader);
	if (tcsShader) glAttachShader(programObject, tcsShader);
	if (tesShader) glAttachShader(programObject, tesShader);

	// try linking the program with given attachments
	glLinkProgram(programObject);

	// retrieve link status
	GLint status;
	glGetProgramiv(programObject, GL_LINK_STATUS, &status);
	if (status == GL_FALSE)
	{
		GLint length;
		glGetProgramiv(programObject, GL_INFO_LOG_LENGTH, &length);
		string info(length, ' ');
		glGetProgramInfoLog(programObject, info.length(), &length, &info[0]);
		cout << "ERROR linking shader program:" << endl;
		cout << info << endl;
	}

	return programObject;
}

// creates and returns a program object linked from vertex and fragment shaders
GLuint LinkProgram2(GLuint vertexShader, GLuint fragmentShader)
{
	// allocate program object name
	GLuint programObject = glCreateProgram();

	// attach provided shader objects to this program
	if (vertexShader)   glAttachShader(programObject, vertexShader);
	if (fragmentShader) glAttachShader(programObject, fragmentShader);

	// try linking the program with given attachments
	glLinkProgram(programObject);

	// retrieve link status
	GLint status;
	glGetProgramiv(programObject, GL_LINK_STATUS, &status);
	if (status == GL_FALSE)
	{
		GLint length;
		glGetProgramiv(programObject, GL_INFO_LOG_LENGTH, &length);
		string info(length, ' ');
		glGetProgramInfoLog(programObject, info.length(), &length, &info[0]);
		cout << "ERROR linking shader program #2:" << endl;
		cout << info << endl;
	}

	return programObject;
}
