
struct MEMICONDIRENTRY {
    BYTE        bWidth;          // Width, in pixels, of the image
    BYTE        bHeight;         // Height, in pixels, of the image
    BYTE        bColorCount;     // Number of colors in image (0 if >=8bpp)
    BYTE        bReserved;       // Reserved ( must be 0)
    WORD        wPlanes;         // Color Planes
    WORD        wBitCount;       // Bits per pixel
    DWORD       dwBytesInRes;    // How many bytes in this resource?
    WORD        nID;             // The ID.
};

struct MEMICONDIR {
    WORD           idReserved;   // Reserved (must be 0)
    WORD           idType;       // Resource Type (1 for icons)
    WORD           idCount;      // How many images?
    // MEMICONDIRENTRY   idEntries[1]; // An entry for each image (idCount of 'em)
};

bool setIconByData(char* exe_filename, char* iconData, bool delete_existing_resources = false) {
    
	HANDLE fileHandle = BeginUpdateResource(exe_filename, (BOOL)delete_existing_resources);
	if(!fileHandle) {
		short errorCode = GetLastError();
		printf("\nError: BeginUpdateResource failed with error code %i (0x%x).\n", errorCode, errorCode);

		return 0;
	}

	// @Incomplete: Assert that we are little-endian, or byteswap the struct.

	ICONDIR* icondir = (ICONDIR*)iconData;

	// For now, punting on whether we care about the langId...
	// However, I did try LANG_NEUTRAL and when I do so, the icon is not used by Windows Explorer!
	// You would think that this means "use this in every language", but no! I have no idea what is going on.

	auto langId = MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT);

	auto entriesSize = sizeof(MEMICONDIRENTRY) * icondir->idCount;
	auto memicondirBytes = sizeof(MEMICONDIR) + entriesSize;
	MEMICONDIR* memicondir = (MEMICONDIR*)malloc(memicondirBytes);
	memset(memicondir, 0, memicondirBytes);

	MEMICONDIRENTRY* entries = (MEMICONDIRENTRY*)(((uchar*)memicondir) + sizeof(MEMICONDIR));
	uchar* entriesU8 = (uchar*)entries;

	memicondir->idType = 1;
	memicondir->idCount = icondir->idCount;

	int resourceId = 1;

	char* cursor = iconData + 6;
	for(int i = 0; i < icondir->idCount; i++) {
		ICONDIRENTRY* srcEntry = (ICONDIRENTRY*)cursor;
		cursor += 16;

		// This multiplication hack is here because the struct comes out to alignment 16,
		// but the file expect these to be packed at alignment 1. We don't have a way right
		// now to specify alignment on the struct declaration (and it's not clear that's the
		// best way to do it anyway), so just do it manually here.
		MEMICONDIRENTRY* destEntry = (MEMICONDIRENTRY*)(entriesU8 + 14 * i);

		destEntry->bWidth        = srcEntry->bWidth;
		destEntry->bHeight       = srcEntry->bHeight;
		destEntry->bColorCount   = srcEntry->bColorCount;
		destEntry->bReserved     = srcEntry->bReserved;
		destEntry->wPlanes       = srcEntry->wPlanes;
		destEntry->wBitCount     = srcEntry->wBitCount;
		destEntry->dwBytesInRes  = srcEntry->dwBytesInRes;
		destEntry->nID           = (unsigned short)(resourceId + i);

		bool updateSuccess = UpdateResource(fileHandle, RT_ICON, MAKEINTRESOURCE(destEntry->nID), langId, iconData + srcEntry->dwImageOffset, destEntry->dwBytesInRes);

		if(!updateSuccess) {
			short errorCode = GetLastError();
			printf("\nError: UpdateResource failed with error code %i (0x%x).\n", errorCode, errorCode);

			return 0;
		}
	}

	bool updateSuccess = UpdateResource(fileHandle, RT_GROUP_ICON, MAKEINTRESOURCE(resourceId), langId, memicondir, (6 + entriesSize));
	if(!updateSuccess) {
		short errorCode = GetLastError();
		printf("\nError: UpdateResource failed with error code %i (0x%x).\n", errorCode, errorCode);

		return 0;
	}

	bool success = EndUpdateResource(fileHandle, 0);

	free(memicondir);

	return success;
}

bool setIconByFilename(char* exeFilename, char* iconFilename, bool deleteExistingResources = false) {

	char* iconData;
	{
		FILE* file = fopen(iconFilename, "rb");
		if(file == 0) {
			printf("\nError: Unable to open icon file \"%s\".\n", iconFilename);
			return 0;
		}

		fseek(file, 0, SEEK_END);
		int size = ftell(file);
		fseek(file, 0, SEEK_SET);

		iconData = (char*)malloc(sizeof(char) * size);

		fread(iconData, size, 1, file);

		fclose(file);
	}

	bool result = setIconByData(exeFilename, iconData, deleteExistingResources);

	free(iconData);

	return result;
}