/*Copyright (C) 2018 Intel Corporation
  * 
  *SPDX-License-Identifier: MIT
  *
  *MIT License
  *
  *Copyright (c) 2018 Intel Corporation
  *
  *Permission is hereby granted, free of charge, to any person obtaining a copy of
  *this software and associated documentation files (the "Software"),
  *to deal in the Software without restriction, including without limitation the rights to
  *use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
  *and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
  *
  *The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
  *
  *THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
  *INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
  *AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
  *DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  *OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
  */

#include "hddlspipe.h"
#include <json-c/json.h>
#include "jsonparser.h"

using namespace std;
#include <string>
#include <vector>

#define CVDLFILTER_NAME "cvdlfilter0"
#define RESCONVERT_NAME "resconvert0"
#define IPCSINK_NAME "ipcsink0"

const std::vector<std::string> g_filter_name_vec = {
    "resconvert0",  "ipcsink0",  "mfxjpegenc",
    "rtph264depay", "h264parse", "mfxh264dec",
    "rtph265depay", "h265parse", "mfxhevcdec",
    "rtspsrc",      "udpsrc",    "srtpdec"
};


static std::string g_str_pipe_desc = std::string("null");
static std::string g_str_server_uri = std::string("null");
static std::string g_str_algopipeline = std::string("null");
static std::string g_str_default_server_uri = std::string("wss://localhost:8123/binaryEchoWithSize?id=3");
static std::string g_str_srtp_desc = std::string("null");
static std::string g_str_helper_desc = std::string("null");

static gint g_pipe_id = 0;
static gint g_loop_times = 1;

#define DEFAULT_ALGO_PIPELINE "yolov1tiny ! opticalflowtrack ! googlenetv2"

#define HDDLSPIPE_SET_PROPERTY( hp, element_name, ...) \
    do { \
        GstElement *element = gst_bin_get_by_name (GST_BIN((hp)->pipeline), (element_name)); \
        if (NULL != element) { \
            g_object_set (element, __VA_ARGS__); \
            GST_INFO ("Success to set property - element=%s\n", element_name); \
            gst_object_unref (element); \
        } else { \
            g_print ("### Can not find element '%s' ###\n", element_name); \
        } \
    } while (0)

 static void print_usage (const char* program_name, gint exit_code)
{
    g_print ("Usage: %s...\n", program_name);
    g_print (
        " -u --specify uri of ipc server.\n"
        " -i --specify id of ipc client.\n"
        " -l --specify the loop times.\n"
         "-h --help Display this usage information.\n");
    exit (exit_code);
  }

static gboolean  parse_cmdline (int argc, char *argv[])
{
     const char* const brief = "hu:i:l:";
      const struct option details[] = {
                { "serveruri", 1, NULL, 'u'},
                { "clientid", 1, NULL, 'i',},
                { "looptimes", 1, NULL, 'l'},
                { "help", 0, NULL, 'h'},
                { NULL, 0, NULL, 0 }
       };
        
    int opt = 0;
    while (opt != -1) {
        opt = getopt_long (argc, argv, brief, details, NULL);
        switch (opt) {
            case 'u':
                //g_snprintf(g_server_uri, 128, "%s", optarg);
                g_str_server_uri = std::string(optarg);
                break;
            case 'i':
                 g_pipe_id  = atoi(optarg);
                break;
            case 'l':
                g_loop_times = atoi(optarg);
                if(g_loop_times<1)
                    g_loop_times = 1;
                break;
            case 'h': /* help */
                print_usage (argv[0], 0);
                break;
            case '?': /* an invalid option. */
                print_usage (argv[0], 1);
                break;
            case -1: /* Done with options. */
                break;
             default: /* unexpected. */
                print_usage (argv[0], 1);
                abort ();
         }
     }
     return TRUE;
}
    
static int set_property(struct json_object *parent, HddlsPipe *hp, const char *filter_name)
{
        struct json_object *element = NULL, *property;
        struct json_object_iterator iter, end;
        const char *property_name = NULL;
        std::string  str_property_name;
        enum json_type property_type;
        const char *property_string = NULL;
        int property_int = 0;
        double property_double = 0.0;
        int ret = 0;
        
        if( !json_get_object_d2(parent, "command_set_property", filter_name, &element) ) {
               g_print("It didn't find new property for %s\n", filter_name);
               return -1;
         }

         end = json_object_iter_end (element);
         iter = json_object_iter_begin (element);
         while (!json_object_iter_equal (&iter, &end)) {
                    property_name = json_object_iter_peek_name (&iter);
                    property = json_object_iter_peek_value (&iter);
                    str_property_name = std::string(property_name);
                    if(!str_property_name.compare(0, 12, "algopipeline")) {
                        property_string = json_object_get_string (property);
                        g_str_algopipeline =  std::string(property_string);
                        GST_INFO("g_algopipeline = %s\n ",g_str_algopipeline.c_str());
                        ret = 1;
                        json_object_iter_next (&iter);
                        continue;
                    }
                    property_type = json_object_get_type (property);
                    switch (property_type) {
                        case json_type_string:
                                property_string = json_object_get_string (property);
                                HDDLSPIPE_SET_PROPERTY( hp, filter_name, property_name, property_string, NULL);
                                break;
                         case json_type_int:
                                property_int = json_object_get_int (property);
                                HDDLSPIPE_SET_PROPERTY( hp, filter_name, property_name, property_int, NULL);
                                break;
                        case json_type_double:
                                property_int = json_object_get_double (property);
                                HDDLSPIPE_SET_PROPERTY( hp, filter_name, property_name, property_double, NULL);
                                break;
                        default:
                                g_print ("Unkown property type!\n");
                                break;
                        }
                    json_object_iter_next (&iter);
          }
          return ret;
}

static int do_set_properties(struct json_object *parent, HddlsPipe *hp)
{
    int ret = 0;
    // set property for algopipline
    ret = set_property(parent, hp, CVDLFILTER_NAME);

    //set property for other plugins
    for(unsigned int i=0;i<g_filter_name_vec.size();i++)
        set_property(parent, hp, g_filter_name_vec[i].c_str());

    return ret;
}
static void process_commands(HddlsPipe *hp, char *desc)
{
     struct json_object *root = NULL;
     enum E_COMMAND_TYPE command_type = eCommand_None;
     int ret = 0;

      root = json_create(desc);
      if(!root) {
            g_print("%s() - failed to create json object from description!\n",__func__);
            return;
     }

     g_print("pipe %d(%d) has got message: %s\n", hp->pipe_id, ipcclient_get_id(hp->ipc),  desc);
     command_type = json_get_command_type(root);
     switch(command_type){
        case eCommand_PipeCreate:
                 GST_WARNING("Error: this command should not be here!!!\n");
                 break;
        case eCommand_PipeDestroy:
                g_print("Receive command: destroy pipeline....\n");
                hddlspipe_stop (hp);
                hp->state = ePipeState_Null;
                break;
        case eCommand_SetProperty:
                GST_INFO("Receive command: set pipeline....\n");
                hddlspipe_pause( hp);
                // set property
                ret = do_set_properties(root, hp);
                hddlspipe_resume(hp);
                break;
         default:
               g_print("Receive invalid message: %s\n", desc);
                break;
      }
      if(ret==1) {
         g_loop_times++;
         //update g_pipe_desc
         // filesrc location=~/1600x1200_concat.mp4  ! qtdemux  ! h264parse
         // ! mfxh264dec  ! cvdlfilter name=cvdlfilter0 algopipeline="yolov1tiny ! opticalflowtrack ! googlenetv2"
         // ! resconvert name=resconvert0  resconvert0.src_pic ! mfxjpegenc
         // ! ipcsink name=ipcsink0 ipcclientid=13   resconvert0.src_txt ! ipcsink0.
         gchar* begin = g_strstr_len(g_str_pipe_desc.c_str(), 1024, "algopipeline=" );
         gchar* end = NULL;
         gchar *secA = NULL, *secC=NULL;
         if(begin) {
                begin = begin+14;
                end = g_strstr_len(begin,64, "\"" );
                if(end) {
                    secA = g_strndup(g_str_pipe_desc.c_str(), begin - g_str_pipe_desc.c_str());
                    secC = end;//g_strndup(end, strnlen_s(end, 1024));
                    // replace g_pipe_desc
                    GST_INFO("old g_pipe_desc = %s\n", g_str_pipe_desc.c_str());
                    g_str_pipe_desc = std::string(secA) + g_str_algopipeline + std::string(secC);
                    GST_INFO("new g_pipe_desc = %s\n", g_str_pipe_desc.c_str());
                    if(secA && secC)
                        hddlspipe_stop (hp);
                    if(secA) g_free(secA);
                }
        }
      }
}

static gpointer thread_handle_message(void *data)
{
        HddlsPipe *hp = (HddlsPipe*)data;
        MessageItem *item = NULL;
        while(TRUE) {
                item = ipcclient_get_data_timed(hp->ipc);
                if(item && item->len>0) {
                     // process command data
                     process_commands(hp, item->data);
                     ipcclient_free_item(item);
                }
                if(hp->state==ePipeState_Null) //for thread quit
                    break;
        }
        return NULL;
}

static const gchar* parse_create_command(char *desc,  gint pipe_id, IPCClientHandle ipc)
{
      struct json_object *root = NULL;
      struct json_object *object = NULL;
      const char *stream_source = NULL;
      const char *stream_codec_type = NULL;
      const char *algo_pipeline_desc = NULL;
      const char *srtp_caps = NULL;
      //char helper_desc[256];
      E_CODEC_TYPE codec_type = eCodecTypeNone;
      int srtp_port = 5000;
      int output_type = 1;// 0 - meta_data; 1 - meta data + jpeg data  

     root = json_create(desc);
     if(!root) {
        g_print("%s() - failed to create json object!\n",__func__);
        return NULL;
     }

     if(json_get_command_type(root) != eCommand_PipeCreate) {
            g_print("%s() - failed to create pipeline due to invalid command from server!\n", __func__);
            json_destroy(&root);
            return NULL;
     }

    // 1. parse command_create
   if (json_object_object_get_ex (root, "command_create", &object) &&
        json_object_is_type (object, json_type_object)) {
            //1.1 parse input stream source
            if(json_get_string(object, "stream_source", &stream_source)) {
                g_print("input source = %s\n",stream_source);
            }
            // 1.2 codec_type
            if(json_get_string(object, "codec_type", &stream_codec_type)) {
                g_print("stream codec type = %s\n",stream_codec_type);
            }
            if(json_get_int(object, "output_type", &output_type)) {
                g_print("output type = %d\n",output_type);
            }
            // 1.3 srtp parameter if need
            if(json_get_string(object, "srtp_caps", &srtp_caps)) {
                g_print("srtp_caps = %s\n",srtp_caps);
           }
           if(json_get_int(object, "srtp_port", &srtp_port)) {
                g_print("srtp_caps = %d\n",srtp_port);
           }
            // 1.4 parse property
            if(json_get_string_d2(object, CVDLFILTER_NAME, "algopipeline", &algo_pipeline_desc)) {
                    GST_INFO("property - algopipeline = %s\n",algo_pipeline_desc);
                    std::string str_algo_pipeline_desc = std::string(algo_pipeline_desc);
                    if(str_algo_pipeline_desc.size()<1) {
                         algo_pipeline_desc = DEFAULT_ALGO_PIPELINE;
                         //g_print("warning - get invalid algopipeline:%s , it will use default value: %s!\n",
                         //   str_algo_pipeline_desc.c_str(),  algo_pipeline_desc);
                         g_print("warning - get empty algopipeline, exit!\n");
                         ipcclient_upload_error_info(ipc, "warning - get empty algopipeline, exit!\n");
                         g_usleep(10000);
                         exit(eErrorInvalideAlgopipeline);
                     }
             } else { //default
                    algo_pipeline_desc = DEFAULT_ALGO_PIPELINE;
                    g_print("warning - failed to get algopipeline, exit!\n");
                    ipcclient_upload_error_info(ipc, "warning - failed to get algopipeline, exit!\n");
                    g_usleep(10000);
                    exit(eErrorInvalideAlgopipeline);
             }
    }
    if(!stream_source || !stream_codec_type) {
            //g_print("error - failed to get input stream source:%s,  stream_codec_type=%s\n",
            //    stream_source, stream_codec_type);
            std::string err_info = std::string("error - failed to get input stream source: ") + std::string(stream_source) +
                std::string(", stream_codec_type=") + std::string(stream_codec_type) + std::string("\n");
            ipcclient_upload_error_info(ipc, err_info.c_str());
            g_print("%s",err_info.c_str());
            json_destroy(&root);
            return NULL;
     }

     // 2. generate pipe desc based on input stream source
     // 2.1 get codec type
     std::string str_stream_codec_type = std::string(stream_codec_type);
     if(!str_stream_codec_type.compare( "H.264") ||!str_stream_codec_type.compare("h.264") ) {
           // H.264
           codec_type = eCodecTypeH264;
     } else if(!str_stream_codec_type.compare("H.265")  || !str_stream_codec_type.compare( "h.265") 
               || !str_stream_codec_type.compare( "HEVC") || !str_stream_codec_type.compare( "hevc")){
          // H.265
          codec_type = eCodecTypeH265;
     } else {
            //g_print("error- failed to get valid codec type : %s\n", stream_codec_type );
            std::string err_info = std::string("error- failed to get valid codec type : ") + str_stream_codec_type + std::string("\n");
            ipcclient_upload_error_info(ipc, err_info.c_str());
            g_print("%s",err_info.c_str());
            json_destroy(&root);
            return NULL;
     }
     if(output_type) {
        // meta data + jpeg data
        g_str_helper_desc = std::string(" ! cvdlfilter name=cvdlfilter0 algopipeline=\"") + std::string(algo_pipeline_desc) + std::string("\" ") +
                          std::string(" ! resconvert name=resconvert0 resconvert0.src_pic ! mfxjpegenc ! ipcsink name=ipcsink0 ipcclientid=") +
                          std::to_string(pipe_id)  + std::string("  resconvert0.src_txt ! ipcsink0.");
     }else {
       // meta data
       g_str_helper_desc = std::string(" ! cvdlfilter name=cvdlfilter0 algopipeline=\"") + std::string(algo_pipeline_desc) + std::string("\" ") +
                          std::string(" ! resconvert name=resconvert0 resconvert0.src_txt ! ipcsink name=ipcsink0 ipcclientid=") +
                          std::to_string(pipe_id);
     }

    if( ((std::string(stream_source).compare("srtp")) || (std::string(stream_source).compare("SRTP"))) && srtp_caps) {
        g_str_srtp_desc = std::string("udpsrc") + std::string(" port=") + std::to_string(srtp_port) +
                                         std::string(" caps=\"") + std::string(srtp_caps) + std::string("\" ") +  std::string(" ! srtpdec ! ");

        if(codec_type == eCodecTypeH264) {
             g_str_pipe_desc = g_str_srtp_desc +
                                               std::string(" rtph264depay  ! h264parse ! mfxh264dec ") + g_str_helper_desc;
        } else if(codec_type == eCodecTypeH265) {
             g_str_pipe_desc =g_str_srtp_desc +
                                             std::string("  rtph265depay  ! h265parse ! mfxhevcdec ") +  g_str_helper_desc;
         }

        g_print("pipeline: %s\n",g_str_pipe_desc.c_str() );
        json_destroy(&root);

        return g_str_pipe_desc.c_str();
    }

     // 2.2 get source type: rtsp or local file
     if( g_strrstr_len(stream_source, 256, "rtsp")  || g_strrstr_len(stream_source, 256, "RTSP")) {
           // rtsp
           if(codec_type == eCodecTypeH264) {
                g_str_pipe_desc = std::string("rtspsrc location=") + std::string(stream_source)
                                                +std::string(" udp-buffer-size=1000000 ! rtph264depay  ! h264parse ! mfxh264dec ")
                                                +g_str_helper_desc;
           } else if(codec_type == eCodecTypeH265) {
                g_str_pipe_desc = std::string("rtspsrc location=") + std::string(stream_source)
                                                +std::string(" udp-buffer-size=1000000 ! rtph265depay  ! h265parse ! mfxhevcdec ")
                                                +g_str_helper_desc;
            }
     } else {
          // local files
           if(codec_type == eCodecTypeH264) {
                g_str_pipe_desc = std::string("filesrc location=") + std::string(stream_source)
                                                +std::string("  ! qtdemux  ! h264parse ! mfxh264dec ")
                                                +g_str_helper_desc;
           } else if(codec_type == eCodecTypeH265) {
                 g_str_pipe_desc = std::string("filesrc location=") + std::string(stream_source)
                                                +std::string("  ! qtdemux  ! h265parse ! mfxhevcdec ")
                                                +g_str_helper_desc;
            }
    }
    g_print("pipeline: %s\n",g_str_pipe_desc.c_str() );
    json_destroy(&root);

    return g_str_pipe_desc.c_str();
}

static gboolean bus_callback (GstBus* bus, GstMessage* msg, gpointer data)
{
    HddlsPipe *hp = (HddlsPipe*) data;

    switch (GST_MESSAGE_TYPE (msg)) {
    case GST_MESSAGE_ERROR: {
        GError *err = NULL; /* error to show to users                 */
        gchar *dbg = NULL;  /* additional debug string for developers */

        gst_message_parse_error (msg, &err, &dbg);
        if (err) {
            g_print ("ERROR: %s\n", err->message);
            ipcclient_upload_error_info(hp->ipc, err->message);
            g_error_free (err);
        }
        if (dbg) {
            GST_INFO ("[Debug details: %s]\n", dbg);
            ipcclient_upload_error_info(hp->ipc, dbg);
            g_free (dbg);
        }
        hddlspipe_stop (hp);
        break;
    }
    case GST_MESSAGE_EOS:
        /* end-of-stream */
        GST_INFO ("EOS\n");
        //ipcclient_upload_error_info(hp->ipc, "Got EOS");
        hddlspipe_stop (hp);
        break;
    default:
        /* unhandled message */
        //LOG_DEBUG ("unhandled message");
        break;
    }

    /* we want to be notified again the next time there is a message
     * on the bus, so returning TRUE (FALSE means we want to stop watching
     * for messages on the bus and our callback should not be called again)
     */
    return TRUE;
}

void hddlspipe_prepare(int argc, char **argv)
{
        parse_cmdline(argc, argv);
        gst_init (&argc, &argv);
}

/**
  *   1. create hddls-pipe, connet to ipc server
  *   2. wait to receive pipe desc from server, and setup pipe based on desc
  **/
 HddlsPipe*   hddlspipe_create( )
{
    HddlsPipe *hp = g_new0(HddlsPipe, 1);
    MessageItem *item = NULL;
    const gchar* pipeline_desc = NULL;
    GError     *error = NULL;

   // 1. create ipc client
   //if(!strcmp_s(g_server_uri, 4, "null", &indicator)) {
   if(!g_str_server_uri.compare("null")) {
        hp->ipc = ipcclient_setup(g_str_default_server_uri.c_str(),  g_pipe_id);
   } else {
         hp->ipc = ipcclient_setup(g_str_server_uri.c_str(),  g_pipe_id);
    }
   hp->pipe_id = g_pipe_id;
    //it has connected to ipc server.

   // Block wait until get desc data from ipc server
    item = (MessageItem *)ipcclient_get_data(hp->ipc);
    GST_INFO("%s() -pipe %d  received message: %s\n", __func__, hp->pipe_id, item->data);
    hp->state = ePipeState_Null;

    // parse pipeline_create command
    pipeline_desc = parse_create_command(item->data, hp->pipe_id, hp->ipc);
    ipcclient_free_item(item);
    if(!pipeline_desc) {
        g_print("Failed to get pipeline description!\n");
        return NULL;
    }
    //create pipeline
    hp->pipeline = gst_parse_launch (pipeline_desc, &error);
   if (error || ! hp->pipeline) {
       g_print ("failed to build pipeline, error message: %s\n",(error) ? error->message : NULL);
       return NULL;
   }
    HDDLSPIPE_SET_PROPERTY( hp, IPCSINK_NAME, "ipcclientproxy", hp->ipc, NULL);

    // set watch bus
    GstBus *bus = gst_element_get_bus (hp->pipeline);
    hp->bus_watch_id = gst_bus_add_watch (bus, bus_callback,hp);
    gst_object_unref (bus);

    hp->loop = g_main_loop_new (NULL, FALSE);
    hp->state = ePipeState_Ready;

    // start command thread
    hp->message_handle_thread = g_thread_new("message_thread",thread_handle_message, (void *)hp);
    return hp;
}


/**
  *  Thread will loop in this function until calling hddlspipe_stop().
  **/
 void  hddlspipe_start(HddlsPipe * hp)
{
     g_assert (hp);
     gst_element_set_state (hp->pipeline, GST_STATE_PLAYING);
     hp->state = ePipeState_Running;
     g_main_loop_run (hp->loop);
}

 /**
   *  Stop hddlspipe.
   **/
 void hddlspipe_stop(HddlsPipe *hp)
{
     g_assert (hp);
     gst_element_set_state (hp->pipeline, GST_STATE_NULL);
     hp->state = ePipeState_Ready;
     g_main_loop_quit (hp->loop);
}

 /**
   *  Resume hddlspipe.
   **/
 void  hddlspipe_resume (HddlsPipe *hp)
{
    g_assert (hp);
    gst_element_set_state (hp->pipeline, GST_STATE_PLAYING);
     hp->state = ePipeState_Running;
}

  /**
   *  Pause hddlspipe.
   **/
 void hddlspipe_pause(HddlsPipe *hp)
{
     g_assert (hp);
     gst_element_set_state (hp->pipeline, GST_STATE_PAUSED);
    // gst_element_set_state (hp->pipeline, GST_STATE_READY);
     //gst_element_set_state (hp->pipeline, GST_STATE_NULL);
     hp->state = ePipeState_Ready;
}

static void hddlspipes_replay(HddlsPipe *hp)
{
      GError     *error = NULL;
      if(!hp) {
        g_print("Error: hp==null!\n");
        return;
      }

      g_source_remove (hp->bus_watch_id);
      gst_object_unref (hp->pipeline);
      g_main_loop_unref (hp->loop);

      hp->pipeline = gst_parse_launch (g_str_pipe_desc.c_str(), &error);
      if (error) {
          g_print ("ERROR: %s\n", error->message);
          //g_error_free (error);
      }
       HDDLSPIPE_SET_PROPERTY( hp, IPCSINK_NAME, "ipcclientproxy", hp->ipc, NULL);

        // set watch bus
       GstBus *bus = gst_element_get_bus (hp->pipeline);
       hp->bus_watch_id = gst_bus_add_watch (bus, bus_callback,hp);
       gst_object_unref (bus);

       hp->loop = g_main_loop_new (NULL, FALSE);
       hp->state = ePipeState_Ready;

       hddlspipe_start(hp);
}

void  hddlspipes_replay_if_need(HddlsPipe *hp)
{
     while( --g_loop_times > 0) {
          if( hp && hp->state != ePipeState_Null)
                hddlspipes_replay(hp);
     }
}

/**
  *  Destroy hddlspipe.
  **/
void hddlspipe_destroy(HddlsPipe *hp)
{
    hp->state = ePipeState_Null;
    if(hp->message_handle_thread)
        g_thread_join(hp->message_handle_thread);
    hp->message_handle_thread = NULL;

    if(hp->ipc)
        ipcclient_destroy(hp->ipc);
    hp->ipc = NULL;

    g_source_remove (hp->bus_watch_id);
    gst_object_unref (hp->pipeline);
    g_main_loop_unref (hp->loop);

    g_free(hp);
}
