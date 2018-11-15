/*
 * Copyright (c) 2018 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "hddlspipe.h"
#include <json-c/json.h>
#include "jsonparser.h"

#define CVDLFILTER_NAME "cvdlfilter0"
#define RESCONVERT_NAME "resconvert0"
#define WSSINK_NAME "wssink0"

static gchar g_pipe_desc[1024] = "null";
static gchar g_server_uri[128] = "null"; 
static gint g_pipe_id = 0;
static gint g_loop_times = 1;

static gchar g_default_server_uri[]= "wss://localhost:8123/binaryEchoWithSize?id=3";


#define HDDLSPIPE_SET_PROPERTY( hp, element_name, ...) \
    do { \
        GstElement *element = gst_bin_get_by_name (GST_BIN((hp)->pipeline), (element_name)); \
        if (NULL != element) { \
            g_object_set (element, __VA_ARGS__); \
            g_print ("Success to set property - element=%s\n", element_name); \
            gst_object_unref (element); \
        } else { \
            g_print ("### Can not find element '%s' ###\n", element_name); \
        } \
    } while (0)

 static void print_usage (const char* program_name, gint exit_code)
{
    g_print ("Usage: %s...\n", program_name);
    g_print (
        " -u --specify uri of ws server.\n"
        " -i --specify id of ws client.\n"
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
                g_snprintf(g_server_uri, 128, "%s", optarg);
                break;
            case 'i':
                 g_pipe_id  = atoi(optarg);
                break;
            case 'l':
                g_loop_times = atoi(optarg);
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
    
static void set_property(struct json_object *parent, HddlsPipe *hp, const char *filter_name)
{
        struct json_object *element = NULL, *property;
        struct json_object_iterator iter, end;
        const char *property_name = NULL;
        enum json_type property_type;
       const char *property_string = NULL;
        int property_int = 0;
        double property_double = 0.0;
        
        if( !json_get_object_d2(parent, "command_set_property", filter_name, &element) ) {
               g_print("It didn't find new property for %s\n", filter_name);
               return;
         }

         end = json_object_iter_end (element);
         iter = json_object_iter_begin (element);
         while (!json_object_iter_equal (&iter, &end)) {
                    property_name = json_object_iter_peek_name (&iter);
                    property = json_object_iter_peek_value (&iter);
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
}

static void process_commands(HddlsPipe *hp, char *desc)
{
     struct json_object *root = NULL;
     enum E_COMMAND_TYPE command_type = eCommand_None;

      root = json_create(desc);
      if(!root) {
            g_print("%s() - failed to create json object from description!\n",__func__);
            return;
     }

     g_print("pipe %d(%d) has got message: %s\n", hp->pipe_id, wsclient_get_id(hp->ws),  desc);
     command_type = json_get_command_type(root);
     switch(command_type){
        case eCommand_PipeCreate:
                 g_print("Error: this command should not be here!!!\n");
                 break;
        case eCommand_PipeDestroy:
                g_print("Receive command: destroy pipeline....\n");
                hddlspipe_stop (hp);
                 hp->state = ePipeState_Null;
                break;
        case eCommand_SetProperty:
                g_print("Receive command: set pipeline....\n");
                hddlspipe_pause( hp);
                // set property
                set_property(root, hp, CVDLFILTER_NAME);
                set_property(root, hp, RESCONVERT_NAME);
                set_property(root, hp, WSSINK_NAME);
                hddlspipe_resume(hp);
                break;
         default:
               g_print("Receive invalid message: %s\n", desc);
                break;
      }
}

static gpointer thread_handle_message(void *data)
{
        HddlsPipe *hp = (HddlsPipe*)data;
        MessageItem *item = NULL;
        while(TRUE) {
                item = wsclient_get_data_timed(hp->ws);
                if(item && item->len>0) {
                     // process command data
                     process_commands(hp, item->data);
                     wsclient_free_item(item);
                }
                if(hp->state==ePipeState_Null) //for thread quit
                    break;
        }
        return NULL;
}

static gchar* parse_create_command(char *desc,  gint pipe_id )
{
      struct json_object *root = NULL;
      struct json_object *object = NULL;
      const char *stream_source = NULL;
      const char *stream_codec_type = NULL;
      const char *algo_pipeline_desc = NULL;
      char helper_desc[256];
      E_CODEC_TYPE codec_type = eCodecTypeNone;

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
            // 1.3 parse property
            if(json_get_string_d2(object, CVDLFILTER_NAME, "algopipeline", &algo_pipeline_desc)) {
                     g_print("property - algopipeline = %s\n",algo_pipeline_desc);
             } else { //default
                    algo_pipeline_desc = "detection ! track ! classification";
             }
    }
    if(!stream_source || !stream_codec_type) {
            g_print("%s() - failed to get input stream source!\n", __func__);
            json_destroy(&root);
            return NULL;
     }

     // 2. generate pipe desc based on input stream source
     // 2.1 get codec type
     if(!strncmp(stream_codec_type,"H.264", 5) ||!strncmp(stream_codec_type,"h.264", 5) ) {
           // H.264
           codec_type = eCodecTypeH264;
     } else if(!strncmp(stream_codec_type,"H.265", 5)  || !strncmp(stream_codec_type,"h.265", 5) 
               || !strncmp(stream_codec_type,"HEVC", 4) || !strncmp(stream_codec_type,"hevc", 4)){
          // H.265
          codec_type = eCodecTypeH265;
     } else {
            g_print("%s() - failed to get valid codec type : %s\n", __func__,stream_codec_type );
            json_destroy(&root);
            return NULL;
     }

    g_snprintf(helper_desc, 256, 
         " ! cvdlfilter name=cvdlfilter0 algopipeline=\"%s\"  ! resconvert name=resconvert0 "
         " resconvert0.src_pic ! mfxjpegenc ! wssink name=wssink0 wsclientid=%d  "
         " resconvert0.src_txt ! wssink0.",  algo_pipeline_desc,  pipe_id);
     // 2.2 get source type: rtsp or local file
     if( g_strrstr_len(stream_source, 256, "rtsp")  || g_strrstr_len(stream_source, 256, "RTSP")) {
           // rtsp
           if(codec_type == eCodecTypeH264)
                g_snprintf (g_pipe_desc, 1024, "rtspsrc location=%s udp-buff-size=800000 ! rtph264depay "
                                      " ! h264parse ! mfxh264dec %s ",  stream_source,  helper_desc );
           else if(codec_type == eCodecTypeH265)
                 g_snprintf (g_pipe_desc, 1024, "rtspsrc location=%s udp-buff-size=800000 ! rtph265depay "
                                     " ! h265parse ! mfxh265dec  %s ", stream_source,  helper_desc );
     } else {
          // local files
           if(codec_type == eCodecTypeH264)
                g_snprintf (g_pipe_desc, 1024,
                     "filesrc location=%s  ! qtdemux  ! h264parse ! mfxh264dec %s",  stream_source,  helper_desc );
           else if(codec_type == eCodecTypeH265)
                 g_snprintf (g_pipe_desc, 1024,
                     "filesrc location=%s ! qtdemux  ! h265parse ! mfxh265dec  %s",  stream_source,  helper_desc );
    }
    g_print("pipeline: %s\n",g_pipe_desc );
    json_destroy(&root);

    return g_pipe_desc;
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
            g_error_free (err);
        }
        if (dbg) {
            g_print ("[Debug details: %s]\n", dbg);
            g_free (dbg);
        }
        hddlspipe_stop (hp);
        break;
    }
    case GST_MESSAGE_EOS:
        /* end-of-stream */
        g_print ("EOS\n");
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
  *   1. create hddls-pipe, connet to ws server
  *   2. wait to receive pipe desc from server, and setup pipe based on desc
  **/
 HddlsPipe*   hddlspipe_create( )
{
    HddlsPipe *hp = g_new0(HddlsPipe, 1);
    MessageItem *item = NULL;
    gchar* pipeline_desc = NULL;
    GError     *error = NULL;

   // 1. create ws client
   if(!strncmp(g_server_uri,"null", 4)) {
        hp->ws = wsclient_setup(g_default_server_uri,  g_pipe_id);
   } else {
         hp->ws = wsclient_setup(g_server_uri,  g_pipe_id);
    }
   hp->pipe_id = g_pipe_id;
    //it has connected to ws server.

   // Block wait until get desc data from ws server
    item = (MessageItem *)wsclient_get_data(hp->ws);
    g_print("%s() -pipe %d  received message: %s\n", __func__, hp->pipe_id, item->data);
    hp->state = ePipeState_Null;

    // parse pipeline_create command
    pipeline_desc = parse_create_command(item->data, hp->pipe_id);
    wsclient_free_item(item);
    if(!pipeline_desc) {
        g_print("Failed to get pipeline description!\n");
        return NULL;
    }
    //create pipeline
    hp->pipeline = gst_parse_launch (pipeline_desc, &error);
   if (error || ! hp->pipeline) {
       g_print ("failed to build pipeline: error message: %s\n",(error) ? error->message : NULL);
       return NULL;
   }
    HDDLSPIPE_SET_PROPERTY( hp, WSSINK_NAME, "wsclientproxy", hp->ws, NULL);

    // set watch bus
    GstBus *bus = gst_element_get_bus (hp->pipeline);
    hp->bus_watch_id = gst_bus_add_watch (bus, bus_callback,hp);
    gst_object_unref (bus);

    hp->loop = g_main_loop_new (NULL, FALSE);
    hp->state = ePipeState_Ready;

    // start command thread
    hp->message_handle_thread = g_thread_create(thread_handle_message, (void *)hp, TRUE, NULL );
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

      g_source_remove (hp->bus_watch_id);
      gst_object_unref (hp->pipeline);
      g_main_loop_unref (hp->loop);

      hp->pipeline = gst_parse_launch (g_pipe_desc, &error);
      if (error) {
          g_print ("ERROR: %s\n", error->message);
          //g_error_free (error);
      }
       HDDLSPIPE_SET_PROPERTY( hp, WSSINK_NAME, "wsclientproxy", hp->ws, NULL);

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
          if( hp->state != ePipeState_Null)
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

    if(hp->ws)
        wsclient_destroy(hp->ws);
    hp->ws = NULL;

    g_source_remove (hp->bus_watch_id);
    gst_object_unref (hp->pipeline);
    g_main_loop_unref (hp->loop);

    g_free(hp);
}
