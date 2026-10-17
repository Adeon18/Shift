//! The exposure function, here cuz we cannot do it at tonemap as fp16 target compresses lighting data if not preexposed
import Lib.FrameData;

float3 PreExpose(float3 radiance, FrameConstants frame)
{
    return radiance * frame.cameraPosExposure.w;
}
