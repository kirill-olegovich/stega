#include <iostream>
#include <vector>
#include <fstream>
#include "RSAnalysis.h"
#include "ChiSquare.h"

#pragma pack(push,1)
struct file_header{
    unsigned short id;
    unsigned int f_size;
    unsigned short rez_1, rez_2;
    unsigned int bm_offset;
};

struct v3_header{
    unsigned int h_size;
    unsigned int width;
    unsigned int height;
    unsigned short planes, bit_per_pixel;
    unsigned int compression;
    unsigned int size_image;
    unsigned int h_res;
    unsigned int v_res;
    unsigned int clr_used;
    unsigned int clr_imp;
};

struct color_info{
    unsigned char blue;
    unsigned char green;
    unsigned char red;
    unsigned char temp;
};
#pragma pack(pop)

int main(int argc, char** argv) {
    if(argc<3){
        std::cout<<"no argums\n";
        return 4;
    }
    try{
        int temp=std::stoi(argv[1]);
        if(temp!=1 && temp!=2) throw std::exception();
    }
    catch(std::exception &exp){
        std::cout<<"error algom\n";
        return 4;
    }

    int alg=std::stoi(argv[1]);
    std::ifstream file_in(argv[2],std::ios::binary);
    if(!file_in){
        std::cout<<"error file orig";
        return 4;
    }
    file_header header;
    v3_header v3header;
    color_info color_table[256];
    file_in.read(reinterpret_cast<char*>(&header),sizeof(file_header));
    file_in.read(reinterpret_cast<char*>(&v3header),sizeof(v3_header));
    file_in.read(reinterpret_cast<char*>(color_table),256*sizeof(color_info));
    file_in.seekg(header.bm_offset,std::ios::beg);
    std::vector<std::vector<unsigned char>> mas;
    int rs=0, bpr=floor((v3header.bit_per_pixel*v3header.width+31)/32)*4;
    do{
        mas.push_back(std::vector<unsigned char>(bpr,0));
        file_in.read(reinterpret_cast<char*>(mas.back().data()),bpr);
        rs=file_in.gcount();            
    } while(rs>0);
    file_in.close();
    mas.pop_back();
    std::vector<std::vector<int>> image(v3header.height,std::vector<int>(v3header.width,0));
    for(int i=0;i<v3header.height;++i){
        for(int j=0;j<v3header.width;++j) image[i][j]=(int)mas[i][j];
    }


    if(alg==1){
        RSAnalysis rsa(2, 2);
        int colour = RSAnalysis::ANALYSIS_COLOUR_RED;
        bool overlap = true;
        std::vector<double> rsResults = rsa.doAnalysis(image, colour, overlap);

        std::cout << "RS analysis:\n";
        std::cout << "  Estimated message length (pixels %): " << (double)rsResults[26] << "\n";
        std::cout << "  Estimated message length (bytes): " << (double)rsResults[27] << "\n";
    }
    else{
        int numBlocks = 100;
        int sampleSize = 2048;
        std::vector<double> x(numBlocks, 0.0);
        std::vector<double> chi(numBlocks, 0.0);

        ChiSquare::chiSquareAttackTopToBottom(image, x, chi, sampleSize);

        std::cout << "\nChi-square p-values (top-to-bottom):\n";
        for (int i = 0; i < 25 && i < chi.size(); ++i) {
            std::cout << "  block " << i << ": " << chi[i] << "\n";
        }
    }

    return 0;
}