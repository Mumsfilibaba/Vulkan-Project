#include "ImGuiRenderer.h"
#include "Vulkan/ShaderModule.h"
#include "Vulkan/Sampler.h"
#include "Vulkan/PipelineLayout.h"
#include "Vulkan/DescriptorSetLayout.h"
#include "Vulkan/DescriptorPool.h"
#include "Vulkan/ShaderModule.h"
#include "Vulkan/PipelineState.h"
#include "Vulkan/RenderPass.h"
#include "Vulkan/VulkanContext.h"
#include "Vulkan/SwapChain.h"
#include "Vulkan/Texture.h"
#include "Vulkan/TextureView.h"
#include "Vulkan/CommandBuffer.h"
#include <GLFW/glfw3.h>

#if PLATFORM_WINDOWS
    #define GLFW_EXPOSE_NATIVE_WIN32
#elif PLATFORM_MAC
    #define GLFW_EXPOSE_NATIVE_COCOA
#endif
#include <GLFW/glfw3native.h>

namespace ImGuiRenderer
{
    struct ImGuiBackendData
    {
        ImGuiBackendData()
            : Window(nullptr)
            , Time(0.0)
            , MouseWindow(nullptr)
            , LastValidMousePos{ 0, 0 }
            , bInstalledCallbacks(false)
            , bWantUpdateMonitors(true)
        {
            memset(MouseCursors, 0, sizeof(MouseCursors));
            memset(KeyOwnerWindows, 0, sizeof(KeyOwnerWindows));
        }
        
        GLFWwindow* Window;
        double      Time;
        GLFWwindow* MouseWindow;
        GLFWcursor* MouseCursors[ImGuiMouseCursor_COUNT];
        ImVec2      LastValidMousePos;
        GLFWwindow* KeyOwnerWindows[GLFW_KEY_LAST];
        bool        bInstalledCallbacks;
        bool        bWantUpdateMonitors;
        
        // Chain GLFW callbacks: our callbacks will call the user's previously installed callbacks, if any.
        GLFWwindowfocusfun PrevUserCallbackWindowFocus;
        GLFWcursorposfun   PrevUserCallbackCursorPos;
        GLFWcursorenterfun PrevUserCallbackCursorEnter;
        GLFWmousebuttonfun PrevUserCallbackMousebutton;
        GLFWscrollfun      PrevUserCallbackScroll;
        GLFWkeyfun         PrevUserCallbackKey;
        GLFWcharfun        PrevUserCallbackChar;
        GLFWmonitorfun     PrevUserCallbackMonitor;
        
    };
    
    struct ImGuiRendererBackendData
    {
        ImGuiRendererBackendData()
            : pContext(nullptr)
            , pRenderPass(nullptr)
            , pDescriptorPool(nullptr)
            , pDescriptorSetLayout(nullptr)
            , pPipelineLayout(nullptr)
            , pPipeline(nullptr)
            , pShaderModuleVert(nullptr)
            , pShaderModuleFrag(nullptr)
            , pFontSampler(nullptr)
            , pFontTexture(nullptr)
            , pFontTextureView(nullptr)
            , pFontDescriptorSet(nullptr)
        {
        }
        
        ~ImGuiRendererBackendData()
        {
            pContext = nullptr;
            
            SAFE_DELETE(pRenderPass);
            SAFE_DELETE(pDescriptorPool);
            SAFE_DELETE(pDescriptorSetLayout);
            SAFE_DELETE(pPipelineLayout);
            SAFE_DELETE(pPipeline);
            SAFE_DELETE(pShaderModuleVert);
            SAFE_DELETE(pShaderModuleFrag);
            
            SAFE_DELETE(pFontSampler);
            SAFE_DELETE(pFontTexture);
            SAFE_DELETE(pFontTextureView);
            SAFE_DELETE(pFontDescriptorSet);
        }
        
        // Global objects
        VulkanContext*       pContext;
        RenderPass*          pRenderPass;
        DescriptorPool*      pDescriptorPool;
        DescriptorSetLayout* pDescriptorSetLayout;
        PipelineLayout*      pPipelineLayout;
        GraphicsPipeline*    pPipeline;
        ShaderModule*        pShaderModuleVert;
        ShaderModule*        pShaderModuleFrag;
        
        // Font data
        Sampler*             pFontSampler;
        Texture*             pFontTexture;
        TextureView*         pFontTextureView;
        DescriptorSet*       pFontDescriptorSet;
        
        VkImageView          FontView;
    };
    
    struct ImGuiFrameRenderData
    {
        ImGuiFrameRenderData()
            : pVertexBuffer(nullptr)
            , pIndexBuffer(nullptr)
            , pCommandBuffer(nullptr)
        {
        }
        
        ~ImGuiFrameRenderData()
        {
            SAFE_DELETE(pVertexBuffer);
            SAFE_DELETE(pIndexBuffer);
            SAFE_DELETE(pCommandBuffer);
        }
        
        Buffer*        pVertexBuffer;
        Buffer*        pIndexBuffer;
        CommandBuffer* pCommandBuffer;
    };
    
    struct ImGuiViewportData
    {
        ImGuiViewportData()
            : pWindow(nullptr)
            , pSwapChain(nullptr)
            , pRenderPass(nullptr)
            , bClearEnable(false)
            , bWindowOwned(false)
            , IgnoreWindowPosEventFrame(0)
            , IgnoreWindowSizeEventFrame(0)
        {
        }
        
        ~ImGuiViewportData()
        {
            pWindow = nullptr;
        }
        
        GLFWwindow*                       pWindow;
        SwapChain*                        pSwapChain;
        RenderPass*                       pRenderPass;
        std::vector<Framebuffer*>         FrameBuffers;
        std::vector<ImGuiFrameRenderData> FrameData;
        VkClearValue                      ClearValues;
        
        bool bClearEnable;
        bool bWindowOwned;
        int  IgnoreWindowSizeEventFrame;
        int  IgnoreWindowPosEventFrame;
    };
    
    static ImGuiBackendData* ImGuiGetBackendData()
    {
        return ImGui::GetCurrentContext() ? reinterpret_cast<ImGuiBackendData*>(ImGui::GetIO().BackendPlatformUserData) : nullptr;
    }
    
    static ImGuiRendererBackendData* ImGuiGetRendererBackendData()
    {
        return ImGui::GetCurrentContext() ? reinterpret_cast<ImGuiRendererBackendData*>(ImGui::GetIO().BackendRendererUserData) : nullptr;
    }
    
    static ImGuiKey ImGuiKeyToImGuiKey(int key)
    {
        switch (key)
        {
            case GLFW_KEY_TAB:           return ImGuiKey_Tab;
            case GLFW_KEY_LEFT:          return ImGuiKey_LeftArrow;
            case GLFW_KEY_RIGHT:         return ImGuiKey_RightArrow;
            case GLFW_KEY_UP:            return ImGuiKey_UpArrow;
            case GLFW_KEY_DOWN:          return ImGuiKey_DownArrow;
            case GLFW_KEY_PAGE_UP:       return ImGuiKey_PageUp;
            case GLFW_KEY_PAGE_DOWN:     return ImGuiKey_PageDown;
            case GLFW_KEY_HOME:          return ImGuiKey_Home;
            case GLFW_KEY_END:           return ImGuiKey_End;
            case GLFW_KEY_INSERT:        return ImGuiKey_Insert;
            case GLFW_KEY_DELETE:        return ImGuiKey_Delete;
            case GLFW_KEY_BACKSPACE:     return ImGuiKey_Backspace;
            case GLFW_KEY_SPACE:         return ImGuiKey_Space;
            case GLFW_KEY_ENTER:         return ImGuiKey_Enter;
            case GLFW_KEY_ESCAPE:        return ImGuiKey_Escape;
            case GLFW_KEY_APOSTROPHE:    return ImGuiKey_Apostrophe;
            case GLFW_KEY_COMMA:         return ImGuiKey_Comma;
            case GLFW_KEY_MINUS:         return ImGuiKey_Minus;
            case GLFW_KEY_PERIOD:        return ImGuiKey_Period;
            case GLFW_KEY_SLASH:         return ImGuiKey_Slash;
            case GLFW_KEY_SEMICOLON:     return ImGuiKey_Semicolon;
            case GLFW_KEY_EQUAL:         return ImGuiKey_Equal;
            case GLFW_KEY_LEFT_BRACKET:  return ImGuiKey_LeftBracket;
            case GLFW_KEY_BACKSLASH:     return ImGuiKey_Backslash;
            case GLFW_KEY_RIGHT_BRACKET: return ImGuiKey_RightBracket;
            case GLFW_KEY_GRAVE_ACCENT:  return ImGuiKey_GraveAccent;
            case GLFW_KEY_CAPS_LOCK:     return ImGuiKey_CapsLock;
            case GLFW_KEY_SCROLL_LOCK:   return ImGuiKey_ScrollLock;
            case GLFW_KEY_NUM_LOCK:      return ImGuiKey_NumLock;
            case GLFW_KEY_PRINT_SCREEN:  return ImGuiKey_PrintScreen;
            case GLFW_KEY_PAUSE:         return ImGuiKey_Pause;
            case GLFW_KEY_KP_0:          return ImGuiKey_Keypad0;
            case GLFW_KEY_KP_1:          return ImGuiKey_Keypad1;
            case GLFW_KEY_KP_2:          return ImGuiKey_Keypad2;
            case GLFW_KEY_KP_3:          return ImGuiKey_Keypad3;
            case GLFW_KEY_KP_4:          return ImGuiKey_Keypad4;
            case GLFW_KEY_KP_5:          return ImGuiKey_Keypad5;
            case GLFW_KEY_KP_6:          return ImGuiKey_Keypad6;
            case GLFW_KEY_KP_7:          return ImGuiKey_Keypad7;
            case GLFW_KEY_KP_8:          return ImGuiKey_Keypad8;
            case GLFW_KEY_KP_9:          return ImGuiKey_Keypad9;
            case GLFW_KEY_KP_DECIMAL:    return ImGuiKey_KeypadDecimal;
            case GLFW_KEY_KP_DIVIDE:     return ImGuiKey_KeypadDivide;
            case GLFW_KEY_KP_MULTIPLY:   return ImGuiKey_KeypadMultiply;
            case GLFW_KEY_KP_SUBTRACT:   return ImGuiKey_KeypadSubtract;
            case GLFW_KEY_KP_ADD:        return ImGuiKey_KeypadAdd;
            case GLFW_KEY_KP_ENTER:      return ImGuiKey_KeypadEnter;
            case GLFW_KEY_KP_EQUAL:      return ImGuiKey_KeypadEqual;
            case GLFW_KEY_LEFT_SHIFT:    return ImGuiKey_LeftShift;
            case GLFW_KEY_LEFT_CONTROL:  return ImGuiKey_LeftCtrl;
            case GLFW_KEY_LEFT_ALT:      return ImGuiKey_LeftAlt;
            case GLFW_KEY_LEFT_SUPER:    return ImGuiKey_LeftSuper;
            case GLFW_KEY_RIGHT_SHIFT:   return ImGuiKey_RightShift;
            case GLFW_KEY_RIGHT_CONTROL: return ImGuiKey_RightCtrl;
            case GLFW_KEY_RIGHT_ALT:     return ImGuiKey_RightAlt;
            case GLFW_KEY_RIGHT_SUPER:   return ImGuiKey_RightSuper;
            case GLFW_KEY_MENU:          return ImGuiKey_Menu;
            case GLFW_KEY_0:             return ImGuiKey_0;
            case GLFW_KEY_1:             return ImGuiKey_1;
            case GLFW_KEY_2:             return ImGuiKey_2;
            case GLFW_KEY_3:             return ImGuiKey_3;
            case GLFW_KEY_4:             return ImGuiKey_4;
            case GLFW_KEY_5:             return ImGuiKey_5;
            case GLFW_KEY_6:             return ImGuiKey_6;
            case GLFW_KEY_7:             return ImGuiKey_7;
            case GLFW_KEY_8:             return ImGuiKey_8;
            case GLFW_KEY_9:             return ImGuiKey_9;
            case GLFW_KEY_A:             return ImGuiKey_A;
            case GLFW_KEY_B:             return ImGuiKey_B;
            case GLFW_KEY_C:             return ImGuiKey_C;
            case GLFW_KEY_D:             return ImGuiKey_D;
            case GLFW_KEY_E:             return ImGuiKey_E;
            case GLFW_KEY_F:             return ImGuiKey_F;
            case GLFW_KEY_G:             return ImGuiKey_G;
            case GLFW_KEY_H:             return ImGuiKey_H;
            case GLFW_KEY_I:             return ImGuiKey_I;
            case GLFW_KEY_J:             return ImGuiKey_J;
            case GLFW_KEY_K:             return ImGuiKey_K;
            case GLFW_KEY_L:             return ImGuiKey_L;
            case GLFW_KEY_M:             return ImGuiKey_M;
            case GLFW_KEY_N:             return ImGuiKey_N;
            case GLFW_KEY_O:             return ImGuiKey_O;
            case GLFW_KEY_P:             return ImGuiKey_P;
            case GLFW_KEY_Q:             return ImGuiKey_Q;
            case GLFW_KEY_R:             return ImGuiKey_R;
            case GLFW_KEY_S:             return ImGuiKey_S;
            case GLFW_KEY_T:             return ImGuiKey_T;
            case GLFW_KEY_U:             return ImGuiKey_U;
            case GLFW_KEY_V:             return ImGuiKey_V;
            case GLFW_KEY_W:             return ImGuiKey_W;
            case GLFW_KEY_X:             return ImGuiKey_X;
            case GLFW_KEY_Y:             return ImGuiKey_Y;
            case GLFW_KEY_Z:             return ImGuiKey_Z;
            case GLFW_KEY_F1:            return ImGuiKey_F1;
            case GLFW_KEY_F2:            return ImGuiKey_F2;
            case GLFW_KEY_F3:            return ImGuiKey_F3;
            case GLFW_KEY_F4:            return ImGuiKey_F4;
            case GLFW_KEY_F5:            return ImGuiKey_F5;
            case GLFW_KEY_F6:            return ImGuiKey_F6;
            case GLFW_KEY_F7:            return ImGuiKey_F7;
            case GLFW_KEY_F8:            return ImGuiKey_F8;
            case GLFW_KEY_F9:            return ImGuiKey_F9;
            case GLFW_KEY_F10:           return ImGuiKey_F10;
            case GLFW_KEY_F11:           return ImGuiKey_F11;
            case GLFW_KEY_F12:           return ImGuiKey_F12;
            default:                     return ImGuiKey_None;
        }
    }
    
    static const char* ImGuiGetClipboardText(void* userData)
    {
        return glfwGetClipboardString((GLFWwindow*)userData);
    }
    
    static void ImGuiSetClipboardText(void* userData, const char* text)
    {
        glfwSetClipboardString((GLFWwindow*)userData, text);
    }
    
    static void ImGuiUpdateKeyModifiers()
    {
        ImGuiBackendData* pBackend = ImGuiGetBackendData();
        
        ImGuiIO& io = ImGui::GetIO();
        io.AddKeyEvent(ImGuiMod_Ctrl,  (glfwGetKey(pBackend->Window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) || (glfwGetKey(pBackend->Window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS));
        io.AddKeyEvent(ImGuiMod_Shift, (glfwGetKey(pBackend->Window, GLFW_KEY_LEFT_SHIFT)   == GLFW_PRESS) || (glfwGetKey(pBackend->Window, GLFW_KEY_RIGHT_SHIFT)   == GLFW_PRESS));
        io.AddKeyEvent(ImGuiMod_Alt,   (glfwGetKey(pBackend->Window, GLFW_KEY_LEFT_ALT)     == GLFW_PRESS) || (glfwGetKey(pBackend->Window, GLFW_KEY_RIGHT_ALT)     == GLFW_PRESS));
        io.AddKeyEvent(ImGuiMod_Super, (glfwGetKey(pBackend->Window, GLFW_KEY_LEFT_SUPER)   == GLFW_PRESS) || (glfwGetKey(pBackend->Window, GLFW_KEY_RIGHT_SUPER)   == GLFW_PRESS));
    }
    
    void ImGuiMouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
    {
        ImGuiBackendData* pBackend = ImGuiGetBackendData();
        if (pBackend->PrevUserCallbackMousebutton != nullptr && window == pBackend->Window)
        {
            pBackend->PrevUserCallbackMousebutton(window, button, action, mods);
        }
        
        ImGuiUpdateKeyModifiers();
        
        ImGuiIO& io = ImGui::GetIO();
        if (button >= 0 && button < ImGuiMouseButton_COUNT)
        {
            io.AddMouseButtonEvent(button, action == GLFW_PRESS);
        }
    }
    
    void ImGuiScrollCallback(GLFWwindow* window, double xOffset, double yOffset)
    {
        ImGuiBackendData* pBackend = ImGuiGetBackendData();
        if (pBackend->PrevUserCallbackScroll != nullptr && window == pBackend->Window)
        {
            pBackend->PrevUserCallbackScroll(window, xOffset, yOffset);
        }
        
        ImGuiIO& io = ImGui::GetIO();
        io.AddMouseWheelEvent((float)xOffset, (float)yOffset);
    }
    
    static int ImGuiTranslateUntranslatedKey(int key, int scancode)
    {
        // GLFW 3.1+ attempts to "untranslate" keys, which goes the opposite of what every other framework does, making using lettered shortcuts difficult.
        // (It had reasons to do so: namely GLFW is/was more likely to be used for WASD-type game controls rather than lettered shortcuts, but IHMO the 3.1 change could have been done differently)
        // See https://github.com/glfw/glfw/issues/1502 for details.
        // Adding a workaround to undo this (so our keys are translated->untranslated->translated, likely a lossy process).
        // This won't cover edge cases but this is at least going to cover common cases.
        if (key >= GLFW_KEY_KP_0 && key <= GLFW_KEY_KP_EQUAL)
        {
            return key;
        }
        
        GLFWerrorfun prevErrorCallback = glfwSetErrorCallback(nullptr);
        const char* keyName = glfwGetKeyName(key, scancode);
        glfwSetErrorCallback(prevErrorCallback);
        
        // Eat errors
        (void)glfwGetError(0);
        
        if (keyName && keyName[0] != 0 && keyName[1] == 0)
        {
            const int charKeys[] =
            {
                GLFW_KEY_GRAVE_ACCENT,
                GLFW_KEY_MINUS,
                GLFW_KEY_EQUAL,
                GLFW_KEY_LEFT_BRACKET,
                GLFW_KEY_RIGHT_BRACKET,
                GLFW_KEY_BACKSLASH,
                GLFW_KEY_COMMA,
                GLFW_KEY_SEMICOLON,
                GLFW_KEY_APOSTROPHE,
                GLFW_KEY_PERIOD,
                GLFW_KEY_SLASH,
                0
            };
            
            const char charNames[] = "`-=[]\\,;\'./";
            assert(IM_ARRAYSIZE(charNames) == IM_ARRAYSIZE(charKeys));
            
            if (keyName[0] >= '0' && keyName[0] <= '9')
            {
                key = GLFW_KEY_0 + (keyName[0] - '0');
            }
            else if (keyName[0] >= 'A' && keyName[0] <= 'Z')
            {
                key = GLFW_KEY_A + (keyName[0] - 'A');
            }
            else if (keyName[0] >= 'a' && keyName[0] <= 'z')
            {
                key = GLFW_KEY_A + (keyName[0] - 'a');
            }
            else if (const char* p = strchr(charNames, keyName[0]))
            {
                key = charKeys[p - charNames];
            }
        }
        
        return key;
    }
    
    static void ImGuiKeyCallback(GLFWwindow* window, int keycode, int scancode, int action, int mods)
    {
        ImGuiBackendData* pBackend = ImGuiGetBackendData();
        if (pBackend->PrevUserCallbackKey != nullptr && window == pBackend->Window)
        {
            pBackend->PrevUserCallbackKey(window, keycode, scancode, action, mods);
        }
        
        if (action != GLFW_PRESS && action != GLFW_RELEASE)
        {
            return;
        }
        
        ImGuiUpdateKeyModifiers();
        
        if (keycode >= 0 && keycode < IM_ARRAYSIZE(pBackend->KeyOwnerWindows))
        {
            pBackend->KeyOwnerWindows[keycode] = (action == GLFW_PRESS) ? window : nullptr;
        }
        
        keycode = ImGuiTranslateUntranslatedKey(keycode, scancode);
        
        ImGuiIO& io = ImGui::GetIO();
        ImGuiKey imguiKey = ImGuiKeyToImGuiKey(keycode);
        io.AddKeyEvent(imguiKey, (action == GLFW_PRESS));
        io.SetKeyEventNativeData(imguiKey, keycode, scancode); // To support legacy indexing (<1.87 user code)
    }
    
    static void ImGuiWindowFocusCallback(GLFWwindow* window, int focused)
    {
        ImGuiBackendData* pBackend = ImGuiGetBackendData();
        if (pBackend->PrevUserCallbackWindowFocus != nullptr && window == pBackend->Window)
        {
            pBackend->PrevUserCallbackWindowFocus(window, focused);
        }
        
        ImGuiIO& io = ImGui::GetIO();
        io.AddFocusEvent(focused != 0);
    }
    
    static void ImGuiCursorPosCallback(GLFWwindow* window, double posX, double posY)
    {
        ImGuiBackendData* pBackend = ImGuiGetBackendData();
        if (pBackend->PrevUserCallbackCursorPos != nullptr && window == pBackend->Window)
        {
            pBackend->PrevUserCallbackCursorPos(window, posX, posY);
        }
        
        if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
        {
            return;
        }
        
        ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            int windowPosX;
            int windowPosY;
            glfwGetWindowPos(window, &windowPosX, &windowPosY);
            
            posX += windowPosX;
            posY += windowPosY;
        }
        
        io.AddMousePosEvent((float)posX, (float)posY);
        pBackend->LastValidMousePos = ImVec2((float)posX, (float)posY);
    }
    
    // Workaround: X11 seems to send spurious Leave/Enter events which would make us lose our position,
    // so we back it up and restore on Leave/Enter (see https://github.com/ocornut/imgui/issues/4984)
    static void ImGuiCursorEnterCallback(GLFWwindow* window, int entered)
    {
        ImGuiBackendData* pBackend = ImGuiGetBackendData();
        if (pBackend->PrevUserCallbackCursorEnter != nullptr && window == pBackend->Window)
        {
            pBackend->PrevUserCallbackCursorEnter(window, entered);
        }
        
        if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
        {
            return;
        }
        
        ImGuiIO& io = ImGui::GetIO();
        if (entered)
        {
            pBackend->MouseWindow = window;
            io.AddMousePosEvent(pBackend->LastValidMousePos.x, pBackend->LastValidMousePos.y);
        }
        else if (!entered && pBackend->MouseWindow == window)
        {
            pBackend->LastValidMousePos = io.MousePos;
            pBackend->MouseWindow       = nullptr;
            io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);
        }
    }
    
    static void ImGuiCharCallback(GLFWwindow* window, unsigned int c)
    {
        ImGuiBackendData* pBackend = ImGuiGetBackendData();
        if (pBackend->PrevUserCallbackChar != nullptr && window == pBackend->Window)
        {
            pBackend->PrevUserCallbackChar(window, c);
        }
        
        ImGuiIO& io = ImGui::GetIO();
        io.AddInputCharacter(c);
    }
    
    static void ImGuiWindowCloseCallback(GLFWwindow* window)
    {
        if (ImGuiViewport* pViewport = ImGui::FindViewportByPlatformHandle(window))
        {
            pViewport->PlatformRequestClose = true;
        }
    }
    
    // GLFW may dispatch window pos/size events after calling glfwSetWindowPos()/glfwSetWindowSize().
    // However: depending on the platform the callback may be invoked at different time:
    // - on Windows it appears to be called within the glfwSetWindowPos()/glfwSetWindowSize() call
    // - on Linux it is queued and invoked during glfwPollEvents()
    // Because the event doesn't always fire on glfwSetWindowXXX() we use a frame counter tag to only
    // ignore recent glfwSetWindowXXX() calls.
    static void ImGuiWindowPosCallback(GLFWwindow* window, int, int)
    {
        if (ImGuiViewport* pViewport = ImGui::FindViewportByPlatformHandle(window))
        {
            if (ImGuiViewportData* pViewportData = (ImGuiViewportData*)pViewport->PlatformUserData)
            {
                bool bIgnoreEvent = (ImGui::GetFrameCount() <= pViewportData->IgnoreWindowPosEventFrame + 1);
                if (bIgnoreEvent)
                {
                    return;
                }
            }
            
            pViewport->PlatformRequestMove = true;
        }
    }
    
    static void ImGuiWindowSizeCallback(GLFWwindow* window, int, int)
    {
        if (ImGuiViewport* pViewport = ImGui::FindViewportByPlatformHandle(window))
        {
            if (ImGuiViewportData* pViewportData = (ImGuiViewportData*)pViewport->PlatformUserData)
            {
                bool bIgnoreEvent = (ImGui::GetFrameCount() <= pViewportData->IgnoreWindowSizeEventFrame + 1);
                if (bIgnoreEvent)
                {
                    return;
                }
            }
            
            pViewport->PlatformRequestResize = true;
        }
    }
    
    static void ImGuiMonitorCallback(GLFWmonitor*, int)
    {
        ImGuiBackendData* pBackend = ImGuiGetBackendData();
        pBackend->bWantUpdateMonitors = true;
    }
    
    static void ImguiInstallCallbacks(GLFWwindow* pWindow)
    {
        ImGuiBackendData* pBackend = ImGuiGetBackendData();
        assert(pBackend->bInstalledCallbacks == false && "Callbacks already installed!");
        assert(pBackend->Window == pWindow);
        
        pBackend->PrevUserCallbackWindowFocus = glfwSetWindowFocusCallback(pWindow, ImGuiWindowFocusCallback);
        pBackend->PrevUserCallbackCursorEnter = glfwSetCursorEnterCallback(pWindow, ImGuiCursorEnterCallback);
        pBackend->PrevUserCallbackCursorPos   = glfwSetCursorPosCallback(pWindow, ImGuiCursorPosCallback);
        pBackend->PrevUserCallbackMousebutton = glfwSetMouseButtonCallback(pWindow, ImGuiMouseButtonCallback);
        pBackend->PrevUserCallbackScroll      = glfwSetScrollCallback(pWindow, ImGuiScrollCallback);
        pBackend->PrevUserCallbackKey         = glfwSetKeyCallback(pWindow, ImGuiKeyCallback);
        pBackend->PrevUserCallbackChar        = glfwSetCharCallback(pWindow, ImGuiCharCallback);
        pBackend->PrevUserCallbackMonitor     = glfwSetMonitorCallback(ImGuiMonitorCallback);
        pBackend->bInstalledCallbacks          = true;
    }
    
    static void ImGuiRestoreCallbacks(GLFWwindow* pWindow)
    {
        ImGuiBackendData* pBackend = ImGuiGetBackendData();
        assert(pBackend->bInstalledCallbacks == true && "Callbacks not installed!");
        assert(pBackend->Window == pWindow);
        
        glfwSetWindowFocusCallback(pWindow, pBackend->PrevUserCallbackWindowFocus);
        glfwSetCursorEnterCallback(pWindow, pBackend->PrevUserCallbackCursorEnter);
        glfwSetCursorPosCallback(pWindow, pBackend->PrevUserCallbackCursorPos);
        glfwSetMouseButtonCallback(pWindow, pBackend->PrevUserCallbackMousebutton);
        glfwSetScrollCallback(pWindow, pBackend->PrevUserCallbackScroll);
        glfwSetKeyCallback(pWindow, pBackend->PrevUserCallbackKey);
        glfwSetCharCallback(pWindow, pBackend->PrevUserCallbackChar);
        glfwSetMonitorCallback(pBackend->PrevUserCallbackMonitor);
        
        pBackend->PrevUserCallbackWindowFocus = nullptr;
        pBackend->PrevUserCallbackCursorEnter = nullptr;
        pBackend->PrevUserCallbackCursorPos   = nullptr;
        pBackend->PrevUserCallbackMousebutton = nullptr;
        pBackend->PrevUserCallbackScroll      = nullptr;
        pBackend->PrevUserCallbackKey         = nullptr;
        pBackend->PrevUserCallbackChar        = nullptr;
        pBackend->PrevUserCallbackMonitor     = nullptr;
        pBackend->bInstalledCallbacks         = false;
    }
    
    static void ImGuiUpdateMonitors()
    {
        int monitorsCount = 0;
        GLFWmonitor** glfwMonitors = glfwGetMonitors(&monitorsCount);
        
        ImGuiPlatformIO& platformIO = ImGui::GetPlatformIO();
        platformIO.Monitors.resize(0);
        
        ImGuiBackendData* pBackend = ImGuiGetBackendData();
        pBackend->bWantUpdateMonitors = false;
        
        for (int n = 0; n < monitorsCount; n++)
        {
            int posX;
            int posY;
            glfwGetMonitorPos(glfwMonitors[n], &posX, &posY);
            
            const GLFWvidmode* videoMode = glfwGetVideoMode(glfwMonitors[n]);
            if (videoMode == nullptr)
            {
                continue; // Failed to get Video mode (e.g. Emscripten does not support this function)
            }
            
            ImGuiPlatformMonitor monitor;
            monitor.MainPos  = monitor.WorkPos  = ImVec2((float)posX, (float)posY);
            monitor.MainSize = monitor.WorkSize = ImVec2((float)videoMode->width, (float)videoMode->height);
            
            int width;
            int height;
            glfwGetMonitorWorkarea(glfwMonitors[n], &posX, &posY, &width, &height);
            
            if (width > 0 && height > 0) // Workaround a small GLFW issue reporting zero on monitor changes: https://github.com/glfw/glfw/pull/1761
            {
                monitor.WorkPos  = ImVec2((float)posX, (float)posY);
                monitor.WorkSize = ImVec2((float)width, (float)height);
            }
            
            // Warning: the validity of monitor DPI information on Windows depends on the application DPI awareness settings, which generally needs to be set in the manifest or at runtime.
            float scaleX;
            float scaleY;
            glfwGetMonitorContentScale(glfwMonitors[n], &scaleX, &scaleY);
            monitor.DpiScale = scaleX;
            
            platformIO.Monitors.push_back(monitor);
        }
    }
    
    static void ImGuiCreateWindow(ImGuiViewport* pViewport)
    {
        ImGuiBackendData*  pBackend      = ImGuiGetBackendData();
        ImGuiViewportData* pViewportData = new ImGuiViewportData();
        pViewport->PlatformUserData = pViewportData;
        
        glfwWindowHint(GLFW_VISIBLE, false);
        glfwWindowHint(GLFW_FOCUSED, false);
        glfwWindowHint(GLFW_FOCUS_ON_SHOW, false);
        glfwWindowHint(GLFW_DECORATED, (pViewport->Flags & ImGuiViewportFlags_NoDecoration) ? false : true);
        glfwWindowHint(GLFW_FLOATING, (pViewport->Flags & ImGuiViewportFlags_TopMost) ? true : false);
        
        pViewportData->pWindow      = glfwCreateWindow((int)pViewport->Size.x, (int)pViewport->Size.y, "No Title Yet", nullptr, nullptr);
        pViewportData->bWindowOwned = true;
        pViewport->PlatformHandle   = pViewportData->pWindow;
        
        glfwSetWindowPos(pViewportData->pWindow, (int)pViewport->Pos.x, (int)pViewport->Pos.y);

    #ifdef PLATFORM_WINDOWS
        pViewport->PlatformHandleRaw = glfwGetWin32Window(pViewportData->Window);
    #elif PLATFORM_MAC
        pViewport->PlatformHandleRaw = glfwGetCocoaWindow(pViewportData->pWindow);
    #endif
                
        // Install GLFW callbacks for secondary viewports
        glfwSetWindowFocusCallback(pViewportData->pWindow, ImGuiWindowFocusCallback);
        glfwSetCursorEnterCallback(pViewportData->pWindow, ImGuiCursorEnterCallback);
        glfwSetCursorPosCallback(pViewportData->pWindow, ImGuiCursorPosCallback);
        glfwSetMouseButtonCallback(pViewportData->pWindow, ImGuiMouseButtonCallback);
        glfwSetScrollCallback(pViewportData->pWindow, ImGuiScrollCallback);
        glfwSetKeyCallback(pViewportData->pWindow, ImGuiKeyCallback);
        glfwSetCharCallback(pViewportData->pWindow, ImGuiCharCallback);
        glfwSetWindowCloseCallback(pViewportData->pWindow, ImGuiWindowCloseCallback);
        glfwSetWindowPosCallback(pViewportData->pWindow, ImGuiWindowPosCallback);
        glfwSetWindowSizeCallback(pViewportData->pWindow, ImGuiWindowSizeCallback);
    }
    
    static void ImGuiDestroyWindow(ImGuiViewport* pViewport)
    {
        ImGuiBackendData* pBackend = ImGuiGetBackendData();
        if (ImGuiViewportData* pViewportData = (ImGuiViewportData*)pViewport->PlatformUserData)
        {
            if (pViewportData->bWindowOwned)
            {
                // Release any keys that were pressed in the window being destroyed and are still held down,
                // because we will not receive any release events after window is destroyed.
                for (int i = 0; i < IM_ARRAYSIZE(pBackend->KeyOwnerWindows); i++)
                {
                    if (pBackend->KeyOwnerWindows[i] == pViewportData->pWindow)
                    {
                        ImGuiKeyCallback(pViewportData->pWindow, i, 0, GLFW_RELEASE, 0); // Later params are only used for main pViewport, on which this function is never called.
                    }
                }
                
                glfwDestroyWindow(pViewportData->pWindow);
            }
            
            pViewportData->pWindow = nullptr;
            delete pViewportData;
        }
        
        pViewport->PlatformUserData = pViewport->PlatformHandle = nullptr;
    }
    
    static void ImGuiShowWindow(ImGuiViewport* pViewport)
    {
        ImGuiViewportData* pViewportData = (ImGuiViewportData*)pViewport->PlatformUserData;
        
    #if PLATFORM_WINDOWS
        // GLFW hack: Hide icon from task bar
        HWND hwnd = (HWND)pViewport->PlatformHandleRaw;
        if (pViewport->Flags & ImGuiViewportFlags_NoTaskBarIcon)
        {
            LONG exStyle = ::GetWindowLong(hwnd, GWL_EXSTYLE);
            exStyle &= ~WS_EX_APPWINDOW;
            exStyle |= WS_EX_TOOLWINDOW;
            ::SetWindowLong(hwnd, GWL_EXSTYLE, exStyle);
        }
    #endif
        
        glfwShowWindow(pViewportData->pWindow);
    }
    
    static ImVec2 ImGuiGetWindowPos(ImGuiViewport* pViewport)
    {
        ImGuiViewportData* pViewportData = (ImGuiViewportData*)pViewport->PlatformUserData;
        
        int posX = 0;
        int posY = 0;
        glfwGetWindowPos(pViewportData->pWindow, &posX, &posY);
        return ImVec2((float)posX, (float)posY);
    }
    
    static void ImGuiSetWindowPos(ImGuiViewport* pViewport, ImVec2 pos)
    {
        ImGuiViewportData* pViewportData = (ImGuiViewportData*)pViewport->PlatformUserData;
        pViewportData->IgnoreWindowPosEventFrame = ImGui::GetFrameCount();
        glfwSetWindowPos(pViewportData->pWindow, (int)pos.x, (int)pos.y);
    }
    
    static ImVec2 ImGuiGetWindowSize(ImGuiViewport* pViewport)
    {
        ImGuiViewportData* pViewportData = (ImGuiViewportData*)pViewport->PlatformUserData;
        
        int width  = 0;
        int height = 0;
        glfwGetWindowSize(pViewportData->pWindow, &width, &height);
        return ImVec2((float)width, (float)height);
    }
    
    static void ImGuiSetWindowSize(ImGuiViewport* pViewport, ImVec2 size)
    {
        ImGuiViewportData* pViewportData = (ImGuiViewportData*)pViewport->PlatformUserData;
        pViewportData->IgnoreWindowSizeEventFrame = ImGui::GetFrameCount();
        glfwSetWindowSize(pViewportData->pWindow, (int)size.x, (int)size.y);
    }
    
    static void ImGuiSetWindowTitle(ImGuiViewport* pViewport, const char* title)
    {
        ImGuiViewportData* pViewportData = (ImGuiViewportData*)pViewport->PlatformUserData;
        glfwSetWindowTitle(pViewportData->pWindow, title);
    }
    
    static void ImGuiSetWindowFocus(ImGuiViewport* pViewport)
    {
        ImGuiViewportData* pViewportData = (ImGuiViewportData*)pViewport->PlatformUserData;
        glfwFocusWindow(pViewportData->pWindow);
    }
    
    static bool ImGuiGetWindowFocus(ImGuiViewport* pViewport)
    {
        ImGuiViewportData* pViewportData = (ImGuiViewportData*)pViewport->PlatformUserData;
        return glfwGetWindowAttrib(pViewportData->pWindow, GLFW_FOCUSED) != 0;
    }
    
    static bool ImGuiGetWindowMinimized(ImGuiViewport* pViewport)
    {
        ImGuiViewportData* pViewportData = (ImGuiViewportData*)pViewport->PlatformUserData;
        return glfwGetWindowAttrib(pViewportData->pWindow, GLFW_ICONIFIED) != 0;
    }
    
    static void ImGuiSetWindowAlpha(ImGuiViewport* pViewport, float alpha)
    {
        ImGuiViewportData* pViewportData = (ImGuiViewportData*)pViewport->PlatformUserData;
        glfwSetWindowOpacity(pViewportData->pWindow, alpha);
    }
    
    static void ImGuiRenderWindow(ImGuiViewport* pViewport, void*)
    {
    }
    
    static void ImGuiSwapBuffers(ImGuiViewport* pViewport, void*)
    {
    }
    
    static int ImGuiCreateVkSurface(ImGuiViewport* pViewport, ImU64 instance, const void* allocator, ImU64* outSurface)
    {
        ImGuiViewportData* pViewportData = (ImGuiViewportData*)pViewport->PlatformUserData;
        VkResult result = glfwCreateWindowSurface((VkInstance)instance, pViewportData->pWindow, (const VkAllocationCallbacks*)allocator, (VkSurfaceKHR*)outSurface);
        return (int)result;
    }
    
    static void ImGuiInitPlatformInterface()
    {
        // Register platform interface (will be coupled with a renderer interface)
        ImGuiBackendData* pBackend = ImGuiGetBackendData();
        
        ImGuiPlatformIO& platformIO = ImGui::GetPlatformIO();
        platformIO.Platform_CreateWindow       = ImGuiCreateWindow;
        platformIO.Platform_DestroyWindow      = ImGuiDestroyWindow;
        platformIO.Platform_ShowWindow         = ImGuiShowWindow;
        platformIO.Platform_SetWindowPos       = ImGuiSetWindowPos;
        platformIO.Platform_GetWindowPos       = ImGuiGetWindowPos;
        platformIO.Platform_SetWindowSize      = ImGuiSetWindowSize;
        platformIO.Platform_GetWindowSize      = ImGuiGetWindowSize;
        platformIO.Platform_SetWindowFocus     = ImGuiSetWindowFocus;
        platformIO.Platform_GetWindowFocus     = ImGuiGetWindowFocus;
        platformIO.Platform_GetWindowMinimized = ImGuiGetWindowMinimized;
        platformIO.Platform_SetWindowTitle     = ImGuiSetWindowTitle;
        platformIO.Platform_RenderWindow       = ImGuiRenderWindow;
        platformIO.Platform_SwapBuffers        = ImGuiSwapBuffers;
        platformIO.Platform_SetWindowAlpha     = ImGuiSetWindowAlpha;
        platformIO.Platform_CreateVkSurface    = ImGuiCreateVkSurface;
        
        // Register main window handle (which is owned by the main application, not by us)
        // This is mostly for simplicity and consistency, so that our code (e.g. mouse handling etc.) can use same logic for main and secondary viewports.
        ImGuiViewportData* pViewportData = new ImGuiViewportData();
        pViewportData->pWindow       = pBackend->Window;
        pViewportData->bWindowOwned = false;
        
        ImGuiViewport* pMainViewport = ImGui::GetMainViewport();
        pMainViewport->PlatformUserData = pViewportData;
        pMainViewport->PlatformHandle   = pBackend->Window;
    }
    
    static void ImguiInitBackend(GLFWwindow* pWindow)
    {
        ImGuiBackendData* pBackend = new ImGuiBackendData();
        pBackend->Window             = pWindow;
        pBackend->Time               = 0.0;
        pBackend->bWantUpdateMonitors = true;
        
        ImGuiIO& io = ImGui::GetIO();
        io.BackendPlatformUserData = pBackend;
        io.BackendPlatformName     = "VulkanProject";
        
        io.SetClipboardTextFn = ImGuiSetClipboardText;
        io.GetClipboardTextFn = ImGuiGetClipboardText;
        io.ClipboardUserData  = pBackend->Window;
        
        io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;         // We can honor GetMouseCursor() values (optional)
        io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;          // We can honor io.WantSetMousePos requests (optional, rarely used)
        io.BackendFlags |= ImGuiBackendFlags_PlatformHasViewports;    // We can create multi-viewports on the Platform side (optional)
        io.BackendFlags |= ImGuiBackendFlags_HasMouseHoveredViewport; // We can call io.AddMouseViewportEvent() with correct data (optional)
        
        // Create mouse cursors
        // (By design, on X11 cursors are user configurable and some cursors may be missing. When a cursor doesn't exist,
        // GLFW will emit an error which will often be printed by the app, so we temporarily disable error reporting.
        // Missing cursors will return nullptr and our _UpdateMouseCursor() function will use the Arrow cursor instead.)
        GLFWerrorfun prevErrorCallback = glfwSetErrorCallback(nullptr);
        pBackend->MouseCursors[ImGuiMouseCursor_Arrow]      = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
        pBackend->MouseCursors[ImGuiMouseCursor_TextInput]  = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);
        pBackend->MouseCursors[ImGuiMouseCursor_ResizeNS]   = glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR);
        pBackend->MouseCursors[ImGuiMouseCursor_ResizeEW]   = glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);
        pBackend->MouseCursors[ImGuiMouseCursor_Hand]       = glfwCreateStandardCursor(GLFW_HAND_CURSOR);
        pBackend->MouseCursors[ImGuiMouseCursor_ResizeAll]  = glfwCreateStandardCursor(GLFW_RESIZE_ALL_CURSOR);
        pBackend->MouseCursors[ImGuiMouseCursor_ResizeNESW] = glfwCreateStandardCursor(GLFW_RESIZE_NESW_CURSOR);
        pBackend->MouseCursors[ImGuiMouseCursor_ResizeNWSE] = glfwCreateStandardCursor(GLFW_RESIZE_NWSE_CURSOR);
        pBackend->MouseCursors[ImGuiMouseCursor_NotAllowed] = glfwCreateStandardCursor(GLFW_NOT_ALLOWED_CURSOR);
        glfwSetErrorCallback(prevErrorCallback);
        
        // Eat errors
        (void)glfwGetError(0);
        
        // Install main window callbacks
        ImguiInstallCallbacks(pWindow);
        
        // Update monitors the first time (note: monitor callback are broken in GLFW 3.2 and earlier, see github.com/glfw/glfw/issues/784)
        ImGuiUpdateMonitors();
        glfwSetMonitorCallback(ImGuiMonitorCallback);
        
        // Our mouse update function expect PlatformHandle to be filled for the main pViewport
        ImGuiViewport* pMainViewport = ImGui::GetMainViewport();
        pMainViewport->PlatformHandle = (void*)pBackend->Window;
    #ifdef PLATFORM_WINDOWS
        mainViewport->PlatformHandleRaw = glfwGetWin32Window(pBackend->Window);
    #elif PLATFORM_MAC
        pMainViewport->PlatformHandleRaw = glfwGetCocoaWindow(pBackend->Window);
    #endif
        
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGuiInitPlatformInterface();
        }
    }
    
    bool ImGuiCreateFontsTexture()
    {
        if (ImGuiRendererBackendData* pRendererBackend = ImGuiGetRendererBackendData())
        {
            int width;
            int height;
            unsigned char* pixels;
            
            ImGuiIO& io = ImGui::GetIO();
            io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
            
            TextureParams textureParams = {};
            textureParams.Format    = VK_FORMAT_R8G8B8A8_UNORM;
            textureParams.ImageType = VK_IMAGE_TYPE_2D;
            textureParams.Width     = width;
            textureParams.Height    = height;
            
            pRendererBackend->pFontTexture = Texture::CreateWithData(pRendererBackend->pContext, textureParams, pixels);
            if (!pRendererBackend->pFontTexture)
            {
                return false;
            }
            
            TextureViewParams textureViewParams = {};
            textureViewParams.pTexture = pRendererBackend->pFontTexture;
            
            pRendererBackend->pFontTextureView = TextureView::Create(pRendererBackend->pContext, textureViewParams);
            if (!pRendererBackend->pFontTextureView)
            {
                return false;
            }
            
            pRendererBackend->pFontDescriptorSet->BindCombinedImageSampler(pRendererBackend->pFontTextureView->GetImageView(), pRendererBackend->pFontSampler->GetSampler(), 0);
            io.Fonts->SetTexID((ImTextureID)pRendererBackend->pFontDescriptorSet);
            return true;
        }
        else
        {
            return false;
        }
    }
    
    static void ImGuiCreateShaderModules()
    {
        // glsl_shader.vert, compiled with:
        // # glslangValidator -V -x -o glsl_shader.vert.u32 glsl_shader.vert
        /*
         #version 450 core
         layout(location = 0) in vec2 aPos;
         layout(location = 1) in vec2 aUV;
         layout(location = 2) in vec4 aColor;
         layout(push_constant) uniform uPushConstant { vec2 uScale; vec2 uTranslate; } pc;
         
         out gl_PerVertex { vec4 gl_Position; };
         layout(location = 0) out struct { vec4 Color; vec2 UV; } Out;
         
         void main()
         {
         Out.Color = aColor;
         Out.UV = aUV;
         gl_Position = vec4(aPos * pc.uScale + pc.uTranslate, 0, 1);
         }
         */
        static uint32_t __glsl_shader_vert_spv[] =
        {
            0x07230203,0x00010000,0x00080001,0x0000002e,0x00000000,0x00020011,0x00000001,0x0006000b,
            0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
            0x000a000f,0x00000000,0x00000004,0x6e69616d,0x00000000,0x0000000b,0x0000000f,0x00000015,
            0x0000001b,0x0000001c,0x00030003,0x00000002,0x000001c2,0x00040005,0x00000004,0x6e69616d,
            0x00000000,0x00030005,0x00000009,0x00000000,0x00050006,0x00000009,0x00000000,0x6f6c6f43,
            0x00000072,0x00040006,0x00000009,0x00000001,0x00005655,0x00030005,0x0000000b,0x0074754f,
            0x00040005,0x0000000f,0x6c6f4361,0x0000726f,0x00030005,0x00000015,0x00565561,0x00060005,
            0x00000019,0x505f6c67,0x65567265,0x78657472,0x00000000,0x00060006,0x00000019,0x00000000,
            0x505f6c67,0x7469736f,0x006e6f69,0x00030005,0x0000001b,0x00000000,0x00040005,0x0000001c,
            0x736f5061,0x00000000,0x00060005,0x0000001e,0x73755075,0x6e6f4368,0x6e617473,0x00000074,
            0x00050006,0x0000001e,0x00000000,0x61635375,0x0000656c,0x00060006,0x0000001e,0x00000001,
            0x61725475,0x616c736e,0x00006574,0x00030005,0x00000020,0x00006370,0x00040047,0x0000000b,
            0x0000001e,0x00000000,0x00040047,0x0000000f,0x0000001e,0x00000002,0x00040047,0x00000015,
            0x0000001e,0x00000001,0x00050048,0x00000019,0x00000000,0x0000000b,0x00000000,0x00030047,
            0x00000019,0x00000002,0x00040047,0x0000001c,0x0000001e,0x00000000,0x00050048,0x0000001e,
            0x00000000,0x00000023,0x00000000,0x00050048,0x0000001e,0x00000001,0x00000023,0x00000008,
            0x00030047,0x0000001e,0x00000002,0x00020013,0x00000002,0x00030021,0x00000003,0x00000002,
            0x00030016,0x00000006,0x00000020,0x00040017,0x00000007,0x00000006,0x00000004,0x00040017,
            0x00000008,0x00000006,0x00000002,0x0004001e,0x00000009,0x00000007,0x00000008,0x00040020,
            0x0000000a,0x00000003,0x00000009,0x0004003b,0x0000000a,0x0000000b,0x00000003,0x00040015,
            0x0000000c,0x00000020,0x00000001,0x0004002b,0x0000000c,0x0000000d,0x00000000,0x00040020,
            0x0000000e,0x00000001,0x00000007,0x0004003b,0x0000000e,0x0000000f,0x00000001,0x00040020,
            0x00000011,0x00000003,0x00000007,0x0004002b,0x0000000c,0x00000013,0x00000001,0x00040020,
            0x00000014,0x00000001,0x00000008,0x0004003b,0x00000014,0x00000015,0x00000001,0x00040020,
            0x00000017,0x00000003,0x00000008,0x0003001e,0x00000019,0x00000007,0x00040020,0x0000001a,
            0x00000003,0x00000019,0x0004003b,0x0000001a,0x0000001b,0x00000003,0x0004003b,0x00000014,
            0x0000001c,0x00000001,0x0004001e,0x0000001e,0x00000008,0x00000008,0x00040020,0x0000001f,
            0x00000009,0x0000001e,0x0004003b,0x0000001f,0x00000020,0x00000009,0x00040020,0x00000021,
            0x00000009,0x00000008,0x0004002b,0x00000006,0x00000028,0x00000000,0x0004002b,0x00000006,
            0x00000029,0x3f800000,0x00050036,0x00000002,0x00000004,0x00000000,0x00000003,0x000200f8,
            0x00000005,0x0004003d,0x00000007,0x00000010,0x0000000f,0x00050041,0x00000011,0x00000012,
            0x0000000b,0x0000000d,0x0003003e,0x00000012,0x00000010,0x0004003d,0x00000008,0x00000016,
            0x00000015,0x00050041,0x00000017,0x00000018,0x0000000b,0x00000013,0x0003003e,0x00000018,
            0x00000016,0x0004003d,0x00000008,0x0000001d,0x0000001c,0x00050041,0x00000021,0x00000022,
            0x00000020,0x0000000d,0x0004003d,0x00000008,0x00000023,0x00000022,0x00050085,0x00000008,
            0x00000024,0x0000001d,0x00000023,0x00050041,0x00000021,0x00000025,0x00000020,0x00000013,
            0x0004003d,0x00000008,0x00000026,0x00000025,0x00050081,0x00000008,0x00000027,0x00000024,
            0x00000026,0x00050051,0x00000006,0x0000002a,0x00000027,0x00000000,0x00050051,0x00000006,
            0x0000002b,0x00000027,0x00000001,0x00070050,0x00000007,0x0000002c,0x0000002a,0x0000002b,
            0x00000028,0x00000029,0x00050041,0x00000011,0x0000002d,0x0000001b,0x0000000d,0x0003003e,
            0x0000002d,0x0000002c,0x000100fd,0x00010038
        };
        
        // glsl_shader.frag, compiled with:
        // # glslangValidator -V -x -o glsl_shader.frag.u32 glsl_shader.frag
        /*
         #version 450 core
         layout(location = 0) out vec4 fColor;
         layout(set=0, binding=0) uniform sampler2D sTexture;
         layout(location = 0) in struct { vec4 Color; vec2 UV; } In;
         void main()
         {
         fColor = In.Color * texture(sTexture, In.UV.st);
         }
         */
        static uint32_t __glsl_shader_frag_spv[] =
        {
            0x07230203,0x00010000,0x00080001,0x0000001e,0x00000000,0x00020011,0x00000001,0x0006000b,
            0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
            0x0007000f,0x00000004,0x00000004,0x6e69616d,0x00000000,0x00000009,0x0000000d,0x00030010,
            0x00000004,0x00000007,0x00030003,0x00000002,0x000001c2,0x00040005,0x00000004,0x6e69616d,
            0x00000000,0x00040005,0x00000009,0x6c6f4366,0x0000726f,0x00030005,0x0000000b,0x00000000,
            0x00050006,0x0000000b,0x00000000,0x6f6c6f43,0x00000072,0x00040006,0x0000000b,0x00000001,
            0x00005655,0x00030005,0x0000000d,0x00006e49,0x00050005,0x00000016,0x78655473,0x65727574,
            0x00000000,0x00040047,0x00000009,0x0000001e,0x00000000,0x00040047,0x0000000d,0x0000001e,
            0x00000000,0x00040047,0x00000016,0x00000022,0x00000000,0x00040047,0x00000016,0x00000021,
            0x00000000,0x00020013,0x00000002,0x00030021,0x00000003,0x00000002,0x00030016,0x00000006,
            0x00000020,0x00040017,0x00000007,0x00000006,0x00000004,0x00040020,0x00000008,0x00000003,
            0x00000007,0x0004003b,0x00000008,0x00000009,0x00000003,0x00040017,0x0000000a,0x00000006,
            0x00000002,0x0004001e,0x0000000b,0x00000007,0x0000000a,0x00040020,0x0000000c,0x00000001,
            0x0000000b,0x0004003b,0x0000000c,0x0000000d,0x00000001,0x00040015,0x0000000e,0x00000020,
            0x00000001,0x0004002b,0x0000000e,0x0000000f,0x00000000,0x00040020,0x00000010,0x00000001,
            0x00000007,0x00090019,0x00000013,0x00000006,0x00000001,0x00000000,0x00000000,0x00000000,
            0x00000001,0x00000000,0x0003001b,0x00000014,0x00000013,0x00040020,0x00000015,0x00000000,
            0x00000014,0x0004003b,0x00000015,0x00000016,0x00000000,0x0004002b,0x0000000e,0x00000018,
            0x00000001,0x00040020,0x00000019,0x00000001,0x0000000a,0x00050036,0x00000002,0x00000004,
            0x00000000,0x00000003,0x000200f8,0x00000005,0x00050041,0x00000010,0x00000011,0x0000000d,
            0x0000000f,0x0004003d,0x00000007,0x00000012,0x00000011,0x0004003d,0x00000014,0x00000017,
            0x00000016,0x00050041,0x00000019,0x0000001a,0x0000000d,0x00000018,0x0004003d,0x0000000a,
            0x0000001b,0x0000001a,0x00050057,0x00000007,0x0000001c,0x00000017,0x0000001b,0x00050085,
            0x00000007,0x0000001d,0x00000012,0x0000001c,0x0003003e,0x00000009,0x0000001d,0x000100fd,
            0x00010038
        };
        
        ImGuiRendererBackendData* pRendererBackend = ImGuiGetRendererBackendData();
        if (!pRendererBackend->pShaderModuleVert)
        {
            pRendererBackend->pShaderModuleVert = ShaderModule::Create(pRendererBackend->pContext, __glsl_shader_vert_spv, sizeof(__glsl_shader_vert_spv), "main");
            assert(pRendererBackend->pShaderModuleVert != nullptr);
        }
        
        if (!pRendererBackend->pShaderModuleFrag)
        {
            pRendererBackend->pShaderModuleFrag = ShaderModule::Create(pRendererBackend->pContext, __glsl_shader_frag_spv, sizeof(__glsl_shader_frag_spv), "main");
            assert(pRendererBackend->pShaderModuleFrag != nullptr);
        }
    }
    
    static void ImGuiCreatePipeline()
    {
        ImGuiRendererBackendData* pRendererBackend = ImGuiGetRendererBackendData();
        ImGuiCreateShaderModules();
        
        if (!pRendererBackend->pPipeline)
        {
            GraphicsPipelineStateParams graphicsPipelineStateParams = {};
            graphicsPipelineStateParams.pVertexShader   = pRendererBackend->pShaderModuleVert;
            graphicsPipelineStateParams.pFragmentShader = pRendererBackend->pShaderModuleFrag;
            graphicsPipelineStateParams.pRenderPass     = pRendererBackend->pRenderPass;
            graphicsPipelineStateParams.pPipelineLayout = pRendererBackend->pPipelineLayout;
            
            VkVertexInputBindingDescription bindingDesc[1] = {};
            bindingDesc[0].stride    = sizeof(ImDrawVert);
            bindingDesc[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            
            VkVertexInputAttributeDescription attributeDesc[3] = {};
            attributeDesc[0].location = 0;
            attributeDesc[0].binding  = bindingDesc[0].binding;
            attributeDesc[0].format   = VK_FORMAT_R32G32_SFLOAT;
            attributeDesc[0].offset   = IM_OFFSETOF(ImDrawVert, pos);
            attributeDesc[1].location = 1;
            attributeDesc[1].binding  = bindingDesc[0].binding;
            attributeDesc[1].format   = VK_FORMAT_R32G32_SFLOAT;
            attributeDesc[1].offset   = IM_OFFSETOF(ImDrawVert, uv);
            attributeDesc[2].location = 2;
            attributeDesc[2].binding  = bindingDesc[0].binding;
            attributeDesc[2].format   = VK_FORMAT_R8G8B8A8_UNORM;
            attributeDesc[2].offset   = IM_OFFSETOF(ImDrawVert, col);
            
            graphicsPipelineStateParams.attributeDescriptionCount = 3;
            graphicsPipelineStateParams.pAttributeDescriptions    = attributeDesc;
            
            graphicsPipelineStateParams.bindingDescriptionCount = 1;
            graphicsPipelineStateParams.pBindingDescriptions    = bindingDesc;
            
            graphicsPipelineStateParams.cullMode     = VK_CULL_MODE_NONE;
            graphicsPipelineStateParams.frontFace    = VK_FRONT_FACE_COUNTER_CLOCKWISE;
            graphicsPipelineStateParams.bBlendEnable = true;
            
            pRendererBackend->pPipeline = GraphicsPipeline::Create(pRendererBackend->pContext, graphicsPipelineStateParams);
            assert(pRendererBackend->pPipeline != nullptr);
        }
    }
    
    static bool ImGuiCreateDeviceObjects()
    {
        ImGuiRendererBackendData* pRendererBackend = ImGuiGetRendererBackendData();
        if (!pRendererBackend->pFontSampler)
        {
            // Bilinear sampling is required by default. Set 'io.Fonts->Flags |= ImFontAtlasFlags_NoBakedLines' or 'style.AntiAliasedLinesUseTex = false' to allow point/nearest sampling.
            SamplerParams samplerParams = {};
            samplerParams.magFilter     = VK_FILTER_LINEAR;
            samplerParams.minFilter     = VK_FILTER_LINEAR;
            samplerParams.mipmapMode    = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            samplerParams.addressModeU  = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerParams.addressModeV  = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerParams.addressModeW  = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerParams.minLod        = -1000;
            samplerParams.maxLod        = 1000;
            samplerParams.maxAnisotropy = 1.0f;
            
            pRendererBackend->pFontSampler = Sampler::Create(pRendererBackend->pContext, samplerParams);
            assert(pRendererBackend->pFontSampler != nullptr);
        }
        
        if (!pRendererBackend->pDescriptorSetLayout)
        {
            VkDescriptorSetLayoutBinding binding[1] = {};
            binding[0].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            binding[0].descriptorCount = 1;
            binding[0].stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;
            
            DescriptorSetLayoutParams descriptorSetLayoutParams = {};
            descriptorSetLayoutParams.numBindings = 1;
            descriptorSetLayoutParams.pBindings   = binding;
            
            pRendererBackend->pDescriptorSetLayout = DescriptorSetLayout::Create(pRendererBackend->pContext, descriptorSetLayoutParams);
            assert(pRendererBackend->pDescriptorSetLayout != nullptr);
        }
        
        if (!pRendererBackend->pDescriptorPool)
        {
            DescriptorPoolParams descriptorPoolParams = {};
            descriptorPoolParams.MaxSets                  = 1024;
            descriptorPoolParams.NumCombinedImageSamplers = 1024;
            
            pRendererBackend->pDescriptorPool = DescriptorPool::Create(pRendererBackend->pContext, descriptorPoolParams);
            assert(pRendererBackend->pDescriptorSetLayout != nullptr);
        }
        
        if (!pRendererBackend->pFontDescriptorSet)
        {
            pRendererBackend->pFontDescriptorSet = DescriptorSet::Create(pRendererBackend->pContext, pRendererBackend->pDescriptorPool, pRendererBackend->pDescriptorSetLayout);
            assert(pRendererBackend->pFontDescriptorSet != nullptr);
        }
        
        if (!pRendererBackend->pPipelineLayout)
        {
            PipelineLayoutParams pipelineLayoutParams = {};
            pipelineLayoutParams.ppLayouts        = &pRendererBackend->pDescriptorSetLayout;
            pipelineLayoutParams.numLayouts       = 1;
            pipelineLayoutParams.numPushConstants = 4;
            
            pRendererBackend->pPipelineLayout = PipelineLayout::Create(pRendererBackend->pContext, pipelineLayoutParams);
            assert(pRendererBackend->pPipelineLayout != nullptr);
        }
        
        if (!pRendererBackend->pRenderPass)
        {
            RenderPassAttachment attachments[1] = {};
            attachments[0].Format = pRendererBackend->pContext->GetSwapChain()->GetFormat();
            attachments[0].LoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
            
            RenderPassParams renderPassParams = {};
            renderPassParams.ColorAttachmentCount = 1;
            renderPassParams.pColorAttachments    = attachments;
            
            pRendererBackend->pRenderPass = RenderPass::Create(pRendererBackend->pContext, renderPassParams);
            assert(pRendererBackend->pRenderPass != nullptr);
        }
        
        ImGuiCreatePipeline();
        return true;
    }
    
    
    //--------------------------------------------------------------------------------------------------------
    // MULTI-VIEWPORT / PLATFORM INTERFACE SUPPORT
    // This is an _advanced_ and _optional_ feature, allowing the backend to create and handle multiple viewports simultaneously.
    // If you are new to dear imgui or creating a new binding for dear imgui, it is recommended that you completely ignore this section first..
    //--------------------------------------------------------------------------------------------------------
    
    static void ImGuiDestroyFramebuffers(ImGuiViewportData* pViewportData)
    {
        for (auto& pFramebuffer : pViewportData->FrameBuffers)
        {
            SAFE_DELETE(pFramebuffer);
        }
    }
    
    static void ImGuiDestroyWindowRenderBuffers(ImGuiViewportData* pViewportData)
    {
        pViewportData->FrameData.clear();
    }

    static void ImGuiCreateFramebuffers(VulkanContext* pContext, ImGuiViewportData* pViewportData)
    {
        // SwapChain extent
        VkExtent2D extent = pViewportData->pSwapChain->GetExtent();
        
        // Create FrameBuffers
        FramebufferParams framebufferParams = {};
        framebufferParams.pRenderPass     = pViewportData->pRenderPass;
        framebufferParams.AttachMentCount = 1;
        framebufferParams.Width           = extent.width;
        framebufferParams.Height          = extent.height;
        
        uint32_t numBackbuffers = pViewportData->pSwapChain->GetNumBackBuffers();
        pViewportData->FrameBuffers.resize(numBackbuffers);
        
        for (uint32_t i = 0; i < numBackbuffers; i++)
        {
            VkImageView imageView = pViewportData->pSwapChain->GetImageView(i);
            framebufferParams.pAttachMents = &imageView;
            
            Framebuffer* pFramebuffer = Framebuffer::Create(pContext, framebufferParams);
            assert(pFramebuffer != nullptr);
            pViewportData->FrameBuffers[i] = pFramebuffer;
        }
    }
    
    static void ImGuiCreateWindowRenderBuffers(VulkanContext* pContext, ImGuiViewportData* pViewportData)
    {
        CommandBufferParams commandBufferParams = {};
        commandBufferParams.Level     = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        commandBufferParams.QueueType = ECommandQueueType::Graphics;
        
        uint32_t numBackbuffers = pViewportData->pSwapChain->GetNumBackBuffers();
        pViewportData->FrameData.resize(numBackbuffers);
        
        for (uint32_t i = 0; i < numBackbuffers; i++)
        {
            CommandBuffer* pCommandBuffer = CommandBuffer::Create(pContext, commandBufferParams);
            assert(pCommandBuffer != nullptr);
            pViewportData->FrameData[i].pCommandBuffer = pCommandBuffer;
        }
    }
    
    static void ImGuiRendererCreateWindow(ImGuiViewport* pViewport)
    {
        ImGuiRendererBackendData* pRendererBackend = ImGuiGetRendererBackendData();
        if (ImGuiViewportData* pViewportData = reinterpret_cast<ImGuiViewportData*>(pViewport->PlatformUserData))
        {
            // We have the same structure for platform and rendererdata
            pViewport->RendererUserData = pViewportData;
            
            // Create SwapChain
            pViewportData->pSwapChain = SwapChain::Create(pRendererBackend->pContext, pViewportData->pWindow);
            
            // Create RenderPass for this Viewport
            RenderPassAttachment attachments[1] = {};
            attachments[0].Format = pViewportData->pSwapChain->GetFormat();
            attachments[0].LoadOp = (pViewport->Flags & ImGuiViewportFlags_NoRendererClear) ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_CLEAR;
            
            RenderPassParams renderPassParams = {};
            renderPassParams.ColorAttachmentCount = 1;
            renderPassParams.pColorAttachments    = attachments;
            
            pViewportData->pRenderPass = RenderPass::Create(pRendererBackend->pContext, renderPassParams);
            assert(pViewportData->pRenderPass != nullptr);
            
            ImGuiCreateFramebuffers(pRendererBackend->pContext, pViewportData);
            ImGuiCreateWindowRenderBuffers(pRendererBackend->pContext, pViewportData);
            
            pViewportData->bWindowOwned = true;
        }
    }
    
    static void ImGuiRendererDestroyWindow(ImGuiViewport* pViewport)
    {
        // The main viewport (owned by the application) will always have RendererUserData == NULL since we didn't create the data for it.
        ImGuiRendererBackendData* pRendererBackend = ImGuiGetRendererBackendData();
        if (ImGuiViewportData* pViewportData = reinterpret_cast<ImGuiViewportData*>(pViewport->RendererUserData))
        {
            if (pViewportData->bWindowOwned)
            {
                SAFE_DELETE(pViewportData->pSwapChain);
                SAFE_DELETE(pViewportData->pRenderPass);
                ImGuiDestroyFramebuffers(pViewportData);
            }
            
            ImGuiDestroyWindowRenderBuffers(pViewportData);
        }
        
        pViewport->RendererUserData = nullptr;
    }
    
    static void ImGuiRendererSetWindowSize(ImGuiViewport* pViewport, ImVec2 size)
    {
        if (ImGuiViewportData* pViewportData = reinterpret_cast<ImGuiViewportData*>(pViewport->RendererUserData))
        {
            // Destroy the old framebuffers
            ImGuiDestroyFramebuffers(pViewportData);
                        
            // Resize the swapchain
            ImGuiRendererBackendData* pRendererBackend = ImGuiGetRendererBackendData();
            if (pViewportData->pSwapChain)
            {
                pViewportData->pSwapChain->Resize(size.x, size.y);
            }
            
            // Create new framebuffers
            ImGuiCreateFramebuffers(pRendererBackend->pContext, pViewportData);
        }
    }
    
    static void CreateOrResizeBuffer(Buffer** ppBuffer, VkDeviceSize size, VkBufferUsageFlags usage)
    {
        assert(ppBuffer != nullptr);
        
        // Delete the current buffer
        if (*ppBuffer)
        {
            delete *ppBuffer;
        }
        
        BufferParams bufferParams;
        bufferParams.Usage            = usage;
        bufferParams.MemoryProperties = VK_CPU_BUFFER_USAGE;
        bufferParams.Size             = size;
     
        ImGuiRendererBackendData* pRendererBackend = ImGuiGetRendererBackendData();
        Buffer* pBuffer = Buffer::Create(pRendererBackend->pContext, bufferParams, nullptr);
        if (!pBuffer)
        {
            assert(false);
            return;
        }
        
        *ppBuffer = pBuffer;
    }
    
    static void ImGuiSetupRenderState(ImDrawData* pDrawData, CommandBuffer* pCommandBuffer, GraphicsPipeline* pPipeline, const ImGuiFrameRenderData& renderData, int fbWidth, int fbHeight)
    {
        ImGuiRendererBackendData* pRendererbackend = ImGuiGetRendererBackendData();

        // Bind pipeline:
        pCommandBuffer->BindGraphicsPipelineState(pPipeline);

        // Bind Vertex And Index Buffer:
        if (pDrawData->TotalVtxCount > 0)
        {
            pCommandBuffer->BindVertexBuffer(renderData.pVertexBuffer, 0, 0);
            pCommandBuffer->BindIndexBuffer(renderData.pIndexBuffer, 0, sizeof(ImDrawIdx) == 2 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32);
        }

        // Setup viewport:
        {
            VkViewport viewport;
            viewport.x        = 0;
            viewport.y        = 0;
            viewport.width    = (float)fbWidth;
            viewport.height   = (float)fbHeight;
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            
            pCommandBuffer->SetViewport(viewport);
        }

        // Setup scale and translation:
        // Our visible imgui space lies from draw_data->DisplayPps (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right). DisplayPos is (0,0) for single viewport apps.
        {
            float scale[2];
            scale[0] = 2.0f / pDrawData->DisplaySize.x;
            scale[1] = 2.0f / pDrawData->DisplaySize.y;
            
            float translate[2];
            translate[0] = -1.0f - pDrawData->DisplayPos.x * scale[0];
            translate[1] = -1.0f - pDrawData->DisplayPos.y * scale[1];
            
            pCommandBuffer->PushConstants(pRendererbackend->pPipelineLayout, VK_SHADER_STAGE_ALL, sizeof(float) * 0, sizeof(float) * 2, scale);
            pCommandBuffer->PushConstants(pRendererbackend->pPipelineLayout, VK_SHADER_STAGE_ALL, sizeof(float) * 2, sizeof(float) * 2, translate);
        }
    }

    // Render function
    static void ImGuiRenderDrawData(ImDrawData* pDrawData, CommandBuffer* pCommandBuffer)
    {
        ImGuiRendererBackendData* pRendererbackend = ImGuiGetRendererBackendData();
        
        // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
        int fbWidth  = (int)(pDrawData->DisplaySize.x * pDrawData->FramebufferScale.x);
        int fbHeight = (int)(pDrawData->DisplaySize.y * pDrawData->FramebufferScale.y);
        if (fbWidth <= 0 || fbHeight <= 0)
        {
            return;
        }

        // Allocate array to store enough vertex/index buffers. Each unique viewport gets its own storage.
        ImGuiViewportData* pViewportData = reinterpret_cast<ImGuiViewportData*>(pDrawData->OwnerViewport->RendererUserData);
        assert(pViewportData != nullptr);
        
        const uint32_t frameIndex = pViewportData->pSwapChain->GetCurrentBackBufferIndex();
        ImGuiFrameRenderData& renderData = pViewportData->FrameData[frameIndex];
        if (pDrawData->TotalVtxCount > 0)
        {
            // Create or resize the vertex/index buffers
            const VkDeviceSize vertexSize = pDrawData->TotalVtxCount * sizeof(ImDrawVert);
            if (!renderData.pVertexBuffer || renderData.pVertexBuffer->GetSize() < vertexSize)
            {
                CreateOrResizeBuffer(&renderData.pVertexBuffer, vertexSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
            }
            
            const VkDeviceSize indexSize  = pDrawData->TotalIdxCount * sizeof(ImDrawIdx);
            if (!renderData.pIndexBuffer || renderData.pIndexBuffer->GetSize() < indexSize)
            {
                CreateOrResizeBuffer(&renderData.pIndexBuffer, indexSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
            }

            // Upload vertex/index data into a single contiguous GPU buffer
            ImDrawVert* vertexDst = reinterpret_cast<ImDrawVert*>(renderData.pVertexBuffer->Map());
            assert(vertexDst != nullptr);
            
            ImDrawIdx* indexDst = reinterpret_cast<ImDrawIdx*>(renderData.pIndexBuffer->Map());
            assert(indexDst != nullptr);

            for (int n = 0; n < pDrawData->CmdListsCount; n++)
            {
                const ImDrawList* pCmdList = pDrawData->CmdLists[n];
                memcpy(vertexDst, pCmdList->VtxBuffer.Data, pCmdList->VtxBuffer.Size * sizeof(ImDrawVert));
                vertexDst += pCmdList->VtxBuffer.Size;

                memcpy(indexDst, pCmdList->IdxBuffer.Data, pCmdList->IdxBuffer.Size * sizeof(ImDrawIdx));
                indexDst += pCmdList->IdxBuffer.Size;
            }
            
            renderData.pVertexBuffer->FlushMappedMemoryRange();
            renderData.pVertexBuffer->Unmap();
            
            renderData.pIndexBuffer->FlushMappedMemoryRange();
            renderData.pIndexBuffer->Unmap();
        }

        // Setup desired Vulkan state
        ImGuiSetupRenderState(pDrawData, pCommandBuffer, pRendererbackend->pPipeline, renderData, fbWidth, fbHeight);

        // Will project scissor/clipping rectangles into framebuffer space
        ImVec2 clipOffset = pDrawData->DisplayPos;       // (0,0) unless using multi-viewports
        ImVec2 clipScale  = pDrawData->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

        // Render command lists
        // (Because we merged all buffers into a single one, we maintain our own offset into them)
        int globalVtxOffset = 0;
        int globalIdxOffset = 0;
        for (int n = 0; n < pDrawData->CmdListsCount; n++)
        {
            const ImDrawList* pCmdList = pDrawData->CmdLists[n];
            for (int cmdIndex = 0; cmdIndex < pCmdList->CmdBuffer.Size; cmdIndex++)
            {
                const ImDrawCmd* pDrawCmd = &pCmdList->CmdBuffer[cmdIndex];
                if (pDrawCmd->UserCallback != nullptr)
                {
                    // User callback, registered via ImDrawList::AddCallback()
                    // (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
                    if (pDrawCmd->UserCallback == ImDrawCallback_ResetRenderState)
                    {
                        ImGuiSetupRenderState(pDrawData, pCommandBuffer, pRendererbackend->pPipeline, renderData, fbWidth, fbHeight);
                    }
                    else
                    {
                        pDrawCmd->UserCallback(pCmdList, pDrawCmd);
                    }
                }
                else
                {
                    // Project scissor/clipping rectangles into framebuffer space
                    ImVec2 clipMin((pDrawCmd->ClipRect.x - clipOffset.x) * clipScale.x, (pDrawCmd->ClipRect.y - clipOffset.y) * clipScale.y);
                    ImVec2 clipMax((pDrawCmd->ClipRect.z - clipOffset.x) * clipScale.x, (pDrawCmd->ClipRect.w - clipOffset.y) * clipScale.y);

                    // Clamp to viewport as vkCmdSetScissor() won't accept values that are off bounds
                    if (clipMin.x < 0.0f)
                    {
                        clipMin.x = 0.0f;
                    }
                    if (clipMin.y < 0.0f)
                    {
                        clipMin.y = 0.0f;
                    }
                    if (clipMax.x > fbWidth)
                    {
                        clipMax.x = (float)fbWidth;
                    }
                    if (clipMax.y > fbHeight)
                    {
                        clipMax.y = (float)fbHeight;
                    }
                    
                    if (clipMax.x <= clipMin.x || clipMax.y <= clipMin.y)
                    {
                        continue;
                    }

                    // Apply scissor/clipping rectangle
                    VkRect2D scissor;
                    scissor.offset.x      = (int32_t)(clipMin.x);
                    scissor.offset.y      = (int32_t)(clipMin.y);
                    scissor.extent.width  = (uint32_t)(clipMax.x - clipMin.x);
                    scissor.extent.height = (uint32_t)(clipMax.y - clipMin.y);
                    pCommandBuffer->SetScissorRect(scissor);

                    // Retrieve descriptorset
                    DescriptorSet* pDescriptorSet = (DescriptorSet*)pDrawCmd->TextureId;
                    if (sizeof(ImTextureID) < sizeof(ImU64))
                    {
                        // We don't support texture switches if ImTextureID hasn't been redefined to be 64-bit. Do a flaky check that other textures haven't been used.
                        assert(pDrawCmd->TextureId == (ImTextureID)pRendererbackend->pFontDescriptorSet);
                        pDescriptorSet = pRendererbackend->pFontDescriptorSet;
                    }
                    
                    // Bind descriptorset
                    pCommandBuffer->BindGraphicsDescriptorSet(pRendererbackend->pPipelineLayout, pDescriptorSet);

                    // Draw
                    pCommandBuffer->DrawIndexInstanced(pDrawCmd->ElemCount, 1, pDrawCmd->IdxOffset + globalIdxOffset, pDrawCmd->VtxOffset + globalVtxOffset, 0);
                }
            }
            
            globalIdxOffset += pCmdList->IdxBuffer.Size;
            globalVtxOffset += pCmdList->VtxBuffer.Size;
        }

        // Note: at this point both vkCmdSetViewport() and vkCmdSetScissor() have been called.
        // Our last values will leak into user/application rendering IF:
        // - Your app uses a pipeline with VK_DYNAMIC_STATE_VIEWPORT or VK_DYNAMIC_STATE_SCISSOR dynamic state
        // - And you forgot to call vkCmdSetViewport() and vkCmdSetScissor() yourself to explicitly set that state.
        // If you use VK_DYNAMIC_STATE_VIEWPORT or VK_DYNAMIC_STATE_SCISSOR you are responsible for setting the values before rendering.
        // In theory we should aim to backup/restore those values but I am not sure this is possible.
        // We perform a call to vkCmdSetScissor() to set back a full viewport which is likely to fix things for 99% users but technically this is not perfect. (See github #4644)

        VkRect2D scissor = { { 0, 0 }, { (uint32_t)fbWidth, (uint32_t)fbHeight } };
        pCommandBuffer->SetScissorRect(scissor);
    }
    
    static void ImGuiRendererRenderWindow(ImGuiViewport* pViewport, void*)
    {
        ImGuiViewportData* pViewportData = reinterpret_cast<ImGuiViewportData*>(pViewport->PlatformUserData);
        if (!pViewportData)
        {
            return;
        }
        
        ImGuiRendererBackendData* pRendererBackend = ImGuiGetRendererBackendData();
        SwapChain* pSwapChain = pViewportData->pSwapChain;
        const uint32_t frameIndex = pSwapChain->GetCurrentBackBufferIndex();
        
        ImGuiFrameRenderData& renderData = pViewportData->FrameData[frameIndex];
        renderData.pCommandBuffer->Reset();
        renderData.pCommandBuffer->Begin();
        
        const VkClearValue* pClearValues = (pViewport->Flags & ImGuiViewportFlags_NoRendererClear) ? nullptr : &pViewportData->ClearValues;
        uint32_t clearValueCount         = (pViewport->Flags & ImGuiViewportFlags_NoRendererClear) ? 0 : 1;
        
        Framebuffer* pFramebuffer = pViewportData->FrameBuffers[frameIndex];
        renderData.pCommandBuffer->BeginRenderPass(pViewportData->pRenderPass, pFramebuffer, pClearValues, clearValueCount);
        
        ImGuiRenderDrawData(pViewport->DrawData, renderData.pCommandBuffer);
        
        renderData.pCommandBuffer->EndRenderPass();
        renderData.pCommandBuffer->End();
        
        VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        pRendererBackend->pContext->ExecuteGraphics(renderData.pCommandBuffer, pSwapChain, &waitStage);
    }

    static void ImGuiRendererSwapBuffers(ImGuiViewport* pViewport, void*)
    {
        ImGuiViewportData* pViewportData = reinterpret_cast<ImGuiViewportData*>(pViewport->PlatformUserData);
        if (!pViewportData)
        {
            return;
        }
        
        SwapChain* pSwapChain = pViewportData->pSwapChain;
        pSwapChain->Present();
    }
    
    static void ImGuiInitRendererPlatformInterface()
    {
        ImGuiPlatformIO& platformIO = ImGui::GetPlatformIO();
        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            assert(platformIO.Platform_CreateVkSurface != nullptr);
        }
        
        platformIO.Renderer_CreateWindow  = ImGuiRendererCreateWindow;
        platformIO.Renderer_DestroyWindow = ImGuiRendererDestroyWindow;
        platformIO.Renderer_SetWindowSize = ImGuiRendererSetWindowSize;
        platformIO.Renderer_RenderWindow  = ImGuiRendererRenderWindow;
        platformIO.Renderer_SwapBuffers   = ImGuiRendererSwapBuffers;
    }
    
    static void ImguiInitRendererBackend(GLFWwindow* pWindow, VulkanContext* pContext)
    {
        ImGuiIO& io = ImGui::GetIO();
        assert(io.BackendRendererUserData == nullptr);
        
        ImGuiRendererBackendData* pRendererBackend = new ImGuiRendererBackendData();
        assert(pContext != nullptr);
        pRendererBackend->pContext = pContext;

        io.BackendRendererUserData = pRendererBackend;
        io.BackendRendererName     = "PathTracer";
        io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset; // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.
        io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports; // We can create multi-viewports on the Renderer side (optional)
        
        // Create pipelinelayout, renderpass, descriptorsetlayout, pipeline
        ImGuiCreateDeviceObjects();

        // Our render function expect RendererUserData to be storing the window render buffer we need (for the main pViewport we won't use ->Window)
        ImGuiViewport* pMainViewport = ImGui::GetMainViewport();
        pMainViewport->RendererUserData = pMainViewport->PlatformUserData; // We use the same structure for both Window- management and renderer

        if (ImGuiViewportData* pViewportData = reinterpret_cast<ImGuiViewportData*>(pMainViewport->RendererUserData))
        {
            pViewportData->pSwapChain  = pContext->GetSwapChain();
            pViewportData->pRenderPass = pRendererBackend->pRenderPass;
            
            ImGuiCreateFramebuffers(pContext, pViewportData);
            ImGuiCreateWindowRenderBuffers(pContext, pViewportData);
        }
        
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGuiInitRendererPlatformInterface();
        }
        
        // Create texture, descriptorset etc.
        ImGuiCreateFontsTexture();
    }
    
    static void ImGuiUpdateMouseData()
    {
        ImGuiBackendData* pBackend = ImGuiGetBackendData();

        ImGuiIO& io = ImGui::GetIO();
        if (glfwGetInputMode(pBackend->Window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
        {
            io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);
            return;
        }

        ImGuiID mouseViewportID = 0;
        const ImVec2 mousePosPrev = io.MousePos;
        
        ImGuiPlatformIO& platformIO = ImGui::GetPlatformIO();
        for (int n = 0; n < platformIO.Viewports.Size; n++)
        {
            ImGuiViewport* pViewport = platformIO.Viewports[n];
            GLFWwindow*    pWindow   = (GLFWwindow*)pViewport->PlatformHandle;

            const bool bIsWindowFocused = glfwGetWindowAttrib(pWindow, GLFW_FOCUSED) != 0;
            if (bIsWindowFocused)
            {
                // (Optional) Set OS mouse position from Dear ImGui if requested (rarely used, only when ImGuiConfigFlags_NavEnableSetMousePos is enabled by user)
                // When multi-viewports are enabled, all Dear ImGui positions are same as OS positions.
                if (io.WantSetMousePos)
                {
                    glfwSetCursorPos(pWindow, (double)(mousePosPrev.x - pViewport->Pos.x), (double)(mousePosPrev.y - pViewport->Pos.y));
                }

                // (Optional) Fallback to provide mouse position when focused (ImGui_ImplGlfw_CursorPosCallback already provides this when hovered or captured)
                if (pBackend->MouseWindow == nullptr)
                {
                    double mouseX;
                    double mouseY;
                    glfwGetCursorPos(pWindow, &mouseX, &mouseY);
                    
                    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
                    {
                        // Single viewport mode: mouse position in client window coordinates (io.MousePos is (0,0) when the mouse is on the upper-left corner of the app window)
                        // Multi-viewport mode: mouse position in OS absolute coordinates (io.MousePos is (0,0) when the mouse is on the upper-left of the primary monitor)
                        int window_x;
                        int window_y;
                        glfwGetWindowPos(pWindow, &window_x, &window_y);
                        
                        mouseX += window_x;
                        mouseY += window_y;
                    }
                    
                    pBackend->LastValidMousePos = ImVec2((float)mouseX, (float)mouseY);
                    io.AddMousePosEvent((float)mouseX, (float)mouseY);
                }
            }

            // (Optional) When using multiple viewports: call io.AddMouseViewportEvent() with the viewport the OS mouse cursor is hovering.
            // If ImGuiBackendFlags_HasMouseHoveredViewport is not set by the backend, Dear imGui will ignore this field and infer the information using its flawed heuristic.
            // - [X] GLFW >= 3.3 backend ON WINDOWS ONLY does correctly ignore viewports with the _NoInputs flag.
            // - [!] GLFW <= 3.2 backend CANNOT correctly ignore viewports with the _NoInputs flag, and CANNOT reported Hovered Viewport because of mouse capture.
            //       Some backend are not able to handle that correctly. If a backend report an hovered viewport that has the _NoInputs flag (e.g. when dragging a window
            //       for docking, the viewport has the _NoInputs flag in order to allow us to find the viewport under), then Dear ImGui is forced to ignore the value reported
            //       by the backend, and use its flawed heuristic to guess the viewport behind.
            // - [X] GLFW backend correctly reports this regardless of another viewport behind focused and dragged from (we need this to find a useful drag and drop target).

            const bool bWindowNoInput = (pViewport->Flags & ImGuiViewportFlags_NoInputs) != 0;
            glfwSetWindowAttrib(pWindow, GLFW_MOUSE_PASSTHROUGH, bWindowNoInput);
            if (glfwGetWindowAttrib(pWindow, GLFW_HOVERED) && !bWindowNoInput)
            {
                mouseViewportID = pViewport->ID;
            }
        }

        if (io.BackendFlags & ImGuiBackendFlags_HasMouseHoveredViewport)
        {
            io.AddMouseViewportEvent(mouseViewportID);
        }
    }

    static void ImGuiUpdateMouseCursor()
    {
        ImGuiIO& io = ImGui::GetIO();
        
        ImGuiBackendData* pBackend = ImGuiGetBackendData();
        if ((io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange) || glfwGetInputMode(pBackend->Window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
        {
            return;
        }

        ImGuiMouseCursor imguiCursor = ImGui::GetMouseCursor();
        ImGuiPlatformIO& platformIO  = ImGui::GetPlatformIO();
        for (int n = 0; n < platformIO.Viewports.Size; n++)
        {
            GLFWwindow* window = (GLFWwindow*)platformIO.Viewports[n]->PlatformHandle;
            if (imguiCursor == ImGuiMouseCursor_None || io.MouseDrawCursor)
            {
                // Hide OS mouse cursor if imgui is drawing it or if it wants no cursor
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
            }
            else
            {
                // Show OS mouse cursor
                // FIXME-PLATFORM: Unfocused windows seems to fail changing the mouse cursor with GLFW 3.2, but 3.3 works here.
                glfwSetCursor(window, pBackend->MouseCursors[imguiCursor] ? pBackend->MouseCursors[imguiCursor] : pBackend->MouseCursors[ImGuiMouseCursor_Arrow]);
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            }
        }
    }

    // Update gamepad inputs
    static inline float Saturate(float v) 
    {
        return v < 0.0f ? 0.0f : v  > 1.0f ? 1.0f : v;
    }

    static void ImGuiUpdateGamepads()
    {
        ImGuiIO& io = ImGui::GetIO();
        if ((io.ConfigFlags & ImGuiConfigFlags_NavEnableGamepad) == 0) // FIXME: Technically feeding gamepad shouldn't depend on this now that they are regular inputs.
        {
            return;
        }

        io.BackendFlags &= ~ImGuiBackendFlags_HasGamepad;

        GLFWgamepadstate gamepad;
        if (!glfwGetGamepadState(GLFW_JOYSTICK_1, &gamepad))
        {
            return;
        }
        
    #define MAP_BUTTON(KEY_NO, BUTTON_NO, _UNUSED) \
        do \
        { \
            io.AddKeyEvent(KEY_NO, gamepad.buttons[BUTTON_NO] != 0); \
        } while (0)
        
    #define MAP_ANALOG(KEY_NO, AXIS_NO, _UNUSED, V0, V1) \
        do \
        { \
            float v = gamepad.axes[AXIS_NO]; \
            v = (v - V0) / (V1 - V0); \
            io.AddKeyAnalogEvent(KEY_NO, v > 0.10f, Saturate(v)); \
        } while (0)

        io.BackendFlags |= ImGuiBackendFlags_HasGamepad;
        MAP_BUTTON(ImGuiKey_GamepadStart,       GLFW_GAMEPAD_BUTTON_START,          7);
        MAP_BUTTON(ImGuiKey_GamepadBack,        GLFW_GAMEPAD_BUTTON_BACK,           6);
        MAP_BUTTON(ImGuiKey_GamepadFaceLeft,    GLFW_GAMEPAD_BUTTON_X,              2);     // Xbox X, PS Square
        MAP_BUTTON(ImGuiKey_GamepadFaceRight,   GLFW_GAMEPAD_BUTTON_B,              1);     // Xbox B, PS Circle
        MAP_BUTTON(ImGuiKey_GamepadFaceUp,      GLFW_GAMEPAD_BUTTON_Y,              3);     // Xbox Y, PS Triangle
        MAP_BUTTON(ImGuiKey_GamepadFaceDown,    GLFW_GAMEPAD_BUTTON_A,              0);     // Xbox A, PS Cross
        MAP_BUTTON(ImGuiKey_GamepadDpadLeft,    GLFW_GAMEPAD_BUTTON_DPAD_LEFT,      13);
        MAP_BUTTON(ImGuiKey_GamepadDpadRight,   GLFW_GAMEPAD_BUTTON_DPAD_RIGHT,     11);
        MAP_BUTTON(ImGuiKey_GamepadDpadUp,      GLFW_GAMEPAD_BUTTON_DPAD_UP,        10);
        MAP_BUTTON(ImGuiKey_GamepadDpadDown,    GLFW_GAMEPAD_BUTTON_DPAD_DOWN,      12);
        MAP_BUTTON(ImGuiKey_GamepadL1,          GLFW_GAMEPAD_BUTTON_LEFT_BUMPER,    4);
        MAP_BUTTON(ImGuiKey_GamepadR1,          GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER,   5);
        MAP_ANALOG(ImGuiKey_GamepadL2,          GLFW_GAMEPAD_AXIS_LEFT_TRIGGER,     4,      -0.75f,  +1.0f);
        MAP_ANALOG(ImGuiKey_GamepadR2,          GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER,    5,      -0.75f,  +1.0f);
        MAP_BUTTON(ImGuiKey_GamepadL3,          GLFW_GAMEPAD_BUTTON_LEFT_THUMB,     8);
        MAP_BUTTON(ImGuiKey_GamepadR3,          GLFW_GAMEPAD_BUTTON_RIGHT_THUMB,    9);
        MAP_ANALOG(ImGuiKey_GamepadLStickLeft,  GLFW_GAMEPAD_AXIS_LEFT_X,           0,      -0.25f,  -1.0f);
        MAP_ANALOG(ImGuiKey_GamepadLStickRight, GLFW_GAMEPAD_AXIS_LEFT_X,           0,      +0.25f,  +1.0f);
        MAP_ANALOG(ImGuiKey_GamepadLStickUp,    GLFW_GAMEPAD_AXIS_LEFT_Y,           1,      -0.25f,  -1.0f);
        MAP_ANALOG(ImGuiKey_GamepadLStickDown,  GLFW_GAMEPAD_AXIS_LEFT_Y,           1,      +0.25f,  +1.0f);
        MAP_ANALOG(ImGuiKey_GamepadRStickLeft,  GLFW_GAMEPAD_AXIS_RIGHT_X,          2,      -0.25f,  -1.0f);
        MAP_ANALOG(ImGuiKey_GamepadRStickRight, GLFW_GAMEPAD_AXIS_RIGHT_X,          2,      +0.25f,  +1.0f);
        MAP_ANALOG(ImGuiKey_GamepadRStickUp,    GLFW_GAMEPAD_AXIS_RIGHT_Y,          3,      -0.25f,  -1.0f);
        MAP_ANALOG(ImGuiKey_GamepadRStickDown,  GLFW_GAMEPAD_AXIS_RIGHT_Y,          3,      +0.25f,  +1.0f);

    #undef MAP_BUTTON
    #undef MAP_ANALOG
    }

    /*///////////////////////////////////////////////////////////////////////////////////////////*/
    /* Public API */
    
    void InitializeImgui(GLFWwindow* pWindow, VulkanContext* pContext)
    {
        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        // General ImGui config
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // Enable Docking
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;   // Enable Multi-Viewport / Platform Windows

        // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls
        // io.ConfigViewportsNoAutoMerge   = true;
        // io.ConfigViewportsNoTaskBarIcon = true;

        // Application Backend
        ImguiInitBackend(pWindow);

        // Renderer Backend
        ImguiInitRendererBackend(pWindow, pContext);
        
        // Setup Dear ImGui style
        ImGui::StyleColorsDark();

        // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
        ImGuiStyle& style = ImGui::GetStyle();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            style.WindowRounding = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }
    }
    
    void TickImGui()
    {
        ImGuiBackendData* pBackend = ImGuiGetBackendData();
        assert(pBackend != nullptr);

        // Setup display size (every frame to accommodate for window resizing)
        int width;
        int height;
        glfwGetWindowSize(pBackend->Window, &width, &height);
        
        int displayWidth;
        int displayHeight;
        glfwGetFramebufferSize(pBackend->Window, &displayWidth, &displayHeight);

        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2((float)width, (float)height);
        if (width > 0 && height > 0)
        {
            io.DisplayFramebufferScale = ImVec2((float)displayWidth / (float)width, (float)displayHeight / (float)height);
        }
        
        if (pBackend->bWantUpdateMonitors)
        {
            ImGuiUpdateMonitors();
        }

        // Setup time step
        double currentTime = glfwGetTime();
        io.DeltaTime = pBackend->Time > 0.0 ? (float)(currentTime - pBackend->Time) : (float)(1.0f / 60.0f);
        pBackend->Time = currentTime;

        ImGuiUpdateMouseData();
        ImGuiUpdateMouseCursor();

        // Update game controllers (if enabled and available)
        ImGuiUpdateGamepads();
        
        // Call new frame last
        ImGui::NewFrame();
    }
    
    void RenderImGui()
    {
        ImGui::Render();
        
        // Render the main window
        ImGuiViewport* pMainViewport = ImGui::GetMainViewport();
        ImGuiRendererRenderWindow(pMainViewport, nullptr);
        
        // Update and Render additional Platform Windows
        ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }
    }
    
    void ReleaseImGui()
    {
        ImGuiBackendData* pBackend = ImGuiGetBackendData();
        assert(pBackend != nullptr);
        
        ImGuiRendererBackendData* pRendererBackend = ImGuiGetRendererBackendData();
        assert(pRendererBackend != nullptr);
        
        ImGui::DestroyPlatformWindows();
        
        if (pBackend->bInstalledCallbacks)
        {
            ImGuiRestoreCallbacks(pBackend->Window);
        }

        for (ImGuiMouseCursor n = 0; n < ImGuiMouseCursor_COUNT; n++)
        {
            glfwDestroyCursor(pBackend->MouseCursors[n]);
        }

        ImGuiIO& io = ImGui::GetIO();
        io.BackendPlatformName     = nullptr;
        io.BackendPlatformUserData = nullptr;
        SAFE_DELETE(pBackend);

        io.BackendRendererName     = nullptr;
        io.BackendRendererUserData = nullptr;
        SAFE_DELETE(pRendererBackend);
        
        // Destroy the context last
        ImGui::DestroyContext();
    }
}
