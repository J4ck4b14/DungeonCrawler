#pragma once

namespace TimedInput {

void Flush();
int WaitForKey(int timeoutMs);
bool IsRealtimeSupported();

} // namespace TimedInput
