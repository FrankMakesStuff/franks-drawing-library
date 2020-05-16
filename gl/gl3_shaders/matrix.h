// matrix.h
#pragma once

#include <math.h>

typedef struct matrix4f {
	float m00; float m01; float m02; float m03;
	float m10; float m11; float m12; float m13;
	float m20; float m21; float m22; float m23;
	float m30; float m31; float m32; float m33;
} matrix4f;

typedef struct vector2f {
	float u;
	float v;
} vector2f;

typedef struct vector3f {
	float x;
	float y;
	float z;
} vector3f;

typedef struct vector4f {
	float x;
	float y;
	float z;
	float w;
} vector4f;

double getSquare( float val ){
	return (double)(val * val);	
}

double vec3Dist( vector3f v1, vector3f v2 ){
	return (double)sqrt( getSquare( v2.x - v1.x ) + getSquare( v2.y - v1.y ) + getSquare( v2.z - v1.z ) );
}

void vec3Normalize( vector3f &v ){
	double l = vec3Dist( (vector3f){0.0f, 0.0f, 0.0f}, v );
	v.x /= l;
	v.y /= l;
	v.z /= l;
}

double DotProduct( vector3f v1, vector3f v2 ){
	return (double)(v1.x * v2.x) + (v1.y * v2.y) + (v1.z * v2.z);	
}

vector3d CrossProduct( vector3f v1, vector 3f v2 ){
	return { (v1.y * v2.z) - (v1.z * v2.y),
			 (v1.z * v2.x) - (v1.x * v2.z),
			 (v1.x * v2.y) - (v1.y * v2.x) };	
}

matrix4f MatrixMultiply( matrix4f m1, matrix4f m2 ){
	return {
		(m1.m00*m2.m00) + (m1.m01*m2.m10) + (m1.m02*m2.m20) + (m1.m03*m2.m30), 
		(m1.m00*m2.m01) + (m1.m01*m2.m11) + (m1.m02*m2.m21) + (m1.m03*m2.m31), 
		(m1.m00*m2.m02) + (m1.m01*m2.m12) + (m1.m02*m2.m22) + (m1.m03*m2.m32), 
		(m1.m00*m2.m03) + (m1.m01*m2.m13) + (m1.m02*m2.m23) + (m1.m03*m2.m33),
		
		(m1.m10*m2.m00) + (m1.m11*m2.m10) + (m1.m12*m2.m20) + (m1.m13*m2.m30), 
		(m1.m10*m2.m01) + (m1.m11*m2.m11) + (m1.m12*m2.m21) + (m1.m13*m2.m31), 
		(m1.m10*m2.m02) + (m1.m11*m2.m12) + (m1.m12*m2.m22) + (m1.m13*m2.m32), 
		(m1.m10*m2.m03) + (m1.m11*m2.m13) + (m1.m12*m2.m23) + (m1.m13*m2.m33),
		
		(m1.m20*m2.m00) + (m1.m21*m2.m10) + (m1.m22*m2.m20) + (m1.m23*m2.m30), 
		(m1.m20*m2.m01) + (m1.m21*m2.m11) + (m1.m22*m2.m21) + (m1.m23*m2.m31), 
		(m1.m20*m2.m02) + (m1.m21*m2.m12) + (m1.m22*m2.m22) + (m1.m23*m2.m32), 
		(m1.m20*m2.m03) + (m1.m21*m2.m13) + (m1.m22*m2.m23) + (m1.m23*m2.m33),
		
		(m1.m30*m2.m00) + (m1.m31*m2.m10) + (m1.m32*m2.m20) + (m1.m33*m2.m30), 
		(m1.m30*m2.m01) + (m1.m31*m2.m11) + (m1.m32*m2.m21) + (m1.m33*m2.m31), 
		(m1.m30*m2.m02) + (m1.m31*m2.m12) + (m1.m32*m2.m22) + (m1.m33*m2.m32), 
		(m1.m30*m2.m03) + (m1.m31*m2.m13) + (m1.m32*m2.m23) + (m1.m33*m2.m33)
	};
}

matrix4f identityMatrix() {
	return {
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	};
}

vector4f MatVec4Multiply( matrix4f m, vector4f v ){
	return {
		(m.m00 * v.x) + (m.m01 * v.y) + (m.m02 * v.z) + (m.m03 * v.w),
		(m.m10 * v.x) + (m.m11 * v.y) + (m.m12 * v.z) + (m.m13 * v.w),
		(m.m20 * v.x) + (m.m21 * v.y) + (m.m22 * v.z) + (m.m23 * v.w),
		(m.m30 * v.x) + (m.m31 * v.y) + (m.m32 * v.z) + (m.m33 * v.w)
	};
}

vector3f MatVecMultiply( matrix4f m, vector3f v ){
	vector4f r = MatVec4Multiply( m, (vector4f){ v.x, v.y, v.z, 1.0f } );
	return { r.x, r.y, r.z };	
}

vector3f scaleVector( vector3f v, vector3f s ){
	matrix4f m = identityMatrix();
	m.m00 = s.x; m.m11 = s.y; m.m22 = s.z; m.m33 = 1.0f;
	return MatVecMultiply( m, v );
}

vector3f translateVector( vector3f v, vector3f t ){
	matrix4f m = identityMatrix();
	m.m03 = t.x; m.m13 = t.y; m.m23 = t.z; m.m33 = 1.0f;
	return MatVecMultiply( 	m, v );
}

vector3f rotXVector( vector3f v, float a ){
	matrix4f m = identityMatrix();
	m.m11 = cos( a ); m.m12 = -sin( a );
	m.m21 = sin( a ); m.m22 = cos( a );
	return MatVecMultiply( m, v );
}

vector3f rotYVector( vector3f v, float a ){
	matrix4f m = identityMatrix();
	m.m00 = cos( a ); m.m02 = sin( a );
	m.m10 = -sin( a ); m.m22 = cos( a );
	return MatVecMultiply( m, v );
}

vector3f rotZVector( vector3f v, float a ){
	matrix4f m = identityMatrix();
	m.m00 = cos( a ); m.m01 = -sin( a );
	m.m10 = sin( a ); m.m11 = cos( a );
	return MatVecMultiply( m, v );
}













