
#include <iostream>
#include <time.h>

//Globals.h

#include <string>
#include <cstdarg>
#include <cstdio>

typedef std::string AString;
#define TOLUA_TEMPLATE_BIND(x)
#if defined(__GNUC__)
#define FORMATSTRING(formatIndex, va_argsIndex) __attribute__((format (printf, formatIndex, va_argsIndex)))
#else
#define FORMATSTRING(x,y)
#endif
#define ASSERT(x)
#define UNUSED(x)
#define abstract
#define override
typedef long long Int64;
typedef int       Int32;
typedef short     Int16;

typedef unsigned long long UInt64;
typedef unsigned int       UInt32;
typedef unsigned short     UInt16;

void LOGERROR(const char* a_Format, ...) FORMATSTRING(1,2);
void LOGERROR(const char* a_Format, ...)
{
	va_list argList;
	va_start(argList, a_Format);
	vprintf(a_Format, argList);
	va_end(argList);
}

#include "WorldMock.h"
#include "BenchBlockInfo.h"
#include "LightingThreadOld.h"
#include "LightingThreadHeightMap.h"

int main(int argc, char** argv)
{
	std::cout << "preping for tests" << std::endl;
	cWorld * world = new cWorld();
	unsigned char BlockTypes[16 * 16 * 256];
	memset(BlockTypes, 1, 32768);
	memset(BlockTypes + 32768, 0, 32768);
	for (int i = 0; i < 3 * 3; i++)
	{
		world->m_BlockTypes[i].SetBlockTypes(BlockTypes);
		memset(world->m_HeightMap[i], 127, 256);
	}
	
	std::cout << "starting old version" << std::endl;
	cLightingThreadOld * oldthread = new cLightingThreadOld;
	oldthread->m_World = world;
	clock_t Begin = clock();
	for (int i = 0; i < 100; i++)
	{
		oldthread->LightChunk(1,1);
	}
	
	clock_t Ticks = clock() - Begin;
	std::cout << "Lighting 100 chunks took " << Ticks << ", ticks (" << (double)Ticks / CLOCKS_PER_SEC <<" sec)" << std::endl;
	
	std::cout << "starting height map version" << std::endl;
	cLightingThreadHeightMap * heightthread = new cLightingThreadHeightMap;
	heightthread->m_World = world;
	Begin = clock();
	for (int i = 0; i < 100; i++)
	{
		heightthread->LightChunk(1,1);
	}
	
	Ticks = clock() - Begin;
	std::cout << "Lighting 100 chunks took " << Ticks << ", ticks (" << (double)Ticks / CLOCKS_PER_SEC <<" sec)" << std::endl;
}
