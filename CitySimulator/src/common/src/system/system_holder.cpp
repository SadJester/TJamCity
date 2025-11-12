#include <common/stdafx.h>

#include <common/system/system_holder.h>
#include <common/system/threaded_system.h>


namespace tjs::common::system {

    system_holder::~system_holder() {
        join();
    }

    void system_holder::start() {
        _sync_point.emplace(_systems.size());

        for (auto& sys_pair : _systems) {
            sys_pair.second->start(_sync_point.value());
        }
    }

    void system_holder::finalize() {
        for (auto& sys_pair : _systems) {
            sys_pair.second->finalize();
        }
    }

    void system_holder::join() {
        for (auto& sys_pair : _systems) {
            sys_pair.second.reset();
        }

        _systems.clear();
    }

}
