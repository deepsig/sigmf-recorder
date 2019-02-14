//
// Created by Daniel Brotman on 2019-02-14.
//
#include <sigmf.h>
#include <flatbuffers_json_visitor.h>

#include <iostream>
#include <fstream>
#include <ctype.h>

//using json = nlohmann::json;
void create_manifest();
json manifest_capture();

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
    std::cin >> str_arg;
    std::ofstream mf(str_arg + ".manifest", std::ios::out);
    json j = {};
    for(;;){
        char arg;
        bool valid_arg = false;
        while(!valid_arg) {
            json capture = manifest_capture();
            j.push_back(capture);
            std::cout << "Would you like to create add another capture? (y/n): " << std::endl;
            std::cin >> arg;
            std::cout << arg << std::endl;
            arg = tolower(arg);
            switch (arg) {
                case 'y':
                    valid_arg = true;
                    break;
                case 'n':
                    mf << j.dump(4) << std::endl;
                    mf.close();
                    return;
                default:
                    std::cout << "Invalid Option!" << std::endl;
            }
        }
    }


}

json manifest_capture(){
    json j;
    std::string str_arg;

    std::cout << "Name of Capture:" << std::endl;
    std::cin >> str_arg;
    std::string capture_name(str_arg);
    j[capture_name] = {};
    str_arg.clear();

    std::cout << "Center Frequency in MHz (Default: 880 MHz): " << std::endl;
    std::cin >> str_arg;
    j[capture_name]["freq"] = str_arg.empty() ? 880 : std::stoi(str_arg);
    str_arg.clear();

    std::cout << "Sampling Rate in MSPS (Default: 40 MSPS): " << std::endl;
    std::cin >> str_arg;
    j[capture_name]["rate"] = str_arg.empty() ? 40 : std::stoi(str_arg);
    str_arg.clear();

    std::cout << "Gain in dB (Default: 45 dB): " << std::endl;
    std::cin >> str_arg;
    j[capture_name]["gain"] = str_arg.empty() ? 45 : std::stoi(str_arg);
    str_arg.clear();

    std::cout << "Number of Samples (Default: 100000000 i.e. 1e8): " << std::endl;
    std::cin >> str_arg;
    j[capture_name]["samples"] = str_arg.empty() ? 880 : std::stoi(str_arg);
    str_arg.clear();

    std::cout << "Datatype enter a number (Default: 1. ci16_le): " << std::endl
            << "\t1. ci16_le" << std::endl
            << "\t2. cu16_le" << std::endl
            << "\t3. ci32_le" << std::endl
            << "\t4. cu32_le" << std::endl
            << "\t5. cf32_le" << std::endl
            << "\t6. fc32_le" << std::endl;

    std::cin >> str_arg;
    if (str_arg.empty()){
        j[capture_name]["datatype"] = "ci16_le";
    } else {
        switch(std::stoi(str_arg)){
            case 1:
                j[capture_name]["datatype"] = "ci16_le";
                break;
            case 2:
                j[capture_name]["datatype"] = "cu16_le";
                break;
            case 3:
                j[capture_name]["datatype"] = "ci32_le";
                break;
            case 4:
                j[capture_name]["datatype"] = "cu32_le";
                break;
            case 5:
                j[capture_name]["datatype"] = "cf32_le";
                break;
            case 6:
                j[capture_name]["datatype"] = "cf32_le";
                break;
            default:
                std::cout << "Invalid Option Defaulting to 1. ci16_le" << std::endl;
                j[capture_name]["datatype"] = "ci16_le";
                break;
        }
    }
    str_arg.clear();

    std::cout << "Extra Description: " << std::endl;
    std::cin >> str_arg;
    j[capture_name]["description"] = str_arg.c_str();
    str_arg.clear();

    return j;


}