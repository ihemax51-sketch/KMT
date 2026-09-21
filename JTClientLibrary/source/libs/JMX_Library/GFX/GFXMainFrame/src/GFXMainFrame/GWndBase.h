#pragma once

#include "BSLib/BSLib.h"
#include "support/AbiTest.h"
#include <Windows.h>
#include <list>

/// Holds data about events, mainly in the 3d-space. It's unknown if this structure is also used for the general
/// messaging system.
struct Event3D {
    char pad_0000[4]; //0x0000
    int Msg; //0x0004
    char pad_0008[12]; //0x0008
    int lParam; //0x0014
    int wParam; //0x0018
}; //Size: 0x001C

class CGWndBase;



typedef std::n_list<CGWndBase *> GWND_LIST;

/**
 * This class acts as a parent for any control and provides a container for child-controls
 * \remark As of current state of knowledge, \ref CGWndBase::OnUpdate must be overwritten in any derived class to
 *         avoid an endless loop
 *
 */
class CGWndBase : public CObjChild { // size 32 + 100 = 132
GFX_DECLARE_DYNAMIC_EXISTING(CGWndBase, 0x0110F630)

private:
    enum eListLockType : BYTE {
        LIST_LOCK_READ = 1,
        LIST_LOCK_WRITE = 2
    };

public:
    struct wnd_size {
        int width;
        int height;
    };

    struct wnd_pos {
        int x;
        int y;
    };

    struct wnd_rect {

        int bottom() const {
            return pos.y + size.height;
        }

        int left() const {
            return pos.x;
        }

        int right() const {
            return pos.x + size.width;
        }

        int top() const {
            return pos.y;
        }

        wnd_pos pos;
        wnd_size size;
    };


public:

    /// \address 00B90A80
    virtual void Func_7(CGWndBase *a2);

    /// Add the given control as a child.
    /// \address 00B90AC0
    virtual int AddControlToList(CGWndBase *a2);

    /// Remove the given control from the list of children.
    /// \address 00B913D0
    virtual void RemoveControlFromList(CGWndBase *a2);

    /// Called when this instance is created.
    /// \address 00B8F1F0
    virtual bool OnCreate(long ln);

    /// Called before this instance is deleted.
    /// \address 00B8F200
    virtual bool OnRelease();

    /// Called once per frame (blocking).
    /// \address 00B92070
    virtual void OnUpdate();

    virtual void RenderMyself();

    virtual void RenderMyChildren();

    virtual void Func_15();

    virtual void Render();

    /// \address 00B91440
    virtual bool OnMouseEvent(Event3D *pEvent);

    /// \address 00b8f380
    virtual bool OnWndMessage(const Event3D *pEvent);

    /// \address 00b8f3c0
    virtual void OnGfxMessage(Event3D *pEvent);

    virtual void BringToFront();

    /// Resize this control
    /// \param width Width in pixels
    /// \param height Height in pixels
    virtual void SetGWndSize(int width, int height);

    virtual void Func_22(int x, int y);

    /// Set the visibility state of this control.
    /// \param bVisible New visibility state.
    virtual void ShowGWnd(bool bVisible);


public:

    /// Constructor, what shall I say about this?
    /// \address 00B8FCD0
    CGWndBase();

    /// Get visible state of this control
    /// \retval true Control is visible
    /// \retval false Control is not visible
    bool IsVisible();

    void ApplyGlobalScale(int x);

    /// \address 00B9DA70
    void sub_B9DA70(bool b);

    /// Returns the numeric identifier of this control.
    /// \returns ID of this control
    int UniqueID() const;
    void SetUniqueID(int A);
    void SetFocus_MAYBE();

    /// \brief Get the bounds of this object (position and size)
    /// \return The bounds of this object
    wnd_rect GetBounds() const;

    /// \brief Get the position of this object
    /// \return The position of this object
    /// \address 00B8F460
    wnd_pos GetPos() const;

    /// \brief Get the size of this object
    /// \return The size of this object
    /// \address 00B8F480
    wnd_size GetSize() const;

    /// \address 00b9db00
    bool IsDragable() const;

    /// \address 00b9d8f0
    bool SendMessageToParentDispatcher(DWORD dwMSG, DWORD lParam, DWORD wParam);

    /// \address 00b90d90
    void SetPosition(int nX, int nY);

    /// \address 00b9d940
    bool IsParentOf(CGWndBase *pGWnd) {
        for (; pGWnd != NULL; pGWnd = pGWnd->GetParentControl())
            if (this == pGWnd) {
                return true;
            }
        return false;
    }

    /// \address 00b8f4d0
    int GethgWnd() const;

    /// \brief Allow Window to click or move
    /// \address 00b8f540
    void SetClickable(bool bState);

    /// \brief Set the drag window by mouse
    /// \address 00b9daf0
    void SetDragable(bool bState);

    /// \address 00b9daf0
    void LockList(BYTE btLockType);

    /// \address 00b8f300
    void UnLockList(BYTE btLockType);

    /// \address 00b90b00
    CGWndBase *GetChildControl(DWORD dwID);


protected:
    /// \address 00B8F440
    void sub_B8F440(const RECT &rect);

public: // 0046c8b6 getting call by global ptr
    CGWndBase *GetParentControl() const;

private:
    LONG m_lnListLockRead; //0x0020
    LONG m_lnListLockWrite; //0x0024
    bool N000006F5; //0x0028
    char pad_0029[3]; //0x0029
    int m_hgWnd; //0x002C
    CGWndBase *m_parentControl; //0x0030
    int m_UniqueID; //0x0034
    int N000006F9; //0x0038
    bool N000006FB; //0x003C
    char pad_003D[3]; //0x003D
    wnd_rect bounds; //0x0040 was private, 40
    RECT N000006FD; //0x0050
    char pad_0060[1]; //0x0060
protected:
    bool m_bVisible; //0x0061 61
    bool m_bClickable; //0x0062
    bool N0000074F; //0x0063
    bool m_bDragable; //0x0064
private:
    char pad_0065[3]; //0x0065
    int N00000703; //0x0068
    int m_iflist; //0x006C
    short N00000705; //0x0070
    char pad_0072[6]; //0x0072
public:
    GWND_LIST N00000707;//0x0078
private:
BEGIN_FIXTURE()
        ENSURE_SIZE(132)
    END_FIXTURE()

    RUN_FIXTURE(CGWndBase)
};
