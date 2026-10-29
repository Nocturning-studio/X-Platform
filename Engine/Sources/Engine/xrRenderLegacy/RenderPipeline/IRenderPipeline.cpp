////////////////////////////////////////////////////////////////////////////////
//  X-Platform — расширение функциональности X-Ray Engine
//
//  Автор:        NSDeathman
//  Студия:       Nocturning Studio
//
//  SPDX-FileCopyrightText: 2026 Nocturning Studio и участники X-Platform
//  SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
//
//  Этот файл распространяется на условиях PolyForm Noncommercial 1.0.0.
//  Копирование, изменение и распространение разрешены для некоммерческих
//  целей. Публичное указание авторства в вашем проекте, документации или
//  рекламе не требуется — оно приветствуется, но не является условием
//  использования.
//
//  При распространении копий или производных работ лицензия обязывает
//  сохранять настоящие уведомления и передавать получателю текст лицензии
//  или ссылку на него. Пожалуйста, не удаляйте эту плашку — это помогает
//  сохранять историю проекта и корректно соблюдать лицензию.
//
//  Коммерческое использование без отдельного разрешения правообладателей
//  запрещено.
//
//  Полный текст лицензии:
//      https://polyformproject.org/licenses/noncommercial/1.0.0
//
//  Этот файл является source-available, а не open source.
////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "IRenderPipeline.h"
////////////////////////////////////////////////////////////////////////////////
void IRenderPipeline::Execute()
{
    for (auto& stage : m_stages)
    {
        if (!stage->IsEnabled())
            continue;
        stage->Execute();
    }
}

void IRenderPipeline::AddStage(std::unique_ptr<IRenderStage> stage)
{
    stage->OnAttach();
    m_stages.push_back(std::move(stage));
}

void IRenderPipeline::InsertStage(size_t index, std::unique_ptr<IRenderStage> stage)
{
    stage->OnAttach();
    m_stages.insert(m_stages.begin() + index, std::move(stage));
}

bool IRenderPipeline::RemoveStage(std::string_view name)
{
    auto it = std::find_if(m_stages.begin(), m_stages.end(), [name](const auto& s) { return s->GetName() == name; });
    if (it == m_stages.end())
        return false;
    (*it)->OnDetach();
    m_stages.erase(it);
    return true;
}

IRenderStage* IRenderPipeline::FindStage(std::string_view name) const
{
    auto it = std::find_if(m_stages.begin(), m_stages.end(), [name](const auto& s) { return s->GetName() == name; });
    return it != m_stages.end() ? it->get() : nullptr;
}
////////////////////////////////////////////////////////////////////////////////
