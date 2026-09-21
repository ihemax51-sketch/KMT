#include "Controler.h"
#include <windows.h>

// Yeni imleci yüklemek için fonksiyon
void CControler::SetCustomCursor(int resourceId)
{
    // Kaynak dosyasından yeni imleç dosyasını yükleyin
    HCURSOR hCursor = LoadCursor(GetModuleHandle(NULL), MAKEINTRESOURCE(resourceId));
    if (hCursor)
    {
        // Geçerli imleci serbest bırakın (eğer varsa)
        if (this->m_currentCursor)
        {
            DestroyCursor(this->m_currentCursor);
        }

        // Yeni imleci ayarlayın
        this->m_currentCursor = hCursor;
        SetCursor(hCursor);
    }
    else
    {
        // İmleç yüklenemedi
        //printf("Failed to load cursor from resource.");
    }
}
