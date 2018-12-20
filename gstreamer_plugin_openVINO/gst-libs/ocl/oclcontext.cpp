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
#include <CL/va_ext.h>

#include "common/log.h"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

using namespace std;
using namespace cv;
using namespace cv::ocl;
using namespace cv::va_intel;

namespace HDDLStreamFilter {

#ifndef HDDLS_CVDL_KERNEL_PATH_DEFAULT
#define HDDLS_CVDL_KERNEL_PATH_DEFAULT "/usr/lib/x86_64-linux-gnu/libgstcvdl/kernels"
#endif

///internal class hold device id and make sure *.cl compile in serial
class OclDevice {

public:
    static SharedPtr<OclDevice> getInstance (VADisplay display);

    void setDisplay(VADisplay display) {m_display = display;};
    cl_context getContext ();
    cl_command_queue getCommandQueue ();
    cl_kernel acquireKernel (const char* name, const char* file = NULL);

    gboolean createFromVA_Intel (cl_mem_flags flags, VASurfaceID* surface, cl_uint plane, cl_mem* mem);
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

    cl_int InitPlatform ();
    size_t readFile (const char* filename, guint8 **data);
    cl_program createProgramFromSource (const char* filename);
    cl_program createProgramFromBinary (const char* filename);
    cl_kernel loadKernel (const char* name, const char* file);
    gboolean saveProgramBinary (cl_program program, const char* filename);
    gpointer getExtensionFunctionAddress (const char* name);
    gboolean releaseKernelMap ();
    gboolean releaseKernelCVMap ();
    cv::ocl::Kernel loadKernelCV (const char* name, const char* file);

    static WeakPtr<OclDevice> m_instance;
    static Lock m_lock;

    cv::ocl::Context m_ocvContext;
    //OclKernelCVMap m_kernel_cv_map;
    OclProgramCVMap m_program_cv_map;

    OclKernelMap m_kernel_map;
    cl_context m_context;
    cl_command_queue m_queue;

    VADisplay m_display;
    //all operations need procted by m_lock
    cl_platform_id m_platform;
    cl_device_id m_device;

    clGetDeviceIDsFromVA_APIMediaAdapterINTEL_fn clGetDeviceIDsFromVA_APIMediaAdapterINTEL;
    clCreateFromVA_APIMediaSurfaceINTEL_fn       clCreateFromVA_APIMediaSurfaceINTEL;
    clEnqueueAcquireVA_APIMediaSurfacesINTEL_fn  clEnqueueAcquireVA_APIMediaSurfacesINTEL;
    clEnqueueReleaseVA_APIMediaSurfacesINTEL_fn  clEnqueueReleaseVA_APIMediaSurfacesINTEL;

    friend class OclContext;

    DISALLOW_COPY_AND_ASSIGN (OclDevice)
};

Lock OclDevice::m_lock;
WeakPtr<OclDevice> OclDevice::m_instance;

SharedPtr<OclContext> OclContext::create (VADisplay display)
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

gboolean OclContext::init (VADisplay display)
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

cl_kernel
OclContext::acquireKernel (const char* name, const char* file)
{
    if (!name) {
        g_print ("OclContext: please specify the kernel file and kernel name.\
                 If kernel name is same with kernel file, kernel file could be null.\n");
        return NULL;
    }

    return m_device->acquireKernel (name, file);
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
OclContext::acquireVAMemoryCL (VASurfaceID* surface, const cl_uint num_planes, const cl_mem_flags flags)
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
OclContext::setDestSurface (VASurfaceID* surface)
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
    m_platform(0), m_device(0), clCreateFromVA_APIMediaSurfaceINTEL(0)
{
}

OclDevice::~OclDevice ()
{
#ifdef USE_CV_OCL
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
#else
    releaseKernelMap ();
    finish ();
    clReleaseCommandQueue (m_queue);
    clReleaseContext (m_context);
#endif
}

SharedPtr<OclDevice>
OclDevice::getInstance (VADisplay display)
{
    AutoLock lock(m_lock);
    SharedPtr<OclDevice> device = m_instance.lock ();
    if (device)
        return device;

    device.reset (new OclDevice);

    // init ocl based on VADisplay
    device->m_ocvContext = cv::va_intel::ocl::initializeContextFromVA(display, true);

    device->setDisplay(display);
    if (!device->init ()) {
        device.reset ();
    }

    // make it can be created only once for multiple threads
    m_instance = device;
    return device;
}

cl_int
OclDevice::InitDevice ()
{
    cl_int error = CL_SUCCESS;
    cl_uint nDevices = 0;

    error = clGetDeviceIDsFromVA_APIMediaAdapterINTEL (m_platform,
        CL_VA_API_DISPLAY_INTEL, m_display, CL_PREFERRED_DEVICES_FOR_VA_API_INTEL,
        1, &m_device, &nDevices);

    if (error) {
        return error;
    }

    if (!nDevices)
        return CL_INVALID_PLATFORM;

    return error;
}

#define MAX_STRING_LEN 1024

cl_int
OclDevice::InitPlatform ()
{
    cl_uint num_platforms = 0;
    cl_int status = CL_SUCCESS;

    // Determine the number of installed OpenCL platforms
    status = clGetPlatformIDs (0, NULL, &num_platforms);

    if (status || !num_platforms) {
        g_print ("OclDevice: Couldn't get platform IDs. \
            Make sure your platform supports OpenCL and can find a proper library.\n");
        return status;
    }

    // Get all of the handles to the installed OpenCL platforms
    vector<cl_platform_id> platforms(num_platforms);
    status = clGetPlatformIDs (num_platforms, &platforms[0], &num_platforms);
    if (CL_ERROR_PRINT (status, "clGetPlatformIDs"))
        return status;

    // Find the platform handle for the installed Gen driver
    char platform[MAX_STRING_LEN];
    cl_device_id device_ids[2] = {0};

    for (guint index = 0; index < num_platforms; ++index) {
        status = clGetPlatformInfo (platforms[index], CL_PLATFORM_NAME, MAX_STRING_LEN, platform, NULL);
        if (status) return status;

        // Choose only GPU devices
        if (clGetDeviceIDs (platforms[index], CL_DEVICE_TYPE_GPU,
            sizeof(device_ids)/sizeof(device_ids[0]), device_ids, 0) != CL_SUCCESS)
            continue;

        if(strstr (platform, "Intel")) // Use only Intel platfroms
        {
            g_print ("OpenCL platform %s is used\n", platform);
            m_platform = platforms[index];
            break;
        }
    }

    return (m_platform ? status : CL_INVALID_PLATFORM);
}

gboolean
OclDevice::init ()
{
#if 1
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
    g_print ("OpenCL platform %s is used, status = %d\n", platform, status);
    //debug end

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
#else
    cl_int status;
    InitPlatform ();

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

    InitDevice ();

    cl_context_properties props[] = { CL_CONTEXT_VA_API_DISPLAY_INTEL,
        (cl_context_properties) m_display, CL_CONTEXT_INTEROP_USER_SYNC, 1, 0};

    m_context = clCreateContext (props, 1, &m_device, NULL, NULL, &status);
    if (status != CL_SUCCESS) {
        g_print ("OclDevice: clCreateContext failed, error=%d\n", status);
        return FALSE;
    }

    m_queue = clCreateCommandQueue (m_context, m_device, 0, &status);
    if (status != CL_SUCCESS) {
        g_print ("OclDevice: clCreateCommandQueue failed, error=%d\n", status);
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

cl_program
OclDevice::createProgramFromSource (const char* filename)
{
    guint8 *data = NULL;
    cl_program program = NULL;

    if (!readFile (filename, &data))
        return NULL;

    program = clCreateProgramWithSource (m_context, 1, (const char**)&data, NULL, NULL);
    g_free (data);
    if (!program) {
        g_print ("Failed to create CL program from source.\n");
        return NULL;
    }

    //char buildOptions[] = "-I. -Werror -cl-fast-relaxed-math";
    char buildOptions[] = "-I. -cl-fast-relaxed-math -cl-kernel-arg-info";
    if (CL_ERROR_PRINT (clBuildProgram (program, 1, &m_device,
                        buildOptions, NULL, NULL), "clBuildProgram")) {
        char log[1024];
        if (!CL_ERROR_PRINT (clGetProgramBuildInfo (program, m_device,
               CL_PROGRAM_BUILD_LOG, sizeof(log), log, NULL), "clGetProgramBuildInfo")) {
            log[sizeof(log) - 1] = '\0';
            g_print ("Log Error in kernel: %s\n", log);
        }
        if (!CL_ERROR_PRINT (clGetProgramBuildInfo (program, m_device,
               CL_PROGRAM_BUILD_STATUS, sizeof(log), log, NULL), "clGetProgramBuildInfo")) {
            log[sizeof(log) - 1] = '\0';
            g_print ("Status Error in kernel: %s\n", log);
        }
        clReleaseProgram (program);
        return NULL;
    }

    return program;
}

cl_program
OclDevice::createProgramFromBinary (const char* filename)
{
    size_t binary_size = 0;
    guint8 *program_binary = NULL;

    if (!(binary_size = readFile (filename, &program_binary)))
        return NULL;

    cl_int error_code, binary_status;
    cl_program program = clCreateProgramWithBinary (m_context, 1, &m_device,
        &binary_size, (const guint8**)&program_binary, &binary_status, &error_code);
    g_free (program_binary);
    if (CL_ERROR_PRINT (error_code | binary_status, "clCreateProgramWithBinary"))
        return NULL;

    char buildOptions[] = "-I. -Werror -cl-fast-relaxed-math";
    if (CL_ERROR_PRINT (clBuildProgram (program, 1, &m_device,
                              buildOptions, NULL, NULL), "clBuildProgram")) {
        char log[1024];
        if (!CL_ERROR_PRINT (clGetProgramBuildInfo (program, m_device,
               CL_PROGRAM_BUILD_LOG, sizeof(log), log, NULL), "clGetProgramBuildInfo")) {
            log[sizeof(log) - 1] = '\0';
            g_print ("Error in kernel: %s\n", log);
        }
        clReleaseProgram (program);
        return NULL;
    }

    return program;
}

gboolean
OclDevice::saveProgramBinary (cl_program program, const char* filename)
{
    gboolean success = FALSE;
    cl_uint num_devices = 0;
    cl_device_id* devices = NULL;
    size_t* binary_sizes = NULL;
    guint8** program_binaries = NULL;

    // 1 - Query for number of devices attached to program
    if (CL_ERROR_PRINT (clGetProgramInfo(program, CL_PROGRAM_NUM_DEVICES,
        sizeof(num_devices), &num_devices, NULL),"clGetProgramInfo") || !num_devices)
        goto err;

    // 2 - Get all of the Device IDs
    devices = g_new0 (cl_device_id, num_devices);
    if (CL_ERROR_PRINT (clGetProgramInfo (program, CL_PROGRAM_DEVICES,
        sizeof(cl_device_id) * num_devices, devices, NULL), "clGetProgramInfo"))
        goto err;

    // 3 - Determine the size of each program binary
    binary_sizes = g_new0 (size_t, num_devices);
    if (CL_ERROR_PRINT (clGetProgramInfo (program, CL_PROGRAM_BINARY_SIZES,
        sizeof(size_t) * num_devices, binary_sizes, NULL), "clGetProgramInfo"))
        goto err;

    program_binaries  = g_new0 (guint8*, num_devices);
    for (cl_uint i = 0; i < num_devices; ++i) {
        program_binaries[i] = g_new0 (guint8, binary_sizes[i]);
    }

    // 4 - Get all of the program binaries
    if (CL_ERROR_PRINT (clGetProgramInfo (program, CL_PROGRAM_BINARIES,
        sizeof(guint8*) * num_devices, program_binaries, NULL), "clGetProgramInfo"))
        goto err;

    // 5 - Finally store the binaries for the device requested out to disk for future reading.
    for (cl_uint i = 0; i < num_devices; i++)
    {
        // Store the binary just for the device requested.  In a scenario where
        // multiple devices were being used you would save all of the binaries out here.
        if (devices[i] == m_device)
        {
            FILE *fp = fopen (filename, "wb");
            if (fp) {
                fwrite (program_binaries[i], 1, binary_sizes[i], fp);
                fclose (fp);
                success = TRUE;
            } else {
                success = FALSE;
            }
            break;
        }
    }

err:
    g_free (devices);
    g_free (binary_sizes);
    if (program_binaries) {
        for (cl_uint i = 0; i < num_devices; ++i) {
            if (program_binaries[i])
                g_free (program_binaries[i]);
        }
        g_free (program_binaries);
    }

    return success;
}

cl_kernel
OclDevice::loadKernel (const char* name, const char* file)
{
    cl_kernel kernel = NULL;
    GString *fullname = g_string_new (NULL);

    const gchar *env = g_getenv("HDDLS_CVDL_KERNEL_PATH");
    if(env) {
        g_string_printf (fullname, "%s/%s.cl.bin", env, (file ? file : name));
    } else {
        g_string_printf (fullname, "%s/%s.cl.bin", HDDLS_CVDL_KERNEL_PATH_DEFAULT, (file ? file : name));
    }

    cl_program program = createProgramFromBinary (fullname->str);
    if (!program) {
        *(fullname->str + fullname->len - 4) = '\0';
        program = createProgramFromSource (fullname->str);
        if (!program) {
            g_string_free (fullname, TRUE);
            return NULL;
        }

        *(fullname->str + fullname->len - 4) = '.';
        saveProgramBinary (program, fullname->str);
    }

    g_string_free (fullname, TRUE);

    size_t num_kernels = 0;
    if (CL_ERROR_PRINT (clGetProgramInfo (program, CL_PROGRAM_NUM_KERNELS,
                sizeof(num_kernels), &num_kernels, NULL), "clGetProgramInfo")) {
        clReleaseProgram (program);
        return NULL;
    }

    vector<cl_kernel> kernels(num_kernels, NULL);
    if (CL_ERROR_PRINT (clCreateKernelsInProgram (program, num_kernels,
            kernels.data(), NULL), "clCreateKernelsInProgram")) {
        clReleaseProgram (program);
        return NULL;
    }

    char kernel_name[128];
    for (size_t index = 0; index < num_kernels; ++index) {
        if (CL_ERROR_PRINT (clGetKernelInfo (kernels[index], CL_KERNEL_FUNCTION_NAME,
                           sizeof(kernel_name), kernel_name, NULL), "clGetKernelInfo"))
            break;

        m_kernel_map[kernel_name] = kernels[index];

        if (!strcmp (name, kernel_name))
            kernel = kernels[index];
    }

    clReleaseProgram (program);

    return kernel;
}

cl_kernel
OclDevice::acquireKernel (const char* name, const char* file)
{
    if (!name) {
        g_print ("OclDevice: please specify the kernel file and kernel name.\
                 If kernel name is same with kernel file, kernel file could be null.\n");
        return NULL;
    }

    AutoLock lock(m_lock);

    OclKernelMapIterator it = m_kernel_map.find (name);

    return ((it == m_kernel_map.end()) ? loadKernel(name, file) : it->second);
}

gboolean
OclDevice::releaseKernelMap ()
{
    gboolean succ = TRUE;

    AutoLock lock(m_lock);

    for(OclKernelMapIterator it = m_kernel_map.begin(); it != m_kernel_map.end(); ++it) {
        if (CL_ERROR_PRINT (clReleaseKernel (it->second), "clReleaseKernel"))
            succ = FALSE;
    }

    m_kernel_map.clear();

    return succ;
}


cv::ocl::Kernel
OclDevice::loadKernelCV (const char* name, const char* file)
{
    cv::ocl::Kernel kernel;
    cv::ocl::Program program;
    GString *fullname = g_string_new (NULL);
    const gchar *env = g_getenv("HDDLS_CVDL_KERNEL_PATH");

    if(env) {
        g_string_printf (fullname, "%s/%s.cl", env, (file ? file : name));
    } else {
        g_string_printf (fullname, "%s/%s.cl", HDDLS_CVDL_KERNEL_PATH_DEFAULT, (file ? file : name));
    }

    guint8 *data = NULL;
    if (!readFile (fullname->str, &data)) {
        g_print("It cannot find kernel source, please set HDDLS_CVDL_KERNEL_PATH!\n");
        return cv::ocl::Kernel();
    }
    g_string_free (fullname, TRUE);

    cv::ocl::ProgramSource programSource((const char*)data);
    //if (programSource.empty()) {
    //    g_print("Error in create programSource...\n");
    //    return cv::ocl::Kernel();
    //}

    cv::String buildOptions("-I. -cl-fast-relaxed-math -cl-kernel-arg-info");
    cv::String buildError;
    program = cv::ocl::Program(programSource, buildOptions, buildError);
    g_print("Build program: %s\n", buildError.c_str());
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
OclDevice::createFromVA_Intel (cl_mem_flags flags, VASurfaceID* surface, cl_uint plane, cl_mem* mem)
{
    cl_int status = CL_SUCCESS;

    m_context = getContext();
    *mem = clCreateFromVA_APIMediaSurfaceINTEL (m_context, flags, surface, plane, &status);

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
    if (CL_ERROR_PRINT (clEnqueueAcquireVA_APIMediaSurfacesINTEL (m_queue, num_objects, mem_objects,
            num_events_in_wait_list, event_wait_list, ocl_event), "clEnqueueAcquireVA_APIMediaSurfacesINTEL"))
        return FALSE;

     return TRUE;
}

gboolean
OclDevice::clEnqueueReleaseVA_Intel(const cl_uint num_objects, const cl_mem *mem_objects,
    cl_uint num_events_in_wait_list, const cl_event *event_wait_list, cl_event *ocl_event)
{
    gboolean succ = TRUE;

    if (CL_ERROR_PRINT (clEnqueueReleaseVA_APIMediaSurfacesINTEL (m_queue, num_objects, mem_objects,
            num_events_in_wait_list, event_wait_list, ocl_event), "clEnqueueReleaseVA_APIMediaSurfacesINTEL"))
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
        g_print("Warning - m_context is changed!!!\n");
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
        g_print ("(%s,%d) %s failed: %s\n", file, line, func, getCLErrorString(status));
        return TRUE;
    }
    return FALSE;
}

const char *getCLErrorString (cl_int error)
{
switch(error) {
    // run-time and JIT compiler errors
    case 0: return "CL_SUCCESS";
    case -1: return "CL_DEVICE_NOT_FOUND";
    case -2: return "CL_DEVICE_NOT_AVAILABLE";
    case -3: return "CL_COMPILER_NOT_AVAILABLE";
    case -4: return "CL_MEM_OBJECT_ALLOCATION_FAILURE";
    case -5: return "CL_OUT_OF_RESOURCES";
    case -6: return "CL_OUT_OF_HOST_MEMORY";
    case -7: return "CL_PROFILING_INFO_NOT_AVAILABLE";
    case -8: return "CL_MEM_COPY_OVERLAP";
    case -9: return "CL_IMAGE_FORMAT_MISMATCH";
    case -10: return "CL_IMAGE_FORMAT_NOT_SUPPORTED";
    case -11: return "CL_BUILD_PROGRAM_FAILURE";
    case -12: return "CL_MAP_FAILURE";
    case -13: return "CL_MISALIGNED_SUB_BUFFER_OFFSET";
    case -14: return "CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST";
    case -15: return "CL_COMPILE_PROGRAM_FAILURE";
    case -16: return "CL_LINKER_NOT_AVAILABLE";
    case -17: return "CL_LINK_PROGRAM_FAILURE";
    case -18: return "CL_DEVICE_PARTITION_FAILED";
    case -19: return "CL_KERNEL_ARG_INFO_NOT_AVAILABLE";

    // compile-time errors
    case -30: return "CL_INVALID_VALUE";
    case -31: return "CL_INVALID_DEVICE_TYPE";
    case -32: return "CL_INVALID_PLATFORM";
    case -33: return "CL_INVALID_DEVICE";
    case -34: return "CL_INVALID_CONTEXT";
    case -35: return "CL_INVALID_QUEUE_PROPERTIES";
    case -36: return "CL_INVALID_COMMAND_QUEUE";
    case -37: return "CL_INVALID_HOST_PTR";
    case -38: return "CL_INVALID_MEM_OBJECT";
    case -39: return "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR";
    case -40: return "CL_INVALID_IMAGE_SIZE";
    case -41: return "CL_INVALID_SAMPLER";
    case -42: return "CL_INVALID_BINARY";
    case -43: return "CL_INVALID_BUILD_OPTIONS";
    case -44: return "CL_INVALID_PROGRAM";
    case -45: return "CL_INVALID_PROGRAM_EXECUTABLE";
    case -46: return "CL_INVALID_KERNEL_NAME";
    case -47: return "CL_INVALID_KERNEL_DEFINITION";
    case -48: return "CL_INVALID_KERNEL";
    case -49: return "CL_INVALID_ARG_INDEX";
    case -50: return "CL_INVALID_ARG_VALUE";
    case -51: return "CL_INVALID_ARG_SIZE";
    case -52: return "CL_INVALID_KERNEL_ARGS";
    case -53: return "CL_INVALID_WORK_DIMENSION";
    case -54: return "CL_INVALID_WORK_GROUP_SIZE";
    case -55: return "CL_INVALID_WORK_ITEM_SIZE";
    case -56: return "CL_INVALID_GLOBAL_OFFSET";
    case -57: return "CL_INVALID_EVENT_WAIT_LIST";
    case -58: return "CL_INVALID_EVENT";
    case -59: return "CL_INVALID_OPERATION";
    case -60: return "CL_INVALID_GL_OBJECT";
    case -61: return "CL_INVALID_BUFFER_SIZE";
    case -62: return "CL_INVALID_MIP_LEVEL";
    case -63: return "CL_INVALID_GLOBAL_WORK_SIZE";
    case -64: return "CL_INVALID_PROPERTY";
    case -65: return "CL_INVALID_IMAGE_DESCRIPTOR";
    case -66: return "CL_INVALID_COMPILER_OPTIONS";
    case -67: return "CL_INVALID_LINKER_OPTIONS";
    case -68: return "CL_INVALID_DEVICE_PARTITION_COUNT";

    // extension errors
    case -1000: return "CL_INVALID_GL_SHAREGROUP_REFERENCE_KHR";
    case -1001: return "CL_PLATFORM_NOT_FOUND_KHR";
    case -1002: return "CL_INVALID_D3D10_DEVICE_KHR";
    case -1003: return "CL_INVALID_D3D10_RESOURCE_KHR";
    case -1004: return "CL_D3D10_RESOURCE_ALREADY_ACQUIRED_KHR";
    case -1005: return "CL_D3D10_RESOURCE_NOT_ACQUIRED_KHR";

    // extension errors for cl intel va media sharing
    case -1098: return "CL_INVALID_VA_API_MEDIA_ADAPTER_INTEL";
    case -1099: return "CL_INVALID_VA_API_MEDIA_SURFACE_INTEL";
    case -1100: return "CL_VA_API_MEDIA_SURFACE_ALREADY_ACQUIRED_INTEL";
    case -1101: return "CL_VA_API_MEDIA_SURFACE_NOT_ACQUIRED_INTEL";

    default: return "Unknown OpenCL error";
    }
}
