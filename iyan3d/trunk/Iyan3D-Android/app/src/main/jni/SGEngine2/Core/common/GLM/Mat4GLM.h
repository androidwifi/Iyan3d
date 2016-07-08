//
//  Mat4GLM.hpp
//  Iyan3D
//
//  Created by Karthik on 25/04/16.
//  Copyright © 2016 Smackall Games. All rights reserved.
//

#ifdef OPTIMGLM

#ifndef Mat4GLM_h
#define Mat4GLM_h

#include "Vector4GLM.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#ifdef ANDROID
#include "Utilities/Maths.h"
#elif IOS
#include "Maths.h"
#elif UBUNTU
#include "Maths.h"
#include <string.h>
#endif



class Mat4 {
    
public:
    glm::mat4 matrix;
    Mat4();
    Mat4(float* pMat);
    Mat4(const Vector3& axis_x, const Vector3& axis_y, const Vector3& axis_z, const Vector3& trans);
    ~Mat4();
    
    Mat4& operator=(const Mat4& m);
    bool operator==(const Mat4& m) const;
    Mat4& operator*=(const Mat4& m);
    Mat4 operator*(const Mat4& m) const;
    Vector4 operator*(const Vector4& v) const;
    
//    float& operator[](unsigned i);
    void setElement(unsigned int index, float value);
    float operator[](unsigned i) const;
    
    void perspective(float fov, float aspect, float nearz, float farz);
    void ortho(float left, float right, float top, float bottom, float nearz, float farz);
    void translate(float x, float y, float z);
    void translate(const Vector3& v);
    void scale(float x, float y, float z);
    void scale(const Vector3& v);
    void scale(float s);
    void copyMatTo(float *pointer);
    //    void rotate(const Quaternion &q);
    void setRotationRadians(Vector3 rotation);
    void bias();
    bool invert();
    void transpose();
    float* pointer();
    void buildCameraLookAtMatrixLH(Vector3 position, Vector3 target, Vector3 upVector);
    void buildProjectionMatrixPerspectiveFovLH(float fieldOfViewRadians, float aspectRatio, float zNear, float zFar);
    Vector3 getTranslation();
    Vector3 getRotationInDegree();
    Vector3 getRotation();
    Vector3 getScale();
    Mat4 setbyproduct(Mat4& other_a, Mat4& other_b);
    void rotateVect(Vector3& vect) const;
};

#endif /* Mat4GLK_hpp */

#endif