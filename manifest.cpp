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
    for (;;) {
        create_manifest();
        bool valid_arg = false;
        while (!valid_arg) {
            std::cout << "Would you like to create another manifest file? (y/n): " << std::endl;
            std::string str_arg;
            std::getline(std::cin, str_arg);
            char arg;
            try {
                arg = (char) tolower(str_arg.at(0));
            }
            catch (...) {
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

void create_manifest() {
    std::string str_arg;
    while (str_arg.empty()) {
        std::cout << "Name of File (without extension):" << std::endl;
        std::getline(std::cin, str_arg);
        if (str_arg.empty()) {
            std::cout << "File Name Cannot Be Empty!" << std::endl;
        }
    }

    bool create_new_capture = true;
    std::ofstream mf(str_arg + ".manifest", std::ios::out);
    json all_captures = {};
    do {
        json capture = manifest_capture();
        all_captures.push_back(capture);

        std::cout << "Would you like to create add another capture? (y/n): " << std::endl;
        std::getline(std::cin, str_arg);
        char new_capture_input;
        try {
            new_capture_input = (char) tolower(str_arg.at(0));
        }
        catch (...) {
            std::cout << "Invalid Option!" << std::endl;
            continue;
        }

        if (new_capture_input == 'n') {
            create_new_capture = false;
        }
    } while (create_new_capture);

    mf << all_captures.dump(4) << std::endl;
    mf.close();
}

json manifest_capture() {
    std::string str_arg;

    json new_capture = {};
    str_arg.clear();

    std::cout << "Center Frequency in MHz (Default: 880 MHz): " << std::endl;
    std::getline(std::cin, str_arg);
    new_capture["freq"] = str_arg.empty() ? 880 : std::stoi(str_arg);
    str_arg.clear();

    std::cout << "Sampling Rate in MSPS (Default: 40 MSPS): " << std::endl;
    std::getline(std::cin, str_arg);
    new_capture["rate"] = str_arg.empty() ? 40 : std::stoi(str_arg);
    str_arg.clear();

    std::cout << "Gain in dB (Default: 45 dB): " << std::endl;
    std::getline(std::cin, str_arg);
    new_capture["gain"] = str_arg.empty() ? 45 : std::stoi(str_arg);
    str_arg.clear();

    std::cout << "Number of Samples (Default: 100000000 i.e. 1e8): " << std::endl;
    std::getline(std::cin, str_arg);
    new_capture["samples"] = static_cast<uint64_t>(str_arg.empty() ? 1e8 : std::stod(str_arg));
    str_arg.clear();

    std::cout << "Datatype enter a number (Default: 1. ci16_le): " << std::endl
              << "\t1. ci16_le" << std::endl
              << "\t2. cu16_le" << std::endl
              << "\t3. ci32_le" << std::endl
              << "\t4. cu32_le" << std::endl
              << "\t5. cf32_le" << std::endl
              << "\t6. fc32_le" << std::endl;

    std::getline(std::cin, str_arg);
    if (str_arg.empty()) {
        new_capture["datatype"] = "ci16_le";
    } else {
        switch (std::stoi(str_arg)) {
            case 1:
                new_capture["datatype"] = "ci16_le";
                break;
            case 2:
                new_capture["datatype"] = "cu16_le";
                break;
            case 3:
                new_capture["datatype"] = "ci32_le";
                break;
            case 4:
                new_capture["datatype"] = "cu32_le";
                break;
            case 5:
                new_capture["datatype"] = "cf32_le";
                break;
            case 6:
                new_capture["datatype"] = "cf32_le";
                break;
            default:
                std::cout << "Invalid Option Defaulting to 1. ci16_le" << std::endl;
                new_capture["datatype"] = "ci16_le";
                break;
        }
    }
    str_arg.clear();

    std::cout << "Extra Description: " << std::endl;
    std::getline(std::cin, str_arg);
    new_capture["name"] = str_arg.c_str();
    str_arg.clear();

    std::cout << std::endl;

    return new_capture;
}