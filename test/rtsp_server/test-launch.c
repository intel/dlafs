/* GStreamer
 * Copyright (C) 2008 Wim Taymans <wim.taymans at gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <math.h>

#include <gst/gst.h>

#include <gst/rtsp-server/rtsp-server.h>

#define DEFAULT_RTSP_PORT "8554"

static char *port = (char *) DEFAULT_RTSP_PORT;

static GOptionEntry entries[] = {
  {"port", 'p', 0, G_OPTION_ARG_STRING, &port,
      "Port to listen on (default: " DEFAULT_RTSP_PORT ")", "PORT"},
  {NULL}
};

#define RTSP_SERVER_NUM 24
#define BASE_PORT 8554

static const char*
get_local_ip_addr (void)
{
    int sock_fd;
    char* local_addr = NULL;
    struct sockaddr_in* addr;

    if ((sock_fd = socket (AF_INET, SOCK_DGRAM, 0)) >= 0) {
        struct ifreq ifr_ip;

        memset (&ifr_ip, 0, sizeof(ifr_ip));
        strncpy (ifr_ip.ifr_name, "enp3s0", sizeof(ifr_ip.ifr_name) - 1);

        if(ioctl (sock_fd, SIOCGIFADDR, &ifr_ip) >= 0) {
            addr = (struct sockaddr_in*)&ifr_ip.ifr_addr;
            local_addr = inet_ntoa (addr->sin_addr);
        }
        close (sock_fd);
    }

    return (const char*)local_addr;
}

int
main (int argc, char *argv[])
{
  GMainLoop *loop;
  GstRTSPServer *server[RTSP_SERVER_NUM];
  GstRTSPMountPoints *mounts;
  GstRTSPMediaFactory *factory[RTSP_SERVER_NUM];
  GOptionContext *optctx;
  GError *error = NULL;
  int i;
  char rtsp_port[RTSP_SERVER_NUM][12];
  char mount_point[RTSP_SERVER_NUM][12];

  optctx = g_option_context_new ("<launch line> - Test RTSP Server, Launch\n\n"
      "Example: \"( videotestsrc ! x264enc ! rtph264pay name=pay0 pt=96 )\"");
  g_option_context_add_main_entries (optctx, entries, NULL);
  g_option_context_add_group (optctx, gst_init_get_option_group ());
  if (!g_option_context_parse (optctx, &argc, &argv, &error)) {
    g_printerr ("Error parsing options: %s\n", error->message);
    g_option_context_free (optctx);
    g_clear_error (&error);
    return -1;
  }
  g_option_context_free (optctx);

  loop = g_main_loop_new (NULL, FALSE);

  for(i=0;i<RTSP_SERVER_NUM;i++) {
    /* create a server instance */
    server[i] = gst_rtsp_server_new ();
    g_snprintf(rtsp_port[i],8,"%d", BASE_PORT+i);
    g_object_set (server[i], "service", rtsp_port[i], NULL);

    /* get the mount points for this server, every server has a default object
       * that be used to map uri mount points to media factories */
    mounts = gst_rtsp_server_get_mount_points (server[i]);

    /* make a media factory for a test stream. The default media factory can use
       * gst-launch syntax to create pipelines.
       * any launch line works as long as it contains elements named pay%d. Each
       * element with pay%d names will be a stream */
    factory[i] = gst_rtsp_media_factory_new ();
    gst_rtsp_media_factory_set_launch (factory[i], argv[1]);
    gst_rtsp_media_factory_set_shared (factory[i], TRUE);

    /* attach the test factory to the /test url */
    g_snprintf(mount_point[i],12,"/video%d", i);
    gst_rtsp_mount_points_add_factory (mounts, mount_point[i], factory[i]);

    /* don't need the ref to the mapper anymore */
    g_object_unref (mounts);

    /* attach the server to the default maincontext */
    gst_rtsp_server_attach (server[i], NULL);
    g_print ("stream ready at rtsp://%s:%s/%s\n",get_local_ip_addr(), rtsp_port[i],mount_point[i]);
  }
  /* start serving */
  //g_print ("stream ready at rtsp://%s:%s/test\n",get_local_ip_addr(), port);
  g_main_loop_run (loop);

  return 0;
}
