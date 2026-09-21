#include "IDamageText.h"
#include "Game.h"
#include "unsorted.h"
#include "BSLib/Debug.h"
#include "EntityManagerClient.h"

void CIDamageText::Func_9_IMPL() {

    // Delete object if 3 seconds have passed.
    if (5000 < m_ageInMilliseconds) {
        BS_ASSERT_MSG(m_someId == 0, "Illegal destructor call.\n(Only creations using CreateEntityIC should use DestroyEntityIC).");
        g_pGfxEttManager->DestroyEntityIC(this);
        return;
    }
    pad_0x158 = 3;
    const float width = CGame::GetClientDimensionStuff().width;
    const float height = CGame::GetClientDimensionStuff().height / 1200.0f;

    m_positionAnimator.FUN_009c7bc0(g_Timebase_AnimationSpeedCorrection);
    m_scaleAnimator.FUN_009c7bc0(g_Timebase_AnimationSpeedCorrection);
    m_opacityAnimator.FUN_009c7bc0(g_Timebase_AnimationSpeedCorrection);

    // Opacity
    const float fVar3 = m_opacityAnimator.field_0;
    byte byVar1;
    if (fVar3 >= 1.0) {
        byVar1 = 255;
    }
    else {
        byVar1 = static_cast<byte>(fVar3 * 255.0f);
    }

    m_color &= D3DCOLOR_RGBA(255, 255, 255, 0);
    m_color |= D3DCOLOR_RGBA(0, 0, 0, byVar1);

    // Scale ?
    const float fVar4 = m_scaleAnimator.field_0 + 0.05;
    field_0x164 = fVar4 * (width / 1600.0f);
    field_0x168 = fVar4 * height;

    // Move up or down
    if (m_moveUp) {
        field_0x15c = m_positionAnimator.field_0 * (height + 2);
    }
    else {
        field_0x15c = -m_positionAnimator.field_0 * (height + 1.5);
    }
    //printf("%p \n", this);
    FUN_00a626f0();
}
