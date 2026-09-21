#include <IFEdit.h>
#include <IFMainFrame.h>
#include <IFStretchWnd.h>


class CIFSoxEffect : public CIFWnd
{
GFX_DECLARE_DYNCREATE(CIFSoxEffect)
GFX_DECLARE_MESSAGE_MAP(CIFSoxEffect)

private:
    CIFSoxEffect(void);
    ~CIFSoxEffect(void);
    bool OnCreate(long ln) override;
    void OnUpdate() override;
public:
    CIFStatic* m_itempic;
    CIFStatic* m_ddj;
    std::string ddjPath;
    int ddjCurrentIndex;
    int ddjCount;
    float ddjMinSpeed;
    bool operational;
    void UpdateMenuSize();
};