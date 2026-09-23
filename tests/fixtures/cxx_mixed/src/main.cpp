#include <iostream>
#include <string>
#include <vector>
#include <mix/api.h>

int main() {
    std::vector<std::string> parts = {"cxx", "mix"};
    std::cout << parts[0] << "+" << parts[1] << " double=" << c_double(21) << std::endl;
    return 0;
}
