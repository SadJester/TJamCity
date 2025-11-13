#include <common/stdafx.h>

#include <common/system/system_holder.h>
#include <common/system/system_holder_delegate.h>

#include <common/system/threaded_system.h>


namespace tjs::common::system {

    system_holder::~system_holder() {
        join();
    }

    void system_holder::start(common::system::system_holder_delegate& delegate) {
        auto on_completion = [this, &delegate]() noexcept {
            if (!this->_finalized) {
                delegate.on_initialization_done();
            }
            else {
                delegate.on_release_done();
            }
        };

        _sync_point.emplace(_systems.size(), __barrier_callback{on_completion});
        
        _finalized = false;
        for (auto& sys_pair : _systems) {
            sys_pair.second->start(_sync_point.value());
        }

        if (_systems.empty()) {
            delegate.on_initialization_done();
        }
    }

    void system_holder::finalize() {
        _finalized = true;
        for (auto& sys_pair : _systems) {
            sys_pair.second->finalize();
        }
    }

    void system_holder::join() {
        for (auto& sys_pair : _systems) {
            sys_pair.second->join();
        }

        _systems.clear();
    }

}
