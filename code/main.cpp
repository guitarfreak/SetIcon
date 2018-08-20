
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

#define arrayCount(array) (sizeof(array) / sizeof((array)[0]))
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
		printf("Usage: %s.exe image_path exe_path  [-s size1 size2 ...] |\n", APP_NAME);
		printf("       %s.exe image_path icon_path [-s size1 size2 ...] |\n", APP_NAME);
		printf("       %s.exe icon_path  exe_path\n", APP_NAME);
		printf("\n");
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
		if(!strstr(arg, "-s")) {
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

	char* srcPath = (char*)argv[1];
	char* dstPath = (char*)argv[2];

	bool srcIsIcon = strstr(srcPath, ".ico");
	bool dstIsIcon = strstr(dstPath, ".ico");
	bool dstIsExe  = strstr(dstPath, ".exe");

	bool srcIsImage = false;
	char* imageExtensions[] = {".jpg", ".png", ".bmp", ".psd", ".tga", ".gif", ".pic", ".ppm", ".pgm"};
	for(int i = 0; i < arrayCount(imageExtensions); i++) {
		if(strstr(srcPath, imageExtensions[i])) {
			srcIsImage = true;
			break;
		}
	}

	int mode;
	if(srcIsImage && dstIsExe) {
		// Create and set icon.

		char* iconData;
		int size = createIcoFileFromBitmapFilename(srcPath, sizes, sizeCount, &iconData); 
		if(!size) return 0;

		bool result = setIconByData(dstPath, iconData);
		if(!result) return 0;

	} else if(srcIsImage && dstIsIcon) {
		// Create icon.

		if(!srcIsImage) {
			printf("\nError: %s is not an image file.\n", srcPath);
			return 0;
		}

		char* iconData;
		int size = createIcoFileFromBitmapFilename(srcPath, sizes, sizeCount, &iconData); 
		if(!size) return 0;

		FILE* file = fopen(dstPath, "wb");
		if(!file) {
			printf("\nError: Could not create icon file %s.\n", dstPath);
			return 0;
		}

		fwrite(iconData, size, 1, file);
		fclose(file);

	} else if(srcIsIcon && dstIsExe) {
		// Set icon.

		FILE* file = fopen(srcPath, "rb");
		if(file == 0) {
			printf("\nError: Could not open icon file %s.\n", srcPath);
			return 0;
		}

		fseek(file, 0, SEEK_END);
		int size = ftell(file);
		fseek(file, 0, SEEK_SET);

		char* iconData = (char*)malloc(sizeof(char)*size);
		fread(iconData, size, 1, file);

		fclose(file);

		bool result = setIconByData(dstPath, iconData);
		if(!result) return 0;

	} else {
		printf("\nError: Invalid files.\n");
		printf(" Possible combinations: image exe | image icon | icon exe.\n");
		return 0;
	}

	return 0;
}