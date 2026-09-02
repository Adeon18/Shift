//! Material fetching logic.
import Lib.FrameData;

MaterialData LoadMaterial(FrameConstants frame, uint materialIndex)
{
    MaterialData material = ((MaterialData*)frame.materialBufferRef)[materialIndex];
    return material;
}
