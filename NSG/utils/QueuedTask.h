/*
-------------------------------------------------------------------------------
This file is part of nsg-library.
http://github.com/woodjazz/nsg-library

Copyright (c) 2014-2017 N�stor Silveira Gorski

-------------------------------------------------------------------------------
This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
claim that you wrote the original software. If you use this software
in a product, an acknowledgment in the product documentation would be
appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
-------------------------------------------------------------------------------
*/
#pragma once
#include "NonCopyable.h"
#include "Task.h"
#include "Worker.h"
#include <condition_variable>
#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>

namespace NSG {
namespace Task {
class QueuedTask : Worker, NonCopyable {
public:
    QueuedTask(const std::string& name);
    ~QueuedTask();
    int AddTask(PTask pTask);
    bool CancelTask(int id);
    void CancelAllTasks();

private:
    void RunWorker() override;
    bool IsEmpty() const;
    void InternalTask();
    struct Data {
        int id_;
        PTask pTask_;
        bool canceled_;

        Data(int id, PTask pTask);
    };
    typedef std::shared_ptr<Data> PData;
    PData Pop();
    typedef std::deque<PData> QUEUE;
    typedef std::map<int, PData> MAP_ID_DATA;
    typedef std::mutex Mutex;
    typedef std::condition_variable Condition;
    typedef std::thread Thread;

    bool taskAlive_;
    mutable Mutex mtx_;
    QUEUE queue_;
    MAP_ID_DATA keyDataMap_;
    Condition condition_;
    Thread thread_;
};
}
}
