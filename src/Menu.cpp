#include "Menu.h"
#include "Manager.h"
#include "Utils.h"

void Menu::SettingsMenu()
{
	if (ImGui::Button("[PH] Configure SSEReshadeToggler"))
		m_openSettingsMenu = true;

	if (m_openSettingsMenu)
	{
		// TODO: ensure that we are only putting the colors onto our own window and its subwindows
		SetColors();

		// Create the main settings window with docking enabled
		ImGui::Begin("[PH] Settings Window", &m_openSettingsMenu);
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

		RemoveColors();
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
		m_inputBuffer[0] = '\0';
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
	std::vector<MenuToggleInformation> infoList = Manager::GetSingleton()->getMenuToggleInfo();
	std::vector<MenuToggleInformation> updatedInfoList = infoList;

	// Group menus
	std::unordered_map<std::string, std::vector<MenuToggleInformation>> menuEffectMap;
	for (auto& info : infoList)
	{
		if (!info.effectName.empty())
		{
			menuEffectMap[info.menuName].emplace_back(info);
		}
	}

	static char inputBuffer[256] = "";
	ImGui::InputTextWithHint("##Search", "Search Menus...", inputBuffer, sizeof(inputBuffer));

	int headerId = -1;
	int globalIndex = 0;
	static std::string currentEditingEffect;
	static int editingEffectIndex = -1;

	for (const auto& [menuName, effects] : menuEffectMap)
	{
		if (strlen(inputBuffer) > 0 && menuName.find(inputBuffer) == std::string::npos)
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
				if (CreateCombo(effectComboId.c_str(), currentEffectName, m_effects, ImGuiComboFlags_None)) { valueChanged = true; }
				ImGui::TableNextColumn();
				if (ImGui::Checkbox(effectStateId.c_str(), &currentEffectState)) { valueChanged = true; }
				ImGui::TableNextColumn();
				if (ImGui::Button(removeId.c_str()))
				{
					updatedInfoList.erase(updatedInfoList.begin() + globalIndex);
					globalIndex--;
					continue;
				}
				if (ImGui::Button(editId.c_str()))
				{
					currentEditingEffect = currentEffectName;
					editingEffectIndex = globalIndex; // Store the index of the effect being edited
					ImGui::OpenPopup("Edit Effect Values");
				}

				ImGui::TableNextColumn();
				ImGui::Text("%s", menuName.c_str());

				if (valueChanged)
				{
					info.menuName = menuName;
					info.effectName = currentEffectName;
					info.state = currentEffectState;
					updatedInfoList[globalIndex] = info;
				}

				if (editingEffectIndex == globalIndex)
				{
					std::vector<UniformInfo> uniformInfos = EditValues(currentEditingEffect);
					if (!uniformInfos.empty())
					{
						for (auto& uniformInfo : uniformInfos)
						{
							updatedInfoList[editingEffectIndex].uniforms.emplace_back(uniformInfo);
						}
					}

					if (!ImGui::IsPopupOpen("Edit Effect Values"))
					{
						editingEffectIndex = -1;
						currentEditingEffect.clear();
					}
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

void Menu::AddNewMenu(std::vector<MenuToggleInformation>& updatedInfoList)
{
	static std::vector<std::string> currentMenus;
	static std::vector<std::string> currentEffects;
	static bool toggled = false;

	if (ImGui::BeginPopupModal("Create Menu Entries", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		// Reset static variables for each popup
		if (ImGui::IsWindowAppearing())
		{
			currentMenus.clear();
			currentEffects.clear();
			toggled = false;
		}

		ImGui::Text("Select Menus");
		CreateTreeNode("Menus", currentMenus, m_menuNames);
		ImGui::Separator();

		ImGui::Text("Select Effects");
		CreateTreeNode("Effects", currentEffects, m_effects);
		ImGui::SameLine();
		ImGui::Checkbox("Toggled On", &toggled);

		ImGui::Separator();
		if (ImGui::Button("Finish"))
		{
			CombineVectorsToStructs(currentMenus, currentEffects, updatedInfoList, toggled);

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
	ImGui::InputTextWithHint("##Search", "Search Worldspace...", inputBuffer, sizeof(inputBuffer));

	int headerId = -1;
	int globalIndex = 0;
	for (const auto& [cellName, effects] : infoList)
	{
		if (strlen(inputBuffer) > 0 && cellName.find(inputBuffer) == std::string::npos)
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

					if (updatedInfoList[cellName].empty())
					{
						updatedInfoList.erase(cellName);
					}

					globalIndex--;
					continue;
				}
				if (ImGui::Button(editId.c_str())) { ImGui::Text("I do nothing"); }
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
			}
			ImGui::EndTable();
		}
	}

	Manager::GetSingleton()->setTimeToggleInfo(updatedInfoList);

	ImGui::SeparatorText("Add New");

	// Add new effect
	if (ImGui::Button("Add New Effect"))
	{
		ImGui::OpenPopup("Create Time Entry");
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
		if (strlen(inputBuffer) > 0 && cellName.find(inputBuffer) == std::string::npos)
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

					if (updatedInfoList[cellName].empty())
					{
						updatedInfoList.erase(cellName);
					}

					globalIndex--;
					continue;
				}
				if (ImGui::Button(editId.c_str())) { ImGui::Text("I do nothing"); }
				ImGui::TableNextColumn();
				ImGui::Text("%s", cellName.c_str());

				if (valueChanged)
				{
					info.effectName = currentEffectName;
					info.state = currentEffectState;
					updatedInfoList[cellName].at(i) = info;
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
		ImGui::OpenPopup("Create Interior Entry");
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
		if (strlen(inputBuffer) > 0 && worldSpaceName.find(inputBuffer) == std::string::npos)
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
				std::string currentWeather = info.weatherFlag;
				bool currentEffectState = info.state;

				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				if (CreateCombo(effectComboId.c_str(), currentEffectName, m_effects, ImGuiComboFlags_None)) { valueChanged = true; }
				ImGui::TableNextColumn();
				if (ImGui::Checkbox(effectStateId.c_str(), &currentEffectState)) { valueChanged = true; }
				ImGui::TableNextColumn();
				if (CreateCombo(weatherId.c_str(), currentWeather, m_weatherFlags, ImGuiComboFlags_None)) { valueChanged = true; }
				ImGui::TableNextColumn();
				if (ImGui::Button(removeId.c_str()))
				{
					updatedInfoList[worldSpaceName].erase(
						std::remove_if(updatedInfoList[worldSpaceName].begin(), updatedInfoList[worldSpaceName].end(),
						[&info](const WeatherToggleInformation& weatherInfo) {
							return weatherInfo.effectName == info.effectName && weatherInfo.weatherFlag == info.weatherFlag;
						}
					),
						updatedInfoList[worldSpaceName].end()
					);

					if (updatedInfoList[worldSpaceName].empty())
					{
						updatedInfoList.erase(worldSpaceName);
					}

					globalIndex--;
					continue;
				}
				if (ImGui::Button(editId.c_str())) { ImGui::Text("I do nothing"); }
				ImGui::TableNextColumn();
				ImGui::Text("%s", worldSpaceName.c_str());

				if (valueChanged)
				{
					info.effectName = currentEffectName;
					info.weatherFlag = currentWeather;
					info.state = currentEffectState;
					updatedInfoList[worldSpaceName].at(i) = info;
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
		ImGui::OpenPopup("Create Weather Entry");
	}
	AddNewWeather(updatedInfoList);

	ImGui::End();
}

void Menu::AddNewTime(std::map<std::string, std::vector<TimeToggleInformation>>& updatedInfoList)
{
	static std::string currentWorldSpace;
	static std::string currentEffect;
	static float currentStartTime;
	static float currentStopTime;
	static bool toggled = false;

	static char startHourStr[3] = "00", startMinuteStr[3] = "00";
	static char stopHourStr[3] = "00", stopMinuteStr[3] = "00";

	if (ImGui::BeginPopupModal("Create Time Entry", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		// Reset static variables for each popup
		if (ImGui::IsWindowAppearing())
		{
			currentWorldSpace.clear();
			currentEffect.clear();
			toggled = false;
			strcpy(startHourStr, "00");
			strcpy(startMinuteStr, "00");
			strcpy(stopHourStr, "00");
			strcpy(stopMinuteStr, "00");
		}

		ImGui::Text("Select a Worldspace");
		CreateCombo("Worldspace", currentWorldSpace, m_worldSpaces, ImGuiComboFlags_None);
		ImGui::Separator();

		// Start time
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

		ImGui::Separator();
		ImGui::Text("Select the Effect");
		CreateCombo("Effect", currentEffect, m_effects, ImGuiComboFlags_None);
		ImGui::SameLine();
		ImGui::Checkbox("Toggled On", &toggled);

		ImGui::Separator();
		if (ImGui::Button("Finish"))
		{
			// Convert the input text to integers
			int startHours = strlen(startHourStr) > 0 ? std::stoi(startHourStr) : 0;
			int startMinutes = strlen(startMinuteStr) > 0 ? std::stoi(startMinuteStr) : 0;
			int stopHours = strlen(stopHourStr) > 0 ? std::stoi(stopHourStr) : 0;
			int stopMinutes = strlen(stopMinuteStr) > 0 ? std::stoi(stopMinuteStr) : 0;

			// Convert hours and minutes into float (HH.MM) format
			currentStartTime = startHours + (startMinutes / 100.0f);
			currentStopTime = stopHours + (stopMinutes / 100.0f);

			TimeToggleInformation info;
			info.effectName = currentEffect;
			info.startTime = currentStartTime;
			info.stopTime = currentStopTime;
			info.state = toggled;

			updatedInfoList[currentWorldSpace].emplace_back(info);
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

std::vector<UniformInfo> Menu::EditValues(const std::string& effectName)
{

	if (ImGui::BeginPopupModal("Edit Effect Values", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		std::vector<UniformInfo> toReturn;
		ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);

		ImGui::Text("Editing effect: %s", effectName.c_str());


		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
		std::vector<UniformInfo> uniforms = Manager::GetSingleton()->enumerateUniformNames(effectName);

		for (auto& uniformInfo : uniforms)
		{
			std::string type = Manager::GetSingleton()->getUniformType(uniformInfo.uniformVariable);

			ImGui::Text("Uniform: %s", uniformInfo.uniformName.c_str());
			ImGui::SameLine();
			ImGui::TextDisabled("[%s]", type.c_str());


			if (type.find("bool") != std::string::npos)
			{
				bool value = false;
				Manager::GetSingleton()->getUniformValue<bool>(uniformInfo.uniformVariable, &value, 1);
				if (ImGui::Checkbox(uniformInfo.uniformName.c_str(), &value)) {
					uniformInfo.setBoolValues(reinterpret_cast<char*>(&value), 1);
				}
			}
			else if (type.find("float") != std::string::npos)
			{
				float values[4] = { 0.0f };
				int numElements = std::min(4, Manager::GetSingleton()->getUniformDimension(uniformInfo.uniformVariable));

				Manager::GetSingleton()->getUniformValue<float>(uniformInfo.uniformVariable, values, numElements);

				// Handle other float types (float1 or float2)
				switch (numElements)
				{
				case 1:
					if (ImGui::SliderFloat(uniformInfo.uniformName.c_str(), &values[0], 0.0f, 1.0f)) {
						uniformInfo.setFloatValues(values, 1);
					}
					break;
				case 2:
					if (ImGui::SliderFloat2(uniformInfo.uniformName.c_str(), values, 0.0f, 1.0f)) {
						uniformInfo.setFloatValues(values, 2);
					}
					break;
				case 3:
					if (ImGui::ColorEdit3(uniformInfo.uniformName.c_str(), values)) {
						uniformInfo.setFloatValues(values, 3);
					}
					break;
				case 4:
					if (ImGui::ColorEdit4(uniformInfo.uniformName.c_str(), values)) {
						uniformInfo.setFloatValues(values, 4);
					}
					break;
				}

			}
			else if (type.find("int") != std::string::npos)
			{
				// Handle multi-dimensional int types (int, int2, int3, int4)
				int values[4] = { 0 };
				int numElements = std::min(4, Manager::GetSingleton()->getUniformDimension(uniformInfo.uniformVariable));

				Manager::GetSingleton()->getUniformValue<int>(uniformInfo.uniformVariable, values, numElements);

				switch (numElements)
				{
				case 1:
					if (ImGui::SliderInt(uniformInfo.uniformName.c_str(), &values[0], 0, 100)) {
						uniformInfo.setIntValues(values, 1);
					}
					break;
				case 2:
					if (ImGui::SliderInt2(uniformInfo.uniformName.c_str(), values, 0, 100)) {
						uniformInfo.setIntValues(values, 2);
						break;
				case 3:
					if (ImGui::SliderInt3(uniformInfo.uniformName.c_str(), values, 0, 100)) {
						uniformInfo.setIntValues(values, 3);
					}
					break;
				case 4:
					if (ImGui::SliderInt4(uniformInfo.uniformName.c_str(), values, 0, 100)) {
						uniformInfo.setIntValues(values, 4);
					}
					break;
					}

				}
			}
			else if (type.find("unsigned int") != std::string::npos)
			{
				unsigned int values[4] = { 0 };  // Max size for vec4
				int numElements = std::min(4, Manager::GetSingleton()->getUniformDimension(uniformInfo.uniformVariable));

				Manager::GetSingleton()->getUniformValue<unsigned int>(uniformInfo.uniformVariable, values, numElements);

				switch (numElements)
				{
				case 1:
					if (ImGui::SliderScalar(uniformInfo.uniformName.c_str(), ImGuiDataType_U32, &values[0], 0, reinterpret_cast<void*>(100))) {
						uniformInfo.setUIntValues(values, 1);
					}
					break;
				case 2:
					if (ImGui::SliderScalarN(uniformInfo.uniformName.c_str(), ImGuiDataType_U32, values, 2, 0, reinterpret_cast<void*>(100))) {
						uniformInfo.setUIntValues(values, 2);
					}
					break;
				case 3:
					if (ImGui::SliderScalarN(uniformInfo.uniformName.c_str(), ImGuiDataType_U32, values, 3, 0, reinterpret_cast<void*>(100))) {
						uniformInfo.setUIntValues(values, 3);
					}
					break;
				case 4:
					if (ImGui::SliderScalarN(uniformInfo.uniformName.c_str(), ImGuiDataType_U32, values, 4, 0, reinterpret_cast<void*>(100))) {
						uniformInfo.setUIntValues(values, 4);
					}
					break;
				}
			}

			// Add a separator between different uniforms for visual clarity
			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			toReturn.emplace_back(uniformInfo);
		}

		if (ImGui::Button("Save"))
		{
			ImGui::CloseCurrentPopup();
			ImGui::PopFont();
			ImGui::EndPopup();
			return toReturn;
		}

		// Add a cancel button to close the modal
		if (ImGui::Button("Cancel"))
		{
			ImGui::CloseCurrentPopup();
		}

		ImGui::PopFont();
		ImGui::EndPopup();
	}

	return {};
}

void Menu::AddNewInterior(std::map<std::string, std::vector<InteriorToggleInformation>>& updatedInfoList)
{
	static std::string currentCell;
	static std::string currentEffect;
	static bool toggled = false;

	if (ImGui::BeginPopupModal("Create Interior Entry", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		// Reset static variables for each popup
		if (ImGui::IsWindowAppearing())
		{
			currentCell.clear();
			currentEffect.clear();
			toggled = false;
		}

		ImGui::Text("Select an Interior Cell");
		CreateCombo("Cell", currentCell, m_interiorCells, ImGuiComboFlags_None);
		ImGui::Separator();

		ImGui::Text("Select the Effect");
		CreateCombo("Effect", currentEffect, m_effects, ImGuiComboFlags_None);
		ImGui::SameLine();
		ImGui::Checkbox("Toggled On", &toggled);

		ImGui::Separator();
		if (ImGui::Button("Finish"))
		{
			InteriorToggleInformation info;
			info.effectName = currentEffect;
			info.state = toggled;

			updatedInfoList[currentCell].emplace_back(info);
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
	static std::string currentWorldSpace;
	static std::string currentWeatherFlag;
	static std::string currentEffect;
	static bool toggled = false;

	if (ImGui::BeginPopupModal("Create Weather Entry", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		// Reset static variables for each popup
		if (ImGui::IsWindowAppearing())
		{
			currentWorldSpace.clear();
			currentWeatherFlag.clear();
			currentEffect.clear();
			toggled = false;
		}

		ImGui::Text("Select a Worldspace");
		CreateCombo("Worldspace", currentWorldSpace, m_worldSpaces, ImGuiComboFlags_None);
		ImGui::Text("Select a Weather");
		CreateCombo("Weather", currentWeatherFlag, m_weatherFlags, ImGuiComboFlags_None);
		ImGui::Separator();

		ImGui::Text("Select the Effect");
		CreateCombo("Effect", currentEffect, m_effects, ImGuiComboFlags_None);
		ImGui::SameLine();
		ImGui::Checkbox("Toggled On", &toggled);

		ImGui::Separator();
		if (ImGui::Button("Finish"))
		{
			WeatherToggleInformation info;
			info.weatherFlag = currentWeatherFlag;
			info.effectName = currentEffect;
			info.state = toggled;

			updatedInfoList[currentWorldSpace].emplace_back(info);
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
			ImGui::Text("Enter the filename:");

			ImGui::InputText("##FileName", m_inputBuffer, sizeof(m_inputBuffer));

			if (ImGui::Button("Ok, Save!"))
			{
				// Use the provided filename or the default if empty
				std::string filename = (m_inputBuffer[0] != '\0') ? m_inputBuffer : "NewPreset";
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
