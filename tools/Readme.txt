1：rgb24 to yuv420p(i420)
ffmpeg -s 1280x720 -pix_fmt rgb24 -i sakura_1280x720.jpg -pix_fmt yuv420p sakura_1280x720_i420.yuv