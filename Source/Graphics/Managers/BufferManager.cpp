//
// Created by otrush on 6/29/2026.
//
#include "BufferManager.hpp"

namespace Shift::Graphics {
    void BufferManager::Init(RenderBackend* rhi) {
        m_rhi = rhi;
        m_backend = rhi->CreateInterface();
    }

    BufferHandle BufferManager::CreateBuffer(const BufferDescriptor& desc) {
        Buffer* buffer = m_backend->CreateBuffer(desc);
        return m_pool.Insert(buffer);
    }

    Buffer* BufferManager::Get(BufferHandle handle) const {
        return m_pool.Get(handle);
    }

    void BufferManager::DestroyBuffer(BufferHandle handle) {
        Buffer* retired = m_pool.Release(handle);
        if (!retired) { return; }

        auto payload = m_rhi->GetGraphicsWaitPayload();
        m_rhi->DeferExecute(payload.semaphore, payload.value, [retired]() { delete retired; });
    }

    void BufferManager::Destroy() {
        m_pool.ForEachLive([](uint32_t, Buffer* buffer) { delete buffer; });
        m_pool.Clear();
    }
}
