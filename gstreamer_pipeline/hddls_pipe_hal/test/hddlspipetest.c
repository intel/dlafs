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

  /*
  * Author: River,Li
  * Email: river.li@intel.com
  * Date: 2018.10
  */

#include "stdio.h"
#include "hddlspipe.h"
#include <json-c/json.h>
#include "jsonparser.h"

#define CVDLFILTER_NAME "cvdlfilter0"
#define RESCONVERT_NAME "resconvert0"
#define WSSINK_NAME "wssink0"

static gchar g_pipe_desc[1024] = "null";
static gchar g_json_file_name[256] = "null"; 
static gint g_pipe_id = 0;
static gint g_loop_times = 1;

#define DEFAULT_ALGO_PIPELINE "yolov1tiny ! opticalflowtrack ! googlenetv2"

 static void print_usage (const char* program_name, gint exit_code)
{
    g_print ("Usage: %s...\n", program_name);
    g_print (
        " -j --specify json file name with full path.\n"
        " -i --specify id of ws client.\n"
        " -l --specify the loop times.\n"
         "-h --help Display this usage information.\n");
    exit (exit_code);
  }

static gboolean  parse_cmdline (int argc, char *argv[])
{
     const char* const brief = "hj:i:l:";
      const struct option details[] = {
                { "jsonfile", 1, NULL, 'j'},
                { "clientid", 1, NULL, 'i',},
                { "looptimes", 1, NULL, 'l'},
                { "help", 0, NULL, 'h'},
                { NULL, 0, NULL, 0 }
       };
        
    int opt = 0;
    while (opt != -1) {
        opt = getopt_long (argc, argv, brief, details, NULL);
        switch (opt) {
            case 'j':
                g_snprintf(g_json_file_name, 256, "%s", optarg);
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
                     if(strlen(algo_pipeline_desc)<2)
                         algo_pipeline_desc = DEFAULT_ALGO_PIPELINE;
             } else { //default
                    algo_pipeline_desc = DEFAULT_ALGO_PIPELINE;
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
         " resconvert0.src_pic ! mfxjpegenc ! filesink location=./temp/hddls.jpeg",  algo_pipeline_desc);
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
    gchar* pipeline_desc = NULL;
    GError     *error = NULL;
    FILE *pfJson = fopen(g_json_file_name,"r");
   gchar  json_buffer[2048];

    if(!pfJson) {
        g_print("Cannot open %s\n",g_json_file_name );
        exit(1);
   }
    int count = fread(json_buffer, 1,2048,pfJson);
	if(count<=0)
		g_print("Error: empty json file data!\n");
    fclose(pfJson);

   hp->pipe_id = g_pipe_id;
   hp->state = ePipeState_Null;

    // parse pipeline_create command
    pipeline_desc = parse_create_command(json_buffer, hp->pipe_id);
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

    // set watch bus
    GstBus *bus = gst_element_get_bus (hp->pipeline);
    hp->bus_watch_id = gst_bus_add_watch (bus, bus_callback,hp);
    gst_object_unref (bus);

    hp->loop = g_main_loop_new (NULL, FALSE);
    hp->state = ePipeState_Ready;

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

    g_source_remove (hp->bus_watch_id);
    gst_object_unref (hp->pipeline);
    g_main_loop_unref (hp->loop);

    g_free(hp);
}
