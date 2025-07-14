# Frame Pacing Concept - Non-Blocking Approach

## Current Problem
The existing implementation uses `std::this_thread::sleep_for()` which blocks the render thread.
Pancake's feedback: "Stalling the render thread is not an option I want to merge."

## Proposed Solutions

### Option 1: Vulkan Timeline Semaphores
```cpp
class FramePacer {
private:
    VkSemaphore timelineSemaphore;
    uint64_t targetFrameTime; // in nanoseconds
    std::chrono::high_resolution_clock::time_point lastPresentTime;
    
public:
    void schedulePresent(VkQueue queue, const VkPresentInfoKHR& presentInfo) {
        auto now = std::chrono::high_resolution_clock::now();
        auto elapsed = now - lastPresentTime;
        
        if (elapsed < targetFrameTime) {
            // Use timeline semaphore to delay presentation
            uint64_t waitValue = calculateWaitValue(targetFrameTime - elapsed);
            // Signal timeline semaphore at the right time
        }
        
        // Present without blocking render thread
        vkQueuePresentKHR(queue, &presentInfo);
        lastPresentTime = now;
    }
};
```

### Option 2: Dedicated Frame Pacing Thread
```cpp
class AsyncFramePacer {
private:
    std::thread pacingThread;
    std::queue<PresentRequest> pendingPresents;
    std::mutex queueMutex;
    std::condition_variable cv;
    std::atomic<bool> running{true};
    
    struct PresentRequest {
        VkQueue queue;
        VkPresentInfoKHR presentInfo;
        std::chrono::high_resolution_clock::time_point targetTime;
    };
    
    void pacingThreadFunction() {
        while (running) {
            std::unique_lock<std::mutex> lock(queueMutex);
            cv.wait(lock, [this] { return !pendingPresents.empty() || !running; });
            
            if (!running) break;
            
            auto request = pendingPresents.front();
            pendingPresents.pop();
            lock.unlock();
            
            // Sleep until target time (on dedicated thread, not render thread)
            std::this_thread::sleep_until(request.targetTime);
            
            // Present frame
            vkQueuePresentKHR(request.queue, &request.presentInfo);
        }
    }
    
public:
    void schedulePresent(VkQueue queue, const VkPresentInfoKHR& presentInfo, 
                        std::chrono::milliseconds delay) {
        auto targetTime = std::chrono::high_resolution_clock::now() + delay;
        
        std::lock_guard<std::mutex> lock(queueMutex);
        pendingPresents.push({queue, presentInfo, targetTime});
        cv.notify_one();
    }
};
```

### Option 3: GPU-Based Frame Timing
```cpp
class GPUFramePacer {
private:
    VkQueryPool timestampPool;
    VkFence frameFence;
    
public:
    void paceFrame(VkCommandBuffer cmdBuf, uint32_t targetFrameTimeNs) {
        // Write timestamp at start of frame
        vkCmdWriteTimestamp(cmdBuf, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 
                           timestampPool, 0);
        
        // Render frame...
        
        // Write timestamp at end of frame
        vkCmdWriteTimestamp(cmdBuf, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 
                           timestampPool, 1);
        
        // Use GPU timing to control next frame submission
        // No CPU blocking required
    }
};
```

## Recommended Approach

**Option 2 (Dedicated Thread)** seems most practical because:
- ✅ No render thread blocking
- ✅ Precise timing control
- ✅ Easy to implement
- ✅ Compatible with existing code
- ✅ Provides introspection capabilities

## Implementation Strategy

1. **Refactor current sleep-based approach**
2. **Add async frame pacing class**
3. **Maintain same environment variable interface**
4. **Add frame timing introspection/logging**
5. **Keep fallback to immediate presentation**

This way we maintain the same user interface (`LSFG_FRAME_PACING_DELAY`) but implement it properly without blocking the render thread.
