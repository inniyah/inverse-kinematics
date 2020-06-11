#include "segment.h"

#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <map> 
#include <algorithm>

enum class CSVState {
    UnquotedField,
    QuotedField,
    QuotedQuote
};

static const std::string WHITESPACE = " \n\r\t\f\v";

static std::string ltrim(const std::string & s) {
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
}

static std::string rtrim(const std::string & s) {
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

static std::string trim(const std::string & s) {
    return rtrim(ltrim(s));
}

std::vector<std::string> readCSVRow(const std::string &row) {
    CSVState state = CSVState::UnquotedField;

    const char *vinit[] = {""};
    std::vector<std::string> fields(vinit, std::end(vinit));

    size_t i = 0; // index of the current field
    for (char c : row) {
        switch (state) {
            case CSVState::UnquotedField:
                switch (c) {
                    case ',': // end of field
                              fields.push_back(""); i++;
                              break;
                    case '"': state = CSVState::QuotedField;
                              break;
                    default:  fields[i].push_back(c);
                              break; }
                break;
            case CSVState::QuotedField:
                switch (c) {
                    case '"': state = CSVState::QuotedQuote;
                              break;
                    default:  fields[i].push_back(c);
                              break; }
                break;
            case CSVState::QuotedQuote:
                switch (c) {
                    case ',': // , after closing quote
                              fields.push_back(""); i++;
                              state = CSVState::UnquotedField;
                              break;
                    case '"': // "" -> "
                              fields[i].push_back('"');
                              state = CSVState::QuotedField;
                              break;
                    default:  // end of quote
                              state = CSVState::UnquotedField;
                              break; }
                break;
        }
    }
    return fields;
}

/// Read CSV file, Excel dialect. Accept "quoted fields ""with quotes"""
std::vector<std::vector<std::string>> readCSV(std::istream &in) {
    std::vector<std::vector<std::string>> table;
    std::string row;
    while (!in.eof()) {
        std::getline(in, row);
        if (in.bad() || in.fail()) {
            break;
        }
        row = trim(row);
        if (!row.length()) continue;
        auto fields = readCSVRow(row);
        table.push_back(fields);
    }
    return table;
}

std::map<std::string, vector<Segment*> > readSkeletonFile(const std::string &filename) {
    std::ifstream csv_file(filename);
    std::vector<std::vector<std::string>> csv_data = readCSV(csv_file);

    std::vector<std::pair<std::string,std::string>> symmetries;
    std::vector<std::string> foot_bones;
    std::vector<std::string> fat_bones;

    std::map<std::string, vector<Segment*> > segments;
    std::map<std::string, Point3f > positions;

    int row_number = 0;
    int next_segment_id = 1;
    std::vector<std::string> csv_header;
    for (auto const & row_data: csv_data) {
        if (!row_number) {
            csv_header = row_data;
            for (std::vector<std::string>::size_type i = 0; i != row_data.size(); i++) {
                std::string & field_name = csv_header[i];
                transform(field_name.begin(), field_name.end(), field_name.begin(), ::toupper);
            }
        } else {
            std::string bone_name, parent_bone, symmetric_bone;
            float x_coord = 0.0f, y_coord = 0.0f, z_coord = 0.0f;
            bool is_foot = false, is_fat = false;

            for (std::vector<std::string>::size_type i = 0; i != row_data.size(); i++) {
                if (std::string("BONE") == csv_header[i])
                    bone_name = trim(row_data[i]);
                if (std::string("X") == csv_header[i])
                    x_coord = std::stof(trim(row_data[i]));
                if (std::string("Y") == csv_header[i])
                    y_coord = std::stof(trim(row_data[i]));
                if (std::string("Z") == csv_header[i])
                    z_coord = std::stof(trim(row_data[i]));
                if (std::string("PARENT") == csv_header[i])
                    parent_bone = trim(row_data[i]);
                if (std::string("SYMMETRIC") == csv_header[i])
                    symmetric_bone = trim(row_data[i]);
                if (std::string("FOOT") == csv_header[i])
                    is_foot = (trim(row_data[i]).length() != 0);
                if (std::string("FAT") == csv_header[i])
                    is_fat = (trim(row_data[i]).length() != 0);
            }
            if (bone_name.length() > 0 && symmetric_bone.length() > 0) {
                symmetries.push_back(make_pair(bone_name, symmetric_bone));
            }
            if (is_foot) {
                foot_bones.push_back(bone_name);
            }
            if (is_fat) {
                fat_bones.push_back(bone_name);
            }

            std::cout << "Bone: " << bone_name << " = " << x_coord << ", " << y_coord << ", " << z_coord << "; Parent = " << parent_bone << std::endl;

            positions[bone_name] = Vector3f(x_coord, y_coord, z_coord);

            if (parent_bone.length()) {
                for (unsigned int i = 0; i < segments[parent_bone].size(); i++) 
                    segments[bone_name].push_back(segments[parent_bone][i]); 
                Segment *segment = new Segment(next_segment_id++, (positions[bone_name] - positions[parent_bone]));
                segment->name = bone_name;
                segment->parent_name = parent_bone;
                segments[bone_name].push_back(segment);
            }

        }
        row_number++;
    }

    for (auto const & symmetry: symmetries) {
        std::cout << "Symmetry: " << symmetry.first << " <-> " << symmetry.second << std::endl;
    }

    for (auto const & bone: foot_bones) {
        std::cout << "Foot Bone: " << bone << std::endl;
    }

    for (auto const & bone: fat_bones) {
        std::cout << "Fat Bone: " << bone << std::endl;
    }

    return segments;
}

std::map<std::string, vector<Segment*> > readPoseFile(const std::string &filename) {
    std::ifstream csv_file(filename);
    std::vector<std::vector<std::string>> csv_data = readCSV(csv_file);

    std::map<std::string, vector<Segment*> > segments;

    int row_number = 0;
    int next_segment_id = 1;
    std::vector<std::string> csv_header;
    for (auto const & row_data: csv_data) {
        if (!row_number) {
            csv_header = row_data;
            for (std::vector<std::string>::size_type i = 0; i != row_data.size(); i++) {
                std::string & field_name = csv_header[i];
                transform(field_name.begin(), field_name.end(), field_name.begin(), ::toupper);
            }
        } else {
            std::string bone_name, parent_bone, symmetric_bone;
            float begin_x = 0.0f, begin_y = 0.0f, begin_z = 0.0f, magnitude = 0.0f, angle = 0.0f, axis_x = 0.0f, axis_y = 0.0f, axis_z = 0.0f;

            for (std::vector<std::string>::size_type i = 0; i != row_data.size(); i++) {
                if (std::string("BONE") == csv_header[i])
                    bone_name = trim(row_data[i]);
                if (std::string("BEGINX") == csv_header[i])
                    begin_x = std::stof(trim(row_data[i]));
                if (std::string("BEGINY") == csv_header[i])
                    begin_y = std::stof(trim(row_data[i]));
                if (std::string("BEGINZ") == csv_header[i])
                    begin_z = std::stof(trim(row_data[i]));
                if (std::string("PARENT") == csv_header[i])
                    parent_bone = trim(row_data[i]);
                if (std::string("MAGNITUDE") == csv_header[i])
                    magnitude = std::stof(trim(row_data[i]));
                if (std::string("ANGLE") == csv_header[i])
                    angle = std::stof(trim(row_data[i]));
                if (std::string("AXISX") == csv_header[i])
                    axis_x = std::stof(trim(row_data[i]));
                if (std::string("AXISY") == csv_header[i])
                    axis_y = std::stof(trim(row_data[i]));
                if (std::string("AXISZ") == csv_header[i])
                    axis_z = std::stof(trim(row_data[i]));
            }

            std::cout << "Bone: " << bone_name << " = " << begin_x << ", " << begin_y << ", " << begin_z << "; Parent = " << parent_bone << std::endl;

            if (parent_bone.length()) {
                for (unsigned int i = 0; i < segments[parent_bone].size(); i++)
                    segments[bone_name].push_back(segments[parent_bone][i]);
                Segment *segment = new Segment(next_segment_id++, magnitude, angle, Vector3f(axis_x, axis_y, axis_z));
                segment->name = bone_name;
                segment->parent_name = parent_bone;
                segments[bone_name].push_back(segment);
            }

        }
        row_number++;
    }

    return segments;
}
