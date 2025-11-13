#pragma once

namespace tjs::common::system
{
    class system_holder_delegate {
    public:
        virtual void on_initialization_done() noexcept = 0;
        virtual void on_release_done() noexcept = 0;
    };
}