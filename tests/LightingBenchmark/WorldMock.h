
#include "../../src/ChunkDataCallback.h"

class cWorld
{
public:
	void ChunkLighted(int X, int Z, unsigned char * BlockLight, unsigned char * SkyLight)
	{
	}
	bool GetChunkData(int X, int Z, cChunkDataCallback& Reader)
	{
		unsigned char (*ptr) [256] = &(m_HeightMap[(X + Z * 3) * 16 * 16]);
		Reader.HeightMap(ptr); 
		Reader.ChunkData(m_BlockTypes[X + Z * 3]);
		return true;
	}
	
	cChunkData m_BlockTypes[3*3];
	unsigned char m_HeightMap[3*3][256];
};
