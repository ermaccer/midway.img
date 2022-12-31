// img.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "image.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <memory>
#include <filesystem>
#include <algorithm>
#include <sstream>

#if defined(__linux__) || defined(__APPLE__)
#define printf_s printf
#define sscanf_s sscanf
#define fopen_s(fp, fmt, mode)   *(fp)=fopen( (fmt), (mode))
#endif

#pragma pack(push,1)
struct IMG_ENTRY {
    char name[16];
    short unks[3];
    short x;
    short y;
    char palInd;
    char flags;
    int   imageOffset;
    char unk[18];
};
#pragma pack(pop)

#pragma pack(push,1)
struct PAL_ENTRY {
    char name[8];
    short field0;
    short field2;
    short colorsUsed;
    short offset;
    short field8;
    short field10;
    short field12;
    short field14;
    short field16;
};
#pragma pack(pop)


#pragma pack(push,1)
struct HEADER {
    short numImgs;
    short numPals;
    int infoPtr;
    int lastImgPtr;
    char unk[16];
};
#pragma pack(pop)


#pragma pack(push,1)
struct BDD_ENTRY {
    int id;
    int offset;
    int x, y;

};
#pragma pack(pop)

#pragma pack(push,1)
struct BDB_ENTRY {
    int id;
    int palId;

};
#pragma pack(pop)


#pragma pack(push,1)
struct BDD_PAL {
    int colors;
    int offset;
};
#pragma pack(pop)



std::string get_str(std::ifstream& file)
{
    std::vector<char> str;

    char data = 0x0;
    
    while(true)
    {

        file.read(&data, sizeof(char));
        if (data == 0xA)
        {
            str.push_back(0x00);
            break;
        }
        else
           str.push_back(data);

    }
    return std::string(str.data());
}

// not really the best code, but works for extraction and thats all required
int main(int argc, char* argv[])
{
    if (argc > 1)
    {

        std::ifstream pFile(argv[1], std::ifstream::binary);



        if (pFile)
        {
            std::string input = argv[1];
            std::transform(input.begin(), input.end(), input.begin(), tolower);
            std::string extension = input.substr(input.find_last_of(".") + 1);
            if (extension == "img")
            {
                std::vector<IMG_ENTRY> imgs;
                std::vector<PAL_ENTRY> pals;
                int numPal = 0;

                HEADER hdr;
                pFile.read((char*)&hdr, sizeof(HEADER));

                pFile.seekg(hdr.infoPtr, pFile.beg);
                for (int i = 0; i < hdr.numImgs; i++)
                {
                    IMG_ENTRY img;
                    pFile.read((char*)&img, sizeof(IMG_ENTRY));
                    if (img.palInd > numPal)
                        numPal = img.palInd;
                    imgs.push_back(img);
                }

                numPal -= 2;
                if (numPal > 0)
                {
                    for (int i = 0; i < numPal; i++)
                    {
                        PAL_ENTRY pal;
                        pFile.read((char*)&pal, sizeof(PAL_ENTRY));
                        pals.push_back(pal);
                    }

                    // image extraction
                    std::string folder = argv[1];
                    folder += "_out";
                    std::filesystem::create_directory(folder);
                    std::filesystem::current_path(folder);

                    for (int i = 0; i < imgs.size(); i++)
                    {
                        std::string output = imgs[i].name;
                        std::replace(output.begin(), output.end(), '/', '_');
                        output.insert(0, "_");
                        output.insert(0, std::to_string(i));
                        output += ".bmp";
                        int width = (imgs[i].x + 3) & ~3;
                        int height = imgs[i].y;


                        int palNum = imgs[i].palInd - 3;
                        PAL_ENTRY pal = pals[palNum];


                        // create bmp
                        bmp_header bmp;
                        bmp_info_header bmpf;
                        bmp.bfType = 'MB';
                        bmp.bfSize = width * height;
                        bmp.bfReserved1 = 0;
                        bmp.bfReserved2 = 0;
                        bmp.bfOffBits = sizeof(bmp_header) + sizeof(bmp_info_header) + (sizeof(rgbr_pal_entry) * pal.colorsUsed);
                        bmpf.biSize = sizeof(bmp_info_header);
                        bmpf.biWidth = width;
                        bmpf.biHeight = height;
                        bmpf.biPlanes = 1;
                        bmpf.biBitCount = 8;
                        bmpf.biCompression = 0;
                        bmpf.biXPelsPerMeter = 0;
                        bmpf.biYPelsPerMeter = 0;
                        bmpf.biClrUsed = pal.colorsUsed;
                        bmpf.biClrImportant = 0;


                        std::ofstream oFile(output, std::ofstream::binary);

                        oFile.write((char*)&bmp, sizeof(bmp_header));
                        oFile.write((char*)&bmpf, sizeof(bmp_info_header));

                        // get colors
                        pFile.seekg(pal.offset, pFile.beg);

                        std::vector<rgbr_pal_entry> colors;

                        for (int a = 0; a < pal.colorsUsed; a++)
                        {
                            short m_sPixel;
                            pFile.read((char*)&m_sPixel, sizeof(short));

                            rgbr_pal_entry color = {};
                            color.r = 8 * (m_sPixel & 31);	m_sPixel >>= 5;
                            color.g = 8 * (m_sPixel & 31);	m_sPixel >>= 5;
                            color.b = 8 * (m_sPixel & 31);

                            rgbr_pal_entry cr = color;
                            colors.push_back(cr);
                        }

                        if (argv[2])
                        {
                            if (strcmp("bg", argv[2]) == 0)
                                colors[0] = { 255,0,255 };
                        }



                        // write colors

                        for (int i = 0; i < pal.colorsUsed; i++)
                        {
                            oFile.write((char*)&colors[i], sizeof(rgbr_pal_entry));
                        }


                        int imgSize = width * height;
                        std::unique_ptr<unsigned char[]> dataBuff = std::make_unique<unsigned char[]>(imgSize);
                        pFile.seekg(imgs[i].imageOffset, pFile.beg);
                        pFile.read((char*)dataBuff.get(), imgSize);


                        for (int y = height - 1; y >= 0; y--)
                        {
                            for (int x = 0; x < width; x++)
                            {
                                // int id = dataBuff[x + (y * width)];
                                // oFile.write((char*)&colors[id], sizeof(rgb_pal_entry));
                                oFile.write((char*)&dataBuff[x + (y * width)], sizeof(char));
                            }
                        }
                        printf("%s saved!\n", output.c_str());
                    }

                    std::filesystem::current_path("..");

                    std::filesystem::create_directory("pal");
                    std::filesystem::current_path("pal");
                    for (int i = 0; i < numPal; i++)
                    {
                        // color palettes

                        int width = 64;
                        int height = 64;


                        std::vector<rgb_pal_entry> palImage;


                        // get colors
                        pFile.seekg(pals[i].offset, pFile.beg);

                        std::vector<rgbr_pal_entry> colors;

                        for (int a = 0; a < pals[i].colorsUsed; a++)
                        {
                            short m_sPixel;
                            pFile.read((char*)&m_sPixel, sizeof(short));

                            rgbr_pal_entry color = {};
                            color.r = 8 * (m_sPixel & 31);	m_sPixel >>= 5;
                            color.g = 8 * (m_sPixel & 31);	m_sPixel >>= 5;
                            color.b = 8 * (m_sPixel & 31);

                            rgbr_pal_entry cr = color;
                            colors.push_back(cr);
                        }

                        for (unsigned int i = 0; i < width * height; i++)
                        {
                            if (i < colors.size())
                            {
                                rgbr_pal_entry cr = colors[i];
                                rgb_pal_entry color;
                                color.r = cr.r;
                                color.g = cr.g;
                                color.b = cr.b;
                                palImage.push_back(color);
                            }
                            else
                            {
                                palImage.push_back({ 255,0,255 });
                            }
                        }

                        std::string output = pals[i].name;
                        output.insert(0, "_");
                        output.insert(0, std::to_string(i));
                        output += "_pal";
                        output += ".bmp";

                        // create bmp
                        bmp_header bmp;
                        bmp_info_header bmpf;
                        bmp.bfType = 'MB';
                        bmp.bfSize = sizeof(palImage) + sizeof(bmp_header) + sizeof(bmp_info_header);
                        bmp.bfReserved1 = 0;
                        bmp.bfReserved2 = 0;
                        bmp.bfOffBits = sizeof(bmp_header) + sizeof(bmp_info_header);
                        bmpf.biSize = sizeof(bmp_info_header);
                        bmpf.biWidth = width;
                        bmpf.biHeight = height;
                        bmpf.biPlanes = 1;
                        bmpf.biBitCount = 24;
                        bmpf.biSizeImage = width * height * sizeof(rgb_pal_entry);
                        bmpf.biCompression = 0;
                        bmpf.biXPelsPerMeter = 2880;
                        bmpf.biYPelsPerMeter = 2880;
                        bmpf.biClrUsed = 0;
                        bmpf.biClrImportant = 0;

                        std::ofstream oFile(output, std::ofstream::binary);
                        oFile.write((char*)&bmp, sizeof(bmp_header));
                        oFile.write((char*)&bmpf, sizeof(bmp_info_header));


                        for (int y = height - 1; y >= 0; y--)
                        {
                            for (int x = 0; x < width; x++)
                            {
                                oFile.write((char*)&palImage[x + (y * width)], sizeof(rgb_pal_entry));
                            }
                        }
                        printf_s("Palette saved as %s\n", output.c_str());
                    }
                }
            }
            else if (extension == "bdd")
            {
                
                std::vector<BDB_ENTRY> bdb_data;
                std::vector<BDD_ENTRY> bdd_data;
                std::vector<BDD_PAL> pal_data;

                std::string bdb_file = input;
                bdb_file[bdb_file.length() - 1] = 'b';

                FILE* bdb = nullptr;
                fopen_s(&bdb, bdb_file.c_str(), "rb");

                if (!bdb)
                {
                    printf_s("ERROR: Could not open BDB file!\n");
                    return 1;
                }

                int numPals = 0;
                std::string mainName;

                {
                    char szLine[512] = {};

                    fgets(szLine, sizeof(szLine), bdb);

                    char name[256] = {};
                    int null;
                    int x = 0,y = 0,z = 0;
                    int modules = 0;
                    int entries = 0;
                    sscanf_s(szLine, "%s %d %d %d %d %d %d", &name, &x, &y, &z, &null, &modules, &numPals, &entries);
                    mainName = name;
                    for (int i = 0; i < modules; i++)
                         fgets(szLine, sizeof(szLine), bdb);

                    for (int i = 0; i < entries; i++)
                    {
                        fgets(szLine, sizeof(szLine), bdb);
                        int imgNo;
                        int palNo;

                        sscanf_s(szLine, "%x %d %d %x %d", &null, &null, &null, &imgNo, &palNo);
                        BDB_ENTRY ent = { imgNo, palNo };
                        bdb_data.push_back(ent);
                    }


                }



                std::string header = get_str(pFile);
                int numImages = 0;
                sscanf_s(header.c_str(), "%d", &numImages);

                // image extraction
                std::string folder = argv[1];
                folder += "_out";
                std::filesystem::create_directory(folder);
                std::filesystem::current_path(folder);

                for (int i = 0; i < numImages; i++)
                {
                    std::string info = get_str(pFile);
                    int old_hdr, x, y, pal;
                    sscanf_s(info.c_str(), "%x %d %d %d", &old_hdr, &x, &y, &pal);
                    BDD_ENTRY bdd = {};
                    bdd.id = old_hdr;
                    bdd.offset = (int)pFile.tellg();

                    std::unique_ptr<unsigned char[]> dataBuff = std::make_unique<unsigned char[]>(x * y);
                    pFile.read((char*)dataBuff.get(), x* y);

                    bdd.x = x;
                    bdd.y = y;
                    bdd_data.push_back(bdd);

                }
                

                for (int i = 0; i < numPals; i++)
                {
                    BDD_PAL pal;
                    std::string palInfo = get_str(pFile);
                    std::string name;
                    int numColors = 0;
                    std::stringstream ss(palInfo);

                    ss >> name >> numColors;


                    pal.colors = numColors;
                    pal.offset = (int)pFile.tellg();
                    for (int a = 0; a < numColors; a++)
                    {
                        short m_sPixel;
                        pFile.read((char*)&m_sPixel, sizeof(short));
                    }
                    pal_data.push_back(pal);
                }
                pFile.clear();
                pFile.seekg(0, pFile.beg);

                for (int i = 0; i < bdb_data.size(); i++)
                {
                    std::string output = std::to_string(i);;
                    output += "_";
                    output += mainName;
                    output += ".bmp";

                    BDB_ENTRY bdb = bdb_data[i];
                    BDD_ENTRY ent;
                    for (int a = 0; a < bdd_data.size(); a++)
                    {
                        if (bdd_data[a].id == bdb.id)
                        {
                            ent = bdd_data[a];
                            break;
                        }
                    }


                    int width = ent.x;
                    int height = ent.y;

                  //  printf("data: %d %d %d Image %d x %d offset: %x\n",bdb.id, bdb.palId, bdd_data.size(), width, height, ent.offset);
                    BDD_PAL pal = pal_data[bdb.palId];


                    // create bmp
                    bmp_header bmp;
                    bmp_info_header bmpf;
                    bmp.bfType = 'MB';
                    bmp.bfSize = width * height;
                    bmp.bfReserved1 = 0;
                    bmp.bfReserved2 = 0;
                    bmp.bfOffBits = sizeof(bmp_header) + sizeof(bmp_info_header) + (sizeof(rgbr_pal_entry) * pal.colors);
                    bmpf.biSize = sizeof(bmp_info_header);
                    bmpf.biWidth = width;
                    bmpf.biHeight = height;
                    bmpf.biPlanes = 1;
                    bmpf.biBitCount = 8;
                    bmpf.biCompression = 0;
                    bmpf.biXPelsPerMeter = 0;
                    bmpf.biYPelsPerMeter = 0;
                    bmpf.biClrUsed = pal.colors;
                    bmpf.biClrImportant = 0;


                    std::ofstream oFile(output, std::ofstream::binary);

                    oFile.write((char*)&bmp, sizeof(bmp_header));
                    oFile.write((char*)&bmpf, sizeof(bmp_info_header));

                    // get colors
                    pFile.seekg(pal.offset, pFile.beg);

                    std::vector<rgbr_pal_entry> colors;

                    for (int a = 0; a < pal.colors; a++)
                    {
                        short m_sPixel;
                        pFile.read((char*)&m_sPixel, sizeof(short));

                        rgbr_pal_entry color = {};
                        color.r = 8 * (m_sPixel & 31);	m_sPixel >>= 5;
                        color.g = 8 * (m_sPixel & 31);	m_sPixel >>= 5;
                        color.b = 8 * (m_sPixel & 31);

                        rgbr_pal_entry cr = color;
                        colors.push_back(cr);
                    }

                    if (argv[2])
                    {
                        if (strcmp("bg", argv[2]) == 0)
                            colors[0] = { 255,0,255 };
                    }



                    // write colors

                    for (int i = 0; i < colors.size(); i++)
                    {
                        oFile.write((char*)&colors[i], sizeof(rgbr_pal_entry));
                    }


                    // write colors

                    int imgSize = width * height;
                    std::unique_ptr<unsigned char[]> dataBuff = std::make_unique<unsigned char[]>(imgSize);
                    pFile.seekg(ent.offset, pFile.beg);

                    pFile.read((char*)dataBuff.get(), imgSize);


                    for (int y = height - 1; y >= 0; y--)
                    {
                        for (int x = 0; x < width; x++)
                        {
                            oFile.write((char*)&dataBuff[x + (y * width)], sizeof(char));
                        }
                    }
                   printf("%s saved!\n", output.c_str());
                }
            }
            pFile.close();
        }
        else
        {
            printf_s("ERROR: Could not open file %s!\n", argv[1]);
            return 1;
        }
        
    }
    else
    {
        printf_s("USAGE: img <file> <flag(optional)>\n\n");
        printf_s("Set flag to 'bg' to extract images with magenta as the alpha color\n");
    }
    return 0;
}
