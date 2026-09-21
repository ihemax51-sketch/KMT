#pragma once

#include "IFWnd.h"
#include "IFScrollManager.h"
#include "LinkedScroll.h"

class CIFPML : public CIFWnd, public CLinkedScroll
{
GFX_DECLARE_DYNAMIC_EXISTING(CIFPML, 0x00ee9634)
}; //Size: 0x0384C