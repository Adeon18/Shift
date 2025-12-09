//
// Created by otrush on 12/1/2025.
//

#ifndef SHIFT_FILEWATCHER_HPP
#define SHIFT_FILEWATCHER_HPP

#include <filesystem>
#include <functional>
#include <mutex>

#include <efsw/efsw.hpp>

namespace Shift::Util {
    class FileWatcher: public efsw::FileWatchListener {
    public:
        enum class EFileAction {
            Undefined,
            Add,
            Delete,
            Modified,
            Moved
        };
        using WatchID = efsw::WatchID;
        using FileSystemCallback = std::function<void(const std::string&, EFileAction)>;

        static FileWatcher& Get() {
            static FileWatcher instance;
            return instance;
        }

        void Init();

        void Destroy();

        WatchID AddWatch(const std::string& directory, FileSystemCallback callback, bool isRecursive = true);
        void Poll();
    private:
        FileWatcher() = default;
        ~FileWatcher() = default;

        void handleFileAction(efsw::WatchID watchid, const std::string& dir,
                              const std::string& filename, efsw::Action action,
                              std::string oldFilename) override;

        //! Event is represented by a watchId, filesystem path and action
        struct Event {
            WatchID id;
            std::filesystem::path path;
            EFileAction action;
        };

    private:
        efsw::FileWatcher* m_systemFileWatcher = nullptr;
        std::unordered_map<WatchID, std::vector<FileSystemCallback>> m_callbacks;
        std::vector<Event> m_fileChangeEvents;

        //! Mutexes are needed as efsw runs the file event management on its own thread and we poll from main thread
        std::mutex m_eventMutex;
    };
} // Shift::Util

#endif //SHIFT_FILEWATCHER_HPP