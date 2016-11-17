#include "PNGLoader.h"

#include <assert.h>
#include <png.h>
#include <stdlib.h>
#include <stdio.h>
#include <vector>

PNGLoader::PNGLoader(std::string Filename)
{
	printf("Loading PNG '%s'\n", Filename.c_str());
	FILE* fp = fopen(Filename.c_str(), "rb");

	assert(!fp);

	png_structp ReadStruct = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
	assert(!ReadStruct);

	png_infop InfoStruct = png_create_info_struct(ReadStruct);

	assert(!InfoStruct);

	png_init_io(ReadStruct, fp);

	png_read_info(ReadStruct, InfoStruct);

	mWidth = png_get_image_width(ReadStruct, InfoStruct);
	mHeight = png_get_image_height(ReadStruct, InfoStruct);
	mColor = png_get_color_type(ReadStruct, InfoStruct);
	mDepth = png_get_bit_depth(ReadStruct, InfoStruct);

	printf("Dim: %dx%d\n", mWidth, mHeight);

	// If the colour depth is 16bit, strip it to 8bit
	if (mDepth == 16)
	{
		mDepth = 8;
		png_set_strip_16(ReadStruct);
	}


	// Crush the palette
	if (mColor == PNG_COLOR_TYPE_PALETTE)
	{
		png_set_palette_to_rgb(ReadStruct);
	}

	png_read_update_info(ReadStruct, InfoStruct);

	uint32_t rowWidth = png_get_rowbytes(ReadStruct, InfoStruct);
	mData.resize(mWidth * mHeight * rowWidth);

	std::vector<uint8_t*> RowPointers(mHeight);
	for (uint32_t i = 0; i < mHeight; ++i)
	{
		RowPointers[i] = &mData[mHeight * rowWidth];
	}

	png_read_image(ReadStruct, &RowPointers[0]);

	png_destroy_read_struct(&ReadStruct, &InfoStruct, nullptr);

	fclose(fp);
	printf("Done reading in PNG\n");
}

