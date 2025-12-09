//
// Created by otrush on 12/1/2025.
//

#include "FileWatcher.hpp"

#include "Utility/Assertions.hpp"

namespace Shift::Util {
    void FileWatcher::Init() {
        m_systemFileWatcher = new efsw::FileWatcher();
        m_systemFileWatcher->watch();
    }

    void FileWatcher::Destroy() {
        delete m_systemFileWatcher;
    }

    FileWatcher::WatchID FileWatcher::AddWatch(const std::string &directory, FileSystemCallback callback, bool isRecursive) {
        WatchID id = m_systemFileWatcher->addWatch(directory, this, isRecursive);

        Check(Error, id != -1, "Failed to add watch!");

        m_callbacks[id].push_back(callback);

        return id;
    }

    void FileWatcher::Poll() {
        std::vector<Event> events;
        {
            std::lock_guard lock{m_eventMutex};
            if (m_fileChangeEvents.empty()) { return;}
            events.swap(m_fileChangeEvents);
        }

        for (const auto& event : events) {
            //! Check to avoid shitty bugs
            if (m_callbacks.contains(event.id)) {
                for (const auto& callback : m_callbacks[event.id]) {
                    callback(event.path.string(), event.action);
                }
            }
        }
    }

    void FileWatcher::handleFileAction(efsw::WatchID watchid, const std::string &dir, const std::string &filename,
        efsw::Action action, std::string oldFilename)
    {
        EFileAction engineAction;
        switch (action) {
            case efsw::Actions::Add:      engineAction = EFileAction::Add; break;
            case efsw::Actions::Modified: engineAction = EFileAction::Modified; break;
            case efsw::Actions::Delete:   engineAction = EFileAction::Delete; break;
            case efsw::Actions::Moved:    engineAction = EFileAction::Moved; break;
            default: return;
        }

        std::lock_guard lock{m_eventMutex};
        m_fileChangeEvents.emplace_back(watchid, std::filesystem::path(dir) / filename, engineAction);
    }
} // Shift::Util