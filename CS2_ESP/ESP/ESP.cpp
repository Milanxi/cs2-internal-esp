#include "windows.h"
#include <cstdint>
#include "../cs2_dumper/offsets.hpp"
#include "../cs2_dumper/client_dll.hpp"
#include "../vina/Vector.h"
#include <cmath>
#include <optional>
#include "../imgui_d11/imgui.h"
#include "../GUI/GUI.h"

uintptr_t GetBaseEntity(int index, uintptr_t client) {
	auto entListBase = *reinterpret_cast<std::uintptr_t*>(
		client + cs2_dumper::offsets::client_dll::dwEntityList
		);

	if (entListBase == 0) {
		return 0;
	}

	auto entitylistbase = *reinterpret_cast<std::uintptr_t*>(
		entListBase + 0x8 * (index >> 9) + 16
		);

	if (entitylistbase == 0) {
		return 0;
	}

	return *reinterpret_cast<std::uintptr_t*>(
		entitylistbase + (0x70 * (index & 0x1FF))
		);
}

uintptr_t GetBaseEntityFromHandle(uint32_t uHandle, uintptr_t client) {
	auto entListBase = *reinterpret_cast<std::uintptr_t*>(
		client + cs2_dumper::offsets::client_dll::dwEntityList
		);

	if (entListBase == 0) {
		return 0;
	}

	const int nIndex = uHandle & 0x7FFF;

	auto entitylistbase = *reinterpret_cast<std::uintptr_t*>(
		entListBase + 8 * (nIndex >> 9) + 16
		);

	if (entitylistbase == 0) {
		return 0;
	}

	return *reinterpret_cast<std::uintptr_t*>(
		entitylistbase + 0x70 * (nIndex & 0x1FF)
		);
}

std::optional<Vector3> GetEyePos(uintptr_t addr) noexcept {
	auto* Origin = reinterpret_cast<Vector3*>(
		addr + cs2_dumper::schemas::client_dll::C_BasePlayerPawn::m_vOldOrigin
		);

	auto* ViewOffset = reinterpret_cast<Vector3*>(
		addr + cs2_dumper::schemas::client_dll::C_BaseModelEntity::m_vecViewOffset
		);

	Vector3 LocalEye = *Origin + *ViewOffset;

	if (!std::isfinite(LocalEye.x) ||
		!std::isfinite(LocalEye.y) ||
		!std::isfinite(LocalEye.z))
		return std::nullopt;

	if (LocalEye.Length() < 0.1f)
		return std::nullopt;

	return LocalEye;
}


bool WorldToScreen(Vector3 pWorldPos, Vector3& pScreenPos, float* pMatrixPtr, const FLOAT pWinWidth, const FLOAT pWinHeight)
{

	float matrix2[4][4];
	memcpy(matrix2, pMatrixPtr, 16 * sizeof(float));


	const float mX = pWinWidth / 2.0f;
	const float mY = pWinHeight / 2.0f;


	const float w = matrix2[3][0] * pWorldPos.x +
		matrix2[3][1] * pWorldPos.y +
		matrix2[3][2] * pWorldPos.z +
		matrix2[3][3];

	if (w < 0.65f) return false;


	const float x = matrix2[0][0] * pWorldPos.x +
		matrix2[0][1] * pWorldPos.y +
		matrix2[0][2] * pWorldPos.z +
		matrix2[0][3];

	const float y = matrix2[1][0] * pWorldPos.x +
		matrix2[1][1] * pWorldPos.y +
		matrix2[1][2] * pWorldPos.z +
		matrix2[1][3];


	pScreenPos.x = mX + mX * x / w;
	pScreenPos.y = mY - mY * y / w;  
	pScreenPos.z = 0.0f;

	return true;
}

void draw_esp() {
	static auto client = reinterpret_cast<uintptr_t>(GetModuleHandleW(L"client.dll"));
	if (!client) return;

	uintptr_t localplayerPawn = *reinterpret_cast<uintptr_t*>(client + cs2_dumper::offsets::client_dll::dwLocalPlayerPawn);
	if (localplayerPawn == 0) return; 

	int health = *reinterpret_cast<int*>(localplayerPawn + cs2_dumper::schemas::client_dll::C_BaseEntity::m_iHealth);
	if (health <= 0) return;       

	int localplayer_team = *reinterpret_cast<int*>(localplayerPawn + cs2_dumper::schemas::client_dll::C_BaseEntity::m_iTeamNum);

	float* viewMatrix = reinterpret_cast<float*>(client + cs2_dumper::offsets::client_dll::dwViewMatrix);
	if (!viewMatrix) return;

	for (int i = 0; i < 64; i++)
	{
			uintptr_t player_co = GetBaseEntity(i, client);
			if (!player_co) continue;

			uint32_t hplayer_pawn = *reinterpret_cast<uint32_t*>(player_co + cs2_dumper::schemas::client_dll::CBasePlayerController::m_hPawn);
			if (hplayer_pawn == 0) continue; 

			uintptr_t player_pawn = GetBaseEntityFromHandle(hplayer_pawn, client);
			if (player_pawn == 0 || player_pawn == (uintptr_t)-1) continue;

			if (player_pawn == localplayerPawn) continue;

			int player_team = *reinterpret_cast<int*>(player_pawn + cs2_dumper::schemas::client_dll::C_BaseEntity::m_iTeamNum);
			int player_health = *reinterpret_cast<int*>(player_pawn + cs2_dumper::schemas::client_dll::C_BaseEntity::m_iHealth);

			if (player_team == localplayer_team || player_health <= 0) continue;

			using namespace cs2_dumper::schemas::client_dll;

			Vector3 player_origin = *reinterpret_cast<Vector3*>(player_pawn + C_BasePlayerPawn::m_vOldOrigin);

			auto player_eyepos_opt = GetEyePos(player_pawn);
			if (!player_eyepos_opt.has_value())
				continue;
			Vector3 player_eyepos = player_eyepos_opt.value();

			const float screen_w = ImGui::GetIO().DisplaySize.x;
			const float screen_h = ImGui::GetIO().DisplaySize.y;

			Vector3 head_pos_2d; 
			Vector3 abs_pos_2d;   


			if (!viewMatrix) return;

			if (!WorldToScreen(player_eyepos, head_pos_2d, viewMatrix, screen_w, screen_h))
			{
				continue;
			}

			if (!WorldToScreen(player_origin, abs_pos_2d, viewMatrix, screen_w, screen_h))
			{
				continue;
			}

			// ---------- 方框上下边可调参数（改这里即可）----------
			const float HEAD_MARGIN_RATIO = 0.16f;  // 头顶：眼睛到框顶 = 身体高度×此值。调大=框更高，调小=框顶下移
			const float FOOT_MARGIN_RATIO = 0.05f; // 脚底：脚到框底再往下延伸 = 身体高度×此值。调大=框更往下包住脚
			// ----------------------------------------------------

			const float body_height = abs_pos_2d.y - head_pos_2d.y;
			const float head_margin = body_height * HEAD_MARGIN_RATIO;
			const float foot_margin = body_height * FOOT_MARGIN_RATIO;
			const float box_top = head_pos_2d.y - head_margin;
			const float box_bottom = abs_pos_2d.y + foot_margin;

			const float height = (box_bottom - box_top);
			const float width = height / 2.f;
			const float left = head_pos_2d.x - width / 2.5f;
			const float right = head_pos_2d.x + width / 2.f;

			if (height <= 0.0f || (right - left) <= 0.0f) continue;

		if (MyNamespace::visuals::box)
		{
			ImDrawList* draw_list = ImGui::GetForegroundDrawList();
			if (draw_list)
			{
				draw_list->AddRect(
					ImVec2(left, box_top),
					ImVec2(right, box_bottom),
					IM_COL32(255, 0, 0, 255), 0.0f, 0, 1.5f);
			}
		}
	}
}