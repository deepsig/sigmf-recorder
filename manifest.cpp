//
// Created by Daniel Brotman on 2019-02-14.
//

//#include <sigmf.h>

#include <iostream>
#include <fstream>
#include <ctype.h>

//using json = nlohmann::json;
void create_manifest();

int main(int argc, char *argv[]) {
    std::cout << "SigMF Manifest Generator" << std::endl;
    //bool more_manifests = true;
    for(;;){
        create_manifest();
        char arg;
        bool valid_arg = false;
        while(!valid_arg) {
            std::cout << "Would you like to create another manifest file? (y/n): " << std::endl;
            std::cin >> arg;
            std::cout << arg << std::endl;
            arg = tolower(arg);
            switch (arg) {
                case 'y':
                    valid_arg = true;
                    break;
                case 'n':
                    return 0;
                default:
                    std::cout << "Invalid Option!" << std::endl;
            }
        }
    }



}

void create_manifest(){
    //std::ofstream mf(fname, std::ios::out);
    std::string str_arg;
    char char_arg;
    int int_arg;
    std::cout << "Name of File (without extension):" << std::endl;


}