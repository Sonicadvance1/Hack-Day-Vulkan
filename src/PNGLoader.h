#pragma once

#include <stdint.h>
#include <string>
#include <vector>

class PNGLoader
{
public:
	PNGLoader(std::string Filename);

	// Data
	const std::vector<uint8_t>& GetData() const { return mData; }

	// Information
	uint32_t GetWidth() const { return mWidth; }
	uint32_t GetHeight() const { return mHeight; }

private:
	uint32_t mWidth, mHeight;
	uint8_t mColor, mDepth;
	std::vector<uint8_t> mData;

};
