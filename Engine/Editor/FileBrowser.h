#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace TC
{
    enum class FileBrowserMode
    {
        Open,
        SaveAs
    };

    enum class FileBrowserAction
    {
        None,
        Open,
        Save
    };

    struct FileBrowserResult
    {
        FileBrowserAction action = FileBrowserAction::None;
        std::filesystem::path path;
    };

    class FileBrowser
    {
    public:
        void Open(FileBrowserMode mode, const std::filesystem::path& startPath);
        bool IsOpen() const { return m_IsOpen; }
        void Render();
        bool ConsumeResult(FileBrowserResult& result);

    private:
        void RefreshEntries();
        void NavigateTo(const std::filesystem::path& path);
        bool IsSceneFile(const std::filesystem::path& path) const;
        void SetStatus(const std::string& message);

        bool m_IsOpen = false;
        FileBrowserMode m_Mode = FileBrowserMode::Open;
        std::filesystem::path m_CurrentDirectory;
        std::vector<std::filesystem::directory_entry> m_Entries;
        std::filesystem::path m_SelectedPath;
        std::string m_FileName;
        std::string m_NewFolderName;
        std::string m_RenameName;
        std::string m_Status;
        bool m_HasPendingResult = false;
        FileBrowserResult m_PendingResult;
        bool m_ConfirmDelete = false;
        bool m_ConfirmOverwrite = false;
    };
}
