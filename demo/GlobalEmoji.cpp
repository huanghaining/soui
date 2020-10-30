#include "stdafx.h"
#include "GlobalEmoji.h"
#include "../components/render-skia/render-skia.h"
#include "../SOUI/include/SApp.h"

GlobalEmoji::GlobalEmoji()
{
	SBitmap_Skia * bitmap = (SBitmap_Skia*)SApplication::getSingleton().LoadImage2("C:\\Users\\Administrator\\Desktop\\123.jpg");
	m_bitMapArray.push_back(bitmap);
}


GlobalEmoji::~GlobalEmoji()
{
}

SOUI::SBitmap_Skia* GlobalEmoji::getBitMap(int index)
{
	return m_bitMapArray[index];
}

GlobalEmoji* GlobalEmoji::getInstance()
{
	static GlobalEmoji instance;
	return &instance;
}
