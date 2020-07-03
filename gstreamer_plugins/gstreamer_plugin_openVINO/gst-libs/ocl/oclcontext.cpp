/*
 *Copyright (C) 2018 Intel Corporation
 *
 *SPDX-License-Identifier: LGPL-2.1-only
 *
 *This library is free software; you can redistribute it and/or modify it under the terms
 * of the GNU Lesser General Public License as published by the Free Software Foundation;
 * version 2.1.
 *
 *This library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along with this library;
 * if not, write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "oclcommon.h"
#include "oclcontext.h"
#include <vector>
#include <iterator>


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

using namespace std;
using namespace cv;
using namespace cv::ocl;

#ifdef __WIN32__
#include <opencv2/core/directx.hpp>
using namespace cv::directx::ocl;
#else
#include <CL/va_ext.h>
#include <opencv2/core/va_intel.hpp>
using namespace cv::va_intel;
#endif

namespace HDDLStreamFilter {

#ifndef HDDLS_CVDL_KERNEL_PATH_DEFAULT
#define HDDLS_CVDL_KERNEL_PATH_DEFAULT "/usr/lib/x86_64-linux-gnu/libgstcvdl/kernels"
#endif

///internal class hold device id and make sure *.cl compile in serial
class OclDevice {

public:
    static SharedPtr<OclDevice> getInstance (VideoDisplayID display);

    void setDisplay(VideoDisplayID display) {m_display = display;};
    cl_context getContext ();
    cl_command_queue getCommandQueue ();
    gboolean createFromVA_Intel (cl_mem_flags flags, VideoSurfaceID* surface, cl_uint plane, cl_mem* mem);
    gboolean clEnqueueAcquireVA_Intel (cl_uint num_objects, const cl_mem *mem_objects,
        cl_uint num_events_in_wait_list = 0, const cl_event *event_wait_list = NULL, cl_event *ocl_event = NULL);
    gboolean clEnqueueReleaseVA_Intel (cl_uint num_objects, const cl_mem *mem_objects,
        cl_uint num_events_in_wait_list = 0, const cl_event *event_wait_list = NULL, cl_event *ocl_event = NULL);
    gboolean finish ();

    cv::ocl::Kernel acquireKernelCV(const char* name, const char* file);

    virtual ~OclDevice ();

private:
    OclDevice ();
    gboolean init ();
    cl_int InitDevice ();
    size_t readFile (const char* filename, guint8 **data);
    gpointer getExtensionFunctionAddress (const char* name);
    gboolean releaseKernelCVMap ();
    cv::ocl::Kernel loadKernelCV (const char* name, const char* file);

    static WeakPtr<OclDevice> m_instance;
    static Lock m_lock;

    #ifdef __WIN32__
    cv::directx::ocl::Context m_ocvContext;
    #else
    cv::ocl::Context m_ocvContext;
    #endif
    //OclKernelCVMap m_kernel_cv_map;
    OclProgramCVMap m_program_cv_map;

    cl_context m_context;
    cl_command_queue m_queue;

    VideoDisplayID m_display;
    //all operations need procted by m_lock
    cl_platform_id m_platform;
    cl_device_id m_device;

#ifdef __WIN32__
    clCreateFromD3D11Texture2DKHR_fn clCreateFromD3D11Texture2DKHR;
    clEnqueueAcquireD3D11ObjectsKHR_fn clEnqueueAcquireD3D11ObjectsKHR;
    clEnqueueReleaseD3D11ObjectsKHR_fn clEnqueueReleaseD3D11ObjectsKHR;
#else
    clGetDeviceIDsFromVA_APIMediaAdapterINTEL_fn clGetDeviceIDsFromVA_APIMediaAdapterINTEL;
    clCreateFromVA_APIMediaSurfaceINTEL_fn       clCreateFromVA_APIMediaSurfaceINTEL;
    clEnqueueAcquireVA_APIMediaSurfacesINTEL_fn  clEnqueueAcquireVA_APIMediaSurfacesINTEL;
    clEnqueueReleaseVA_APIMediaSurfacesINTEL_fn  clEnqueueReleaseVA_APIMediaSurfacesINTEL;
#endif
    friend class OclContext;

    DISALLOW_COPY_AND_ASSIGN (OclDevice)
};

Lock OclDevice::m_lock;
WeakPtr<OclDevice> OclDevice::m_instance;

SharedPtr<OclContext> OclContext::create (VideoDisplayID display)
{
    SharedPtr<OclContext> context (new OclContext);
    if (!context->init (display))
        context.reset ();
    return context;
}

OclContext::OclContext () : m_dest_mem(NULL)
{
}

OclContext::~OclContext ()
{
    releaseVAMemoryCL (&m_dest_mem);
}

gboolean OclContext::init (VideoDisplayID display)
{
    m_device = OclDevice::getInstance (display);

    return (m_device.get() != NULL);
}

cl_command_queue
OclContext::getCommandQueue ()
{
    return m_device->getCommandQueue ();
}

cl_context
OclContext::getContext ()
{
    return m_device->getContext ();
}


cv::ocl::Kernel
OclContext::acquireKernelCV (const char* name, const char* file)
{
    if (!name) {
        g_print ("OclContext: please specify the kernel file and kernel name.\
                 If kernel name is same with kernel file, kernel file could be null.\n");
        return cv::ocl::Kernel();
    }
    return m_device->acquireKernelCV(name, file);
}

gpointer
OclContext::acquireMemoryCL (cl_mem mem, const cl_uint num_planes)
{
    OclCLMemInfo* mem_info = NULL;

    if (!mem || num_planes < 1 || num_planes > 3) {
        return NULL;
    }

    mem_info = g_new0 (OclCLMemInfo, 1);
    mem_info->cl_memory[0] = mem;
    mem_info->num_planes = num_planes;

    return mem_info;
}

void
OclContext::releaseMemoryCL (gpointer info)
{
    OclCLMemInfo** mem_info = (OclCLMemInfo**) info;

    if (!mem_info || !*mem_info || !(*mem_info)->num_planes || !(*mem_info)->cl_memory[0])
        return;

    g_free (*mem_info);
    *mem_info = NULL;
}

gpointer
OclContext::acquireVAMemoryCL (VideoSurfaceID* surface, const cl_uint num_planes, const cl_mem_flags flags)
{
    OclCLMemInfo* mem_info = NULL;

    if (!surface || num_planes < 1 || num_planes > 3) {
        return NULL;
    }

    mem_info = g_new0 (OclCLMemInfo, 1);

    cl_uint plane = 0;
    for (plane = 0; plane < num_planes; ++plane) {
        if (!m_device->createFromVA_Intel (flags, surface, plane, &mem_info->cl_memory[plane]))
            break;
    }

    if (!plane ||
        !m_device->clEnqueueAcquireVA_Intel (plane, &mem_info->cl_memory[0])) {
        g_free (mem_info);
        return NULL;
    }

    mem_info->num_planes = plane;

    return mem_info;
}

void
OclContext::releaseVAMemoryCL (gpointer info)
{
    OclCLMemInfo** mem_info = (OclCLMemInfo**) info;

    if (!mem_info || !*mem_info || !(*mem_info)->num_planes || !(*mem_info)->cl_memory[0]) {
        return;
    }

    m_device->clEnqueueReleaseVA_Intel ((*mem_info)->num_planes, &(*mem_info)->cl_memory[0]);

    for (cl_uint plane = 0; plane < (*mem_info)->num_planes; ++plane) {
        CL_ERROR_PRINT (clReleaseMemObject ((*mem_info)->cl_memory[plane]), "clReleaseMemObject");
    }

    g_free (*mem_info);

    *mem_info = NULL;
}

gboolean
OclContext::setDestSurface (VideoSurfaceID* surface)
{
    m_dest_mem = (OclCLMemInfo*) acquireVAMemoryCL (surface, 2);

    return (m_dest_mem != NULL);
}

void
OclContext::finish()
{
    if (m_dest_mem)
        releaseVAMemoryCL (&m_dest_mem);

    m_device->finish ();
}

OclDevice::OclDevice () : m_context(0), m_queue(0),
    m_platform(0), m_device(0), clCreateFromVA_APIMediaSurfaceINTEL(0), m_display(nullptr), clGetDeviceIDsFromVA_APIMediaAdapterINTEL(nullptr),
clEnqueueAcquireVA_APIMediaSurfacesINTEL(nullptr), clEnqueueReleaseVA_APIMediaSurfacesINTEL(nullptr)
{
}

OclDevice::~OclDevice ()
{
    releaseKernelCVMap ();
    //cv::ocl::Context::initializeContextFromHandle(Context::getDefault(false), NULL, NULL, NULL);
    //if(m_instance.use_count()==1)
    {
        Queue::getDefault().finish();
        Queue &q = Queue::getDefault();
        q = Queue();
        Context& ctx = Context::getDefault();
        ctx = Context();
    }
}

SharedPtr<OclDevice>
OclDevice::getInstance (VideoDisplayID display)
{
    AutoLock lock(m_lock);
    SharedPtr<OclDevice> device = m_instance.lock ();
    if (device)
        return device;

    device.reset (new OclDevice);

#ifdef __WIN32__
    device->m_ocvContext = cv::directx::ocl::initializeContextFromD3D11Device((ID3D11Device *)display);
#else
    // init ocl based on VideoDisplayID
    device->m_ocvContext = cv::va_intel::ocl::initializeContextFromVA(display, true);
#endif
    device->setDisplay(display);
    if (!device->init ()) {
        device.reset ();
    }

    // make it can be created only once for multiple threads
    m_instance = device;
    return device;
}

#define MAX_STRING_LEN 1024
gboolean
OclDevice::init ()
{
    m_context  = (cl_context)Context::getDefault().ptr();
    m_device  = (cl_device_id)Context::getDefault().device(0).ptr();

    m_platform = (cl_platform_id)cv::ocl::Platform::getDefault().ptr();
    m_queue = (cl_command_queue)Queue::getDefault().ptr();

    if(!m_context || !m_device || !m_platform || !m_queue) {
        g_print ("OclDevice: failed to init oclDevice\n");
        return FALSE;
    }

    //debug
    cl_int status = CL_SUCCESS;
    char platform[MAX_STRING_LEN];
    status = clGetPlatformInfo (m_platform, CL_PLATFORM_VERSION, MAX_STRING_LEN, platform, NULL);
    GST_INFO ("OpenCL platform %s is used, status = %d\n", platform, status);
    //debug end

#ifdef __WIN32__
    clCreateFromD3D11Texture2DKHR = (clCreateFromD3D11Texture2DKHR_fn)
         getExtensionFunctionAddress(platform, "clCreateFromD3D11Texture2DKHR");
    clEnqueueAcquireD3D11ObjectsKHR = (clEnqueueAcquireD3D11ObjectsKHR_fn)
         getExtensionFunctionAddress(platform, "clEnqueueAcquireD3D11ObjectsKHR");
    clEnqueueReleaseD3D11ObjectsKHR = (clEnqueueReleaseD3D11ObjectsKHR_fn)
         getExtensionFunctionAddress(platform, "clEnqueueReleaseD3D11ObjectsKHR");
    if (!clCreateFromD3D11Texture2DKHR ||
        !clEnqueueAcquireD3D11ObjectsKHR ||
        !clEnqueueReleaseD3D11ObjectsKHR) {
        g_print ("OclDevice: failed to get extension function\n");
        return FALSE;
    }
#else
    clGetDeviceIDsFromVA_APIMediaAdapterINTEL = (clGetDeviceIDsFromVA_APIMediaAdapterINTEL_fn)
        getExtensionFunctionAddress ("clGetDeviceIDsFromVA_APIMediaAdapterINTEL");
    clCreateFromVA_APIMediaSurfaceINTEL = (clCreateFromVA_APIMediaSurfaceINTEL_fn)
        getExtensionFunctionAddress ("clCreateFromVA_APIMediaSurfaceINTEL");
    clEnqueueAcquireVA_APIMediaSurfacesINTEL = (clEnqueueAcquireVA_APIMediaSurfacesINTEL_fn)
        getExtensionFunctionAddress ("clEnqueueAcquireVA_APIMediaSurfacesINTEL");
    clEnqueueReleaseVA_APIMediaSurfacesINTEL = (clEnqueueReleaseVA_APIMediaSurfacesINTEL_fn)
        getExtensionFunctionAddress ("clEnqueueReleaseVA_APIMediaSurfacesINTEL");

    if (!clGetDeviceIDsFromVA_APIMediaAdapterINTEL ||
        !clCreateFromVA_APIMediaSurfaceINTEL ||
        !clEnqueueAcquireVA_APIMediaSurfacesINTEL ||
        !clEnqueueReleaseVA_APIMediaSurfacesINTEL) {
        g_print ("OclDevice: failed to get extension function\n");
        return FALSE;
    }
 #endif
    return TRUE;
}

gpointer
OclDevice::getExtensionFunctionAddress (const char* name)
{
#ifdef CL_VERSION_1_2
    return clGetExtensionFunctionAddressForPlatform (m_platform, name);
#else
    return clGetExtensionFunctionAddress (name);
#endif
}

size_t
OclDevice::readFile (const char* filename, guint8 **data)
{
    FILE  *fp = NULL;
    size_t data_size = 0;

    if (!(fp = fopen (filename, "rb")))
        return 0;

    fseek (fp, 0, SEEK_END);
    data_size = ftell (fp);
    rewind (fp);

    *data = g_new0 (guint8, data_size + 1);
    if ( data_size != fread (*data, 1, data_size, fp)) {
        g_free (*data);
        *data = NULL;
    }

    fclose (fp);

    return *data ? data_size : 0;
}


cv::ocl::Kernel
OclDevice::loadKernelCV (const char* name, const char* file)
{
    cv::ocl::Kernel kernel;
    cv::ocl::Program program;
    const gchar *env = g_getenv("HDDLS_CVDL_KERNEL_PATH");

    std::ostringstream   path_str;
    if(env) {
        path_str   <<   env   <<   "/"   <<    (file ? file : name)   <<  ".cl";
    } else {
        path_str   <<   HDDLS_CVDL_KERNEL_PATH_DEFAULT   <<   "/"   <<    (file ? file : name)   <<  ".cl";
    }
    std::string str =  path_str.str();
    const char *filename = str.c_str();

    guint8 *data = NULL;
    if (!readFile (filename, &data)) {
        g_print("It cannot find kernel source from %s\nplease set HDDLS_CVDL_KERNEL_PATH!\n", filename);
        return cv::ocl::Kernel();
    }

    cv::ocl::ProgramSource programSource((const char*)data);
    //if (programSource.empty()) {
    //    g_print("Error in create programSource...\n");
    //    return cv::ocl::Kernel();
    //}

    cv::String buildOptions("-I. -cl-fast-relaxed-math -cl-kernel-arg-info");
    cv::String buildError;
    program = cv::ocl::Program(programSource, buildOptions, buildError);
    GST_INFO("Build program: %s\n", buildError.c_str());
    g_free (data);
    //if(program.empty()) {
    //    g_print("Error in build program...\n");
    //    return cv::ocl::Kernel();
    //}

    kernel.create(name, program);
    if(kernel.empty()){
        g_print("Error in create kernel...\n");
        return cv::ocl::Kernel();
    }
    m_program_cv_map[file] = program;
    return kernel;

    //m_kernel_cv_map[name] = kernel;
    //return kernel;
}


cv::ocl::Kernel
OclDevice::acquireKernelCV(const char* name, const char* file)
{
    if (!name) {
        g_print ("OclDevice: please specify the kernel file and kernel name.\
                 If kernel name is same with kernel file, kernel file could be null.\n");
        return cv::ocl::Kernel();
    }
    AutoLock lock(m_lock);

    #if 0
    OclKernelCVMapIterator it = m_kernel_cv_map.find (name);
    return ((it == m_kernel_cv_map.end()) ? loadKernelCV(name, file) : it->second);
    #else
    OclProgramCVMapIterator it = m_program_cv_map.find (file);
    if(it == m_program_cv_map.end()) {
        return loadKernelCV(name, file);
    }

    // kernel must be created every time, if not NV12 plane will be got with garbage
    cv::ocl::Kernel kernel;
    kernel.create(name, it->second);
    return kernel;
    #endif
}


gboolean
OclDevice::releaseKernelCVMap ()
{
    gboolean succ = TRUE;

    AutoLock lock(m_lock);

    //for(OclKernelCVMapIterator it = m_kernel_cv_map.begin(); it != m_kernel_cv_map.end(); ++it) {
    //    it->second = cv::ocl::Kernel();
    //    succ = FALSE;
    //}
    //m_kernel_cv_map.clear();

    for(OclProgramCVMapIterator it = m_program_cv_map.begin(); it != m_program_cv_map.end(); ++it) {
        it->second = cv::ocl::Program();
        succ = FALSE;
    }
    m_program_cv_map.clear();
    return succ;
}

gboolean
OclDevice::createFromVA_Intel (cl_mem_flags flags, VideoSurfaceID* surface, cl_uint plane, cl_mem* mem)
{
    cl_int status = CL_SUCCESS;

    m_context = getContext();

    #ifdef __WIN32__
        *mem = clCreateFromD3D11Texture2DKHR(m_context, flags, surface, plane, &status);
    #else
        *mem = clCreateFromVA_APIMediaSurfaceINTEL (m_context, flags, surface, plane, &status);
    #endif
    if (CL_ERROR_PRINT (status, "clCreateFromVA_APIMediaSurfaceINTEL")) {
        GST_ERROR("error - surface = %d, plane = %d, flags = %ld, m_context = %p\n", *surface, plane, flags, m_context);
        return FALSE;
    }
    GST_LOG("surface = %d, plane = %d, flags = %ld, m_context = %p\n", *surface, plane, flags, m_context);
    return TRUE;
}

gboolean
OclDevice::clEnqueueAcquireVA_Intel(cl_uint num_objects, const cl_mem *mem_objects,
    cl_uint num_events_in_wait_list, const cl_event *event_wait_list, cl_event *ocl_event)
{
    m_queue = (cl_command_queue)Queue::getDefault().ptr();
    cl_int status = CL_SUCCESS;

    #ifdef __WIN32__
        status = clEnqueueAcquireD3D11ObjectsKHR(m_queue, num_objects, &mem_objects,
            0, NULL, NULL);
    #else
        status = clEnqueueAcquireVA_APIMediaSurfacesINTEL (m_queue, num_objects, mem_objects,
            num_events_in_wait_list, event_wait_list, ocl_event);
    #endif
    if (CL_ERROR_PRINT (status, "clEnqueueAcquireVA_APIMediaSurfacesINTEL"))
        return FALSE;

     return TRUE;
}

gboolean
OclDevice::clEnqueueReleaseVA_Intel(const cl_uint num_objects, const cl_mem *mem_objects,
    cl_uint num_events_in_wait_list, const cl_event *event_wait_list, cl_event *ocl_event)
{
    gboolean succ = TRUE;
    cl_int status = CL_SUCCESS;

    #ifdef __WIN32__
        status = clEnqueueReleaseD3D11ObjectsKHR(m_queue, num_objects, mem_objects,
            0, NULL, NULL);
    #else
        status = clEnqueueReleaseVA_APIMediaSurfacesINTEL (m_queue, num_objects, mem_objects,
            num_events_in_wait_list, event_wait_list, ocl_event);
    #endif

    if (CL_ERROR_PRINT (status, "clEnqueueReleaseVA_APIMediaSurfacesINTEL"))
        succ = FALSE;

    if (CL_ERROR_PRINT (clFlush (m_queue), "clFlush"))
        succ = FALSE;

    return succ;
}

cl_command_queue
OclDevice::getCommandQueue ()
{
    return m_queue;
}

cl_context
OclDevice::getContext ()
{
    cl_context context = (cl_context)Context::getDefault().ptr();

    if(context != m_context) {
        GST_ERROR("Warning - m_context is changed!!!\n");
    }

    return m_context;
}

gboolean
OclDevice::finish ()
{
    return (CL_ERROR_PRINT (clFinish (m_queue), "clFinish") == FALSE);
}

}

gboolean
checkCLError (cl_int status, const char* func, const char* file, const int line)
{
    if (status != CL_SUCCESS) {
        g_print ("(%s,%d) %s failed: %s\n", file, line, func, getErrorString(status));
        return TRUE;
    }
    return FALSE;
}

const char *getErrorString (cl_int error)
{
switch(error) {
    case 0:
        return "OCL_SUCCESS";
        break;
    default:
        return "OCL_ERROR";
    }
   return "Unknown OCL error";
}
