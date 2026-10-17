//! Light fetching logic
//! TODO: A small-ass file, should it be a file?
import Lib.FrameData;

LightData LoadLight(FrameConstants frame, uint lightIndex)
{
    LightData light = ((LightData*)frame.lightBufferRef)[lightIndex];
    return light;
}
