#include "Menu.h"
#include "Manager.h"
#include "Utils.h"

void Menu::SettingsMenu()
{
	if (ImGui::Button("Configure ReShade Effect Toggler"))
		m_openSettingsMenu = true;

	if (m_openSettingsMenu)
	{
		// Create the main settings window with docking enabled
		ImGui::SetNextWindowSize(GetNativeViewportSizeScaled(0.6f), ImGuiCond_FirstUseEver);
		ImGui::Begin("Settings", &m_openSettingsMenu);
		static int currentTab = 0;

		if (ImGui::BeginTabBar("SettingsTabBar"))
		{
			if (ImGui::BeginTabItem("Main Page"))
			{
				currentTab = 0;
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Menu Settings"))
			{
				currentTab = 1;
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Time Settings"))
			{
				currentTab = 2;
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Interior Settings"))
			{
				currentTab = 3;
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Weather Settings"))
			{
				currentTab = 4;
				ImGui::EndTabItem();
			}
		}

		ImGuiID dockspaceId = ImGui::GetID("SettingsDockspace");
		ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

		switch (currentTab)
		{
		case 0: SpawnMainPage(dockspaceId); break;
		case 1: SpawnMenuSettings(dockspaceId); break;
		case 2: SpawnTimeSettings(dockspaceId); break;
		case 3: SpawnInteriorSettings(dockspaceId); break;
		case 4: SpawnWeatherSettings(dockspaceId); break;
		}
	}
}

void Menu::SpawnMainPage(ImGuiID dockspace_id)
{
	ImGui::SetNextWindowDockID(dockspace_id, ImGuiCond_Always);
	ImGui::Begin("Main", nullptr, ImGuiWindowFlags_NoCollapse);

	CreateCombo("Select Preset", m_selectedPreset, m_presets, ImGuiComboFlags_None);
	ImGui::SameLine();
	if (ImGui::Button("Reload Preset List"))
	{
		m_presets.clear();
		m_presets = Manager::GetSingleton()->enumeratePresets();
	}

	if (ImGui::Button("Load Preset"))
	{
		const std::string selectedPresetPath = Manager::GetSingleton()->getPresetPath(m_selectedPreset);
		if (std::filesystem::exists(selectedPresetPath))
		{
			const auto start = std::chrono::high_resolution_clock::now();
			const bool success = Manager::GetSingleton()->parseJSONPreset(m_selectedPreset);
			const auto end = std::chrono::high_resolution_clock::now();
			std::chrono::duration<double, std::milli> duration = end - start;

			if (!success)
			{
				m_lastMessage = "Failed to load preset: '" + m_selectedPreset + "'.";
				m_lastMessageColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
			}
			else
			{
				Manager::GetSingleton()->setLastPreset(m_selectedPreset);
				Manager::GetSingleton()->serializeINI();

				m_lastMessage = "Successfully loaded preset: '" + m_selectedPreset + "'! Took: " + std::to_string(duration.count()) + "ms";
				m_lastMessageColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
			}
		}
		else
		{
			m_lastMessage = "Tried to load preset '" + selectedPresetPath + "'. Preset doesn't exist";
			m_lastMessageColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
		}
	}

	ImGui::SameLine();
	if (ImGui::Button("Save"))
	{
		m_saveConfigPopupOpen = true;
		m_inputBuffer01[0] = '\0';
	}
	SaveFile();

	if (!m_lastMessage.empty())
	{
		ImGui::TextColored(m_lastMessageColor, "%s", m_lastMessage.c_str());
	}

	ImGui::End();
}

void Menu::SpawnMenuSettings(ImGuiID dockspace_id)
{
	ImGui::SetNextWindowDockID(dockspace_id, ImGuiCond_Always);
	ImGui::Begin("Menu Settings", &m_showMenuSettings, ImGuiWindowFlags_NoCollapse);
	ImGui::Text("Configure menu toggling settings here.");

	ImGui::SeparatorText("Effects");

	// Get the list of toggle information
	std::map<std::string, std::vector<MenuToggleInformation>> infoList = Manager::GetSingleton()->getMenuToggleInfo();
	std::map<std::string, std::vector<MenuToggleInformation>> updatedInfoList = infoList;
	static char inputBuffer[256] = "";
	ImGui::InputTextWithHint("##Search", "Search Menus...", inputBuffer, sizeof(inputBuffer));

	int headerId = -1;
	int globalIndex = 0;

	for (const auto& [menuName, effects] : infoList)
	{
		if (strlen(inputBuffer) > 0 && Utils::tolower(menuName).find(Utils::tolower(inputBuffer)) == std::string::npos)
			continue;

		headerId++;
		std::string headerUniqueId = menuName + std::to_string(headerId);

		if (ImGui::CollapsingHeader((menuName + "##" + headerUniqueId + "##Header").c_str(), ImGuiTreeNodeFlags_AllowOverlap | ImGuiTreeNodeFlags_AllowItemOverlap))
		{
			ImGui::BeginTable(("EffectsTable##" + headerUniqueId).c_str(), 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg);
			ImGui::TableSetupColumn(("Effect##" + headerUniqueId).c_str());
			ImGui::TableSetupColumn(("State##" + headerUniqueId).c_str());
			ImGui::TableSetupColumn(("Actions##" + headerUniqueId).c_str());
			ImGui::TableSetupColumn(("MenuName##" + headerUniqueId).c_str());
			ImGui::TableHeadersRow();
			for (int i = 0; i < effects.size(); i++, globalIndex++)
			{
				MenuToggleInformation info = effects[i];
				bool valueChanged = false;

				std::string effectComboId = "Effect##" + headerUniqueId + std::to_string(i);
				std::string effectStateId = "State##" + headerUniqueId + std::to_string(i);
				std::string removeId = "RemoveEffect##" + headerUniqueId + std::to_string(i);
				std::string editId = "EditEffect##" + headerUniqueId + std::to_string(i);

				std::string currentEffectName = info.effectName;
				bool currentEffectState = info.state;

				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				if (CreateCombo(effectComboId.c_str(), currentEffectName, m_effects, ImGuiComboFlags_None))
				{
					valueChanged = true;
				}
				ImGui::TableNextColumn();
				if (ImGui::Checkbox(effectStateId.c_str(), &currentEffectState))
				{
					valueChanged = true;
				}
				ImGui::TableNextColumn();
				if (ImGui::Button(removeId.c_str()))
				{
					// Remove effect
					updatedInfoList[menuName].erase(
						std::remove_if(updatedInfoList[menuName].begin(), updatedInfoList[menuName].end(),
						[&info](const MenuToggleInformation& timeInfo) {
							return timeInfo.effectName == info.effectName;
						}),
						updatedInfoList[menuName].end()
					);

					if (updatedInfoList[menuName].empty())
					{
						updatedInfoList.erase(menuName);
					}

					globalIndex--;
					continue;
				}
				if (ImGui::Button(editId.c_str()))
				{
					m_currentEditingEffect = currentEffectName;
					m_editingEffectIndex = globalIndex; // Store the index of the effect being edited
					ImGui::OpenPopup("Edit Effect Values");
				}

				ImGui::TableNextColumn();
				ImGui::Text("%s", menuName.c_str());

				if (valueChanged)
				{
					info.menuName = menuName;
					info.effectName = currentEffectName;
					info.state = currentEffectState;
					updatedInfoList[menuName].at(i) = info;
				}

				if (m_editingEffectIndex == globalIndex)
				{
					auto& targetUniforms = updatedInfoList[menuName].at(i).uniforms;
					HandleEffectEditing(targetUniforms, m_currentEditingEffect, m_editingEffectIndex);
				}

			}

			ImGui::EndTable();
		}
	}


	Manager::GetSingleton()->setMenuToggleInfo(updatedInfoList);

	ImGui::SeparatorText("Add New");
	// Add new effect
	if (ImGui::Button("Add New Effect"))
	{
		ImGui::OpenPopup("Create Menu Entries");
	}
	AddNewMenu(updatedInfoList);

	ImGui::End();
}

void Menu::AddNewMenu(std::map<std::string, std::vector<MenuToggleInformation>>& updatedInfoList)
{

	if (ImGui::BeginPopupModal("Create Menu Entries", NULL, ImGuiWindowFlags_None))
	{
		if (ImGui::IsWindowAppearing())
		{
			m_currentToggleReason.clear();
			m_currentEffects.clear();
			m_toggleState = false;
			m_entireReShadeToggleOn = false;
			m_inputBuffer01[0] = '\0';
			m_inputBuffer02[0] = '\0';
		}

		ImVec2 availableSpace = ImGui::GetContentRegionAvail();
		float childHeight = availableSpace.y * 0.35f;
		ImGui::SeparatorText("Select Menus");
		ImGui::BeginChild("MenusRegion", ImVec2(availableSpace.x, childHeight), true, ImGuiWindowFlags_HorizontalScrollbar);
		CreateTreeNode("Menus", m_currentToggleReason, m_menuNames, m_inputBuffer01, sizeof(m_inputBuffer01), false);
		ImGui::EndChild();

		EffectOptions();

		ImGui::SeparatorText("Select Effects");
		ImGui::BeginChild("EffectsRegion", ImVec2(availableSpace.x, childHeight), true, ImGuiWindowFlags_HorizontalScrollbar);
		CreateTreeNode("Effects", m_currentEffects, m_effects, m_inputBuffer02, sizeof(m_inputBuffer02), m_entireReShadeToggleOn);
		ImGui::EndChild();

		ImGui::Separator();
		if (ImGui::Button("Finish"))
		{
			for (const auto& menu : m_currentToggleReason)
			{
				for (const auto& effect : m_currentEffects)
				{
					updatedInfoList[menu].emplace_back(MenuToggleInformation{ effect, menu, m_toggleState });
				}
			}

			Manager::GetSingleton()->setMenuToggleInfo(updatedInfoList);

			ImGui::CloseCurrentPopup();
		}

		ImGui::SameLine();
		if (ImGui::Button("Cancel"))
		{
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}

void Menu::SpawnTimeSettings(ImGuiID dockspace_id)
{
	ImGui::SetNextWindowDockID(dockspace_id, ImGuiCond_Always);
	ImGui::Begin("Time Settings", &m_showTimeSettings, ImGuiWindowFlags_NoCollapse);
	ImGui::Text("Configure time toggling settings here.");

	ImGui::SeparatorText("Effects");

	// Get the list of toggle information
	std::map<std::string, std::vector<TimeToggleInformation>> infoList = Manager::GetSingleton()->getTimeToggleInfo();
	std::map<std::string, std::vector<TimeToggleInformation>> updatedInfoList = infoList;
	static char inputBuffer[256] = "";
	ImGui::InputTextWithHint("##Search", "Search Cell...", inputBuffer, sizeof(inputBuffer));

	int headerId = -1;
	int globalIndex = 0;

	for (const auto& [cellName, effects] : infoList)
	{
		if (strlen(inputBuffer) > 0 && Utils::tolower(cellName).find(Utils::tolower(inputBuffer)) == std::string::npos)
			continue;

		headerId++;
		std::string headerUniqueId = cellName + std::to_string(headerId);

		if (ImGui::CollapsingHeader((cellName + "##" + headerUniqueId + "##Header").c_str(), ImGuiTreeNodeFlags_AllowOverlap | ImGuiTreeNodeFlags_AllowItemOverlap))
		{
			ImGui::BeginTable(("EffectsTable##" + headerUniqueId).c_str(), 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg);
			ImGui::TableSetupColumn(("Effect##" + headerUniqueId).c_str());
			ImGui::TableSetupColumn(("State##" + headerUniqueId).c_str());
			ImGui::TableSetupColumn(("StartTime##" + headerUniqueId).c_str());
			ImGui::TableSetupColumn(("StopTime##" + headerUniqueId).c_str());
			ImGui::TableSetupColumn(("Actions##" + headerUniqueId).c_str());
			ImGui::TableSetupColumn(("Cell##" + headerUniqueId).c_str());
			ImGui::TableHeadersRow();

			for (int i = 0; i < effects.size(); i++, globalIndex++)
			{
				TimeToggleInformation info = effects[i];
				bool valueChanged = false;

				std::string effectComboId = "Effect##" + headerUniqueId + std::to_string(i);
				std::string effectStateId = "State##" + headerUniqueId + std::to_string(i);
				std::string startTimeId = "StartTime##" + headerUniqueId + std::to_string(i);
				std::string stopTimeId = "StopTime##" + headerUniqueId + std::to_string(i);
				std::string removeId = "RemoveEffect##" + headerUniqueId + std::to_string(i);
				std::string editId = "EditEffect##" + headerUniqueId + std::to_string(i);

				std::string currentEffectName = info.effectName;
				float currentStartTime = info.startTime;
				float currentStopTime = info.stopTime;
				bool currentEffectState = info.state;

				int startHours = static_cast<int>(currentStartTime);
				int startMinutes = static_cast<int>((currentStartTime - startHours) * 100);
				int stopHours = static_cast<int>(currentStopTime);
				int stopMinutes = static_cast<int>((currentStopTime - stopHours) * 100);

				static char startHourStr[3] = "00";
				static char startMinuteStr[3] = "00";
				static char stopHourStr[3] = "00";
				static char stopMinuteStr[3] = "00";

				snprintf(startHourStr, sizeof(startHourStr), "%02d", startHours);
				snprintf(startMinuteStr, sizeof(startMinuteStr), "%02d", startMinutes);
				snprintf(stopHourStr, sizeof(stopHourStr), "%02d", stopHours);
				snprintf(stopMinuteStr, sizeof(stopMinuteStr), "%02d", stopMinutes);

				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				if (CreateCombo(effectComboId.c_str(), currentEffectName, m_effects, ImGuiComboFlags_None)) { valueChanged = true; }
				ImGui::TableNextColumn();
				if (ImGui::Checkbox(effectStateId.c_str(), &currentEffectState)) { valueChanged = true; }

				// Start time
				ImGui::TableNextColumn();
				ImGui::PushItemWidth(35);
				if (ImGui::InputText(("##StartHours" + startTimeId).c_str(), startHourStr, sizeof(startHourStr), ImGuiInputTextFlags_CharsDecimal)) { valueChanged = true; }
				ClampInputValue(startHourStr, 23);
				if (strcmp(startHourStr, "00") == 0 && strlen(startHourStr) == 0) strcpy(startHourStr, "0");
				ImGui::SameLine();
				ImGui::Text(":");
				ImGui::SameLine();
				if (ImGui::InputText(("##StartMinutes" + startTimeId).c_str(), startMinuteStr, sizeof(startMinuteStr), ImGuiInputTextFlags_CharsDecimal)) { valueChanged = true; }
				ClampInputValue(startMinuteStr, 59);
				if (strcmp(startMinuteStr, "00") == 0 && strlen(startMinuteStr) == 0) strcpy(startMinuteStr, "0");
				ImGui::PopItemWidth();

				// Stop Time
				ImGui::TableNextColumn();
				ImGui::PushItemWidth(35);
				if (ImGui::InputText(("##StopHours" + stopTimeId).c_str(), stopHourStr, sizeof(stopHourStr), ImGuiInputTextFlags_CharsDecimal)) { valueChanged = true; }
				ClampInputValue(stopHourStr, 23);
				if (strcmp(stopHourStr, "00") == 0 && strlen(stopHourStr) == 0) strcpy(stopHourStr, "0");
				ImGui::SameLine();
				ImGui::Text(":");
				ImGui::SameLine();
				if (ImGui::InputText(("##StopMinutes" + stopTimeId).c_str(), stopMinuteStr, sizeof(stopMinuteStr), ImGuiInputTextFlags_CharsDecimal)) { valueChanged = true; }
				ClampInputValue(stopMinuteStr, 59);
				if (strcmp(stopMinuteStr, "00") == 0 && strlen(stopMinuteStr) == 0) strcpy(stopMinuteStr, "0");
				ImGui::PopItemWidth();

				// Convert baby
				startHours = strlen(startHourStr) > 0 ? std::stoi(startHourStr) : 0;
				startMinutes = strlen(startMinuteStr) > 0 ? std::stoi(startMinuteStr) : 0;
				stopHours = strlen(stopHourStr) > 0 ? std::stoi(stopHourStr) : 0;
				stopMinutes = strlen(stopMinuteStr) > 0 ? std::stoi(stopMinuteStr) : 0;

				currentStartTime = startHours + (startMinutes / 100.0f);
				currentStopTime = stopHours + (stopMinutes / 100.0f);

				if (currentStartTime > currentStopTime)
				{
					ImGui::TextColored(ImVec4(1, 0, 0, 1), "Invalid Time Range");
				}

				ImGui::TableNextColumn();
				if (ImGui::Button(removeId.c_str()))
				{
					updatedInfoList[cellName].erase(
						std::remove_if(updatedInfoList[cellName].begin(), updatedInfoList[cellName].end(),
						[&info](const TimeToggleInformation& timeInfo) {
							return timeInfo.effectName == info.effectName;
						}
					),
						updatedInfoList[cellName].end()
					);

					Manager::GetSingleton()->removeTimeById(info);

					if (updatedInfoList[cellName].empty())
					{
						updatedInfoList.erase(cellName);
					}

					globalIndex--;
					continue;
				}
				if (ImGui::Button(editId.c_str()))
				{
					m_currentEditingEffect = currentEffectName;
					m_editingEffectIndex = globalIndex;
					ImGui::OpenPopup("Edit Effect Values");
				}
				ImGui::TableNextColumn();
				ImGui::Text("%s", cellName.c_str());

				if (valueChanged)
				{
					info.effectName = currentEffectName;
					info.startTime = currentStartTime;
					info.stopTime = currentStopTime;
					info.state = currentEffectState;

					updatedInfoList[cellName].at(i) = info;
				}

				if (m_editingEffectIndex == globalIndex)
				{
					auto& targetUniforms = updatedInfoList[cellName].at(i).uniforms;
					HandleEffectEditing(targetUniforms, m_currentEditingEffect, m_editingEffectIndex);
				}
			}
			ImGui::EndTable();
		}
	}

	Manager::GetSingleton()->setTimeToggleInfo(updatedInfoList);

	ImGui::SeparatorText("Add New");

	// Add new effect
	if (ImGui::Button("Add New Effect"))
	{
		ImGui::OpenPopup("Create Time Entries");
	}
	AddNewTime(updatedInfoList);
	ImGui::End();
}

void Menu::SpawnInteriorSettings(ImGuiID dockspace_id)
{
	ImGui::SetNextWindowDockID(dockspace_id, ImGuiCond_Always);
	ImGui::Begin("Interior Settings", &m_showInteriorSettings, ImGuiWindowFlags_NoCollapse);
	ImGui::Text("Configure interior toggling settings here.");

	// Retrieve the current weather toggle info
	std::map<std::string, std::vector<InteriorToggleInformation>> infoList = Manager::GetSingleton()->getInteriorToggleInfo();
	std::map<std::string, std::vector<InteriorToggleInformation>> updatedInfoList = infoList; // Start with existing info
	static char inputBuffer[256] = "";
	ImGui::InputTextWithHint("##Search", "Search Interior Cell...", inputBuffer, sizeof(inputBuffer));

	int headerId = -1;
	int globalIndex = 0;

	for (const auto& [cellName, effects] : infoList)
	{
		if (strlen(inputBuffer) > 0 && Utils::tolower(cellName).find(Utils::tolower(inputBuffer)) == std::string::npos)
			continue;

		headerId++;
		std::string headerUniqueId = cellName + std::to_string(headerId);

		if (ImGui::CollapsingHeader((cellName + "##" + headerUniqueId + "##Header").c_str(), ImGuiTreeNodeFlags_AllowOverlap | ImGuiTreeNodeFlags_AllowItemOverlap))
		{
			ImGui::BeginTable(("EffectsTable##" + headerUniqueId).c_str(), 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg);
			ImGui::TableSetupColumn(("Effect##" + headerUniqueId).c_str());
			ImGui::TableSetupColumn(("State##" + headerUniqueId).c_str());
			ImGui::TableSetupColumn(("Actions##" + headerUniqueId).c_str());
			ImGui::TableSetupColumn(("Cell##" + headerUniqueId).c_str());
			ImGui::TableHeadersRow();

			for (int i = 0; i < effects.size(); i++, globalIndex++)
			{
				InteriorToggleInformation info = effects[i];
				bool valueChanged = false;

				std::string effectComboId = "Effect##" + headerUniqueId + std::to_string(i);
				std::string effectStateId = "State##" + headerUniqueId + std::to_string(i);
				std::string removeId = "RemoveEffect##" + headerUniqueId + std::to_string(i);
				std::string editId = "EditEffect##" + headerUniqueId + std::to_string(i);

				std::string currentEffectName = info.effectName;
				bool currentEffectState = info.state;

				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				if (CreateCombo(effectComboId.c_str(), currentEffectName, m_effects, ImGuiComboFlags_None)) { valueChanged = true; }
				ImGui::TableNextColumn();
				if (ImGui::Checkbox(effectStateId.c_str(), &currentEffectState)) { valueChanged = true; }
				ImGui::TableNextColumn();
				if (ImGui::Button(removeId.c_str()))
				{
					updatedInfoList[cellName].erase(
						std::remove_if(updatedInfoList[cellName].begin(), updatedInfoList[cellName].end(),
						[&info](const InteriorToggleInformation& interiorInfo) {
							return interiorInfo.effectName == info.effectName;
						}
					),
						updatedInfoList[cellName].end()
					);

					Manager::GetSingleton()->removeInteriorById(info);

					if (updatedInfoList[cellName].empty())
					{
						updatedInfoList.erase(cellName);
					}

					globalIndex--;
					continue;
				}
				if (ImGui::Button(editId.c_str()))
				{
					m_currentEditingEffect = currentEffectName;
					m_editingEffectIndex = globalIndex;
					ImGui::OpenPopup("Edit Effect Values");
				}

				ImGui::TableNextColumn();
				ImGui::Text("%s", cellName.c_str());

				if (valueChanged)
				{
					info.effectName = currentEffectName;
					info.state = currentEffectState;
					updatedInfoList[cellName].at(i) = info;
				}

				if (m_editingEffectIndex == globalIndex)
				{
					auto& targetUniforms = updatedInfoList[cellName].at(i).uniforms;
					HandleEffectEditing(targetUniforms, m_currentEditingEffect, m_editingEffectIndex);
				}
			}
			ImGui::EndTable();
		}

	}

	// Update the manager with the new list
	Manager::GetSingleton()->setInteriorToggleInfo(updatedInfoList);

	ImGui::SeparatorText("Add New");
	// Add new effect
	if (ImGui::Button("Add New Effect"))
	{
		ImGui::OpenPopup("Create Interior Entries");
	}
	AddNewInterior(updatedInfoList);

	ImGui::End();
}

void Menu::SpawnWeatherSettings(ImGuiID dockspace_id)
{
	ImGui::SetNextWindowDockID(dockspace_id, ImGuiCond_Always);
	ImGui::Begin("Weather Settings", &m_showWeatherSettings, ImGuiWindowFlags_NoCollapse);
	ImGui::Text("Configure weather toggling settings here.");
	ImGui::SeparatorText("Effects");

	// Retrieve the current weather toggle info
	std::map<std::string, std::vector<WeatherToggleInformation>> infoList = Manager::GetSingleton()->getWeatherToggleInfo();
	std::map<std::string, std::vector<WeatherToggleInformation>> updatedInfoList = infoList; // Start with existing info
	static char inputBuffer[256] = "";
	ImGui::InputTextWithHint("##Search", "Search Worldspaces...", inputBuffer, sizeof(inputBuffer));

	int headerId = -1;
	int globalIndex = 0;

	for (const auto& [worldSpaceName, effects] : infoList)
	{
		if (strlen(inputBuffer) > 0 && Utils::tolower(worldSpaceName).find(Utils::tolower(inputBuffer)) == std::string::npos)
			continue;

		headerId++;
		std::string headerUniqueId = worldSpaceName + std::to_string(headerId);

		if (ImGui::CollapsingHeader((worldSpaceName + "##" + headerUniqueId + "##Header").c_str(), ImGuiTreeNodeFlags_AllowOverlap | ImGuiTreeNodeFlags_AllowItemOverlap))
		{
			ImGui::BeginTable(("EffectsTable##" + headerUniqueId).c_str(), 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg);
			ImGui::TableSetupColumn(("Effect##" + headerUniqueId).c_str());
			ImGui::TableSetupColumn(("State##" + headerUniqueId).c_str());
			ImGui::TableSetupColumn(("Weather##" + headerUniqueId).c_str());
			ImGui::TableSetupColumn(("Actions##" + headerUniqueId).c_str());
			ImGui::TableSetupColumn(("Worldspace##" + headerUniqueId).c_str());
			ImGui::TableHeadersRow();

			for (int i = 0; i < effects.size(); i++, globalIndex++)
			{
				WeatherToggleInformation info = effects[i];
				bool valueChanged = false;

				std::string effectComboId = "Effect##" + headerUniqueId + std::to_string(i);
				std::string effectStateId = "State##" + headerUniqueId + std::to_string(i);
				std::string weatherId = "Weather##" + headerUniqueId + std::to_string(i);
				std::string removeId = "RemoveEffect##" + headerUniqueId + std::to_string(i);
				std::string editId = "EditEffect##" + headerUniqueId + std::to_string(i);

				std::string currentEffectName = info.effectName;
				std::string currentWeather = info.weather;
				bool currentEffectState = info.state;

				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				if (CreateCombo(effectComboId.c_str(), currentEffectName, m_effects, ImGuiComboFlags_None)) { valueChanged = true; }
				ImGui::TableNextColumn();
				if (ImGui::Checkbox(effectStateId.c_str(), &currentEffectState)) { valueChanged = true; }
				ImGui::TableNextColumn();
				if (CreateCombo(weatherId.c_str(), currentWeather, m_weathers, ImGuiComboFlags_None)) { valueChanged = true; }
				ImGui::TableNextColumn();
				if (ImGui::Button(removeId.c_str()))
				{
					updatedInfoList[worldSpaceName].erase(
						std::remove_if(updatedInfoList[worldSpaceName].begin(), updatedInfoList[worldSpaceName].end(),
						[&info](const WeatherToggleInformation& weatherInfo) {
							return weatherInfo.effectName == info.effectName && weatherInfo.weather == info.weather;
						}
					),
						updatedInfoList[worldSpaceName].end()
					);

					Manager::GetSingleton()->removeWeatherById(info);

					if (updatedInfoList[worldSpaceName].empty())
					{
						updatedInfoList.erase(worldSpaceName);
					}

					globalIndex--;
					continue;
				}
				if (ImGui::Button(editId.c_str()))
				{
					m_currentEditingEffect = currentEffectName;
					m_editingEffectIndex = globalIndex;
					ImGui::OpenPopup("Edit Effect Values");
				}

				ImGui::TableNextColumn();
				ImGui::Text("%s", worldSpaceName.c_str());

				if (valueChanged)
				{
					info.effectName = currentEffectName;
					info.weather = currentWeather;
					info.state = currentEffectState;
					updatedInfoList[worldSpaceName].at(i) = info;
				}

				if (m_editingEffectIndex == globalIndex)
				{
					auto& targetUniforms = updatedInfoList[worldSpaceName].at(i).uniforms;
					HandleEffectEditing(targetUniforms, m_currentEditingEffect, m_editingEffectIndex);
				}
			}
			ImGui::EndTable();
		}

	}

	// Update the manager with the new list
	Manager::GetSingleton()->setWeatherToggleInfo(updatedInfoList);

	ImGui::SeparatorText("Add New");
	// Add new effect
	if (ImGui::Button("Add New Effect"))
	{
		ImGui::OpenPopup("Create Weather Entries");
	}
	AddNewWeather(updatedInfoList);

	ImGui::End();
}

void Menu::AddNewTime(std::map<std::string, std::vector<TimeToggleInformation>>& updatedInfoList)
{
	static float currentStartTime;
	static float currentStopTime;

	static char startHourStr[3] = "00", startMinuteStr[3] = "00";
	static char stopHourStr[3] = "00", stopMinuteStr[3] = "00";

	if (ImGui::BeginPopupModal("Create Time Entries", NULL, ImGuiWindowFlags_None))
	{
		if (ImGui::IsWindowAppearing())
		{
			m_currentToggleReason.clear();
			m_currentEffects.clear();
			m_toggleState = false;
			m_entireReShadeToggleOn = false;
			m_inputBuffer01[0] = '\0';
			m_inputBuffer02[0] = '\0';
			strcpy(startHourStr, "00");
			strcpy(startMinuteStr, "00");
			strcpy(stopHourStr, "00");
			strcpy(stopMinuteStr, "00");
		}

		ImVec2 availableSpace = ImGui::GetContentRegionAvail();
		float childHeight = availableSpace.y * 0.30f;
		ImGui::SeparatorText("Select Worldspaces");
		ImGui::BeginChild("eBicWorldSpaceRegion", ImVec2(availableSpace.x, childHeight), true, ImGuiWindowFlags_HorizontalScrollbar);
		CreateTreeNode("Worldspaces", m_currentToggleReason, m_worldSpaces, m_inputBuffer01, sizeof(m_inputBuffer01), false);
		CreateTreeNode("Cells", m_currentToggleReason, m_interiorCells, m_inputBuffer01, sizeof(m_inputBuffer01), false);
		ImGui::EndChild();

		ImGui::SeparatorText("Select Time Period");

		// Start time
		ImGui::Text("Start Time: ");
		ImGui::SameLine();
		ImGui::TableNextColumn();
		ImGui::PushItemWidth(35);
		ImGui::InputText("##StartHours", startHourStr, sizeof(startHourStr), ImGuiInputTextFlags_CharsDecimal);
		ClampInputValue(startHourStr, 23);
		if (strcmp(startHourStr, "00") == 0 && strlen(startHourStr) == 0) strcpy(startHourStr, "0");
		ImGui::SameLine();
		ImGui::Text(":");
		ImGui::SameLine();
		ImGui::InputText("##StartMinutes", startMinuteStr, sizeof(startMinuteStr), ImGuiInputTextFlags_CharsDecimal);
		ClampInputValue(startMinuteStr, 59);
		if (strcmp(startMinuteStr, "00") == 0 && strlen(startMinuteStr) == 0) strcpy(startMinuteStr, "0");
		ImGui::PopItemWidth();

		// Stop Time
		ImGui::Text("Stop Time: ");
		ImGui::SameLine();
		ImGui::TableNextColumn();
		ImGui::PushItemWidth(35);
		ImGui::InputText("##StopHours", stopHourStr, sizeof(stopHourStr), ImGuiInputTextFlags_CharsDecimal);
		ClampInputValue(stopHourStr, 23);
		if (strcmp(stopHourStr, "00") == 0 && strlen(stopHourStr) == 0) strcpy(stopHourStr, "0");
		ImGui::SameLine();
		ImGui::Text(":");
		ImGui::SameLine();
		ImGui::InputText("##StopMinutes", stopMinuteStr, sizeof(stopMinuteStr), ImGuiInputTextFlags_CharsDecimal);
		ClampInputValue(stopMinuteStr, 59);
		if (strcmp(stopMinuteStr, "00") == 0 && strlen(stopMinuteStr) == 0) strcpy(stopMinuteStr, "0");
		ImGui::PopItemWidth();

		EffectOptions();

		ImGui::SeparatorText("Select Effects");
		ImGui::BeginChild("EffectsRegion", ImVec2(availableSpace.x, childHeight), true, ImGuiWindowFlags_HorizontalScrollbar);
		CreateTreeNode("Effects", m_currentEffects, m_effects, m_inputBuffer02, sizeof(m_inputBuffer02), m_entireReShadeToggleOn);
		ImGui::EndChild();

		ImGui::Separator();
		if (ImGui::Button("Finish"))
		{
			// Convert the input text to integers
			const int startHours = strlen(startHourStr) > 0 ? std::stoi(startHourStr) : 0;
			const int startMinutes = strlen(startMinuteStr) > 0 ? std::stoi(startMinuteStr) : 0;
			const int stopHours = strlen(stopHourStr) > 0 ? std::stoi(stopHourStr) : 0;
			const int stopMinutes = strlen(stopMinuteStr) > 0 ? std::stoi(stopMinuteStr) : 0;

			// Convert hours and minutes into float (HH.MM) format
			currentStartTime = startHours + (startMinutes / 100.0f);
			currentStopTime = stopHours + (stopMinutes / 100.0f);

			for (const auto& ws : m_currentToggleReason)
			{
				for (const auto& effect : m_currentEffects)
				{
					updatedInfoList[ws].emplace_back(TimeToggleInformation{ effect, currentStartTime, currentStopTime, m_toggleState });
				}
			}

			Manager::GetSingleton()->setTimeToggleInfo(updatedInfoList);

			ImGui::CloseCurrentPopup();
		}

		ImGui::SameLine();
		if (ImGui::Button("Cancel"))
		{
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}

void Menu::ClampInputValue(char* inputStr, int maxVal)
{
	int val = strlen(inputStr) > 0 ? std::stoi(inputStr) : 0;
	if (val > maxVal)
	{
		val = maxVal;
		snprintf(inputStr, 3, "%02d", val);
	}
}

void Menu::EditValues(const std::string& effectName, std::vector<UniformInfo>& toReturn)
{
	ImGui::GetIO().ConfigDragClickToInputText = true;
	if (ImGui::BeginPopupModal("Edit Effect Values", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("Editing effect: %s", effectName.c_str());
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		using format = reshade::api::format;
		const auto manager = Manager::GetSingleton();

		// Retrieve all uniforms for the effect
		std::vector<UniformInfo> uniforms = manager->enumerateUniformNames(effectName);

		for (auto& var : toReturn)
		{
			if (var.uniformVariable.handle == 0) // if 0 it's loaded from a preset
			{
				var.uniformVariable = s_pRuntime->find_uniform_variable(effectName.c_str(), var.uniformName.c_str());
				format baseType = format::unknown;

				s_pRuntime->get_uniform_variable_type(var.uniformVariable, &baseType);

				switch (baseType)
				{
				case format::r32_float:
					std::copy(var.floatValues.begin(), var.floatValues.begin() + std::min(var.floatValues.size(), size_t(4)), var.tempFloatValues);
					break;
				case format::r32_sint:
					std::copy(var.intValues.begin(), var.intValues.begin() + std::min(var.intValues.size(), size_t(4)), var.tempIntValues);
					break;
				case format::r32_uint:
					std::copy(var.uintValues.begin(), var.uintValues.begin() + std::min(var.uintValues.size(), size_t(4)), var.tempUIntValues);
					break;
				case format::r32_typeless:
					var.tempBoolValue = static_cast<bool>(var.boolValue);
					break;
				default:
					break;
				}
			}
		}

		auto FetchUniformValue = [&](UniformInfo& info) {

			if (info.prefetched)
				return;

			format baseType = format::unknown;
			s_pRuntime->get_uniform_variable_type(info.uniformVariable, &baseType);

			switch (baseType)
			{
			case format::r32_float:
			{
				float values[4] = {};
				manager->getUniformValue<float>(info.uniformVariable, values, 4);
				std::copy(std::begin(values), std::end(values), std::begin(info.tempFloatValues));
				break;
			}
			case format::r32_sint:
			{
				int values[4] = {};
				manager->getUniformValue<int>(info.uniformVariable, values, 4);
				std::copy(std::begin(values), std::end(values), std::begin(info.tempIntValues));
				break;
			}
			case format::r32_uint:
			{
				unsigned int values[4] = {};
				manager->getUniformValue<unsigned int>(info.uniformVariable, values, 4);
				std::copy(std::begin(values), std::end(values), std::begin(info.tempUIntValues));
				break;
			}
			case format::r32_typeless:
			{
				bool value = false;
				manager->getUniformValue<bool>(info.uniformVariable, &value, 1);
				info.tempBoolValue = value;
				break;
			}
			default:
				break;
			}
			info.prefetched = true;
			};

		// Ensure all uniforms are in `toReturn` before UI loop
		for (auto& uniformInfo : uniforms)
		{
			auto it = std::find_if(toReturn.begin(), toReturn.end(), [&uniformInfo](const UniformInfo& existingInfo) {
				return existingInfo.uniformVariable == uniformInfo.uniformVariable;
				});

			if (it == toReturn.end())
			{
				FetchUniformValue(uniformInfo);
				toReturn.emplace_back(uniformInfo);
			}
		}

		// Iterate through `toReturn` to handle UI interaction
		for (auto& uniformInfo : toReturn)
		{
			format baseType = format::unknown;
			s_pRuntime->get_uniform_variable_type(uniformInfo.uniformVariable, &baseType);

			if (uniformInfo.prefetched)
			{
				switch (baseType)
				{
				case format::r32_float:
				{
					int numElements = std::min(4, manager->getUniformDimension(uniformInfo.uniformVariable));

					switch (numElements)
					{
					case 1:
						if (ImGui::SliderFloat(uniformInfo.uniformName.c_str(), &uniformInfo.tempFloatValues[0], -64.0f, 64.0f))
						{
							uniformInfo.setFloatValues(uniformInfo.tempFloatValues, 1);
						}
						break;
					case 2:
						if (ImGui::SliderFloat2(uniformInfo.uniformName.c_str(), uniformInfo.tempFloatValues, -64.0f, 64.0f))
						{
							uniformInfo.setFloatValues(uniformInfo.tempFloatValues, 2);
						}
						break;
					case 3:
						if (ImGui::ColorEdit3(uniformInfo.uniformName.c_str(), uniformInfo.tempFloatValues))
						{
							uniformInfo.setFloatValues(uniformInfo.tempFloatValues, 3);
						}
						break;
					case 4:
						if (ImGui::ColorEdit4(uniformInfo.uniformName.c_str(), uniformInfo.tempFloatValues))
						{
							uniformInfo.setFloatValues(uniformInfo.tempFloatValues, 4);
						}
						break;
					}
				}
				break;
				case format::r32_sint:
				{
					int numElements = std::min(4, manager->getUniformDimension(uniformInfo.uniformVariable));

					switch (numElements)
					{
					case 1:
						if (ImGui::SliderInt(uniformInfo.uniformName.c_str(), &uniformInfo.tempIntValues[0], -64, 64))
						{
							uniformInfo.setIntValues(uniformInfo.tempIntValues, 1);
						}
						break;
					case 2:
						if (ImGui::SliderInt2(uniformInfo.uniformName.c_str(), uniformInfo.tempIntValues, -64, 64))
						{
							uniformInfo.setIntValues(uniformInfo.tempIntValues, 2);
							break;
					case 3:
						if (ImGui::SliderInt3(uniformInfo.uniformName.c_str(), uniformInfo.tempIntValues, -64, 64))
						{
							uniformInfo.setIntValues(uniformInfo.tempIntValues, 3);
						}
						break;
					case 4:
						if (ImGui::SliderInt4(uniformInfo.uniformName.c_str(), uniformInfo.tempIntValues, -64, 64))
						{
							uniformInfo.setIntValues(uniformInfo.tempIntValues, 4);
						}
						break;
						}

					}
				}
				break;
				case format::r32_uint:
				{
					int numElements = std::min(4, manager->getUniformDimension(uniformInfo.uniformVariable));

					switch (numElements)
					{
					case 1:
						if (ImGui::SliderScalar(uniformInfo.uniformName.c_str(), ImGuiDataType_U32, &uniformInfo.tempUIntValues[0], 0, reinterpret_cast<void*>(64)))
						{
							uniformInfo.setUIntValues(uniformInfo.tempUIntValues, 1);
						}
						break;
					case 2:
						if (ImGui::SliderScalarN(uniformInfo.uniformName.c_str(), ImGuiDataType_U32, uniformInfo.tempUIntValues, 2, 0, reinterpret_cast<void*>(64)))
						{
							uniformInfo.setUIntValues(uniformInfo.tempUIntValues, 2);
						}
						break;
					case 3:
						if (ImGui::SliderScalarN(uniformInfo.uniformName.c_str(), ImGuiDataType_U32, uniformInfo.tempUIntValues, 3, 0, reinterpret_cast<void*>(64)))
						{
							uniformInfo.setUIntValues(uniformInfo.tempUIntValues, 3);
						}
						break;
					case 4:
						if (ImGui::SliderScalarN(uniformInfo.uniformName.c_str(), ImGuiDataType_U32, uniformInfo.tempUIntValues, 4, 0, reinterpret_cast<void*>(64)))
						{
							uniformInfo.setUIntValues(uniformInfo.tempUIntValues, 4);
						}
						break;
					}
				}
				break;
				case format::r32_typeless:
				{
					if (ImGui::Checkbox(uniformInfo.uniformName.c_str(), &uniformInfo.tempBoolValue))
					{
						uniformInfo.setBoolValues(static_cast<uint8_t>(uniformInfo.tempBoolValue));
					}
				}
				break;
				default:
					break;
				}

				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Spacing();
			}
		}

		// Close button
		if (ImGui::Button("Close"))
		{
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}


void Menu::AddNewInterior(std::map<std::string, std::vector<InteriorToggleInformation>>& updatedInfoList)
{

	if (ImGui::BeginPopupModal("Create Interior Entries", NULL, ImGuiWindowFlags_None))
	{
		// Reset static variables for each popup
		if (ImGui::IsWindowAppearing())
		{
			m_currentToggleReason.clear();
			m_currentEffects.clear();
			m_toggleState = false;
			m_entireReShadeToggleOn = false;
			m_inputBuffer01[0] = '\0';
			m_inputBuffer02[0] = '\0';
		}

		ImVec2 availableSpace = ImGui::GetContentRegionAvail();
		float childHeight = availableSpace.y * 0.35f;
		ImGui::SeparatorText("Select Interior Cells");
		ImGui::BeginChild("CellRegion", ImVec2(availableSpace.x, childHeight), true, ImGuiWindowFlags_HorizontalScrollbar);
		CreateTreeNode("Cells", m_currentToggleReason, m_interiorCells, m_inputBuffer01, sizeof(m_inputBuffer01), false);
		ImGui::EndChild();

		EffectOptions();

		ImGui::SeparatorText("Select Effects");
		ImGui::BeginChild("EffectsRegion", ImVec2(availableSpace.x, childHeight), true, ImGuiWindowFlags_HorizontalScrollbar);
		CreateTreeNode("Effects", m_currentEffects, m_effects, m_inputBuffer02, sizeof(m_inputBuffer02), m_entireReShadeToggleOn);
		ImGui::EndChild();

		ImGui::Separator();
		if (ImGui::Button("Finish"))
		{
			for (const auto& cell : m_currentToggleReason)
			{
				for (const auto& effect : m_currentEffects)
				{
					updatedInfoList[cell].emplace_back(InteriorToggleInformation{ effect, m_toggleState });
				}
			}

			Manager::GetSingleton()->setInteriorToggleInfo(updatedInfoList);

			ImGui::CloseCurrentPopup();
		}

		ImGui::SameLine();
		if (ImGui::Button("Cancel"))
		{
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}

void Menu::AddNewWeather(std::map<std::string, std::vector<WeatherToggleInformation>>& updatedInfoList)
{
	static std::vector<std::string> currentWeather;

	if (ImGui::BeginPopupModal("Create Weather Entries", NULL, ImGuiWindowFlags_None))
	{
		if (ImGui::IsWindowAppearing())
		{
			m_currentToggleReason.clear();
			currentWeather.clear();
			m_currentEffects.clear();
			m_toggleState = false;
			m_entireReShadeToggleOn = false;
			m_inputBuffer01[0] = '\0';
			m_inputBuffer02[0] = '\0';
			m_inputBuffer03[0] = '\0';
		}

		ImVec2 availableSpace = ImGui::GetContentRegionAvail();
		float childHeight = availableSpace.y * 0.25f;
		ImGui::SeparatorText("Select Worldspaces");
		ImGui::BeginChild("WorldspacesRegion", ImVec2(availableSpace.x, childHeight), true, ImGuiWindowFlags_HorizontalScrollbar);
		CreateTreeNode("Worldspaces", m_currentToggleReason, m_worldSpaces, m_inputBuffer01, sizeof(m_inputBuffer01), false);
		ImGui::EndChild();

		ImGui::SeparatorText("Select Weather");
		ImGui::BeginChild("WeatherRegion", ImVec2(availableSpace.x, childHeight), true, ImGuiWindowFlags_HorizontalScrollbar);
		CreateTreeNode("Weather", currentWeather, m_weathers, m_inputBuffer02, sizeof(m_inputBuffer02), false);
		ImGui::EndChild();

		EffectOptions();

		ImGui::SeparatorText("Select Effects");
		ImGui::BeginChild("EffectsRegion", ImVec2(availableSpace.x, childHeight), true, ImGuiWindowFlags_HorizontalScrollbar);
		CreateTreeNode("Effects", m_currentEffects, m_effects, m_inputBuffer03, sizeof(m_inputBuffer03), m_entireReShadeToggleOn);
		ImGui::EndChild();

		ImGui::Separator();
		if (ImGui::Button("Finish"))
		{
			for (const auto& ws : m_currentToggleReason)
			{
				for (const auto& effect : m_currentEffects)
				{
					for (const auto& weather : currentWeather)
					{
						updatedInfoList[ws].emplace_back(WeatherToggleInformation{ effect, weather, m_toggleState });
					}
				}
			}

			Manager::GetSingleton()->setWeatherToggleInfo(updatedInfoList);

			ImGui::CloseCurrentPopup();
		}

		ImGui::SameLine();
		if (ImGui::Button("Cancel"))
		{
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}

void Menu::SaveFile()
{
	if (m_saveConfigPopupOpen)
	{
		ImGui::OpenPopup("Save Config");

		// Check if the "Save Config" modal is open
		if (ImGui::BeginPopupModal("Save Config", NULL, ImGuiWindowFlags_AlwaysAutoResize))
		{
			if (ImGui::IsWindowAppearing())
			{
				m_inputBuffer01[0] = '\0';
			}

			ImGui::Text("Enter the filename:");

			ImGui::InputText("##FileName", m_inputBuffer01, sizeof(m_inputBuffer01));

			if (ImGui::Button("Ok, Save!"))
			{
				// Use the provided filename or the default if empty
				std::string filename = (m_inputBuffer01[0] != '\0') ? m_inputBuffer01 : "NewPreset";
				filename = filename + ".json";
				const auto start = std::chrono::high_resolution_clock::now();
				const bool success = Manager::GetSingleton()->serializeJSONPreset(filename);
				const auto end = std::chrono::high_resolution_clock::now();
				std::chrono::duration<double, std::milli> duration = end - start;

				// update preset list
				m_presets.clear();
				m_presets = Manager::GetSingleton()->enumeratePresets();

				if (!success)
				{
					m_lastMessage = "Failed to save preset '" + filename + "'.";
					m_lastMessageColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
				}
				else
				{
					m_selectedPreset = filename;
					Manager::GetSingleton()->setLastPreset(m_selectedPreset);
					Manager::GetSingleton()->serializeINI();

					m_lastMessage = "Successfully saved Preset: '" + filename + "'! Took: " + std::to_string(duration.count()) + "ms";
					m_lastMessageColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
				}

				ImGui::CloseCurrentPopup();
				m_saveConfigPopupOpen = false;
			}

			ImGui::SameLine();

			if (ImGui::Button("Cancel"))
			{
				ImGui::CloseCurrentPopup();
				m_saveConfigPopupOpen = false;
			}

			ImGui::EndPopup();
		}
	}
}


void Menu::HandleEffectEditing(std::vector<UniformInfo>& targetUniforms, std::string& currentEditingEffect, int& editingEffectIndex)
{
	static std::vector<UniformInfo> uniformInfos;

	//SKSE::log::debug("Before updating, uniforms size: {}", targetUniforms.size());

	uniformInfos = std::move(targetUniforms);

	EditValues(currentEditingEffect, uniformInfos);

	if (!uniformInfos.empty())
	{
		targetUniforms = std::move(uniformInfos);
		//SKSE::log::debug("After updating, uniforms size: {}", targetUniforms.size());
	}

	if (!ImGui::IsPopupOpen("Edit Effect Values"))
	{
		editingEffectIndex = -1;
		currentEditingEffect.clear();
		uniformInfos.clear();
	}
}

void Menu::EffectOptions()
{
	ImGui::SeparatorText("Effect Options");
	ImGui::Checkbox("Toggle State (on/off)", &m_toggleState);

	bool hasSelection = !m_currentEffects.empty();
	if (m_currentEffects.size() == 1 && m_currentEffects[0] == "EntireReShade")
	{
		hasSelection = false;
		m_entireReShadeToggleOn = true;
	}

	if (hasSelection)
	{
		ImGui::BeginDisabled();
	}

	if (ImGui::Checkbox("Toggle Entire ReShade", &m_entireReShadeToggleOn))
	{
		if (m_entireReShadeToggleOn)
		{
			// Add "EntireReShade" only if the list is empty and it is not already in the list
			if (m_currentEffects.empty())
			{
				m_currentEffects.emplace_back("EntireReShade");
			}
		}
		else
		{
			// Remove "EntireReShade" if it is being toggled off
			m_currentEffects.erase(
				std::remove(m_currentEffects.begin(), m_currentEffects.end(), "EntireReShade"),
				m_currentEffects.end()
			);
		}
	}

	if (hasSelection)
	{
		ImGui::EndDisabled();
	}
}
