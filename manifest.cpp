//
// Created by Daniel Brotman on 2019-02-14.
//
#include <sigmf.h>
#include <flatbuffers_json_visitor.h>

#include <iostream>
#include <fstream>
#include <ctype.h>

void create_manifest();
json manifest_capture();

int main(int argc, char *argv[]) {
    std::cout << "SigMF Manifest Generator" << std::endl;
    for(;;){
        create_manifest();
        bool valid_arg = false;
        while(!valid_arg) {
            std::cout << "Would you like to create another manifest file? (y/n): " << std::endl;
            std::string str_arg;
            std::getline(std::cin, str_arg);
            char arg;
            try{
                arg = (char)tolower(str_arg.at(0));
            }
            catch (...){
                std::cout << "Invalid Option!" << std::endl;
                continue;
            }

            switch (arg) {
                case 'y':
                    valid_arg = true;
                    break;
                case 'n':
                    return 0;
                default:
                    std::cout << "Invalid Option!" << std::endl;
                    break;
            }
        }
    }
}

void create_manifest(){
    std::string str_arg;
    while (str_arg.empty()){
        std::cout << "Name of File (without extension):" << std::endl;
        std::getline(std::cin, str_arg);
        if(str_arg.empty()){
            std::cout << "File Name Cannot Be Empty!" << std::endl;
        }
    }

    std::ofstream mf(str_arg + ".manifest", std::ios::out);
    json j = {};
    for(;;){
        bool valid_arg = false;

        json capture = manifest_capture();
        json::iterator cap_beg = capture.begin();
        j[cap_beg.key()] = cap_beg.value();
        while(!valid_arg) {
            std::cout << "Would you like to create add another capture? (y/n): " << std::endl;

            std::getline(std::cin, str_arg);
            char arg;
            try{
                arg = (char)tolower(str_arg.at(0));
            }
            catch (...){
                std::cout << "Invalid Option!" << std::endl;
                continue;
            }


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
                    break;
            }
        }
    }


}

json manifest_capture(){
    json j;
    std::string str_arg;

    while(str_arg.empty()){
        std::cout << "Name of Capture:" << std::endl;
        std::getline(std::cin, str_arg);
        if(str_arg.empty()){
            std::cout << "Capture Name Cannot Be Empty!" << std::endl;
        }
    }

    std::string capture_name(str_arg);
    j[capture_name] = {};
    str_arg.clear();

    std::cout << "Center Frequency in MHz (Default: 880 MHz): " << std::endl;
    std::getline(std::cin, str_arg);
    j[capture_name]["freq"] = str_arg.empty() ? 880 : std::stoi(str_arg);
    str_arg.clear();

    std::cout << "Sampling Rate in MSPS (Default: 40 MSPS): " << std::endl;
    std::getline(std::cin, str_arg);
    j[capture_name]["rate"] = str_arg.empty() ? 40 : std::stoi(str_arg);
    str_arg.clear();

    std::cout << "Gain in dB (Default: 45 dB): " << std::endl;
    std::getline(std::cin, str_arg);
    j[capture_name]["gain"] = str_arg.empty() ? 45 : std::stoi(str_arg);
    str_arg.clear();

    std::cout << "Number of Samples (Default: 100000000 i.e. 1e8): " << std::endl;
    std::getline(std::cin, str_arg);
    j[capture_name]["samples"] = str_arg.empty() ? 1e8 : std::stod(str_arg);
    str_arg.clear();

    std::cout << "Datatype enter a number (Default: 1. ci16_le): " << std::endl
            << "\t1. ci16_le" << std::endl
            << "\t2. cu16_le" << std::endl
            << "\t3. ci32_le" << std::endl
            << "\t4. cu32_le" << std::endl
            << "\t5. cf32_le" << std::endl
            << "\t6. fc32_le" << std::endl;

    std::getline(std::cin, str_arg);
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
    std::getline(std::cin, str_arg);
    j[capture_name]["description"] = str_arg.c_str();
    str_arg.clear();

    std::cout << std::endl;

    return j;
}