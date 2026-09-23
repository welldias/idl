#include <iostream>
#include <string>
#include <vector>
#include <mix/api.h>

int main() {
    std::vector<std::string> partes = {"cxx", "mix"};
    std::cout << partes[0] << "+" << partes[1] << " dobro=" << c_dobro(21) << std::endl;
    return 0;
}
