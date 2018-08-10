
// @Endian: This whole file assumes we are on a little-endian architecture.
// We need to do the appropriate byte-swapping, etc!

struct ICONDIRENTRY {
    BYTE        bWidth;          // Width, in pixels, of the image
    BYTE        bHeight;         // Height, in pixels, of the image
    BYTE        bColorCount;     // Number of colors in image (0 if >=8bpp)
    BYTE        bReserved;       // Reserved ( must be 0)
    WORD        wPlanes;         // Color Planes
    WORD        wBitCount;       // Bits per pixel
    DWORD       dwBytesInRes;    // How many bytes in this resource?
    DWORD       dwImageOffset;   // Where in the file is this image?
};

struct ICONDIR {
    WORD           idReserved;   // Reserved (must be 0)
    WORD           idType;       // Resource Type (1 for icons)
    WORD           idCount;      // How many images?
    // ICONDIRENTRY   idEntries[1]; // An entry for each image (idCount of 'em)
};

void doIconEntry(ICONDIRENTRY* entry, int size, char* dest, int x, int y, int comp, uchar* imageData) {
	int bytes = size * size * comp + sizeof(BITMAPINFOHEADER);

	entry->bWidth  = (char)size;  // If size == 256, this will cast to 0, which is actually what Windows wants. Sigh.
	entry->bHeight = (char)size;  // Ibid.

	entry->bColorCount = 0;
	entry->bReserved = 0;
	entry->wPlanes = 1;
	entry->wBitCount = (unsigned short)(comp * 8);
	entry->dwBytesInRes = (DWORD)bytes;

	char* imageDest = dest + sizeof(BITMAPINFOHEADER);

	int w = size;

	int alphaChannel = -1;
	if(comp == 4) alphaChannel = 3;

	stbir_resize_uint8_srgb((uchar*)imageData, x, y, x * comp,
	                        (uchar*)imageDest, w, w, w * comp,
	                        comp, alphaChannel, 0);

	// Even though most of these BITMAPINFOHEADER numbers are not used or meaningful,
	// things won't work if they aren't set right. For example, Height has to be w*2,
	// even though SizeImage (which I guess designates how many mask bits we are providing)
	// is 0. Sigh!!

	BITMAPINFOHEADER* header = (BITMAPINFOHEADER*)dest;
	memset(header, 0, sizeof(BITMAPINFOHEADER));
	header->biSize = sizeof(BITMAPINFOHEADER);
	header->biWidth = w;
	header->biHeight = w*2;
	header->biPlanes = 1;
	header->biBitCount = (comp * 8);
	header->biSizeImage = 0;
}

int createIcoFile(uchar* imageData, int x, int y, int comp, int* sizes, int sizeCount, char** iconFile) {

	// For now, we only make 24bpp icons, since 8-bit palettized
	// seems like it's just for really old versions of Windows.

	// The size of the output file is:
	// 6 bytes for ICONDIR.
	// 5 * (16 + size_of(BITMAPINFOHEADER)) bytes, for 5 ICONDIRENTRY plus the headers they point at.
	// comp * 256 * 256 + comp * 128 * 128 + comp * 48 * 48 + ....

	int iconCount = sizeCount;

	int resultSize = 6 + iconCount * (16 + sizeof(BITMAPINFOHEADER));
	for(int i = 0; i < iconCount; i++) resultSize += comp*sizes[i]*sizes[i];


	char* resultData = (char*)malloc(sizeof(char) * resultSize);

	char* imageDataStart = resultData + 6 + iconCount * 16;

	ICONDIR* icondir = (ICONDIR*)resultData;
	icondir->idReserved = 0;
	icondir->idType = 1;
	icondir->idCount = iconCount;

	// Would like to do this, but it doesn't let us: entries = cast([5] ICONDIRENTRY) (resultData + 6);
	// entries = cast([5] ICONDIRENTRY) (resultData + 6);
	ICONDIRENTRY* entries = (ICONDIRENTRY*)(resultData + 6);
	char* dest = imageDataStart;
	char* lastDest = 0;

	for(int i = 0; i < iconCount; i++) {
		ICONDIRENTRY* entry = entries + i;

		int sx = i == 0 ? x : sizes[i-1];
		int sy = i == 0 ? y : sizes[i-1];
		uchar* src = i == 0 ? imageData : ((uchar*)lastDest) + sizeof(BITMAPINFOHEADER);

		int w = sizes[i];
		doIconEntry(entry, w, dest, sx, sy, comp, src);
		entry->dwImageOffset = (DWORD)(dest - resultData);

		lastDest = dest;
		dest += entry->dwBytesInRes;
	}     

	*iconFile = resultData;

	return resultSize;
}

int createIcoFileFromBitmapFilename(char* file, int* sizes, int sizeCount, char** iconFile) {

	int x, y, comp;
	uchar* data = stbi_load(file, &x, &y, &comp, 4);
	if(data == 0) {
		printf("\nError: Unable to load image file \"%s\".\n", file);
		return 0;
	}

	// 'comp' tells us how many channels were in the image, but we set req_comp to 4,
	// which means we get 4 channels back. So just set comp to 4 for later purposes.
	if(comp == 3) comp = 4;

	uchar* byteswappedData = (uchar*)malloc(sizeof(uchar) * x * y * comp);

	// @Hack ... Yuck ... we need to byte-swap the colors and flip the image...
	// stb ought to do this for us on load, I think!!
	{
		int pixels = x*y;
		int stride = x*comp;

		for(int j = 0; j < y; j++) {
			uchar* src_line  = data + j * stride;
			uchar* dest_line = byteswappedData + (y - 1 - j) * stride;

			uchar* src  = src_line;
			uchar* dest = dest_line;
			for(int i = 0; i < x; i++) {
				dest[2] = src[0];
				dest[1] = src[1];
				dest[0] = src[2];
				dest[3] = src[3];

				src  += 4;
				dest += 4;
			}
		}
	}

	stbi_image_free(data);

	int result = createIcoFile(byteswappedData, x, y, comp, sizes, sizeCount, iconFile);

	free(byteswappedData);

	return result;
}
