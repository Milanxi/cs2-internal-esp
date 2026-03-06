#include "GUI.h"
#include"/C++/CS2_ESP/CS2_ESP/imgui_d11/imgui.h"

void draw_Menu() {
	ImGui::Begin("Esp");
	{
		ImGui::Checkbox("box", &MyNamespace::visuals::box);
		ImGui::Checkbox("Aimbot", &MyNamespace::visuals::Aimbot);
	}
	ImGui::End();
}
