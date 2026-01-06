#include "stdafx.h"

#include <common/sync/commands_queue.h>

using namespace tjs::common;

TEST(CommandsQueueTests, Simple)
{
    using namespace sync::_testing;

    system_holder systems;
    auto& t_sys = systems.create<test_system>();
    auto& commands = t_sys.commands();
    commands.add_command(open_map{.world_data = 1});

    open_map x;
    commands.add_command(x);
    commands.add_command(std::move(x));

    t_sys.update();
}