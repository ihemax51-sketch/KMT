#include "IFSoxEffect.h"
#include <BSLib/multibyte.h>
#include <GlobalHelpersThatHaveNoHomeYet.h>
#include <ctime>
#include <sstream>
#include <Game.h>

GFX_IMPLEMENT_DYNCREATE(CIFSoxEffect, CIFWnd)
GFX_BEGIN_MESSAGE_MAP(CIFSoxEffect, CIFWnd)
GFX_END_MESSAGE_MAP()

time_t lastUpdates = clock();

CIFSoxEffect::CIFSoxEffect(void)
{
    m_itempic = 0;
    m_ddj = 0;
    ddjPath = "";
    ddjCurrentIndex = 0;
    ddjCount = 0;
    ddjMinSpeed = 0;
    operational = 0;
}
CIFSoxEffect::~CIFSoxEffect(void)
{
}
bool CIFSoxEffect::OnCreate(long ln)
{
    CIFWnd::OnCreate(ln);

    wnd_rect sz;
    sz.pos.x = 0;
    sz.pos.y = 0;
    sz.size.width = 256;
    sz.size.height = 256;
    m_ddj = (CIFStatic*)CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), sz, 0, 0);
    m_ddj->TB_Func_13("clientlibrary\\soxeffect\\pickup_rarebox_10.ddj", 0, 0);
    sz.pos.x = 109;
    sz.pos.y = 109;
    sz.size.width = 38;
    sz.size.height = 38;
    m_itempic = (CIFStatic*)CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), sz, 1, 0);

    UpdateMenuSize();
    this->ShowGWnd(false);
    return true;
}
void CIFSoxEffect::UpdateMenuSize()
{
    int PosX = 0, PosY = 0;
    PosY = (g_CGame->GetRes().res->height/2) - (this->GetSize().height/2) - 250;
    PosX = (g_CGame->GetRes().res->width/2) - (this->GetSize().width/2);
    this->MoveGWnd(PosX, PosY);
    BringToFront();
}
void CIFSoxEffect::OnUpdate()
{
    if(operational)
    {
        if(float(clock() - lastUpdates) / CLOCKS_PER_SEC > ddjMinSpeed)
        {
            lastUpdates = clock();

            std::ostringstream temp;
            temp << ddjCurrentIndex;
            std::string num = temp.str();

            std::string currentDDJ = ddjPath + num + ".ddj";

            m_ddj->TB_Func_13(currentDDJ.c_str(),0,0);

            ddjCurrentIndex++;

            if(ddjCurrentIndex == ddjCount)
            {
                ddjCurrentIndex = 0;
                operational = false;
                this->ShowGWnd(false);
            }
        }
    }
}
