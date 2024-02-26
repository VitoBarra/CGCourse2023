
#include <iostream>
#include <fstream>


static std::string textFileRead(const char *fn) {
    std::ifstream ifs(fn);
    std::string content((std::istreambuf_iterator<char>(ifs)),
                        (std::istreambuf_iterator<char>()));
    if (content.empty()) {
        std::cout << "No content for " << fn << std::endl;
        exit(0);
    }
    return content;
}