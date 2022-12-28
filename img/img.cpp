// img.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "image.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <memory>
#include <filesystem>


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


// not really the best code, but works for extraction and thats all required
int main(int argc, char* argv[])
{
    if (argc > 1)
    {
        std::ifstream pFile(argv[1], std::ifstream::binary);

        if (pFile)
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

                std::filesystem::current_path("...");

                std::filesystem::create_directory("pal");
                std::filesystem::current_path("pal");
                for (int i = 0; i < numPal; i++)
                {
                    // color palettes



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

                    for (unsigned int i = 0; i < 64 * 64; i++)
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
                    bmpf.biWidth = 64;
                    bmpf.biHeight = 64;
                    bmpf.biPlanes = 1;
                    bmpf.biBitCount = 24;
                    bmpf.biSizeImage = 64 * 64 * sizeof(rgb_pal_entry);
                    bmpf.biCompression = 0;
                    bmpf.biXPelsPerMeter = 2880;
                    bmpf.biYPelsPerMeter = 2880;
                    bmpf.biClrUsed = 0;
                    bmpf.biClrImportant = 0;

                    std::ofstream oFile(output, std::ofstream::binary);
                    oFile.write((char*)&bmp, sizeof(bmp_header));
                    oFile.write((char*)&bmpf, sizeof(bmp_info_header));


                    for (int y = 64 - 1; y >= 0; y--)
                    {
                        for (int x = 0; x < 64; x++)
                        {
                            oFile.write((char*)&palImage[x + (y * 64)], sizeof(rgb_pal_entry));
                        }
                    }


                    printf_s("Palette saved as %s\n", output.c_str());



                }
            }
        }
        
    }
}
