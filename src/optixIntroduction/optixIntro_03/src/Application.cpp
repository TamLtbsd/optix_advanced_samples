/* 
 * Copyright (c) 2013-2018, NVIDIA CORPORATION. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of NVIDIA CORPORATION nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "shaders/app_config.h"

#include "inc/Application.h"

#include <optix.h>
#include <optixu/optixpp_namespace.h>
#include <optixu/optixu_math_namespace.h>
#include <optixu/optixu_matrix_namespace.h>

#include <algorithm>
#include <cstring>
#include <iostream>
#include <sstream>

// DAR Only for sutil::samplesPTXDir() and sutil::writeBufferToFile()
#include <sutil.h>

#include "inc/MyAssert.h"

const char* const SAMPLE_NAME = "optixIntro_03";

// This only runs inside the OptiX Advanced Samples location,
// unless the environment variable OPTIX_SAMPLES_SDK_PTX_DIR is set.
// A standalone application which should run anywhere would place the *.ptx files 
// into a subdirectory next to the executable and use a relative file path here!
static std::string ptxPath(std::string const& cuda_file)
{
  return std::string(sutil::samplesPTXDir()) + std::string("/") + 
         std::string(SAMPLE_NAME) + std::string("_generated_") + cuda_file + std::string(".ptx");
}


Application::Application(GLFWwindow* window,
                         const int width,
                         const int height,
                         const unsigned int devices, 
                         const unsigned int stackSize,
                         const bool interop)
: m_window(window)
, m_width(width)
, m_height(height)
, m_devicesEncoding(devices)
, m_stackSize(stackSize)
, m_interop(interop)
{
  // Setup ImGui binding.
  ImGui::CreateContext();
  ImGui_ImplGlfwGL2_Init(window, true);

  // This initializes the GLFW part including the font texture.
  ImGui_ImplGlfwGL2_NewFrame();
  ImGui::EndFrame();

  ImGuiStyle& style = ImGui::GetStyle();
  
  // Style the GUI colors to a neutral greyscale with plenty of transaparency to concentrate on the image.
  // Change these RGB values to get any other tint.
  const float r = 1.0f;
  const float g = 1.0f;
  const float b = 1.0f;
  
  style.Colors[ImGuiCol_Text]                  = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
  style.Colors[ImGuiCol_TextDisabled]          = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
  style.Colors[ImGuiCol_WindowBg]              = ImVec4(r * 0.2f, g * 0.2f, b * 0.2f, 0.6f);
  style.Colors[ImGuiCol_ChildWindowBg]         = ImVec4(r * 0.2f, g * 0.2f, b * 0.2f, 1.0f);
  style.Colors[ImGuiCol_PopupBg]               = ImVec4(r * 0.2f, g * 0.2f, b * 0.2f, 1.0f);
  style.Colors[ImGuiCol_Border]                = ImVec4(r * 0.4f, g * 0.4f, b * 0.4f, 0.4f);
  style.Colors[ImGuiCol_BorderShadow]          = ImVec4(r * 0.0f, g * 0.0f, b * 0.0f, 0.4f);
  style.Colors[ImGuiCol_FrameBg]               = ImVec4(r * 0.4f, g * 0.4f, b * 0.4f, 0.4f);
  style.Colors[ImGuiCol_FrameBgHovered]        = ImVec4(r * 0.6f, g * 0.6f, b * 0.6f, 0.6f);
  style.Colors[ImGuiCol_FrameBgActive]         = ImVec4(r * 0.8f, g * 0.8f, b * 0.8f, 0.8f);
  style.Colors[ImGuiCol_TitleBg]               = ImVec4(r * 0.6f, g * 0.6f, b * 0.6f, 0.6f);
  style.Colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(r * 0.2f, g * 0.2f, b * 0.2f, 0.2f);
  style.Colors[ImGuiCol_TitleBgActive]         = ImVec4(r * 0.8f, g * 0.8f, b * 0.8f, 0.8f);
  style.Colors[ImGuiCol_MenuBarBg]             = ImVec4(r * 0.2f, g * 0.2f, b * 0.2f, 1.0f);
  style.Colors[ImGuiCol_ScrollbarBg]           = ImVec4(r * 0.2f, g * 0.2f, b * 0.2f, 0.2f);
  style.Colors[ImGuiCol_ScrollbarGrab]         = ImVec4(r * 0.4f, g * 0.4f, b * 0.4f, 0.4f);
  style.Colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(r * 0.6f, g * 0.6f, b * 0.6f, 0.6f);
  style.Colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(r * 0.8f, g * 0.8f, b * 0.8f, 0.8f);
  style.Colors[ImGuiCol_CheckMark]             = ImVec4(r * 0.8f, g * 0.8f, b * 0.8f, 0.8f);
  style.Colors[ImGuiCol_SliderGrab]            = ImVec4(r * 0.4f, g * 0.4f, b * 0.4f, 0.4f);
  style.Colors[ImGuiCol_SliderGrabActive]      = ImVec4(r * 0.8f, g * 0.8f, b * 0.8f, 0.8f);
  style.Colors[ImGuiCol_Button]                = ImVec4(r * 0.4f, g * 0.4f, b * 0.4f, 0.4f);
  style.Colors[ImGuiCol_ButtonHovered]         = ImVec4(r * 0.6f, g * 0.6f, b * 0.6f, 0.6f);
  style.Colors[ImGuiCol_ButtonActive]          = ImVec4(r * 0.8f, g * 0.8f, b * 0.8f, 0.8f);
  style.Colors[ImGuiCol_Header]                = ImVec4(r * 0.4f, g * 0.4f, b * 0.4f, 0.4f);
  style.Colors[ImGuiCol_HeaderHovered]         = ImVec4(r * 0.6f, g * 0.6f, b * 0.6f, 0.6f);
  style.Colors[ImGuiCol_HeaderActive]          = ImVec4(r * 0.8f, g * 0.8f, b * 0.8f, 0.8f);
  style.Colors[ImGuiCol_Column]                = ImVec4(r * 0.4f, g * 0.4f, b * 0.4f, 0.4f);
  style.Colors[ImGuiCol_ColumnHovered]         = ImVec4(r * 0.6f, g * 0.6f, b * 0.6f, 0.6f);
  style.Colors[ImGuiCol_ColumnActive]          = ImVec4(r * 0.8f, g * 0.8f, b * 0.8f, 0.8f);
  style.Colors[ImGuiCol_ResizeGrip]            = ImVec4(r * 0.6f, g * 0.6f, b * 0.6f, 0.6f);
  style.Colors[ImGuiCol_ResizeGripHovered]     = ImVec4(r * 0.8f, g * 0.8f, b * 0.8f, 0.8f);
  style.Colors[ImGuiCol_ResizeGripActive]      = ImVec4(r * 1.0f, g * 1.0f, b * 1.0f, 1.0f);
  style.Colors[ImGuiCol_CloseButton]           = ImVec4(r * 0.4f, g * 0.4f, b * 0.4f, 0.4f);
  style.Colors[ImGuiCol_CloseButtonHovered]    = ImVec4(r * 0.6f, g * 0.6f, b * 0.6f, 0.6f);
  style.Colors[ImGuiCol_CloseButtonActive]     = ImVec4(r * 0.8f, g * 0.8f, b * 0.8f, 0.8f);
  style.Colors[ImGuiCol_PlotLines]             = ImVec4(r * 0.8f, g * 0.8f, b * 0.8f, 1.0f);
  style.Colors[ImGuiCol_PlotLinesHovered]      = ImVec4(r * 1.0f, g * 1.0f, b * 1.0f, 1.0f);
  style.Colors[ImGuiCol_PlotHistogram]         = ImVec4(r * 0.8f, g * 0.8f, b * 0.8f, 1.0f);
  style.Colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(r * 1.0f, g * 1.0f, b * 1.0f, 1.0f);
  style.Colors[ImGuiCol_TextSelectedBg]        = ImVec4(r * 0.5f, g * 0.5f, b * 0.5f, 1.0f);
  style.Colors[ImGuiCol_ModalWindowDarkening]  = ImVec4(r * 0.2f, g * 0.2f, b * 0.2f, 0.2f);
  style.Colors[ImGuiCol_DragDropTarget]        = ImVec4(r * 1.0f, g * 1.0f, b * 0.0f, 1.0f); // Yellow
  style.Colors[ImGuiCol_NavHighlight]          = ImVec4(r * 1.0f, g * 1.0f, b * 1.0f, 1.0f);
  style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(r * 1.0f, g * 1.0f, b * 1.0f, 1.0f);

  // Renderer setup and GUI parameters.
  m_builder = std::string("Trbvh");
  
  // GLSL shaders objects and program. 
  m_glslVS      = 0;
  m_glslFS      = 0;
  m_glslProgram = 0;

  m_guiState = GUI_STATE_NONE;

  m_isWindowVisible = true;

  m_mouseSpeedRatio = 10.0f;

  m_pinholeCamera.setViewport(m_width, m_height);

  initOpenGL();
  initOptiX(); // Sets m_isValid when OptiX initialization was successful.
}

Application::~Application()
{
  // DAR FIXME Do any other destruction here.
  if (m_isValid)
  {
    m_context->destroy();
  }

  ImGui_ImplGlfwGL2_Shutdown();
  ImGui::DestroyContext();
}

bool Application::isValid() const
{
  return m_isValid;
}

void Application::reshape(int width, int height)
{
  if ((width != 0 && height != 0) && // Zero sized interop buffers are not allowed in OptiX.
      (m_width != width || m_height != height))
  {
    m_width  = width;
    m_height = height;

    glViewport(0, 0, m_width, m_height);
    try
    {
      m_bufferOutput->setSize(m_width, m_height); // RGBA32F buffer.

      if (m_interop)
      {
        m_bufferOutput->unregisterGLBuffer(); // Must unregister or CUDA won't notice the size change and crash.
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_bufferOutput->getGLBOId());
        glBufferData(GL_PIXEL_UNPACK_BUFFER, m_bufferOutput->getElementSize() * m_width * m_height, nullptr, GL_STREAM_DRAW);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
        m_bufferOutput->registerGLBuffer();
      }
    }
    catch(optix::Exception& e)
    {
      std::cerr << e.getErrorString() << std::endl;
    }

    m_pinholeCamera.setViewport(m_width, m_height);
  }
}

void Application::guiNewFrame()
{
  ImGui_ImplGlfwGL2_NewFrame();
}

void Application::guiReferenceManual()
{
  ImGui::ShowTestWindow();
}

void Application::guiRender()
{
  ImGui::Render();
  ImGui_ImplGlfwGL2_RenderDrawData(ImGui::GetDrawData());
}


void Application::getSystemInformation()
{
  unsigned int optixVersion;
  RT_CHECK_ERROR_NO_CONTEXT(rtGetVersion(&optixVersion));

  unsigned int major = optixVersion / 1000; // Check major with old formula.
  unsigned int minor;
  unsigned int micro;
  if (3 < major) // New encoding since OptiX 4.0.0 to get two digits micro numbers?
  {
    major =  optixVersion / 10000;
    minor = (optixVersion % 10000) / 100;
    micro =  optixVersion % 100;
  }
  else // Old encoding with only one digit for the micro number.
  {
    minor = (optixVersion % 1000) / 10;
    micro =  optixVersion % 10;
  }
  std::cout << "OptiX " << major << "." << minor << "." << micro << std::endl;
  
  unsigned int numberOfDevices = 0;
  RT_CHECK_ERROR_NO_CONTEXT(rtDeviceGetDeviceCount(&numberOfDevices));
  std::cout << "Number of Devices = " << numberOfDevices << std::endl << std::endl;

  for (unsigned int i = 0; i < numberOfDevices; ++i)
  {
    char name[256];
    RT_CHECK_ERROR_NO_CONTEXT(rtDeviceGetAttribute(i, RT_DEVICE_ATTRIBUTE_NAME, sizeof(name), name));
    std::cout << "Device " << i << ": " << name << std::endl;
  
    int computeCapability[2] = {0, 0};
    RT_CHECK_ERROR_NO_CONTEXT(rtDeviceGetAttribute(i, RT_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY, sizeof(computeCapability), &computeCapability));
    std::cout << "  Compute Support: " << computeCapability[0] << "." << computeCapability[1] << std::endl;

    RTsize totalMemory = 0;
    RT_CHECK_ERROR_NO_CONTEXT(rtDeviceGetAttribute(i, RT_DEVICE_ATTRIBUTE_TOTAL_MEMORY, sizeof(totalMemory), &totalMemory));
    std::cout << "  Total Memory: " << (unsigned long long) totalMemory << std::endl;

    int clockRate = 0;
    RT_CHECK_ERROR_NO_CONTEXT(rtDeviceGetAttribute(i, RT_DEVICE_ATTRIBUTE_CLOCK_RATE, sizeof(clockRate), &clockRate));
    std::cout << "  Clock Rate: " << clockRate << " kHz" << std::endl;

    int maxThreadsPerBlock = 0;
    RT_CHECK_ERROR_NO_CONTEXT(rtDeviceGetAttribute(i, RT_DEVICE_ATTRIBUTE_MAX_THREADS_PER_BLOCK, sizeof(maxThreadsPerBlock), &maxThreadsPerBlock));
    std::cout << "  Max. Threads per Block: " << maxThreadsPerBlock << std::endl;

    int smCount = 0;
    RT_CHECK_ERROR_NO_CONTEXT(rtDeviceGetAttribute(i, RT_DEVICE_ATTRIBUTE_MULTIPROCESSOR_COUNT, sizeof(smCount), &smCount));
    std::cout << "  Streaming Multiprocessor Count: " << smCount << std::endl;

    int executionTimeoutEnabled = 0;
    RT_CHECK_ERROR_NO_CONTEXT(rtDeviceGetAttribute(i, RT_DEVICE_ATTRIBUTE_EXECUTION_TIMEOUT_ENABLED, sizeof(executionTimeoutEnabled), &executionTimeoutEnabled));
    std::cout << "  Execution Timeout Enabled: " << executionTimeoutEnabled << std::endl;

    int maxHardwareTextureCount = 0 ;
    RT_CHECK_ERROR_NO_CONTEXT(rtDeviceGetAttribute(i, RT_DEVICE_ATTRIBUTE_MAX_HARDWARE_TEXTURE_COUNT, sizeof(maxHardwareTextureCount), &maxHardwareTextureCount));
    std::cout << "  Max. Hardware Texture Count: " << maxHardwareTextureCount << std::endl;
 
    int tccDriver = 0;
    RT_CHECK_ERROR_NO_CONTEXT(rtDeviceGetAttribute(i, RT_DEVICE_ATTRIBUTE_TCC_DRIVER, sizeof(tccDriver), &tccDriver));
    std::cout << "  TCC Driver enabled: " << tccDriver << std::endl;
 
    int cudaDeviceOrdinal = 0;
    RT_CHECK_ERROR_NO_CONTEXT(rtDeviceGetAttribute(i, RT_DEVICE_ATTRIBUTE_CUDA_DEVICE_ORDINAL, sizeof(cudaDeviceOrdinal), &cudaDeviceOrdinal));
    std::cout << "  CUDA Device Ordinal: " << cudaDeviceOrdinal << std::endl << std::endl;
  }
}

void Application::initOpenGL()
{
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

  glViewport(0, 0, m_width, m_height);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  if (m_interop)
  {
    // PBO for the fast OptiX sysOutputBuffer to texture transfer.
    glGenBuffers(1, &m_pboOutputBuffer);
    MY_ASSERT(m_pboOutputBuffer != 0); 
    // Buffer size must be > 0 or OptiX can't create a buffer from it.
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_pboOutputBuffer);
    glBufferData(GL_PIXEL_UNPACK_BUFFER, m_width * m_height * sizeof(float) * 4, (void*) 0, GL_STREAM_READ); // RGBA32F from byte offset 0 in the pixel unpack buffer.
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
  }

  // glPixelStorei(GL_UNPACK_ALIGNMENT, 4); // default, works for BGRA8, RGBA16F, and RGBA32F.

  glGenTextures(1, &m_hdrTexture);
  MY_ASSERT(m_hdrTexture != 0);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, m_hdrTexture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glBindTexture(GL_TEXTURE_2D, 0);

  // DAR ImGui has been changed to push the GL_TEXTURE_BIT so that this works. 
  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

  initGLSL();
}


void Application::initOptiX()
{
  try
  {
    getSystemInformation();

    m_context = optix::Context::create();

    // Select the GPUs to use with this context.
    unsigned int numberOfDevices = 0;
    RT_CHECK_ERROR_NO_CONTEXT(rtDeviceGetDeviceCount(&numberOfDevices));
    std::cout << "Number of Devices = " << numberOfDevices << std::endl << std::endl;

    std::vector<int> devices;

    int devicesEncoding = m_devicesEncoding; // Preserve this information, it can be stored in the system file.
    unsigned int i = 0;
    do 
    {
      int device = devicesEncoding % 10;
      devices.push_back(device); // DAR FIXME Should be a std::set to prevent duplicate device IDs in m_devicesEncoding.
      devicesEncoding /= 10;
      ++i;
    } while (i < numberOfDevices && devicesEncoding);

    m_context->setDevices(devices.begin(), devices.end());
    
    // Print out the current configuration to make sure what's currently running.
    devices = m_context->getEnabledDevices();
    for (size_t i = 0; i < devices.size(); ++i) 
    {
      std::cout << "m_context is using local device " << devices[i] << ": " << m_context->getDeviceName(devices[i]) << std::endl;
    }
    std::cout << "OpenGL interop is " << ((m_interop) ? "enabled" : "disabled") << std::endl;

    initPrograms();
    initRenderer(); 
    initScene();

    m_isValid = true; // If we get here with no exception, flag the initialization as successful. Otherwise the app will exit with error message.
  }
  catch(optix::Exception& e)
  {
    std::cerr << e.getErrorString() << std::endl;
  }
}

void Application::initRenderer() 
{
  try
  {
    m_context->setEntryPointCount(1); // 0 = render
    m_context->setRayTypeCount(1);    // 0 = radiance
    
    m_context->setStackSize(m_stackSize);
    std::cout << "stackSize = " << m_stackSize << std::endl;

#if USE_DEBUG_EXCEPTIONS
    // Disable this by default for performance, otherwise the stitched PTX code will have lots of exception handling inside. 
    m_context->setPrintEnabled(true);
    //m_context->setPrintLaunchIndex(256, 256);
    m_context->setExceptionEnabled(RT_EXCEPTION_ALL, true);
#endif 

    // Add context-global variables here.
  
    // RT_BUFFER_INPUT_OUTPUT to support accumulation.
    // (In case of an OpenGL interop buffer, that is automatically registered with CUDA now! Must unregister/register around size changes.)
    m_bufferOutput = (m_interop) ? m_context->createBufferFromGLBO(RT_BUFFER_INPUT_OUTPUT, m_pboOutputBuffer)
                                 : m_context->createBuffer(RT_BUFFER_INPUT_OUTPUT);
    m_bufferOutput->setFormat(RT_FORMAT_FLOAT4); // RGBA32F
    m_bufferOutput->setSize(m_width, m_height);

    m_context["sysOutputBuffer"]->set(m_bufferOutput);

    std::map<std::string, optix::Program>::const_iterator it = m_mapOfPrograms.find("raygeneration");
    MY_ASSERT(it != m_mapOfPrograms.end()); 
    m_context->setRayGenerationProgram(0, it->second); // entrypoint

    it = m_mapOfPrograms.find("exception");
    MY_ASSERT(it != m_mapOfPrograms.end()); 
    m_context->setExceptionProgram(0, it->second); // entrypoint

    it = m_mapOfPrograms.find("miss");
    MY_ASSERT(it != m_mapOfPrograms.end()); 
    m_context->setMissProgram(0, it->second); // raytype

    // Default initialization. Will be overwritten on the first frame.
    m_context["sysCameraPosition"]->setFloat(0.0f, 0.0f, 1.0f);
    m_context["sysCameraU"]->setFloat(1.0f, 0.0f, 0.0f);
    m_context["sysCameraV"]->setFloat(0.0f, 1.0f, 0.0f);
    m_context["sysCameraW"]->setFloat(0.0f, 0.0f, -1.0f);
  }
  catch(optix::Exception& e)
  {
    std::cerr << e.getErrorString() << std::endl;
  }
}


void Application::initScene()
{
  try
  {
    m_timer.restart();
    const double timeInit = m_timer.getTime();

    std::cout << "createScene()" << std::endl;
    createScene();
    const double timeScene = m_timer.getTime();

    std::cout << "m_context->validate()" << std::endl;
    m_context->validate();
    const double timeValidate = m_timer.getTime();

    std::cout << "m_context->launch()" << std::endl;
    m_context->launch(0, 0, 0); // Dummy launch to build everything (entrypoint, width, height)
    const double timeLaunch = m_timer.getTime();

    std::cout << "initScene(): " << timeLaunch - timeInit << " seconds overall" << std::endl;
    std::cout << "{" << std::endl;
    std::cout << "  createScene() = " << timeScene    - timeInit     << " seconds" << std::endl;
    std::cout << "  validate()    = " << timeValidate - timeScene    << " seconds" << std::endl;
    std::cout << "  launch()      = " << timeLaunch   - timeValidate << " seconds" << std::endl;
    std::cout << "}" << std::endl;
  }
  catch(optix::Exception& e)
  {
    std::cerr << e.getErrorString() << std::endl;
  }
}

bool Application::render()
{
  bool repaint = false;

  try
  {
    optix::float3 cameraPosition;
    optix::float3 cameraU;
    optix::float3 cameraV;
    optix::float3 cameraW;

    bool cameraChanged = m_pinholeCamera.getFrustum(cameraPosition, cameraU, cameraV, cameraW);
    if (cameraChanged)
    {
      m_context["sysCameraPosition"]->setFloat(cameraPosition);
      m_context["sysCameraU"]->setFloat(cameraU);
      m_context["sysCameraV"]->setFloat(cameraV);
      m_context["sysCameraW"]->setFloat(cameraW);
    }
  
    m_context->launch(0, m_width, m_height);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_hdrTexture);

    if (m_interop) 
    {
      glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_bufferOutput->getGLBOId());
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, (GLsizei) m_width, (GLsizei) m_height, 0, GL_RGBA, GL_FLOAT, (void*) 0); // RGBA32F from byte offset 0 in the pixel unpack buffer.
      glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    }
    else
    {
      const void* data = m_bufferOutput->map(0, RT_BUFFER_MAP_READ);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, (GLsizei) m_width, (GLsizei) m_height, 0, GL_RGBA, GL_FLOAT, data); // RGBA32F
      m_bufferOutput->unmap();
    }

    repaint = true; // Indicate that there is a new image.
  }
  catch(optix::Exception& e)
  {
    std::cerr << e.getErrorString() << std::endl;
  }
  return repaint;
}

void Application::display()
{
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, m_hdrTexture);

  glUseProgram(m_glslProgram);

  glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(-1.0f, -1.0f);
    glTexCoord2f(1.0f, 0.0f);
    glVertex2f(1.0f, -1.0f);
    glTexCoord2f(1.0f, 1.0f);
    glVertex2f(1.0f, 1.0f);
    glTexCoord2f(0.0f, 1.0f);
    glVertex2f(-1.0f, 1.0f);
  glEnd();

  glUseProgram(0);
}

void Application::screenshot(std::string const& filename)
{
  sutil::writeBufferToFile(filename.c_str(), m_bufferOutput);
  std::cerr << "Wrote " << filename << std::endl;
}

// Helper functions:
void Application::checkInfoLog(const char *msg, GLuint object)
{
  GLint maxLength;
  GLint length;
  GLchar *infoLog;

  if (glIsProgram(object))
  {
    glGetProgramiv(object, GL_INFO_LOG_LENGTH, &maxLength);
  }
  else
  {
    glGetShaderiv(object, GL_INFO_LOG_LENGTH, &maxLength);
  }
  if (maxLength > 1) 
  {
    infoLog = (GLchar *) malloc(maxLength);
    if (infoLog != NULL)
    {
      if (glIsShader(object))
      {
        glGetShaderInfoLog(object, maxLength, &length, infoLog);
      }
      else
      {
        glGetProgramInfoLog(object, maxLength, &length, infoLog);
      }
      //fprintf(fileLog, "-- tried to compile (len=%d): %s\n", (unsigned int)strlen(msg), msg);
      //fprintf(fileLog, "--- info log contents (len=%d) ---\n", (int) maxLength);
      //fprintf(fileLog, "%s", infoLog);
      //fprintf(fileLog, "--- end ---\n");
      std::cout << infoLog << std::endl;
      // Look at the info log string here...
      free(infoLog);
    }
  }
}


void Application::initGLSL()
{
  static const std::string vsSource =
    "#version 330\n"
    "layout(location = 0) in vec4 attrPosition;\n"
    "layout(location = 8) in vec2 attrTexCoord0;\n"
    "out vec2 varTexCoord0;\n"
    "void main()\n"
    "{\n"
    "  gl_Position  = attrPosition;\n"
    "  varTexCoord0 = attrTexCoord0;\n"
    "}\n";

  static const std::string fsSource =
    "#version 330\n"
    "uniform sampler2D samplerHDR;\n"
    "in vec2 varTexCoord0;\n"
    "layout(location = 0, index = 0) out vec4 outColor;\n"
    "void main()\n"
    "{\n"
    "  outColor = texture(samplerHDR, varTexCoord0);\n"
    "}\n";

  GLint vsCompiled = 0;
  GLint fsCompiled = 0;
    
  m_glslVS = glCreateShader(GL_VERTEX_SHADER);
  if (m_glslVS)
  {
    GLsizei len = (GLsizei) vsSource.size();
    const GLchar *vs = vsSource.c_str();
    glShaderSource(m_glslVS, 1, &vs, &len);
    glCompileShader(m_glslVS);
    checkInfoLog(vs, m_glslVS);

    glGetShaderiv(m_glslVS, GL_COMPILE_STATUS, &vsCompiled);
    MY_ASSERT(vsCompiled);
  }

  m_glslFS = glCreateShader(GL_FRAGMENT_SHADER);
  if (m_glslFS)
  {
    GLsizei len = (GLsizei) fsSource.size();
    const GLchar *fs = fsSource.c_str();
    glShaderSource(m_glslFS, 1, &fs, &len);
    glCompileShader(m_glslFS);
    checkInfoLog(fs, m_glslFS);

    glGetShaderiv(m_glslFS, GL_COMPILE_STATUS, &fsCompiled);
    MY_ASSERT(fsCompiled);
  }

  m_glslProgram = glCreateProgram();
  if (m_glslProgram)
  {
    GLint programLinked = 0;

    if (m_glslVS && vsCompiled)
    {
      glAttachShader(m_glslProgram, m_glslVS);
    }
    if (m_glslFS && fsCompiled)
    {
      glAttachShader(m_glslProgram, m_glslFS);
    }

    glLinkProgram(m_glslProgram);
    checkInfoLog("m_glslProgram", m_glslProgram);

    glGetProgramiv(m_glslProgram, GL_LINK_STATUS, &programLinked);
    MY_ASSERT(programLinked);

    if (programLinked)
    {
      glUseProgram(m_glslProgram);
     
      glUniform1i(glGetUniformLocation(m_glslProgram, "samplerHDR"), 0); // texture image unit 0

      glUseProgram(0);
    }
  }
}


void Application::guiWindow()
{
  if (!m_isWindowVisible) // Use SPACE to toggle the display of the GUI window.
  {
    return;
  }

  ImGui::SetNextWindowSize(ImVec2(200, 200), ImGuiSetCond_FirstUseEver);

  ImGuiWindowFlags window_flags = 0;
  if (!ImGui::Begin("optixIntro_03", nullptr, window_flags)) // No bool flag to omit the close button.
  {
    // Early out if the window is collapsed, as an optimization.
    ImGui::End();
    return;
  }

  ImGui::PushItemWidth(-100); // right-aligned, keep 180 pixels for the labels.

  if (ImGui::CollapsingHeader("System"))
  {
    if (ImGui::DragFloat("Mouse Ratio", &m_mouseSpeedRatio, 0.1f, 0.1f, 100.0f, "%.1f"))
    {
      m_pinholeCamera.setSpeedRatio(m_mouseSpeedRatio);
    }
  }
  ImGui::PopItemWidth();

  ImGui::End();
}

void Application::guiEventHandler()
{
  ImGuiIO const& io = ImGui::GetIO();

  if (ImGui::IsKeyPressed(' ', false)) // Toggle the GUI window display with SPACE key.
  {
    m_isWindowVisible = !m_isWindowVisible;
  }

  const ImVec2 mousePosition = ImGui::GetMousePos(); // Mouse coordinate window client rect.
  const int x = int(mousePosition.x);
  const int y = int(mousePosition.y);

  switch (m_guiState)
  {
    case GUI_STATE_NONE:
      if (!io.WantCaptureMouse) // Only allow camera interactions to begin when not interacting with the GUI.
      {
        if (ImGui::IsMouseDown(0)) // LMB down event?
        {
          m_pinholeCamera.setBaseCoordinates(x, y);
          m_guiState = GUI_STATE_ORBIT;
        }
        else if (ImGui::IsMouseDown(1)) // RMB down event?
        {
          m_pinholeCamera.setBaseCoordinates(x, y);
          m_guiState = GUI_STATE_DOLLY;
        }
        else if (ImGui::IsMouseDown(2)) // MMB down event?
        {
          m_pinholeCamera.setBaseCoordinates(x, y);
          m_guiState = GUI_STATE_PAN;
        }
        else if (io.MouseWheel != 0.0f) // Mouse wheel zoom.
        {
          m_pinholeCamera.zoom(io.MouseWheel);
        }
      }
      break;

    case GUI_STATE_ORBIT:
      if (ImGui::IsMouseReleased(0)) // LMB released? End of orbit mode.
      {
        m_guiState = GUI_STATE_NONE;
      }
      else
      {
        m_pinholeCamera.orbit(x, y);
      }
      break;

    case GUI_STATE_DOLLY:
      if (ImGui::IsMouseReleased(1)) // RMB released? End of dolly mode.
      {
        m_guiState = GUI_STATE_NONE;
      }
      else
      {
        m_pinholeCamera.dolly(x, y);
      }
      break;

    case GUI_STATE_PAN:
      if (ImGui::IsMouseReleased(2)) // MMB released? End of pan mode.
      {
        m_guiState = GUI_STATE_NONE;
      }
      else
      {
        m_pinholeCamera.pan(x, y);
      }
      break;
  }
}


// This part is always identical in the generated geometry creation routines.
optix::Geometry Application::createGeometry(std::vector<VertexAttributes> const& attributes, std::vector<unsigned int> const& indices)
{
  optix::Geometry geometry(nullptr);

  try
  {
    geometry = m_context->createGeometry();

    optix::Buffer attributesBuffer = m_context->createBuffer(RT_BUFFER_INPUT, RT_FORMAT_USER);
    attributesBuffer->setElementSize(sizeof(VertexAttributes));
    attributesBuffer->setSize(attributes.size());

    void *dst = attributesBuffer->map(0, RT_BUFFER_MAP_WRITE_DISCARD);
    memcpy(dst, attributes.data(), sizeof(VertexAttributes) * attributes.size());
    attributesBuffer->unmap();

    optix::Buffer indicesBuffer = m_context->createBuffer(RT_BUFFER_INPUT, RT_FORMAT_UNSIGNED_INT3, indices.size() / 3);
    dst = indicesBuffer->map(0, RT_BUFFER_MAP_WRITE_DISCARD);
    memcpy(dst, indices.data(), sizeof(optix::uint3) * indices.size() / 3);
    indicesBuffer->unmap();

    std::map<std::string, optix::Program>::const_iterator it = m_mapOfPrograms.find("boundingbox_triangle_indexed");
    MY_ASSERT(it != m_mapOfPrograms.end()); 
    geometry->setBoundingBoxProgram(it->second);

    it = m_mapOfPrograms.find("intersection_triangle_indexed");
    MY_ASSERT(it != m_mapOfPrograms.end()); 
    geometry->setIntersectionProgram(it->second);

    geometry["attributesBuffer"]->setBuffer(attributesBuffer);
    geometry["indicesBuffer"]->setBuffer(indicesBuffer);
    geometry->setPrimitiveCount((unsigned int)(indices.size()) / 3);
  }
  catch(optix::Exception& e)
  {
    std::cerr << e.getErrorString() << std::endl;
  }
  return geometry;
}


void Application::initPrograms()
{
  try
  {
    // First load all programs and put them into a map.
    // Programs which are reused multiple times can be queried from that map.
    // (This renderer does not put variables on program scope!)

    // Renderer
    m_mapOfPrograms["raygeneration"] = m_context->createProgramFromPTXFile(ptxPath("raygeneration.cu"), "raygeneration"); // entry point 0
    m_mapOfPrograms["exception"]     = m_context->createProgramFromPTXFile(ptxPath("exception.cu"), "exception"); // entry point 0
    
    // Constant white environment.
    m_mapOfPrograms["miss"] = m_context->createProgramFromPTXFile(ptxPath("miss.cu"), "miss_environment_constant"); // raytype 0

    // Geometry
    m_mapOfPrograms["boundingbox_triangle_indexed"]  = m_context->createProgramFromPTXFile(ptxPath("boundingbox_triangle_indexed.cu"),  "boundingbox_triangle_indexed");
    m_mapOfPrograms["intersection_triangle_indexed"] = m_context->createProgramFromPTXFile(ptxPath("intersection_triangle_indexed.cu"), "intersection_triangle_indexed");

    // Material programs.
    // For the radiance ray type 0:
    m_mapOfPrograms["closesthit"] = m_context->createProgramFromPTXFile(ptxPath("closesthit.cu"), "closesthit");
  }
  catch(optix::Exception& e)
  {
    std::cerr << e.getErrorString() << std::endl;
  }
}

void Application::initMaterials()
{
  try
  {
    // Create the main Material node

    std::map<std::string, optix::Program>::const_iterator it;

    // Used for all materials without cutout opacity. (Faster than using the cutout opacity material for everything.)
    m_opaqueMaterial = m_context->createMaterial();
    it = m_mapOfPrograms.find("closesthit");
    MY_ASSERT(it != m_mapOfPrograms.end());
    m_opaqueMaterial->setClosestHitProgram(0, it->second); // raytype 0 == radiance
    // No anyhit program needed for this Material and ray type.
  }
  catch(optix::Exception& e)
  {
    std::cerr << e.getErrorString() << std::endl;
  }
}


// Scene testing all materials on a single geometry instanced via transforms and sharing one acceleration structure.
void Application::createScene()
{
  initMaterials();

  try
  {
    // OptiX Scene Graph construction.
    m_rootAcceleration = m_context->createAcceleration(m_builder); // No need to set acceleration properties on the top level Acceleration.

    m_rootGroup = m_context->createGroup(); // The scene's root group nodes becomes the sysTopObject.
    m_rootGroup->setAcceleration(m_rootAcceleration);
    
    m_context["sysTopObject"]->set(m_rootGroup); // This is where the rtTrace calls start the BVH traversal. (Same for radiance and shadow rays.)
    
    unsigned int count;

    // Demo code only!
    // Mind that these local OptiX objects will leak when not cleaning up the scene properly on changes.
    // Destroying the OptiX context will clean them up at program exit though.

    // Add a ground plane on the xz-plane at y = 0.0f with a 1x1 tesselation (2 triangles).
    optix::Geometry geoPlane = createPlane(1, 1, 1);

    optix::GeometryInstance giPlane = m_context->createGeometryInstance(); // This connects Geometries with Materials.
    giPlane->setGeometry(geoPlane);
    giPlane->setMaterialCount(1);
    giPlane->setMaterial(0, m_opaqueMaterial);

    optix::Acceleration accPlane = m_context->createAcceleration(m_builder);
    setAccelerationProperties(accPlane);
    
    // This connects GeometryInstances with Acceleration structures. (All OptiX nodes with "Group" in the name hold an Acceleration.)
    optix::GeometryGroup ggPlane = m_context->createGeometryGroup();
    ggPlane->setAcceleration(accPlane);
    ggPlane->setChildCount(1);
    ggPlane->setChild(0, giPlane);

    // The original object coordinates of the plane have unit size, from -1.0f to 1.0f in x-axis and z-axis.
    // Scale the plane to go from -5 to 5.
    float trafoPlane[16] =
    {
      5.0f, 0.0f, 0.0f, 0.0f,
      0.0f, 5.0f, 0.0f, 0.0f,
      0.0f, 0.0f, 5.0f, 0.0f,
      0.0f, 0.0f, 0.0f, 1.0f
    };
    optix::Matrix4x4 matrixPlane(trafoPlane);

    optix::Transform trPlane = m_context->createTransform();
    trPlane->setChild(ggPlane);
    trPlane->setMatrix(false, matrixPlane.getData(), matrixPlane.inverse().getData());
    
    // Add the transform node placeing the plane to the scene's root Group node.
    count = m_rootGroup->getChildCount();
    m_rootGroup->setChildCount(count + 1);
    m_rootGroup->setChild(count, trPlane);

    // Add a tessellated sphere with 180 longitudes and 90 latitudes (32400 triangles) with radius 1.0f around the origin.
    // The last argument is the maximum theta angle, which allows to generate spheres with a whole at the top.
    // (Useful to test thin-walled materials with different materials on the front- and backface.)
    optix::Geometry geoSphere = createSphere(180, 90, 1.0f, M_PIf);

    optix::Acceleration accSphere = m_context->createAcceleration(m_builder);
    setAccelerationProperties(accSphere);
    
    optix::GeometryInstance giSphere = m_context->createGeometryInstance(); // This connects Geometries with Materials.
    giSphere->setGeometry(geoSphere);
    giSphere->setMaterialCount(1);
    giSphere->setMaterial(0, m_opaqueMaterial);

    optix::GeometryGroup ggSphere = m_context->createGeometryGroup();    // This connects GeometryInstances with Acceleration structures. (All OptiX nodes with "Group" in the name hold an Acceleration.)
    ggSphere->setAcceleration(accSphere);
    ggSphere->setChildCount(1);
    ggSphere->setChild(0, giSphere);

    float trafoSphere[16] =
    {
      1.0f, 0.0f, 0.0f, 0.0f,
      0.0f, 1.0f, 0.0f, 1.0f, // Translate the sphere by 1.0f on the y-axis to be above the plane, exactly touching.
      0.0f, 0.0f, 1.0f, 0.0f,
      0.0f, 0.0f, 0.0f, 1.0f
    };
    optix::Matrix4x4 matrixSphere(trafoSphere);

    optix::Transform trSphere = m_context->createTransform();
    trSphere->setChild(ggSphere);
    trSphere->setMatrix(false, matrixSphere.getData(), matrixSphere.inverse().getData());

    count = m_rootGroup->getChildCount();
    m_rootGroup->setChildCount(count + 1);
    m_rootGroup->setChild(count, trSphere);
  }
  catch(optix::Exception& e)
  {
    std::cerr << e.getErrorString() << std::endl;
  }
}


void Application::setAccelerationProperties(optix::Acceleration acceleration)
{
  // To speed up the acceleration structure build for triangles, skip calls to the bounding box program and
  // invoke the special splitting BVH builder for indexed triangles by setting the necessary acceleration properties.
  // Using the fast Trbvh builder which does splitting has a positive effect on the rendering performanc as well.
  if (m_builder == std::string("Trbvh") || m_builder == std::string("Sbvh"))
  {
    // This requires that the position is the first element and it must be float x, y, z.
    acceleration->setProperty("vertex_buffer_name", "attributesBuffer");
    MY_ASSERT(sizeof(VertexAttributes) == 48) ;
    acceleration->setProperty("vertex_buffer_stride", "48");

    acceleration->setProperty("index_buffer_name", "indicesBuffer");
    MY_ASSERT(sizeof(optix::uint3) == 12) ;
    acceleration->setProperty("index_buffer_stride", "12");
  }
}

