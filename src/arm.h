#pragma once

#ifndef ARM_H_A2B05F3A_AC93_11EA_BA05_876B23B9AF26
#define ARM_H_A2B05F3A_AC93_11EA_BA05_876B23B9AF26

#include "include.h"
#include "segment.h"

class Arm {
    private:
        vector<Segment*> segments;
        vector<Segment*> all_segments;

        Point3f abs_base;
        Point3f rel_base;

        Matrix<float,1,3> compute_jacobian_segment(int seg_num, Point3f goal_point, Vector3f angle);

        // computes end_effector up to certain number of segments
        Point3f calculate_end_effector(int segment_num = -1);

        // get the total magnitude of all the segments in the arm
        float get_max_length();

    public:
        // constructors
        Arm();
        Arm(vector<Segment*> segs, Point3f pos);

        // set the position of base
        void set_base(Point3f pos);

        // get the position of the base
        Point3f get_base();

        // set the segments
        void set_segments(vector<Segment*> segs);

        // solve the arm for some point
        void solve(Point3f goal_point, int life_count);

        // update the segment list
        void update_segments();

        // update the reference points of the segments
        void update_points();

        // draw the arm
        void draw();
};

#endif // ARM_H_A2B05F3A_AC93_11EA_BA05_876B23B9AF26
