//! Light fetching logic
//! TODO: A small-ass file, should it be a file?
import Lib.FrameData;
import Lib.BufferAccess;

LightData LoadLight(FrameConstants frame, uint lightIndex)
{
    return LoadAt<LightData>(frame.lightBufferRef, lightIndex);
}
