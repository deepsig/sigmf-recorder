//
// Created by Daniel Brotman on 2019-02-21.
//

#include <sigmf.h>
#include <flatbuffers_json_visitor.h>

#include <iostream>
#include <fstream>
#include <ctype.h>

int main(int argc, char *argv[]) {
    json master;

    for(int i=2; i < argc; i++){
        std::ifstream fp(argv[i]);
        if(i==2){
            fp >> master;

        }
        else {
            json j;
            fp >> j;
            for(auto &anno : j["annotations"]){
                master["annotations"].push_back(anno);
            }
        }
        fp.close();

    }
    
    std::ofstream mp(argv[1]);

    mp << master.dump(4) << std::endl;
    mp.close();
}