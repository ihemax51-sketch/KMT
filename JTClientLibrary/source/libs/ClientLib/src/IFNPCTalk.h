#pragma once

#include "IFFrame.h"
#include "IFTextBox.h"
#include "IFVerticalScroll.h"

class CIF_NPCTalk : public CIFFrame
{
public:
    void OnListChatThingIMPL(int a1, int a2);
    void FUN_006fcd60(int p1, int p2, int p3);
	CIFTextBox* m_textBox; //0x07B0 
	CIFVerticalScroll* m_verticalScroll; //0x07B4 
	char pad_0x07B8[0xB8]; //0x07B8
	float N000029F6; //0x0870 
	float N000029F7; //0x0874 
	char pad_0x0878[0x70]; //0x0878

void WriteNpcInfo_Maybe();
}; //Size=0x08E8