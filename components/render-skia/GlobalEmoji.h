#pragma once
#include<vector>
using namespace std;
class GlobalEmoji
{
public:
	GlobalEmoji();
	~GlobalEmoji();

	SBitmap_Skia* getBitMap(int index);
	static GlobalEmoji* getInstance();

	vector<SBitmap_Skia*> m_bitMapArray;
};

