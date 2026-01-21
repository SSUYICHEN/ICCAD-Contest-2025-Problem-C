#include "headers.h"

int main(int argc, char** argv) {
    if (argc < 3) {
        cout << "Usage: ./discal <init_def_file> <new_def_file>" << endl;
        cout << "Example: ./opt ./ICCAD25_PorbC/aes_cipher_top/aes_cipher_top.def pd25.def" << endl;        // ./opt ../ICCAD25_PorbC
        exit(-1);
    }
    
    auto [init_locations, cell2lib] = getInitData(argv[1]);
    unordered_map<string, pair<int, int>> new_location;
    updatePlacementHelper(new_location, argv[2]);
    cout << "average displacement = " << calDisplacement(init_locations, new_location) << endl;    
}