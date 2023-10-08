#include "Files.h"
#include "Logging.h"

#include <cstdio>
#include <memory>

std::vector<uint8_t> LoadBinaryFile(const char* pFilename)
{
	std::vector<uint8_t> ret;

	FILE* fp = nullptr;
	fopen_s(&fp, pFilename, "rb");
	if (fp == nullptr)
	{
		LOGERROR("Failed to load file (%s)", pFilename);
		return ret;
	}		

	fseek(fp, 0, SEEK_END);
	size_t fileSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	ret.resize(fileSize);
	fread(ret.data(), fileSize, 1, fp);
	fclose(fp);

	return ret;
}
