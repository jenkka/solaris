/*
 * Transforms.c
 *
 *  Created on: 07/09/2016
 *      Author: Cursos
 */

#include "Transforms.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define toRadians(deg) deg * M_PI / 180.0

struct strNode {
	Mat4* matrix;
	struct strNode* prior;
};

typedef struct strNode* Node;

#define OK 0
#define EMPTY -1

Node top = NULL;

void loadIdentity(Mat4* toIdentity) {
	mIdentity(toIdentity);
	Node tmpNode;
	while(top != NULL) {
		tmpNode = top;
		top = top->prior;
		free(tmpNode->matrix);
		free(tmpNode);
	}
}

void pushMatrix(Mat4* csMatrix) {
	Node node = (Node) malloc(sizeof(struct strNode));
	Mat4* mat = (Mat4*) malloc(sizeof(Mat4));
	int i;
	for(i = 0; i < 16; i ++) mat->values[i] = csMatrix->values[i];
	node->matrix  = mat;
	node->prior = top;
	top = node;
}

void popMatrix(Mat4* csMatrix) {
	if(top == NULL) return;
	int i;
	for(i = 0; i < 16; i ++) csMatrix->values[i] = top->matrix->values[i];
	Node toPop = top;
	top = top->prior;
	free(toPop->matrix);
	free(toPop);
}

void rotateAll(Mat4* csMatrix, float degreesX, float degreesY, float degreesZ) {
	Mat4 rotMatrix;
	mIdentity(&rotMatrix);
	float radiansX = degreesX * M_PI / 180;
	float radiansY = degreesY * M_PI / 180;
	float radiansZ = degreesZ * M_PI / 180;

	rotMatrix.at[0][0] =  cos(radiansZ)*cos(radiansY) + sin(radiansZ)*sin(radiansX)*sin(radiansY);
	rotMatrix.at[0][1] =  sin(radiansZ)*cos(radiansX);
	rotMatrix.at[0][2] = -cos(radiansZ)*sin(radiansY) + sin(radiansZ)*sin(radiansX)*cos(radiansY);

	rotMatrix.at[1][0] = -sin(radiansZ)*cos(radiansY) + cos(radiansZ)*sin(radiansX)*sin(radiansY);
	rotMatrix.at[1][1] =  cos(radiansZ)*cos(radiansX);
	rotMatrix.at[1][2] =  sin(radiansZ)*sin(radiansZ) + cos(radiansZ)*sin(radiansX)*cos(radiansY);

	rotMatrix.at[2][0] = cos(radiansX)*sin(radiansY);
	rotMatrix.at[2][1] = -sin(radiansX);
	rotMatrix.at[2][2] = cos(radiansX)*cos(radiansY);

	mMult(csMatrix, rotMatrix);
//	mPrint(*csMatrix);
}

void setOrtho (Mat4* matrix, float left, float right, float bottom, float top,
		              float near, float far) {
	mIdentity(matrix);

	matrix->at[0][0] = 2/(right - left);
	matrix->at[0][3] = (-right - left)/(right - left);
	matrix->at[1][1] = 2/(top - bottom);
	matrix->at[1][3] = (-top - bottom)/(top - bottom);
	matrix->at[2][2] = 2/(near - far);
	matrix->at[2][3] = (-near - far)/(near - far);
}

void setPerspective(Mat4* matrix, float fovy,  float aspect,
		                          float nearZ, float farZ) {
	mIdentity(matrix);
	float angle = tan(toRadians(fovy/2));
	matrix->at[0][0] = 1 / (aspect * angle);
	matrix->at[1][1] = 1 / angle;
	matrix->at[2][2] = (nearZ + farZ) / (nearZ - farZ);
	matrix->at[2][3] = (-2 * nearZ * farZ) / (nearZ - farZ);
	matrix->at[3][2] = -1;
	matrix->at[3][3] = 0;
}


void translate(Mat4 *m, float tx, float ty, float tz) {
//	Construir matriz de traslación tm
	Mat4 tm;
	mIdentity(&tm);
	tm.at[0][3] = tx;
	tm.at[1][3] = ty;
	tm.at[2][3] = tz;
//	Multiplicar mt por la matriz recibida m
//	El resultado almacenarlo en m
	mMult(m, tm);
}

void rotateX(Mat4 *m, float angle) {
	float rad = toRadians(angle);
	Mat4 rm;
	mIdentity(&rm);
	rm.at[1][1] =  cos(rad);
	rm.at[1][2] = -sin(rad);
	rm.at[2][1] =  sin(rad);
	rm.at[2][2] =  cos(rad);
	mMult(m, rm);
}

void rotateY(Mat4 *m, float angle) {
	float rad = toRadians(angle);
	Mat4 rm;
	mIdentity(&rm);
	rm.at[0][0] =  cos(rad);
	rm.at[0][2] =  sin(rad);
	rm.at[2][0] = -sin(rad);
	rm.at[2][2] =  cos(rad);
	mMult(m, rm);
}

void rotateZ(Mat4 *m, float angle) {
	float rad = toRadians(angle);
	Mat4 rm;
	mIdentity(&rm);
	rm.at[0][0] =  cos(rad);
	rm.at[0][1] = -sin(rad);
	rm.at[1][0] =  sin(rad);
	rm.at[1][1] =  cos(rad);
	mMult(m, rm);
}

void scale(Mat4 *m, float sx, float sy, float sz) {
	Mat4 sm;
	mIdentity(&sm);
	sm.at[0][0] = sx;
	sm.at[1][1] = sy;
	sm.at[2][2] = sz;
	mMult(m, sm);
}

float det2x2(float m[4]) {
	return m[0] * m[3] - m[1] * m[2];
}

float det3x3(float m[9]) {
	float m1[] = {m[4], m[5], m[7], m[8]};
    float d1 = m[0] * det2x2(m1);
    float m2[] = {m[3], m[5], m[6], m[8]};
    float d2 = m[1] * det2x2(m2);
    float m3[] = {m[3], m[4], m[6], m[7]};
    float d3 = m[2] * det2x2(m3);
    return d1 - d2 + d3;
}

float determinant(Mat4 m) {
	float m1[] = {m.at[1][1], m.at[2][1], m.at[3][1], m.at[1][2], m.at[2][2], m.at[3][2], m.at[1][3], m.at[2][3], m.at[3][3]};
    float d1 = m.at[0][0] * det3x3(m1);
    float m2[] = {m.at[0][1], m.at[2][1], m.at[3][1], m.at[0][2], m.at[2][2], m.at[3][2], m.at[0][3], m.at[2][3], m.at[3][3]};
    float d2 = m.at[1][0] * det3x3(m2);
    float m3[] = {m.at[0][1], m.at[1][1], m.at[3][1], m.at[0][2], m.at[1][2], m.at[3][2], m.at[0][3], m.at[1][3], m.at[3][3]};
    float d3 = m.at[2][0] * det3x3(m3);
    float m4[] = {m.at[0][1], m.at[1][1], m.at[2][1], m.at[0][2], m.at[1][2], m.at[2][2], m.at[0][3], m.at[1][3], m.at[2][3]};
    float d4 = m.at[3][0] * det3x3(m4);
    return d1 - d2 + d3 - d4;
}

void transpose(Mat4 s, Mat4* t) {
	int r, c;
    for(r = 0; r < 4; r ++) {
    	for(c = 0; c < 4; c ++) {
    		t->at[c][r] = s.at[r][c];
    	}
    }
}

void adjoint(Mat4 s, Mat4* t) {
	int r, c, rr, cc;
	for(r = 0; r < 4; r ++) {
		for(c = 0; c < 4; c ++) {
			float m3[9];
			int i = 0;
			for(rr = 0; rr < 4; rr ++) {
				if(rr == r) continue;
				for(cc = 0; cc < 4; cc ++) {
					if(cc == c) continue;
					m3[i ++] = s.at[cc][rr];
				}
			}
			int sign = (c + r) % 2 == 0? 1 : -1;
			t->at[c][r] = sign * det3x3(m3);
		}
	}
}

void inverse(Mat4 s, Mat4* t) {
	Mat4 adj;
	adjoint(s, &adj);
	transpose(adj, t);
	float d = determinant(s);
	int i, j;
	for(i = 0; i < 4; i ++) {
		for(j = 0; j < 4; j ++) {
			t->at[i][j] /= d;
		}
	}
}
