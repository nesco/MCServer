
// LightingThread.cpp

// Implements the cLightingThreadOld class representing the thread that processes requests for lighting

//Globals.h

#include <string>

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
typedef long long Int64;
typedef int       Int32;
typedef short     Int16;

typedef unsigned long long UInt64;
typedef unsigned int       UInt32;
typedef unsigned short     UInt16;

void inline LOGERROR(const char* a_Format, ...) FORMATSTRING(1,2);
void inline LOGERROR(const char* a_Format, ...)
{
	va_list argList;
	va_start(argList, a_Format);
	vprintf(a_Format, argList);
	va_end(argList);
}

#include "../../src/ChunkDef.h"
#include "BenchBlockInfo.h"
#include "../../src/BlockID.h"

#include "LightingThreadOld.h"
#include "../../src/ChunkDataCallback.h"
#include "WorldMock.h"






/// Chunk data callback that takes the chunk data and puts them into cLightingThreadOld's m_BlockTypes[] / m_HeightMap[]:
class cReader :
	public cChunkDataCallback
{
	virtual void ChunkData(const cChunkData & a_ChunkBuffer) override
	{
		BLOCKTYPE * OutputRows = m_BlockTypes;
		int InputIdx = 0;
		int OutputIdx = m_ReadingChunkX + m_ReadingChunkZ * cChunkDef::Width * 3;
		for (int y = 0; y < cChunkDef::Height; y++)
		{
			for (int z = 0; z < cChunkDef::Width; z++)
			{
				a_ChunkBuffer.CopyBlockTypes(OutputRows + OutputIdx * 16, InputIdx * 16, 16);
				InputIdx++;
				OutputIdx += 3;
			}  // for z
			// Skip into the next y-level in the 3x3 chunk blob; each level has cChunkDef::Width * 9 rows
			// We've already walked cChunkDef::Width * 3 in the "for z" cycle, that makes cChunkDef::Width * 6 rows left to skip
			OutputIdx += cChunkDef::Width * 6;
		}  // for y
	}  // BlockTypes()
	
	
	virtual void HeightMap(const cChunkDef::HeightMap * a_Heightmap) override
	{
		typedef struct {HEIGHTTYPE m_Row[16]; } ROW;
		ROW * InputRows  = (ROW *)a_Heightmap;
		ROW * OutputRows = (ROW *)m_HeightMap;
		int InputIdx = 0;
		int OutputIdx = m_ReadingChunkX + m_ReadingChunkZ * cChunkDef::Width * 3;
		for (int z = 0; z < cChunkDef::Width; z++)
		{
			OutputRows[OutputIdx] = InputRows[InputIdx++];
			OutputIdx += 3;
		}  // for z
	}
	
public:
	int m_ReadingChunkX;  // 0, 1 or 2; x-offset of the chunk we're reading from the BlockTypes start
	int m_ReadingChunkZ;  // 0, 1 or 2; z-offset of the chunk we're reading from the BlockTypes start
	BLOCKTYPE * m_BlockTypes;  // 3x3 chunks of block types, organized as a single XZY blob of data (instead of 3x3 XZY blobs)
	HEIGHTTYPE * m_HeightMap;  // 3x3 chunks of height map,  organized as a single XZY blob of data (instead of 3x3 XZY blobs)
} ;





///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// cLightingThreadOld:

cLightingThreadOld::cLightingThreadOld(void) :
	m_World(NULL)
{
}





cLightingThreadOld::~cLightingThreadOld()
{
}








void cLightingThreadOld::LightChunk(int X, int Z)
{
	cChunkDef::BlockNibbles BlockLight, SkyLight;
	
	ReadChunks(X, Z);
	
	PrepareBlockLight();
	CalcLight(m_BlockLight);
	
	PrepareSkyLight();
	
	/*
	// DEBUG: Save chunk data with highlighted seeds for visual inspection:
	cFile f4;
	if (
		f4.Open(Printf("Chunk_%d_%d_seeds.grab", a_Item.x, a_Item.z), cFile::fmWrite)
	)
	{
		for (int z = 0; z < cChunkDef::Width * 3; z++)
		{
			for (int y = cChunkDef::Height / 2; y >= 0; y--)
			{
				unsigned char Seeds     [cChunkDef::Width * 3];
				memcpy(Seeds, m_BlockTypes + y * BlocksPerYLayer + z * cChunkDef::Width * 3, cChunkDef::Width * 3);
				for (int x = 0; x < cChunkDef::Width * 3; x++)
				{
					if (m_IsSeed1[y * BlocksPerYLayer + z * cChunkDef::Width * 3 + x])
					{
						Seeds[x] = E_BLOCK_DIAMOND_BLOCK;
					}
				}
				f4.Write(Seeds, cChunkDef::Width * 3);
			}
		}
	}
	//*/
	
	CalcLight(m_SkyLight);
	
	/*
	// DEBUG: Save XY slices of the chunk data and lighting for visual inspection:
	cFile f1, f2, f3;
	if (
		f1.Open(Printf("Chunk_%d_%d_data.grab",  a_Item.x, a_Item.z), cFile::fmWrite) &&
		f2.Open(Printf("Chunk_%d_%d_sky.grab",   a_Item.x, a_Item.z), cFile::fmWrite) &&
		f3.Open(Printf("Chunk_%d_%d_glow.grab",  a_Item.x, a_Item.z), cFile::fmWrite)
	)
	{
		for (int z = 0; z < cChunkDef::Width * 3; z++)
		{
			for (int y = cChunkDef::Height / 2; y >= 0; y--)
			{
				f1.Write(m_BlockTypes + y * BlocksPerYLayer + z * cChunkDef::Width * 3, cChunkDef::Width * 3);
				unsigned char SkyLight  [cChunkDef::Width * 3];
				unsigned char BlockLight[cChunkDef::Width * 3];
				for (int x = 0; x < cChunkDef::Width * 3; x++)
				{
					SkyLight[x]   = m_SkyLight  [y * BlocksPerYLayer + z * cChunkDef::Width * 3 + x] << 4;
					BlockLight[x] = m_BlockLight[y * BlocksPerYLayer + z * cChunkDef::Width * 3 + x] << 4;
				}
				f2.Write(SkyLight,   cChunkDef::Width * 3);
				f3.Write(BlockLight, cChunkDef::Width * 3);
			}
		}
	}
	//*/
	
	CompressLight(m_BlockLight, BlockLight);
	CompressLight(m_SkyLight, SkyLight);
	
	m_World->ChunkLighted(X, Z, BlockLight, SkyLight);

}





bool cLightingThreadOld::ReadChunks(int a_ChunkX, int a_ChunkZ)
{
	cReader Reader;
	Reader.m_BlockTypes = m_BlockTypes;
	Reader.m_HeightMap  = m_HeightMap;
	
	for (int z = 0; z < 3; z++)
	{
		Reader.m_ReadingChunkZ = z;
		for (int x = 0; x < 3; x++)
		{
			Reader.m_ReadingChunkX = x;
			if (!m_World->GetChunkData(a_ChunkX + x - 1, a_ChunkZ + z - 1, Reader))
			{
				return false;
			}
		}  // for z
	}  // for x
	
	memset(m_BlockLight, 0, sizeof(m_BlockLight));
	memset(m_SkyLight,   0, sizeof(m_SkyLight));
	return true;
}





void cLightingThreadOld::PrepareSkyLight(void)
{
	// Clear seeds:
	memset(m_IsSeed1, 0, sizeof(m_IsSeed1));
	m_NumSeeds = 0;
	
	// Walk every column that has all XZ neighbors
	for (int z = 1; z < cChunkDef::Width * 3 - 1; z++)
	{
		int BaseZ = z * cChunkDef::Width * 3;
		for (int x = 1; x < cChunkDef::Width * 3 - 1; x++)
		{
			int idx = BaseZ + x;
			int Current   = m_HeightMap[idx] + 1;
			int Neighbor1 = m_HeightMap[idx + 1] + 1;  // X + 1
			int Neighbor2 = m_HeightMap[idx - 1] + 1;  // X - 1
			int Neighbor3 = m_HeightMap[idx + cChunkDef::Width * 3] + 1;  // Z + 1
			int Neighbor4 = m_HeightMap[idx - cChunkDef::Width * 3] + 1;  // Z - 1
			int MaxNeighbor = std::max(std::max(Neighbor1, Neighbor2), std::max(Neighbor3, Neighbor4));  // Maximum of the four neighbors
			
			// Fill the column from the top down to Current with all-light:
			for (int y = cChunkDef::Height - 1, Index = idx + y * BlocksPerYLayer; y >= Current; y--, Index -= BlocksPerYLayer)
			{
				m_SkyLight[Index] = 15;
			}
			
			// Add Current as a seed:
			if (Current < cChunkDef::Height)
			{
				int CurrentIdx = idx + Current * BlocksPerYLayer;
				m_IsSeed1[CurrentIdx] = true;
				m_SeedIdx1[m_NumSeeds++] = CurrentIdx;
			}
			
			// Add seed from Current up to the highest neighbor:
			for (int y = Current + 1, Index = idx + y * BlocksPerYLayer; y < MaxNeighbor; y++, Index += BlocksPerYLayer)
			{
				m_IsSeed1[Index] = true;
				m_SeedIdx1[m_NumSeeds++] = Index;
			}
		}
	}
}





void cLightingThreadOld::PrepareBlockLight(void)
{
	// Clear seeds:
	memset(m_IsSeed1, 0, sizeof(m_IsSeed1));
	memset(m_IsSeed2, 0, sizeof(m_IsSeed2));
	m_NumSeeds = 0;

	// Walk every column that has all XZ neighbors, make a seed for each light-emitting block:
	for (int z = 1; z < cChunkDef::Width * 3 - 1; z++)
	{
		int BaseZ = z * cChunkDef::Width * 3;
		for (int x = 1; x < cChunkDef::Width * 3 - 1; x++)
		{
			int idx = BaseZ + x;
			for (int y = m_HeightMap[idx], Index = idx + y * BlocksPerYLayer; y >= 0; y--, Index -= BlocksPerYLayer)
			{
				if (cBlockInfo::GetLightValue(m_BlockTypes[Index]) == 0)
				{
					continue;
				}
				
				// Add current block as a seed:
				m_IsSeed1[Index] = true;
				m_SeedIdx1[m_NumSeeds++] = Index;

				// Light it up:
				m_BlockLight[Index] = cBlockInfo::GetLightValue(m_BlockTypes[Index]);
			}
		}
	}
}





void cLightingThreadOld::CalcLight(NIBBLETYPE * a_Light)
{
	int NumSeeds2 = 0;
	while (m_NumSeeds > 0)
	{
		// Buffer 1 -> buffer 2
		memset(m_IsSeed2, 0, sizeof(m_IsSeed2));
		NumSeeds2 = 0;
		CalcLightStep(a_Light, m_NumSeeds, m_IsSeed1, m_SeedIdx1, NumSeeds2, m_IsSeed2, m_SeedIdx2);
		if (NumSeeds2 == 0)
		{
			return;
		}
		
		// Buffer 2 -> buffer 1
		memset(m_IsSeed1, 0, sizeof(m_IsSeed1));
		m_NumSeeds = 0;
		CalcLightStep(a_Light, NumSeeds2, m_IsSeed2, m_SeedIdx2, m_NumSeeds, m_IsSeed1, m_SeedIdx1);
	}
}





void cLightingThreadOld::CalcLightStep(
	NIBBLETYPE * a_Light, 
	int a_NumSeedsIn,    unsigned char * a_IsSeedIn,  unsigned int * a_SeedIdxIn,
	int & a_NumSeedsOut, unsigned char * a_IsSeedOut, unsigned int * a_SeedIdxOut
)
{
	UNUSED(a_IsSeedIn);
	int NumSeedsOut = 0;
	for (int i = 0; i < a_NumSeedsIn; i++)
	{
		int SeedIdx = a_SeedIdxIn[i];
		int SeedX = SeedIdx % (cChunkDef::Width * 3);
		int SeedZ = (SeedIdx / (cChunkDef::Width * 3)) % (cChunkDef::Width * 3);
		int SeedY = SeedIdx / BlocksPerYLayer;
		
		// Propagate seed:
		if (SeedX < cChunkDef::Width * 3 - 1)
		{
			PropagateLight(a_Light, SeedIdx, SeedIdx + 1, NumSeedsOut, a_IsSeedOut, a_SeedIdxOut);
		}
		if (SeedX > 0)
		{
			PropagateLight(a_Light, SeedIdx, SeedIdx - 1, NumSeedsOut, a_IsSeedOut, a_SeedIdxOut);
		}
		if (SeedZ < cChunkDef::Width * 3 - 1)
		{
			PropagateLight(a_Light, SeedIdx, SeedIdx + cChunkDef::Width * 3, NumSeedsOut, a_IsSeedOut, a_SeedIdxOut);
		}
		if (SeedZ > 0)
		{
			PropagateLight(a_Light, SeedIdx, SeedIdx - cChunkDef::Width * 3, NumSeedsOut, a_IsSeedOut, a_SeedIdxOut);
		}
		if (SeedY < cChunkDef::Height - 1)
		{
			PropagateLight(a_Light, SeedIdx, SeedIdx + cChunkDef::Width * cChunkDef::Width * 3 * 3, NumSeedsOut, a_IsSeedOut, a_SeedIdxOut);
		}
		if (SeedY > 0)
		{
			PropagateLight(a_Light, SeedIdx, SeedIdx - cChunkDef::Width * cChunkDef::Width * 3 * 3, NumSeedsOut, a_IsSeedOut, a_SeedIdxOut);
		}
	}  // for i - a_SeedIdxIn[]
	a_NumSeedsOut = NumSeedsOut;
}





void cLightingThreadOld::CompressLight(NIBBLETYPE * a_LightArray, NIBBLETYPE * a_ChunkLight)
{
	int InIdx = cChunkDef::Width * 49;  // Index to the first nibble of the middle chunk in the a_LightArray
	int OutIdx = 0;
	for (int y = 0; y < cChunkDef::Height; y++)
	{
		for (int z = 0; z < cChunkDef::Width; z++)
		{
			for (int x = 0; x < cChunkDef::Width; x += 2)
			{
				a_ChunkLight[OutIdx++] = (a_LightArray[InIdx + 1] << 4) | a_LightArray[InIdx];
				InIdx += 2;
			}
			InIdx += cChunkDef::Width * 2;
		}
		// Skip into the next y-level in the 3x3 chunk blob; each level has cChunkDef::Width * 9 rows
		// We've already walked cChunkDef::Width * 3 in the "for z" cycle, that makes cChunkDef::Width * 6 rows left to skip
		InIdx += cChunkDef::Width * cChunkDef::Width * 6;
	}
}




