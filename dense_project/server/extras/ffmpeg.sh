
#ffmpeg -i input.wmv -ss 60 -t 60 -acodec copy -vcodec copy output.wmv
#ffmpeg -i tears_of_steel_1080p.mov -ss 30 -t 120 -c:v libx264 -c:a aac -force_key_frames 'expr:gte(t,n_forced*2)' output2.mov

INPUT='output2.mov'

ffmpeg -err_detect ignore_err -i $INPUT \
                -map v:0 \
                -map v:0 \
                -map v:0 \
                -map a:0 \
                -c:v libx264 -s:v 720x720 -b:v 1000k \
                -force_key_frames 'expr:gte(t,n_forced*2)' \
                -c:a aac -b:a 128k              \
                -b:v:0                    1000k \
                -maxrate:v:0              1200k \
                -bufsize:v:0              1000k \
                -b:v:1                    4000k \
                -maxrate:v:1              4800k \
                -bufsize:v:1              4000k \
                -b:v:2                    6000k \
                -maxrate:v:2              7200k \
                -bufsize:v:2              6000k \
                -f tee \
                "[select=\'v:0,a\':f=hls\:hls_init_time=2\:hls_time=2\:hls_list_size=0\:hls_segment_type=mpegts]new2/first.m3u8|
                [select=\'v:1,a\':f=hls\:hls_init_time=2\:hls_time=2\:hls_list_size=0\:hls_segment_type=mpegts]new2/second.m3u8|
                [select=\'v:2,a\':f=hls\:hls_init_time=2\:hls_time=2\:hls_list_size=0\:hls_segment_type=mpegts]new2/third.m3u8"
  