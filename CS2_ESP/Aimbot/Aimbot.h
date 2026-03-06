#pragma once

#include <cstdint>

// 自瞄配置（自瞄范围、平滑模式）
namespace Aimbot {
    inline float    aimFov  = 5.0f;    // 自瞄范围（度），只在该 FOV 内自瞄
    inline float    smooth  = 0.25f;   // 平滑系数 0.0~1.0，越小越平滑
    inline int      aimKey  = 1;       // 1=左键 2=右键 4=中键，仅按住时自瞄
}

void RunAimbot();
