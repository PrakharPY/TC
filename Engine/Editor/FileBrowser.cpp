#include "Editor/FileBrowser.h"

#include <algorithm>
#include <cctype>
#include <system_error>

#include <imgui.h>

namespace TC
{
    namespace
    {
        bool IsDirectory(const std::filesystem::directory_entry& entry)
        {
            std::error_code ec;
            return entry.is_directory(ec) && !ec;
        }
    }

    void FileBrowser::Open(
        FileBrowserMode mode,
        const std::filesystem::path& startPath)
    {
        m_Mode = mode;
        m_IsOpen = true;
        m_SelectedPath.clear();
        m_Status.clear();
        m_ConfirmDelete = false;
        m_ConfirmOverwrite = false;

        std::error_code ec;
        std::filesystem::path start = startPath;
        if (start.empty())
            start = std::filesystem::current_path(ec);
        if (ec || !std::filesystem::exists(start, ec) ||
            !std::filesystem::is_directory(start, ec))
            start = std::filesystem::current_path(ec);

        m_CurrentDirectory = std::filesystem::weakly_canonical(start, ec);
        if (ec)
            m_CurrentDirectory = std::filesystem::current_path();

        if (m_Mode == FileBrowserMode::SaveAs)
            m_FileName = "Untitled.tcscene";

        RefreshEntries();
    }

    bool FileBrowser::ConsumeResult(FileBrowserResult& result)
    {
        if (!m_HasPendingResult)
            return false;

        result = m_PendingResult;
        m_PendingResult = {};
        m_HasPendingResult = false;
        return true;
    }

    bool FileBrowser::IsSceneFile(const std::filesystem::path& path) const
    {
        std::string extension = path.extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return extension == ".tcscene";
    }

    void FileBrowser::SetStatus(const std::string& message)
    {
        m_Status = message;
    }

    void FileBrowser::NavigateTo(const std::filesystem::path& path)
    {
        std::error_code ec;
        if (std::filesystem::is_directory(path, ec) && !ec)
        {
            m_CurrentDirectory = std::filesystem::weakly_canonical(path, ec);
            if (ec)
                m_CurrentDirectory = path;
            m_SelectedPath.clear();
            RefreshEntries();
        }
        else
        {
            SetStatus("Unable to open that folder.");
        }
    }

    void FileBrowser::RefreshEntries()
    {
        m_Entries.clear();
        std::error_code ec;
        std::filesystem::directory_iterator iterator(
            m_CurrentDirectory,
            std::filesystem::directory_options::skip_permission_denied,
            ec);

        if (ec)
        {
            SetStatus("Cannot read this folder: " + ec.message());
            return;
        }

        for (const auto& entry : iterator)
        {
            std::error_code entryError;
            if (entry.is_directory(entryError) ||
                (!entryError && IsSceneFile(entry.path())))
                m_Entries.push_back(entry);
        }

        std::sort(m_Entries.begin(), m_Entries.end(),
            [](const auto& a, const auto& b)
            {
                const bool aDirectory = IsDirectory(a);
                const bool bDirectory = IsDirectory(b);
                if (aDirectory != bDirectory)
                    return aDirectory;
                return a.path().filename().string() <
                       b.path().filename().string();
            });
    }

    void FileBrowser::Render()
    {
        if (!m_IsOpen)
            return;

        ImGui::SetNextWindowSize(ImVec2(860.0f, 590.0f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSizeConstraints(ImVec2(620.0f, 420.0f),
            ImVec2(1500.0f, 1100.0f));

        if (ImGui::IsKeyPressed(ImGuiKey_Escape))
            m_IsOpen = false;

        if (!ImGui::Begin(
            m_Mode == FileBrowserMode::Open ? "Open Scene" : "Save Scene As",
            &m_IsOpen,
            ImGuiWindowFlags_NoCollapse))
        {
            ImGui::End();
            return;
        }

        if (ImGui::Button("Up") && m_CurrentDirectory.has_parent_path())
            NavigateTo(m_CurrentDirectory.parent_path());
        ImGui::SameLine();
        if (ImGui::Button("Refresh"))
            RefreshEntries();
        ImGui::SameLine();
        ImGui::TextUnformatted(m_CurrentDirectory.string().c_str());

        ImGui::Separator();

        const float bottomArea = m_Mode == FileBrowserMode::SaveAs ? 150.0f : 112.0f;
        ImGui::BeginChild("SceneFileList", ImVec2(0.0f, -bottomArea), true);

        for (const auto& entry : m_Entries)
        {
            const bool directory = IsDirectory(entry);
            const std::filesystem::path path = entry.path();
            const std::string label =
                std::string(directory ? "[DIR]  " : "[SCENE]  ") +
                path.filename().string();

            const bool selected = (m_SelectedPath == path);
            if (ImGui::Selectable(label.c_str(), selected,
                ImGuiSelectableFlags_AllowDoubleClick))
            {
                m_SelectedPath = path;
                if (!directory && m_Mode == FileBrowserMode::SaveAs)
                    m_FileName = path.filename().string();

                if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                {
                    if (directory)
                        NavigateTo(path);
                    else if (m_Mode == FileBrowserMode::Open)
                    {
                        m_PendingResult = { FileBrowserAction::Open, path };
                        m_HasPendingResult = true;
                        m_IsOpen = false;
                    }
                }
            }
        }

        ImGui::EndChild();

        if (m_Mode == FileBrowserMode::SaveAs)
        {
            ImGui::TextUnformatted("File name");
            ImGui::SetNextItemWidth(-1.0f);
            char fileNameBuffer[512] = {};
            const size_t fileNameLength = (std::min)(m_FileName.size(), sizeof(fileNameBuffer) - 1);
            m_FileName.copy(fileNameBuffer, fileNameLength);
            if (ImGui::InputText("##SceneFileName", fileNameBuffer, sizeof(fileNameBuffer)))
                m_FileName = fileNameBuffer;
        }

        if (ImGui::Button("New Folder"))
            ImGui::OpenPopup("Create Folder");
        ImGui::SameLine();
        if (m_SelectedPath.empty())
            ImGui::BeginDisabled();
        if (ImGui::Button("Rename"))
        {
            m_RenameName = m_SelectedPath.filename().string();
            ImGui::OpenPopup("Rename Item");
        }
        ImGui::SameLine();
        if (ImGui::Button("Delete"))
            m_ConfirmDelete = true;
        if (m_SelectedPath.empty())
            ImGui::EndDisabled();

        if (m_Mode == FileBrowserMode::Open)
        {
            ImGui::SameLine();
            if (ImGui::Button("Open", ImVec2(90.0f, 0.0f)) &&
                !m_SelectedPath.empty() && IsSceneFile(m_SelectedPath))
            {
                m_PendingResult = { FileBrowserAction::Open, m_SelectedPath };
                m_HasPendingResult = true;
                m_IsOpen = false;
            }
        }
        else
        {
            ImGui::SameLine();
            if (ImGui::Button("Save", ImVec2(90.0f, 0.0f)))
            {
                if (m_FileName.empty())
                    SetStatus("Enter a file name first.");
                else
                {
                    std::filesystem::path name(m_FileName);
                    if (name.extension().empty())
                        name += ".tcscene";
                    else if (!IsSceneFile(name))
                        SetStatus("Scene files must use the .tcscene extension.");

                    if (IsSceneFile(name))
                    {
                        const auto target = m_CurrentDirectory / name;
                        std::error_code ec;
                        if (std::filesystem::exists(target, ec))
                        {
                            m_SelectedPath = target;
                            m_ConfirmOverwrite = true;
                        }
                        else
                        {
                            m_PendingResult = { FileBrowserAction::Save, target };
                            m_HasPendingResult = true;
                            m_IsOpen = false;
                        }
                    }
                }
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(90.0f, 0.0f)))
            m_IsOpen = false;

        if (!m_Status.empty())
            ImGui::TextWrapped("%s", m_Status.c_str());

        if (ImGui::BeginPopup("Create Folder"))
        {
            char folderBuffer[256] = {};
            const size_t folderLength = (std::min)(m_NewFolderName.size(), sizeof(folderBuffer) - 1);
            m_NewFolderName.copy(folderBuffer, folderLength);
            if (ImGui::InputText("Folder name", folderBuffer, sizeof(folderBuffer)))
                m_NewFolderName = folderBuffer;
            if (ImGui::Button("Create") && !m_NewFolderName.empty())
            {
                std::error_code ec;
                std::filesystem::create_directory(
                    m_CurrentDirectory / m_NewFolderName, ec);
                SetStatus(ec ? "Could not create folder: " + ec.message()
                             : "Folder created.");
                m_NewFolderName.clear();
                RefreshEntries();
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel"))
                ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }

        if (ImGui::BeginPopup("Rename Item"))
        {
            char renameBuffer[256] = {};
            const size_t renameLength = (std::min)(m_RenameName.size(), sizeof(renameBuffer) - 1);
            m_RenameName.copy(renameBuffer, renameLength);
            if (ImGui::InputText("New name", renameBuffer, sizeof(renameBuffer)))
                m_RenameName = renameBuffer;
            if (ImGui::Button("Apply") && !m_RenameName.empty())
            {
                std::error_code ec;
                std::filesystem::rename(
                    m_SelectedPath,
                    m_SelectedPath.parent_path() / m_RenameName,
                    ec);
                SetStatus(ec ? "Rename failed: " + ec.message() : "Item renamed.");
                m_SelectedPath.clear();
                RefreshEntries();
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel"))
                ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }

        if (m_ConfirmDelete)
            ImGui::OpenPopup("Confirm Delete");
        if (ImGui::BeginPopupModal("Confirm Delete", nullptr,
            ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::TextWrapped("Delete %s?", m_SelectedPath.filename().string().c_str());
            if (ImGui::Button("Delete", ImVec2(100.0f, 0.0f)))
            {
                std::error_code ec;
                std::filesystem::remove(m_SelectedPath, ec);
                SetStatus(ec ? "Delete failed: " + ec.message() : "Item deleted.");
                m_SelectedPath.clear();
                RefreshEntries();
                m_ConfirmDelete = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Keep", ImVec2(100.0f, 0.0f)))
            {
                m_ConfirmDelete = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        if (m_ConfirmOverwrite)
            ImGui::OpenPopup("Confirm Overwrite");
        if (ImGui::BeginPopupModal("Confirm Overwrite", nullptr,
            ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::TextWrapped("This file already exists. Overwrite it?");
            if (ImGui::Button("Overwrite", ImVec2(110.0f, 0.0f)))
            {
                m_PendingResult = { FileBrowserAction::Save, m_SelectedPath };
                m_HasPendingResult = true;
                m_IsOpen = false;
                m_ConfirmOverwrite = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(110.0f, 0.0f)))
            {
                m_ConfirmOverwrite = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        ImGui::End();
    }
}
