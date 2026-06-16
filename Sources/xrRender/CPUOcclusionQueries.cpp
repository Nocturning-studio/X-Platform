////////////////////////////////////////////////////////////////////////////////
// Created: 14.06.2026
// Author: NSDeathman
// Nocturning studio for NS Platform X
////////////////////////////////////////////////////////////////////////////////
#include "Stdafx.h"
#include "CPUOcclusion.h"
////////////////////////////////////////////////////////////////////////////////
void CPUOcclusion::BeginOcclusionQueries(const fmat4x4& viewProj, const SoftX::Viewport& viewport)
{
    if (!m_softDevice) return;

    m_currentViewProj = viewProj;
    m_currentViewport = viewport;

    m_activeQuery = m_queryPool[m_currentQueryIndex].get();
    m_currentQueryIndex = (m_currentQueryIndex + 1) % QUERY_POOL_SIZE;

    if (!m_activeQuery->IsReady())
        m_activeQuery->Flush();

    m_activeQuery->SetDepthBuffer(*m_softDepthBuffer[m_readIdx]);
    m_activeQuery->SetViewport(m_currentViewport);
    m_activeQuery->Begin();
}

bool CPUOcclusion::IsQueryReady() const
{
    return m_pendingQuery && m_pendingQuery->IsReady();
}

void CPUOcclusion::ResetPendingQuery()
{
    m_pendingQuery = nullptr;
}

void CPUOcclusion::EndOcclusionQueries()
{
    if (m_activeQuery)
    {
        m_activeQuery->End();
        m_pendingQuery = m_activeQuery;
        m_activeQuery = nullptr;
    }
}

uint32_t CPUOcclusion::GetVisibleSamples(uint32_t queryId) const
{
    if (!m_pendingQuery || !m_pendingQuery->IsReady())
        return 0xfffffffe; // not ready

    uint visible = 0;
    if (m_pendingQuery->GetResult(queryId, &visible))
        return visible;

    return 0;
}
////////////////////////////////////////////////////////////////////////////////
