#include "segment.h"

#include <cmath>
#include <GL/gl.h>

Segment::Segment(int id) : segment_id(id), blocked(false) {
    start_point = Point3f(0, 0, 0);
    T = T.Identity();
}

Segment::Segment(int id, float magnitude, JointType jt) : Segment(id) {
    mag = magnitude;
    joint = jt;
}

Segment::Segment(int id, const Vector3f &v, JointType jt) : segment_id(id), blocked(false) {
    start_point = Point3f(0, 0, 0);
    Vector3f vn = v.normalized();
    T = AngleAxisf( -acos(vn.dot(Vector3f::UnitZ())), vn.cross(Vector3f::UnitZ()) );
    mag = sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    joint = jt;
}

void Segment::set_start_point(Point3f pos) {
    start_point = pos;
}

Point3f Segment::get_start_point() {
    return start_point;
}

Point3f Segment::get_end_point() {
    Point3f a;

    // start with vector going into the Z direction
    a = Point3f(0, 0, mag);

    // transform into the rotation of the segment
    a = T * a;

    return a;
}

Point3f Segment::draw(int seg_count) {
    Point3f a0, a1, a2;

    // calculate the end point of the segment
    // start with vector going into the Z direction
    a2 = Point3f(0, 0, mag);

    // transform into the rotation of the segment
    a2 = T * a2;

    // translate the end point to the start point
    a2 += start_point;

    float scale = 0.1f * mag;

    // number of segments to divide the draw polygon into
    for (int i=0; i<seg_count; i++) {
        // a0 and a1 are points on the unit circle divided by seg_count
        // a0 is i+1 so the points go in counter-clockwise order
        a0 = Point3f(
            scale * cos(i * (2 * M_PI/seg_count)),
            scale * sin(i * (2 * M_PI/seg_count)),
            0);

        a1 = Point3f(
            scale * cos((i + 1) * (2 * M_PI/seg_count)),
            scale * sin((i + 1) * (2 * M_PI/seg_count)),
            0);

        // scale appropriately

        // transform a0 and a1 into the rotation of the segment
        a0 = T * a0;
        a1 = T * a1;

        // a2 is the end point of the segment
        // ^^calculated outside for loop

        // translate the points to the start point
        a0 += start_point;
        a1 += start_point;

        glBegin(GL_TRIANGLES);
            Vector3f n0 = ((a2 - a1).cross(a0 - a1)).normalized();
            glNormal3f(n0[0], n0[1], n0[2]);
            glVertex3f(a0[0], a0[1], a0[2]);
            glVertex3f(a1[0], a1[1], a1[2]);
            glVertex3f(a2[0], a2[1], a2[2]);

        glEnd();
    }

    return a2;
}

Vector3f Segment::get_right() {
    return T*Vector3f(1,0,0);
}

Vector3f Segment::get_up() {
    return T*Vector3f(0,1,0);
}

Vector3f Segment::get_z() {
    return T*Vector3f(0,0,1);
}

AngleAxisf Segment::get_T() {
    return T;
}

float Segment::get_mag() {
    return mag;
}

void Segment::save_last_transformation() {
    last_T = T;
}

void Segment::load_last_transformation() {
    T = last_T;
}

void Segment::save_transformation() {
    saved_T = T;
}

void Segment::load_transformation() {
    T = saved_T;
}

void Segment::apply_angle_change(float rad_change, Vector3f angle) {
    T = AngleAxisf(rad_change, angle) * T;
}

void Segment::transform(AngleAxisf t) {
    T = t * T;
}

void Segment::reset() {
    T = T.Identity();
}

void Segment::randomize() {
    T = AngleAxisf(M_PI / 2., Vector3f::Random()) * T;
}
