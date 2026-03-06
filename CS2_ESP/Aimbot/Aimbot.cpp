#include "Aimbot.h"
#include "../ESP/ESP.h"
#include "../GUI/GUI.h"
#include "../cs2_dumper/offsets.hpp"
#include "../cs2_dumper/client_dll.hpp"
#include "../vina/Vector.h"
#include <cmath>
#include <Windows.h>

using namespace cs2_dumper::schemas::client_dll;

static Vector3 CalcAngle(const Vector3& eye, const Vector3& target) {
    Vector3 delta = target - eye;
    float dist2d = delta.Length2D();
    if (dist2d < 1e-6f) dist2d = 1e-6f;

    float pitch = -Rad2Deg(std::atan2(delta.z, dist2d));
    float yaw   =  Rad2Deg(std::atan2(delta.y, delta.x));
    return Vector3(pitch, yaw, 0.0f);
}

static void NormalizeAngles(Vector3& angles) {
    while (angles.y > 180.0f)  angles.y -= 360.0f;
    while (angles.y < -180.0f) angles.y += 360.0f;
    if (angles.x > 89.0f)  angles.x = 89.0f;
    if (angles.x < -89.0f) angles.x = -89.0f;
}


static Vector3 AngleDeltaShortest(const Vector3& current, const Vector3& target) {
    Vector3 d;
    d.x = target.x - current.x;
    d.y = target.y - current.y;
    d.z = target.z - current.z;

    while (d.y > 180.0f)  d.y -= 360.0f;
    while (d.y < -180.0f) d.y += 360.0f;

    while (d.x > 180.0f)  d.x -= 360.0f;
    while (d.x < -180.0f) d.x += 360.0f;
    return d;
}


static float AngleDiff(const Vector3& a, const Vector3& b) {
    float dp = std::abs(a.x - b.x);
    float dy = std::abs(a.y - b.y);
    if (dy > 180.0f) dy = 360.0f - dy;
    return std::sqrt(dp * dp + dy * dy);
}


static std::optional<Vector3> GetBestTargetHead(uintptr_t client, uintptr_t localPawn,
    const Vector3& localEye, const Vector3& viewAngles) {

    int localTeam = *reinterpret_cast<int*>(localPawn + C_BaseEntity::m_iTeamNum);
    float bestFov = Aimbot::aimFov;
    std::optional<Vector3> bestHead;

    for (int i = 0; i < 64; i++) {
        uintptr_t controller = GetBaseEntity(i, client);
        if (!controller) continue;

        uint32_t hPawn = *reinterpret_cast<uint32_t*>(controller + CBasePlayerController::m_hPawn);
        if (!hPawn) continue;

        uintptr_t pawn = GetBaseEntityFromHandle(hPawn, client);
        if (!pawn || pawn == (uintptr_t)-1 || pawn == localPawn) continue;

        int team = *reinterpret_cast<int*>(pawn + C_BaseEntity::m_iTeamNum);
        int health = *reinterpret_cast<int*>(pawn + C_BaseEntity::m_iHealth);
        if (team == localTeam || health <= 0) continue;

        auto eyeOpt = GetEyePos(pawn);
        if (!eyeOpt.has_value()) continue;
        Vector3 headPos = eyeOpt.value();

        Vector3 aimAngles = CalcAngle(localEye, headPos);
        float fov = AngleDiff(viewAngles, aimAngles);
        if (fov < bestFov) {
            bestFov = fov;
            bestHead = headPos;
        }
    }
    return bestHead;
}

void RunAimbot() {
    if (!MyNamespace::visuals::Aimbot) return;


    int vk = VK_CONTROL;
    if (Aimbot::aimKey == 2) vk = VK_CONTROL;
    else if (Aimbot::aimKey == 4) vk = VK_CONTROL;
    if (!(GetAsyncKeyState(vk) & 0x8000)) return;

    auto client = reinterpret_cast<uintptr_t>(GetModuleHandleW(L"client.dll"));
    if (!client) return;

    uintptr_t localPawn = *reinterpret_cast<uintptr_t*>(client + cs2_dumper::offsets::client_dll::dwLocalPlayerPawn);
    if (!localPawn) return;

    int health = *reinterpret_cast<int*>(localPawn + C_BaseEntity::m_iHealth);
    if (health <= 0) return;

    auto localEyeOpt = GetEyePos(localPawn);
    if (!localEyeOpt.has_value()) return;
    Vector3 localEye = localEyeOpt.value();


    float* pViewAngles = reinterpret_cast<float*>(client + cs2_dumper::offsets::client_dll::dwViewAngles);
    if (!pViewAngles) return;
    Vector3 viewAngles(pViewAngles[0], pViewAngles[1], pViewAngles[2]);

    auto bestHead = GetBestTargetHead(client, localPawn, localEye, viewAngles);
    if (!bestHead.has_value()) return;

    Vector3 targetAngles = CalcAngle(localEye, bestHead.value());
    NormalizeAngles(targetAngles);
    
    float t = Aimbot::smooth;
    if (t <= 0.0f) t = 0.01f;
    if (t > 1.0f) t = 1.0f;
    Vector3 delta = AngleDeltaShortest(viewAngles, targetAngles);
    viewAngles.x += delta.x * t;
    viewAngles.y += delta.y * t;
    viewAngles.z += delta.z * t;
    NormalizeAngles(viewAngles);

    pViewAngles[0] = viewAngles.x;
    pViewAngles[1] = viewAngles.y;
    pViewAngles[2] = viewAngles.z;
}
