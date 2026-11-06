//! Generic buffer access over a GPUBufferRef
//! Vulkan: a device address. D3D12 (not written yet): {heap index, byte offset} packed in the
//! same 64 bits, so only these bodies change and every caller stays as it is.

//! GPUBufferRef is a uint_64 so we do not inclucde and import anything

T LoadAt<T>(uint64_t ref, uint index)
{
    return ((T*)ref)[index];
}

void StoreAt<T>(uint64_t ref, uint index, T value)
{
    ((T*)ref)[index] = value;
}

T Load<T>(uint64_t ref)
{
    return *(T*)ref;
}

void Store<T>(uint64_t ref, T value)
{
    *(T*)ref = value;
}

//! Not generic: uint32_t only for now, maybe conditional uint64_t in the future!
void AtomicAddAt(uint64_t ref, uint index, uint value)
{
    InterlockedAdd(((uint*)ref)[index], value);
}
