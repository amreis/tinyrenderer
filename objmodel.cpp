#include "objmodel.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>

bool ObjModel::Load(std::string_view path) {
    std::ifstream file{path.data()};
    if (file.fail()) {
        std::cerr << "OOOOPs, something went wrong" << std::endl;
        return false;
    }
    std::string line;

    while (std::getline(file, line)) {
        if (line.starts_with("v ")) {
            // Vertex line
            std::stringstream ss(line.substr(2));

            float x, y, z;
            ss >> x >> y >> z;

            this->vertices.emplace_back(x, y, z);
        } else if (line.starts_with("f ")) {
            // Face line
            std::stringstream ss(line);
            auto pos = line.find(' ');
            auto newpos = pos;
            // -1 at the end since OBJ are 1-based.
            int f1 = std::stoi(line.substr(pos + 1, newpos = line.find('/', pos + 1))) - 1;

            // 'f 0123/123/3 124/5/76 5/433/21'

            pos = line.find(' ', newpos);
            newpos = line.find('/', pos + 1);
            int f2 = std::stoi(line.substr(pos + 1, newpos)) - 1;

            pos = line.find(' ', newpos);
            newpos = line.find('/', pos + 1);
            int f3 = std::stoi(line.substr(pos + 1, newpos)) - 1;

            this->faces.emplace_back(f1, f2, f3);
        }

        // We ignore other types of lines for now.
    }
    return true;
}