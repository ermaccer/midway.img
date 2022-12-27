// img.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem>
#include <string>
#include <memory>
#include <algorithm>

#include "image.h"
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



int main(int argc, char* argv[])
{
    if (argv[1])
    {
        std::ifstream pFile(argv[1], std::ifstream::binary);

        if (pFile)
        {
            std::vector<IMG_ENTRY> imgs;
            std::vector<PAL_ENTRY> pals;
            int numPal = 0;

            HEADER hdr;
            pFile.read((char*)&hdr, sizeof(HEADER));

            printf_s("Images: %d\n", hdr.numImgs);

            pFile.seekg(hdr.infoPtr, pFile.beg);
            for (int i = 0; i < hdr.numImgs; i++)
            {
                IMG_ENTRY img;
                pFile.read((char*)&img, sizeof(IMG_ENTRY));
                printf_s("Image %03d - %s %dx%d PAL: %d\n", i, img.name, img.x, img.y, img.palInd - 3);
                if (img.palInd > numPal)
                    numPal = img.palInd;
                imgs.push_back(img);
            }

            numPal -= 3;
            numPal++;
            if (numPal > 0)
            {
                printf("Palettes: %d\n", numPal);

                for (int i = 0; i < numPal; i++)
                {
                    PAL_ENTRY pal;
                    pFile.read((char*)&pal, sizeof(PAL_ENTRY));
                    printf_s("Palette %03d - %s Colors: %d\n", i, pal.name, pal.colorsUsed);
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
                    bmp.bfOffBits = sizeof(bmp_header) + sizeof(bmp_info_header);
                    bmpf.biSize = sizeof(bmp_info_header);
                    bmpf.biWidth = width;
                    bmpf.biHeight = height;
                    bmpf.biPlanes = 1;
                    bmpf.biBitCount = 24;
                    bmpf.biCompression = 0;
                    bmpf.biXPelsPerMeter = 0;
                    bmpf.biYPelsPerMeter = 0;
                    bmpf.biClrUsed = 0;
                    bmpf.biClrImportant = 0;


                    std::ofstream oFile(output, std::ofstream::binary);

                    oFile.write((char*)&bmp, sizeof(bmp_header));
                    oFile.write((char*)&bmpf, sizeof(bmp_info_header));

                    // get colors
                    pFile.seekg(pal.offset, pFile.beg);

                    std::vector<rgb_pal_entry> colors;

                    for (int a = 0; a < pal.colorsUsed; a++)
                    {
                        short m_sPixel;
                        pFile.read((char*)&m_sPixel, sizeof(short));

                        rgb_pal_entry color = {};
                        color.r = 8 * (m_sPixel & 31);	m_sPixel >>= 5;
                        color.g = 8 * (m_sPixel & 31);	m_sPixel >>= 5;
                        color.b = 8 * (m_sPixel & 31);

                        rgb_pal_entry cr = color;
                        colors.push_back(cr);
                    }


                    if (argv[2])
                    {
                        if (strcmp("bg", argv[2]) == 0)
                            colors[0] = { 255,0,255 };
                    }


                    int imgSize = width * height;
                    std::unique_ptr<unsigned char[]> dataBuff = std::make_unique<unsigned char[]>(imgSize);
                    pFile.seekg(imgs[i].imageOffset, pFile.beg);
                    pFile.read((char*)dataBuff.get(), imgSize);


                    for (int y = height - 1; y >= 0; y--)
                    {
                        for (int x = 0; x < width; x++)
                        {
                            int id = dataBuff[x + (y * width)];
                            oFile.write((char*)&colors[id], sizeof(rgb_pal_entry));
                        }
                    }
                    printf_s("Saved %s!\n", output.c_str());

                }
            }


        }

        
    }
}
