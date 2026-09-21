#pragma once

class CIFScrollbar;

class CLinkedScroll {

    struct CPMLLayer {
        // TODO: Reverse me
    };

public:
    virtual void LinkScrollbar(void *a1);

    virtual void Func_01(int a2) = 0;

    /// \brief Get number of items currently stored in this control
    /// \returns Number of items in this control
    virtual int GetNumberOfItems() const = 0;

    virtual void Func_03() = 0;


    CLinkedScroll();


public:
    CIFScrollbar *m_Scrollbar; // 0x36C
    CPMLLayer *m_PMLLayer; // // 0x370
    int SelectedIndexStartby0; // 0x374
    int OnHoverIndexStartby1; // 0x378
    unsigned char N0000969B;

};
