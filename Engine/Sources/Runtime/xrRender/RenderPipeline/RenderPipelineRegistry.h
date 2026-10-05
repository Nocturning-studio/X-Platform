////////////////////////////////////////////////////////////////////////////////
// Created: 04.10.2026 23:01:37
// Author: NS_Deathman
// File: RenderPipelineRegistry.h
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include "IRenderPipeline.h"
#include "RenderPipelineFlow.h"
////////////////////////////////////////////////////////////////////////////////
class CRenderPipelineRegistry
{
  public:
	CRenderPipelineRegistry() = default;
	~CRenderPipelineRegistry();

	CRenderPipelineRegistry(const CRenderPipelineRegistry&) = delete;
	CRenderPipelineRegistry& operator=(const CRenderPipelineRegistry&) = delete;

	template <typename T, typename... Args>
	T* RegisterPipeline(Args&&... args)
	{
		static_assert(std::is_base_of_v<IRenderPipeline, T>, "T must derive from IRenderPipeline");

		auto pipeline = std::make_unique<T>(std::forward<Args>(args)...);
		T* raw = pipeline.get();

		std::string name(raw->GetName());
		m_pipelines.emplace(std::move(name), std::move(pipeline));
		return raw;
	}

	void RegisterFlow(SRenderPipelineFlow flow);
	void RegisterFlow(std::string name, std::vector<std::string> pipelines);

	void InitializeAll();
	void DestroyAll();

	void OnDeviceResetAll();

	bool SetActiveFlow(std::string_view flow_name);
	std::string_view GetActiveFlowName() const { return m_active_flow_name; }

	void ExecuteActiveFlow();

	IRenderPipeline* FindPipeline(std::string_view name);

  private:
	std::unordered_map<std::string, std::unique_ptr<IRenderPipeline>> m_pipelines;
	std::unordered_map<std::string, SRenderPipelineFlow> m_flows;

	std::string m_active_flow_name;
};
////////////////////////////////////////////////////////////////////////////////
