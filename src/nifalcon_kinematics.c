/*
 * Implementation file for NovInt Falcon FindFalcon Example Program
 *
 * Copyright (c) 2008 Kyle Machulis <kyle@nonpolynomial.com> http://www.nonpolynomial.com
 *
 * Sourceforge project @ http://www.sourceforge.net/projects/libnifalcon
 *
 * This library is covered by the MIT License, read LICENSE for details.
 *
 * Various piece of code ripped from NeHe, 'cause it's been a while since I've played with OpenGL
 * http://nehe.gamedev.net/
 */

/*
	float e = 8, f = 16, re = 10.3094, rf = 8, t1=55.19, t2=9.5, t3=37.8;
	float w1,x1,y1,z1,w2,x2,y2,z2,ar, b1, b2, d;
	float x,y,z,a,b,c,p01,p02,p03,p23,p31,p12,p2,tr1,tr2,tr3,tr,dtr,r3,ef,e1x,e1y,e1z,e2x,e2y,e2z,e3x,e3y,e3z;

	// Leaving the original version of the code from the paper here.
	dtr = 3.1415926/180.0;
	r3 = sqrt(3.0);

	tr1 = t1*dtr;
	tr2 = t2*dtr;
	tr3 = t3*dtr;
	tr = 2.0*r3;

	ef = (e-f)/tr;
	e1y=ef-rf*cos(tr1);
	e1z=-rf*sin(tr1);
	printf("e1 %f %f\n", e1y, e1z);
	
	e2y=(rf*cos(tr2)-ef)/2.0;
	e2x=e2y*r3;
	e2z=-rf*sin(tr2);
	printf("e2 %f %f %f\n", e2x, e2y, e2z);

	e3y=(rf*cos(tr3)-ef)/2.0;
	e3x=-e3y*r3;
	e3z=-rf*sin(tr3);
	printf("e3 %f %f %f\n", e3x, e3y, e3z);

	w1=((e1y*e1y)-(e2x*e2x)-(e2y*e2y)+(e1z*e1z)-(e2z*e2z))/2.0;
	x1=e2x;
	y1=e2y-e1y;
	z1=e2z-e1z;

	w2=((e2x*e2x)-(e3x*e3x)+(e2y*e2y)-(e3y*e3y)+(e2z*e2z)-(e3z*e3z))/2.0;
	x2=e3x-e2x;
	y2=e3y-e2y;
	z2=e3z-e2z;
	
	p01=y1*z2-y2*z1;
	if(!p01)
	{
		printf("p01 == 0 : %f %f %f %f %f\n", y1, y2, z1, z2, sin(tr1));
		return 1;
	}
	p02=z1*x2-z2*x1;
	p03=x1*y2-x2*y1;

	p23=w1*x2-w2*x1;
	p31=w1*y2-w2*y1;
	p12=w1*z2-w2*z1;
	p2=p01*p01;

	a=p2+(p02*p02)+(p03*p03);
	b1=p12+p01*e1y;
	b2=p31-p01*e1z;
	b=p02*b1-p03*b2;

	c=(b1*b1)+(b2*b2)-(p2*(re*re));
	ar=(b*b)-(a*c);
	if(ar < 0)
	{
		printf("ar < 0\n");
		return 1;
	}

	d=sqrt(ar);
	x=(b+d)/a;
	y=(p02*x-p12)/p01;
	z=(p03*x+p31)/p01;

	printf("%f %f %f %f\n%f %f %f\n%f %f %f\n", e, f, re, rf, t1, t2, t3, x, y, z);
		
*/	

#include "nifalcon_kinematics.h"
#include "math.h"

#define DEGREES_TO_RADIANS 3.1415926/180.0

void nifalcon_init_kinematics(falcon_kinematics* dk)
{
	/******** Parallel bot unit input section ********/
	/******** Units are assumed to be inches ********/

	//Units for the novint falcon

	//fixed frame length
	//I don't have a damn /clue/ what this really is
	//That's part of why I wrote this simulator, so I can just throw numbers until something seems right.
	//It's probably derivable from the measuring the closest distance between the fixed frame origin and
	//the innermost workspace point, but really, do I care?	
	dk->f = 3.0;

	//End Effector length
	//Measured with a ruler!
	dk->e = 3.0;
	
	//shin length
	//Measued with a ruler! Includes the thigh/shin and shin/effector jointy thingy length
	dk->re = 5.0;
	
	//thigh length
	//It's bent, so it's funky. Let's just say 4.
	dk->rf = 4.0;
}

int nifalcon_direct_kinematics(falcon_kinematics* dk, int encoder1, int encoder2, int encoder3)
{
	float tr, tr1, tr2, tr3, p01, p02, p03, p12, p23, p31, p2, a, b, c, d, b1, b2, ar;
	float ef;
	float r3 = sqrt(3.0);

	// Lots of assumptions being made here.
	// First off, I have no idea what the actual encoder min/max for the falcon is.
	// However, in practice it seems to be around -/+1750. This puts us close to but
	// not exactly at 90 degrees from the center of the fixed frame on either side.
	// Therefore, the encoder values are buffered around 250 ticks in to account for
	// this.
	
	dk->thigh_angle[0] = (((float)encoder1 + 2000.0f)/4000.0f) * 90.0f;
	dk->thigh_angle[1] = (((float)encoder2 + 2000.0f)/4000.0f) * 90.0f;
	dk->thigh_angle[2] = (((float)encoder3 + 2000.0f)/4000.0f) * 90.0f;

	//Convert all input angles to radians since that's what the c math funcs eat
	tr1 = dk->thigh_angle[0] * DEGREES_TO_RADIANS;
	tr2 = dk->thigh_angle[1] * DEGREES_TO_RADIANS;
	tr3 = dk->thigh_angle[2] * DEGREES_TO_RADIANS;
	tr = 2.0*r3;
	
	ef = (dk->e-dk->f)/tr;

	/******** Hip/Hip Restraint/Knee Calculation and Rendering ********/

	// All of our calculations here use rf (thigh length, also the radius of the straint sphere
	// of the thigh) as a scalar.
	// Pseudocode: 
	// for N[1:3]	
	// eNy = (rf*cos(trN)-ef)/2.0)
	// eNx = (eNy * sqrt(3))
	// eNz = -rf*(sin(tr1));

	//No e1x because it's always 0. Yay physical constraints!
	dk->knee[0].x= 0;
	dk->knee[0].y= ef-dk->rf*cos(tr1);
	dk->knee[0].z= -dk->rf*sin(tr1);

	dk->knee[1].y=(dk->rf*cos(tr2)-ef)/2.0;
	dk->knee[1].x=dk->knee[1].y*r3;
	dk->knee[1].z=-dk->rf*sin(tr2);

	dk->knee[2].y=(dk->rf*cos(tr3)-ef)/2.0;
	dk->knee[2].x=-dk->knee[2].y*r3;
	dk->knee[2].z=-dk->rf*sin(tr3);
	
	/******** Shin/Effector Position Calculation and Rendering ********/

	//Basically, we're finding the two intersection points of the 3 constraint spheres
	//with centers at the knee positions and radii of the knee constraint length (re).
	//This is done by finding the intersection of two of the constraint spheres, then
	//testing the third against the resulting circle to find the 2 intersection points.
	//We then need to figure out which point is valid. This choice is made by the location
	//of knees 2 and 3, as can be seen in the code below.
	
	//Compute the homogenenous coordinates of the intersection plane
	//of the constraint spheres of knee e1 and e2, and e2 and e3
	//The original equation for this looks like
	// {w:x:y:z} =
	// { (rf^2 - re^2 + eNx^2 - e(N+1)x^2 + eNy^2 - e(N+1)y^2 + eNz^2 - e(N+1)z^2) / 2 : e(N+1)x - eNx : e(N+1)y - eNy : e(N+1)z - eNz }
	
	//Coords for e1/e2
	//w1 looks funky because e1x is 0, so a few things get shifted for readability
	dk->col[0].w=((dk->knee[0].y*dk->knee[0].y)-(dk->knee[1].x*dk->knee[1].x)-(dk->knee[1].y*dk->knee[1].y)+(dk->knee[0].z*dk->knee[0].z)-(dk->knee[1].z*dk->knee[1].z))/2.0;
	dk->col[0].x=dk->knee[1].x;
	dk->col[0].y=dk->knee[1].y-dk->knee[0].y;
	dk->col[0].z=dk->knee[1].z-dk->knee[0].z;
	
	dk->col[1].w=((dk->knee[1].x*dk->knee[1].x)-(dk->knee[2].x*dk->knee[2].x)+(dk->knee[1].y*dk->knee[1].y)-(dk->knee[2].y*dk->knee[2].y)+(dk->knee[1].z*dk->knee[1].z)-(dk->knee[2].z*dk->knee[2].z))/2.0;
	dk->col[1].x=dk->knee[2].x-dk->knee[1].x;
	dk->col[1].y=dk->knee[2].y-dk->knee[1].y;
	dk->col[1].z=dk->knee[2].z-dk->knee[1].z;
	
	dk->col[2].w=((dk->knee[0].y*dk->knee[0].y)-(dk->knee[2].x*dk->knee[2].x)-(dk->knee[2].y*dk->knee[2].y)+(dk->knee[0].z*dk->knee[0].z)-(dk->knee[2].z*dk->knee[2].z))/2.0;
	dk->col[2].x=dk->knee[2].x;
	dk->col[2].y=dk->knee[2].y-dk->knee[0].y;
	dk->col[2].z=dk->knee[2].z-dk->knee[0].z;

	//Plucker coordinate derivation for intersection of the collision circles
	//Explanation at http://en.wikipedia.org/wiki/Line_geometry - Check the Dual Coordinates section
	
	p01 = dk->col[0].y * dk->col[1].z - dk->col[1].y * dk->col[0].z;
	if(!p01)
	{
//		printf("p01 == 0\n");
		return -1;
	}
	p02 = dk->col[0].z * dk->col[1].x - dk->col[1].z * dk->col[0].x;
	p03 = dk->col[0].x * dk->col[1].y - dk->col[1].x * dk->col[0].y;	
	p23 = dk->col[0].w * dk->col[1].x - dk->col[1].w * dk->col[0].x;
	p31 = dk->col[0].w * dk->col[1].y - dk->col[1].w * dk->col[0].y;
	p12 = dk->col[0].w * dk->col[1].z - dk->col[1].w * dk->col[0].z;
	
//		printf("p %f %f %f %f\n", p01, p02, p03, p23, p31, p12);

	//Now that we've got the Plucker coords, start building the quadratic equation as
	//laid out in the paper		
	p2 = p01 * p01;
	
	a = p2 + (p02 * p02) + (p03 * p03);
	b1 = p12 + p01 * dk->knee[0].y;
	b2 = p31- p01 * dk->knee[0].z;
	b = p02 * b1 - p03 * b2;
	
	c = (b1 * b1) + (b2 * b2) - (p2 * (dk->re * dk->re));
	ar = (b * b) - (a * c);
	if(ar < 0) 
	{
//		printf("ar < 0\n");
		return -1;
	}		
	d = sqrt(ar);
	
//		printf("quad %f %f %f %f\n", a, b, c, d);

	//Solve for X, then use the univariate X to find y and z
	dk->solution[0].x = (b + d) / a;
	dk->solution[0].y = (p02 * dk->solution[0].x - p12) / p01;
	dk->solution[0].z = (p03 * dk->solution[0].x + p31) / p01;

	//Remember, a quadratic gives you back two solutions.
	dk->solution[1].x = (-b + d) / a;
	dk->solution[1].y = -(( p02 * dk->solution[1].x - p12) / p01);
	dk->solution[1].z = -(( p03 * dk->solution[1].x + p31)  /  p01);
	dk->solution[1].x = -dk->solution[1].x;
	return 0;
}

int nifalcon_inverse_kinematics(falcon_kinematics* dk, int effector_x, int effector_y, int effector_z)
{
	return 1;
}
