#pragma once

#include "RenderModeUtils.h"

using namespace Device;

namespace render 
{
	Device::RenderMode GetRenderModeForDepthOnly()
	{
		RenderMode mode;
		mode.SetDepthWriteEnabled(true);
		mode.SetDepthTestEnabled(true);
		mode.SetDepthFunc(CompareOp::Less);
		return mode;
	}

	Device::RenderMode GetRenderModeForOpaque()
	{
		RenderMode mode;
		mode.SetDepthWriteEnabled(false);
		mode.SetDepthTestEnabled(true);
		mode.SetDepthFunc(CompareOp::LessOrEqual);
		return mode;
	}

	Device::RenderMode GetRenderModeForAlphaTest()
	{
		RenderMode mode;
		mode.SetDepthWriteEnabled(false);
		mode.SetDepthTestEnabled(true);
		mode.SetDepthFunc(CompareOp::LessOrEqual);
		return mode;
	}

	Device::RenderMode GetRenderModeForTranslucent()
	{
		RenderMode mode;
		mode.SetDepthWriteEnabled(false);
		mode.SetDepthTestEnabled(true);
		mode.SetDepthFunc(CompareOp::LessOrEqual);
		mode.SetAlphaBlendEnabled(true);
		
		mode.SetSrcBlend(BlendFactor::SrcAlpha);
		mode.SetBlend(BlendOp::Add);
		mode.SetDestBlend(BlendFactor::OneMinusSrcAlpha);

		mode.SetSrcBlendAlpha(BlendFactor::OneMinusDstAlpha);
		mode.SetBlendAlpha(BlendOp::Add);
		mode.SetDestBlendAlpha(BlendFactor::One);

		return mode;
	}

	Device::RenderMode GetRenderModeForAdditive()
	{
		RenderMode mode;
		mode.SetDepthWriteEnabled(false);
		mode.SetDepthTestEnabled(true);
		mode.SetDepthFunc(CompareOp::LessOrEqual);
		mode.SetAlphaBlendEnabled(true);

		mode.SetSrcBlend(BlendFactor::SrcAlpha);
		mode.SetBlend(BlendOp::Add);
		mode.SetDestBlend(BlendFactor::One);

		mode.SetSrcBlendAlpha(BlendFactor::One);
		mode.SetBlendAlpha(BlendOp::Add);
		mode.SetDestBlendAlpha(BlendFactor::One);

		return mode;
	}

	Device::RenderMode GetRenderModeForDebug()
	{
		RenderMode mode;
		mode.SetDepthWriteEnabled(false);
		mode.SetDepthTestEnabled(false);
		return mode;
	}

	Device::RenderMode GetRenderModeForUI()
	{
		RenderMode mode;
		mode.SetDepthTestEnabled(false);
		mode.SetDepthWriteEnabled(false);
		mode.SetPrimitiveTopology(PrimitiveTopology::TriangleList);
		mode.SetPolygonMode(PolygonMode::Fill);
		mode.SetCullMode(CullMode::None);

		mode.SetAlphaBlendEnabled(true);
		mode.SetSrcBlend(BlendFactor::SrcAlpha);
		mode.SetBlend(BlendOp::Add);
		mode.SetDestBlend(BlendFactor::OneMinusSrcAlpha);

		mode.SetSrcBlendAlpha(BlendFactor::OneMinusSrcAlpha);
		mode.SetBlendAlpha(BlendOp::Add);
		mode.SetDestBlendAlpha(BlendFactor::Zero);
		return mode;
	}

	const std::array<RenderMode, (size_t)RenderQueue::Count> RENDER_MODES = {
		GetRenderModeForDepthOnly(),
		GetRenderModeForOpaque(),
		GetRenderModeForAlphaTest(),
		GetRenderModeForTranslucent(),
		GetRenderModeForAdditive(),
		GetRenderModeForDebug(),
		GetRenderModeForUI()
	};

	Device::RenderMode GetRenderModeForQueue(RenderQueue queue)
	{
		return RENDER_MODES[(size_t)queue];
	}

	const std::unordered_map<std::string, RenderQueue> RENDER_QUEUE_NAME_MAP = {
		{"depthonly", RenderQueue::DepthOnly},
		{"opaque", RenderQueue::Opaque},
		{"alphatest", RenderQueue::AlphaTest},
		{"translucent", RenderQueue::Translucent},
		{"additive", RenderQueue::Additive},
		{"debug", RenderQueue::Debug},
		{"ui", RenderQueue::UI}
	};
}