#version 410
/**
* Tessellation Evaluation Shader
*	Determines positions of points in tessellated patches
*	Receives input from gl_in, tcs out variables and gl_TessCoord
*	Run once for every vertex in the output patch
*/

//Type of patch being output
layout(isolines) in;

in vec3 teColour[];
//in gl_in[];

//Information being sent out to fragment shader
//Will be interpolated as if sent from vertex shader
out vec3 Colour;
uniform uint deg;

#define PI 3.14159265359

void main()
{
	//gl_TessCoord.x will parameterize the segments of the line from 0 to 1
	//gl_TessCoord.y will parameterize the number of lines from 0 to 1
	float u = gl_TessCoord.x;

	if  (deg == 3)
	{
		//quadratic bezier curve calculation
		gl_Position = ((1-u) * (1-u) * gl_in[0].gl_Position) + (2 * u * (1-u) * gl_in[1].gl_Position) + (u * u * gl_in[2].gl_Position);

		Colour = ((1-u) * (1-u) * teColour[0]) + (2 * (1-u) * u * teColour[1]) + (u * u * teColour[2]);

	}

	else if (deg == 4)
	{
		//cubic bezier curve calculation
		float p0 = (1-u) * (1-u) * (1-u);
		float p1 = 3 * u * (1-u) * (1-u);
		float p2 = 3 * u * u * (1-u);
		float p3 = u * u * u;
		gl_Position = (p0 * gl_in[0].gl_Position) + (p1 * gl_in[1].gl_Position) + (p2 * gl_in[2].gl_Position) + (p3 * gl_in[3].gl_Position);

		Colour = p0 * teColour[0] +  p1 * teColour[1] +  p3 * teColour[2] +  p3 * teColour[3];
	}



}
