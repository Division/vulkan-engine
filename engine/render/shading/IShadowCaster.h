#pragma once

#include "render/renderer/ICameraParamsProvider.h"

namespace render
{
	class ShadowMap;
}

class IShadowCaster
{
public:
    friend class render::ShadowMap;

    virtual ~IShadowCaster() = default;
protected:
};

