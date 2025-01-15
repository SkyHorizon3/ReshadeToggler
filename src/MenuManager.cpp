#include "MenuManager.h"
#include "Utils.h"

bool MenuManager::CreateCombo(const char* label, std::string& currentItem, std::vector<std::string>& items, ImGuiComboFlags_ flags)
{
	ImGuiStyle& style = ImGui::GetStyle();
	float w = 500.0f;
	float spacing = style.ItemInnerSpacing.x;
	float button_sz = ImGui::GetFrameHeight();
	ImGui::PushItemWidth(w - spacing - button_sz * 2.0f);

	bool itemChanged = false;
	static char searchBuffer[256] = "";

	if (ImGui::BeginCombo(label, currentItem.c_str(), flags))
	{
		ImGui::InputTextWithHint("##Search", "Search...", searchBuffer, sizeof(searchBuffer));

		// Filter items based on search buffer
		for (const auto& item : items)
		{
			if (strcasestr(item.c_str(), searchBuffer))
			{
				bool isSelected = (currentItem == item);
				if (ImGui::Selectable(item.c_str(), isSelected))
				{
					currentItem = item;
					itemChanged = true;
					searchBuffer[0] = '\0'; // Clear search buffer on selection
				}
				if (isSelected) { ImGui::SetItemDefaultFocus(); }
			}
		}
		ImGui::EndCombo();
	}

	ImGui::PopItemWidth();

	return itemChanged;
}

bool MenuManager::CreateTreeNode(const char* label, std::vector<std::string>& selectedItems, std::vector<std::string>& items, char* searchBuffer, size_t bufferSize, bool entireReShadeToggleOn)
{
	bool itemsChanged = false;

	if (ImGui::TreeNode(label))
	{
		const bool isEffectsLabel = strcmp(label, "Effects") == 0;

		if (isEffectsLabel && entireReShadeToggleOn)
		{
			ImGui::BeginDisabled();
		}

		ImGui::InputTextWithHint("##Search", "Search...", searchBuffer, bufferSize);

		if (ImGui::Button("Select All"))
		{
			selectedItems = items;
			itemsChanged = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Deselect All"))
		{
			selectedItems.clear();
			itemsChanged = true;
		}

		ImGui::Separator();

		for (const auto& item : items)
		{
			if (strcasestr(item.c_str(), searchBuffer))
			{
				bool isSelected = std::find(selectedItems.begin(), selectedItems.end(), item) != selectedItems.end();

				if (ImGui::Checkbox(item.c_str(), &isSelected))
				{
					if (isSelected)
					{
						selectedItems.emplace_back(item);
					}
					else
					{
						selectedItems.erase(std::remove(selectedItems.begin(), selectedItems.end(), item), selectedItems.end());
					}
					itemsChanged = true;
				}
			}
		}

		if (isEffectsLabel && entireReShadeToggleOn)
		{
			ImGui::EndDisabled();
		}

		ImGui::TreePop();
	}

	return itemsChanged;
}


// https://github.com/doodlum/skyrim-community-shaders/blob/09ea7f0dcb10da3fae6f56d0c1a119a501c61ca7/src/Utils/UI.cpp#L42
ImVec2 MenuManager::GetNativeViewportSizeScaled(float scale)
{
	const auto Size = ImGui::GetWindowSize();
	return { Size.x * scale, Size.y * scale };
}

#pragma endregion
