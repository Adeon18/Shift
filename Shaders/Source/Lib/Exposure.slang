//! The exposure function, here cuz we cannot do it at tonemap as fp16 target compresses lighting data if not preexposed
import Lib.FrameData;
import Lib.BufferAccess;

float3 PreExpose(float3 radiance, FrameConstants frame)
{
    const float exposure = frame.autoExposureEnabled != 0 ? Load<float>(frame.exposureRef) : frame.cameraPosExposure.w;
    return radiance * exposure;
}
