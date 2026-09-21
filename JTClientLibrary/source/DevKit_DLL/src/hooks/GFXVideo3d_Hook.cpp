#include "Hooks.h"
#include "GFXVideo3d_Hook.h"

#include <string.h>

extern std::vector<endscene_handler_t> hooks_endscene;
extern std::vector<create_handler_t> hooks_create;
extern std::vector<setsize_handler_t> hooks_setsize_pre;
extern std::vector<setsize_handler_t> hooks_setsize_post;

namespace {

void ApplyTextureQualityBoost(IDirect3DDevice9* device)
{
	if (!device)
		return;

	D3DCAPS9 caps;
	if (FAILED(device->GetDeviceCaps(&caps)))
		return;

	const DWORD maxAnisotropy = caps.MaxAnisotropy;
	const DWORD anisotropy = maxAnisotropy >= 16 ? 16 : maxAnisotropy;
	const float lodBiasValue = -0.65f;
	DWORD lodBias = 0;
	memcpy(&lodBias, &lodBiasValue, sizeof(lodBias));

	device->SetRenderState(D3DRS_DITHERENABLE, TRUE);
	device->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, TRUE);
	device->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, TRUE);
	device->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_GOURAUD);

	for (DWORD stage = 0; stage < 8; ++stage) {
		if (anisotropy >= 2) {
			device->SetSamplerState(stage, D3DSAMP_MAXANISOTROPY, anisotropy);
			device->SetSamplerState(stage, D3DSAMP_MINFILTER, D3DTEXF_ANISOTROPIC);
			device->SetSamplerState(stage, D3DSAMP_MAGFILTER, D3DTEXF_ANISOTROPIC);
		} else {
			device->SetSamplerState(stage, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
			device->SetSamplerState(stage, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
		}

		device->SetSamplerState(stage, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
		device->SetSamplerState(stage, D3DSAMP_MIPMAPLODBIAS, lodBias);
	}
}

void ApplyColorQualityBoost(IDirect3DDevice9* device)
{
	if (!device)
		return;

	D3DGAMMARAMP ramp;
	const float contrast = 1.26f;
	const float brightness = -0.018f;
	const float redBoost = 1.02f;
	const float greenBoost = 1.04f;
	const float blueBoost = 1.08f;

	for (int i = 0; i < 256; ++i) {
		float value = static_cast<float>(i) / 255.0f;
		value = ((value - 0.5f) * contrast) + 0.5f + brightness;
		if (value < 0.0f)
			value = 0.0f;
		if (value > 1.0f)
			value = 1.0f;

		float red = value * redBoost;
		float green = value * greenBoost;
		float blue = value * blueBoost;
		if (red > 1.0f)
			red = 1.0f;
		if (green > 1.0f)
			green = 1.0f;
		if (blue > 1.0f)
			blue = 1.0f;

		ramp.red[i] = static_cast<WORD>(red * 65535.0f);
		ramp.green[i] = static_cast<WORD>(green * 65535.0f);
		ramp.blue[i] = static_cast<WORD>(blue * 65535.0f);
	}

	device->SetGammaRamp(0, D3DSGR_NO_CALIBRATION, &ramp);
}

}

bool CGFXVideo3D_Hook::CreateThingsHook(HWND hWindow, void* msghandler, int a3)
{
	bool a = reinterpret_cast<bool (__thiscall*)(CGFXVideo3d*, HWND, void*, int)>(0x00BAE370)(
			this, hWindow, msghandler, a3);
	if (!a || m_pd3dDevice == NULL)
		return a;

	for (std::vector<create_handler_t>::iterator it = hooks_create.begin();
		it != hooks_create.end();
		++it)
	{
		(*it)(hWindow, msghandler, a3);
	}

	return a;
}

bool CGFXVideo3D_Hook::EndSceneHook()
{
	IDirect3DDevice9* const device = m_pd3dDevice;
	if (device == NULL)
		return false;

	for (std::vector<endscene_handler_t>::iterator it = hooks_endscene.begin();
		it != hooks_endscene.end();
		++it)
	{
		(*it)();
	}

	ApplyTextureQualityBoost(device);
	ApplyColorQualityBoost(device);

	// Full qualified name to avoid redirection through the vftable
	//return CGFXVideo3D_Hook::EndScene();

	device->EndScene();
	return true;
}

bool CGFXVideo3D_Hook::SetSizeHook(int width, int height)
{
	for (std::vector<setsize_handler_t>::iterator it = hooks_setsize_pre.begin();
		it != hooks_setsize_pre.end();
		++it)
	{
		(*it)(width, height);
	}

	CGFXVideo3d::SetSize(width, height);

	for (std::vector<setsize_handler_t>::iterator it = hooks_setsize_post.begin();
		it != hooks_setsize_post.end();
		++it)
	{
		(*it)(width, height);
	}

	return true;
}
