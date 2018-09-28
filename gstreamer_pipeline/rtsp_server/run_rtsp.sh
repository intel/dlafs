killall -9 lt-test-launch
ipaddr=`ifconfig | grep "inet addr:10" | cut -d ":" -f 2 | cut -d " " -f 1`
echo "RTSP:rtsp://$ipaddr:8554/video0"
echo "rtsp://$ipaddr:8554/video0" >> rtsp.txt
./test-launch --gst-debug=3 "( filesrc location=/home/rtsp/1600x1200_concat.mp4 ! qtdemux ! rtph264pay name=pay0 pt=96 )"
