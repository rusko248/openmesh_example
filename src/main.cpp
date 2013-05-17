#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif

#include <OpenMesh/Core/IO/Options.hh>
#include <OpenMesh/Core/IO/MeshIO.hh>
#include <iostream>
#include <cmath>
#include <cstdio>
#include <Windows.h>
#include "mesh_definitions.h"

using namespace std;
using namespace OpenMesh;

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/freeglut.h>
#endif

Mesh mesh;

bool leftDown = false, rightDown = false, middleDown = false;
int lastPos[2];
float cameraPos[4] = {0,0,4,1};
Vec3f up(0,1,0), pan(0,0,0);
int windowWidth = 640, windowHeight = 480;
bool showSurface = true, showAxes = true;
bool showNormals = false, showMesh = false, showLighting = true;

float specular[] = { 1.0, 1.0, 1.0, 1.0 };
float shininess[] = { 50.0 };

#define PI 3.14159265

double mark_length = 0.02;

void fitSphere() {
	// Move center of mass to origin
	Vec3f center(0,0,0);
	for (Mesh::ConstVertexIter vIt = mesh.vertices_begin(); vIt != mesh.vertices_end(); ++vIt) center += mesh.point(vIt);
	center /= mesh.n_vertices();
	for (Mesh::VertexIter vIt = mesh.vertices_begin(); vIt != mesh.vertices_end(); ++vIt) mesh.point(vIt) -= center;

	// Fit in the unit sphere
	float maxLength = 0;
	for (Mesh::ConstVertexIter vIt = mesh.vertices_begin(); vIt != mesh.vertices_end(); ++vIt) maxLength = max(maxLength, mesh.point(vIt).length());
	for (Mesh::VertexIter vIt = mesh.vertices_begin(); vIt != mesh.vertices_end(); ++vIt) mesh.point(vIt) /= maxLength;
}

void renderMesh() {
	if (!showSurface) glColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE); // render regardless to remove hidden lines
	
	glEnable(GL_LIGHTING);
	glLightfv(GL_LIGHT0, GL_POSITION, cameraPos);
	if (!showLighting) glDisable(GL_LIGHTING);

	glDepthRange(0.001,1);
	glEnable(GL_NORMALIZE);

	for (Mesh::ConstFaceIter it = mesh.faces_begin(); it != mesh.faces_end(); ++it) {
		Vec3f nA, nB, nC;
		Vec3f pA, pB, pC;
		Mesh::ConstFaceVertexIter cfv_it = mesh.cfv_iter(it.handle());
		nA = mesh.normal(cfv_it.handle());
		pA = mesh.point(cfv_it.handle());
		nB = mesh.normal((++cfv_it).handle());
		pB = mesh.point(cfv_it.handle());
		nC = mesh.normal((++cfv_it).handle());
		pC = mesh.point(cfv_it.handle());
		glBegin(GL_TRIANGLES);
		glColor3f(0.7,0.7,0.7);
		glNormal3f(nA[0],nA[1],nA[2]);
		glVertex3f(pA[0],pA[1],pA[2]);
		glNormal3f(nB[0],nB[1],nB[2]);
		glVertex3f(pB[0],pB[1],pB[2]);
		glNormal3f(nC[0],nC[1],nC[2]);
		glVertex3f(pC[0],pC[1],pC[2]);
		glEnd();
	}
	
	if (!showSurface) glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
	
	glDisable(GL_LIGHTING);
	glDepthRange(0,0.999);
	
	Vec3f actualCamPos(cameraPos[0]+pan[0],cameraPos[1]+pan[1],cameraPos[2]+pan[2]);
	
	glBegin(GL_LINES);
	glColor3f(0,0,0);
	glLineWidth(2.0f);
	for (Mesh::ConstEdgeIter it = mesh.edges_begin(); it != mesh.edges_end(); ++it)
		if (showMesh) {
			Mesh::HalfedgeHandle h0 = mesh.halfedge_handle(it,0);
			Mesh::HalfedgeHandle h1 = mesh.halfedge_handle(it,1);
			Vec3f source(mesh.point(mesh.from_vertex_handle(h0)));
			Vec3f target(mesh.point(mesh.from_vertex_handle(h1)));
			glVertex3f(source[0],source[1],source[2]);
			glVertex3f(target[0],target[1],target[2]);
		}
	glEnd();
	
	if (showNormals) {
		glBegin(GL_LINES);
		glColor3f(0,1,0);
		for (Mesh::ConstVertexIter it = mesh.vertices_begin(); it != mesh.vertices_end(); ++it) {
			Vec3f n = mesh.normal(it.handle());
			Vec3f p = mesh.point(it.handle());
			Vec3f d = p + n*2*mark_length;
			glVertex3f(p[0],p[1],p[2]);
			glVertex3f(d[0],d[1],d[2]);
		}
		glEnd();
	}
	
	glDepthRange(0,1);
}

void display() {
	glClearColor(1,1,1,1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glShadeModel(GL_SMOOTH);
	glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
	glMaterialfv(GL_FRONT, GL_SHININESS, shininess);
	glEnable(GL_LIGHT0);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glViewport(0,0,windowWidth,windowHeight);
	
	float ratio = (float)windowWidth / (float)windowHeight;
	gluPerspective(50, ratio, 1, 1000); // 50 degree vertical viewing angle, zNear = 1, zFar = 1000
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(cameraPos[0]+pan[0], cameraPos[1]+pan[1], cameraPos[2]+pan[2], pan[0], pan[1], pan[2], up[0], up[1], up[2]);
	
	// Draw mesh
	renderMesh();

	// Draw axes
	if (showAxes) {
		glDisable(GL_LIGHTING);
		glBegin(GL_LINES);
		glLineWidth(1);
			glColor3f(1,0,0); glVertex3f(0,0,0); glVertex3f(1,0,0); // x axis
			glColor3f(0,1,0); glVertex3f(0,0,0); glVertex3f(0,1,0); // y axis
			glColor3f(0,0,1); glVertex3f(0,0,0); glVertex3f(0,0,1); // z axis
		glEnd(/*GL_LINES*/);
	}

	glutSwapBuffers();
}

void mouse(int button, int state, int x, int y) {
	if (button == GLUT_LEFT_BUTTON) leftDown = (state == GLUT_DOWN);
	else if (button == GLUT_RIGHT_BUTTON) rightDown = (state == GLUT_DOWN);
	else if (button == GLUT_MIDDLE_BUTTON) middleDown = (state == GLUT_DOWN);
	
	lastPos[0] = x;
	lastPos[1] = y;
}

void mouseMoved(int x, int y) {
	int dx = x - lastPos[0];
	int dy = y - lastPos[1];
	Vec3f curCamera(cameraPos[0],cameraPos[1],cameraPos[2]);
	Vec3f curCameraNormalized = curCamera.normalized();
	Vec3f right = up % curCameraNormalized;

	if (leftDown) {
		// Assume here that up vector is (0,1,0)
		Vec3f newPos = curCamera - 2*(float)((float)dx/(float)windowWidth) * right + 2*(float)((float)dy/(float)windowHeight) * up;
		newPos = newPos.normalized() * curCamera.length();
		
		up = up - (up | newPos) * newPos / newPos.sqrnorm();
		up.normalize();
		
		for (int i = 0; i < 3; i++) cameraPos[i] = newPos[i];
	}
	else if (rightDown) for (int i = 0; i < 3; i++) cameraPos[i] *= pow(1.1,dy*.1);
	else if (middleDown) {
		pan += -2*(float)((float)dx/(float)windowWidth) * right + 2*(float)((float)dy/(float)windowHeight) * up;
	}

	
	lastPos[0] = x;
	lastPos[1] = y;
	
	Vec3f actualCamPos(cameraPos[0]+pan[0],cameraPos[1]+pan[1],cameraPos[2]+pan[2]);
	
	glutPostRedisplay();
}

void keyboard(unsigned char key, int x, int y) {
	Vec3f actualCamPos(cameraPos[0]+pan[0],cameraPos[1]+pan[1],cameraPos[2]+pan[2]);

	if (key == 's' || key == 'S') showSurface = !showSurface;
	else if (key == 'a' || key == 'A') showAxes = !showAxes;
	else if (key == 'n' || key == 'N') showNormals = !showNormals;
	else if (key == 'm' || key == 'M') showMesh = !showMesh;
	else if (key == 'l' || key == 'L') showLighting = !showLighting;
	else if (key == 'q' || key == 'Q') exit(0);

	glutPostRedisplay();
}

void reshape(int width, int height) {
	windowWidth = width;
	windowHeight = height;
	glutPostRedisplay();
}

int main(int argc, char** argv) {
	if (argc < 2) {
		cout << "Usage: " << argv[0] << " obj_filename\n";
		exit(1);
	}
	
	IO::Options opt;
	opt += IO::Options::VertexNormal;
	opt += IO::Options::FaceNormal;
	
	mesh.request_face_normals();
	mesh.request_vertex_normals();
	
	if ( !IO::read_mesh(mesh, argv[1], opt )) {
		cout << "Read failed.\n";
		exit(1);
	}
	
	//mesh.update_normals();

	fitSphere();

	glutInit(&argc, argv); 
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH); 
	glutInitWindowSize(windowWidth, windowHeight); 
	glutCreateWindow(argv[0]);

	glutDisplayFunc(display);
	glutMotionFunc(mouseMoved);
	glutMouseFunc(mouse);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyboard);

	glutMainLoop();
	
	return 0;
}
