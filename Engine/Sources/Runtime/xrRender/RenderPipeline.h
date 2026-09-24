////////////////////////////////////////////////////////////////////////////////
// Created: 18.09.2026
// Author: NSDeathman
// Nocturning studio for NS Platform X
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "IRenderStage.h"
#include <memory>
#include <vector>
////////////////////////////////////////////////////////////////////////////////
class CRenderPipeline
{
public:
	CRenderPipeline();
	~CRenderPipeline();

	void Initialize();
	void Destroy();

	void Execute();

	void AddStage(std::unique_ptr<IRenderStage> stage);
	void InsertStage(size_t index, std::unique_ptr<IRenderStage> stage);
	bool RemoveStage(std::string_view name);
	IRenderStage* FindStage(std::string_view name);

private:
	std::vector<std::unique_ptr<IRenderStage>> m_stages;
};
////////////////////////////////////////////////////////////////////////////////
