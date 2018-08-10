
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_HDR
#include "external\stb_image.h"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "external\stb_image_resize.h"

#define STRINGIFY(n) #n
#define STRINGIFY_MACRO(n) STRINGIFY(n)
#define APP_NAME STRINGIFY_MACRO(A_NAME)

typedef unsigned char uchar;

#include "createIcon.cpp"
#include "setIcon.cpp"

int main(int argc, char** argv) {
	argc--;

	bool printUsage = false;

	if(argc == 0) printUsage = true;
	else if(argc == 1) {
		char* arg = argv[1];
		if(strstr(arg, "-h") || strstr(arg, "-help") || strstr(arg, "--help")) printUsage = true;
		else {
			printf("\nError: Expected 2 arguments, but got 1.\n");
			return 0;
		}
	}

	if(printUsage) {
		printf("\n");
		printf("Usage: %s.exe image_path exe_path [-s size1 size2 ...]", APP_NAME);
		printf("\n\n");
		printf(" Creates an icon file from an image and sets it on an exe.\n");
		printf(" Icons are stored in 32bit.\n");
		printf("\n");
		printf(" Supported image formats: jpg, png, bmp, psd, tga, gif, pic, ppm, pgm.\n");
		printf(" Default icon sizes: 256, 128, 48, 32, 24, 16.\n");

		return 0;
	}

	const int sizeCountMax = 20;
	int sizes[sizeCountMax] = {256, 128, 48, 32, 24, 16};
	int sizeCount = 6;

	if(argc >= 3) {
		char* arg = argv[3];
		if(!strstr("-s", arg)) {
			printf("\nError: Unknown argument \"%s\".\n", arg);
			return 0;
		}

		if(argc == 3) {
			printf("\nError: Need at least one size to create icon file.\n");
			return 0;
		}

		if(argc - 3 >= sizeCountMax) {
			printf("\nError: Too many input sizes.\n");
			return 0;
		}

		sizeCount = argc - 3;
		for(int i = 0; i < sizeCount; i++) {
			char* sizeStr = argv[i + 3 + 1];
			int size = atoi(sizeStr);

			if(size < 1 || size > 1024) {
				printf("\nError: Size argument %i (%s) is invalid.", i+3, sizeStr);
				printf("\n       Valid size range is: 1 <= size <= 1024.\n");
				return 0;
			}

			sizes[i] = size;
		}

		auto cmp = [](const void* a, const void* b) -> int { return *((int*)a) < *((int*)b); };
		qsort(sizes, sizeCount, sizeof(int), cmp);
	}

	char* imagePath = (char*)argv[1];
	char* iconPath  = (char*)argv[2];

	char* iconData;
	int size = createIcoFileFromBitmapFilename(imagePath, sizes, sizeCount, &iconData); 
	if(!size) return 0;

	bool result = setIconByData(iconPath, iconData);
	if(!result) return 0;

	return 0;
}