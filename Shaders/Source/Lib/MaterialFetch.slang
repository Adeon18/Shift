//! Material fetching logic.
import Lib.FrameData;
import Lib.BufferAccess;

MaterialData LoadMaterial(FrameConstants frame, uint materialIndex)
{
    return LoadAt<MaterialData>(frame.materialBufferRef, materialIndex);
}
