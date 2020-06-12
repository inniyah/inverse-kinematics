#pragma once

#ifndef SEGMENT_H_A2B06138_AC93_11EA_BA08_A7E4782DC242
#define SEGMENT_H_A2B06138_AC93_11EA_BA08_A7E4782DC242

#include "include.h"

typedef enum {
    BALLJOINT
} JointType;

class Segment {
    private:
        int segment_id;
        bool blocked;

        // magnitude of the segment
        float mag;

        // transformation matrix (rotation) of the segment
        AngleAxisf T, saved_T, last_T;

        // save the angle when computing the changes
        Vector3f saved_angle;

        // the type of joint the origin of the segment is
        // connected to
        JointType joint;

        Point3f start_point;

    public:
        // constructors
        Segment(int id);
        Segment(int id, float magnitude, JointType jt = BALLJOINT);
        Segment(int id, float magnitude, float angle, const Vector3f &axis, JointType jt = BALLJOINT);
        Segment(int id, const Vector3f &v, JointType jt = BALLJOINT);

        inline int get_segment_id() const {
            return segment_id;
        }

        inline bool get_blocked() const {
            return blocked;
        }

        inline void set_blocked(bool b) {
            blocked = b;
        }

        // set the position of the starting point
        void set_start_point(Point3f pos);

        // get the position of the starting point
        Point3f get_start_point();

        // returns end point in object space
        Point3f get_end_point();

        // draw takes in the startpoint and returns the endpoint
        Point3f draw(int first, int last, int seg_count = 7);

        inline Point3f draw(int seg_count = 7) {
            return draw(0, seg_count, seg_count);
        }

        Vector3f get_right();
        Vector3f get_up();
        Vector3f get_z();

        AngleAxisf get_T();
        Vector3f get_axis();
        float get_angle();
        Vector4f get_quat();

        float get_mag();

        void save_transformation();
        void load_transformation();

        void save_last_transformation();
        void load_last_transformation();

        void apply_angle_change(float rad_change, Vector3f angle);

        // clear transformations
        void reset();

        // randomize transformation
        void randomize();

        // apply transformation
        void transform(AngleAxisf t);

        std::string name;
        std::string parent_name;
};

#endif // SEGMENT_H_A2B06138_AC93_11EA_BA08_A7E4782DC242
