#pragma once

struct bmp_info_header {
	int      biSize;
	int      biWidth;
	int      biHeight;
	short    biPlanes;
	short    biBitCount;
	int      biCompression;
	int      biSizeImage;
	int      biXPelsPerMeter;
	int      biYPelsPerMeter;
	int      biClrUsed;
	int      biClrImportant;
};

#pragma pack(push,1)
struct bmp_header {
	short  bfType;
	int    bfSize;
	short  bfReserved1;
	short  bfReserved2;
	int    bfOffBits;
};
#pragma pack(pop)


#pragma pack(push,1)
struct rgb_pal_entry {
	unsigned char r;
	unsigned char g;
	unsigned char b;
};
#pragma pack(pop)


struct rgbr_pal_entry {
	unsigned char r;
	unsigned char g;
	unsigned char b;
	unsigned char reserved;
};



struct harvester_bm {
	int width;
	int height;
	int pad;
};

#pragma pack(push,1)
struct harvester_abm {
	int		unk[2];
	int		width;
	int		height;
	char	flag;
	int		size;
	int		_pad;
};
#pragma pack(pop)