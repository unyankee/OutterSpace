#pragma once

#include "Pass.h"
#include <volk.h>

namespace ToyEngine
{
    class PassExecutor
    {
    public:
        static void execute(VkCommandBuffer cmd, const RenderBatch& batch, PassContext& ctx);
    };
}
