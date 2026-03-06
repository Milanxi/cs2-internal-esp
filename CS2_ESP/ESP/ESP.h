#pragma once

#include "../vina/Vector.h"
#include <cstdint>
#include <optional>

uintptr_t GetBaseEntity(int index, uintptr_t client);
uintptr_t GetBaseEntityFromHandle(uint32_t uHandle, uintptr_t client);
std::optional<Vector3> GetEyePos(uintptr_t addr);

void draw_esp();