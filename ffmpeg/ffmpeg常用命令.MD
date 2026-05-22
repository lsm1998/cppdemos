# FFmpeg 常用命令

## 1. 查询信息

```bash
# 查看版本信息
ffmpeg -version

# 查看支持的编解码器
ffmpeg -codecs

# 查看支持的封装格式
ffmpeg -formats

# 查看支持的滤镜
ffmpeg -filters

# 查看设备列表（Linux）
ffmpeg -devices

# 查看文件详细信息
ffprobe -v quiet -print_format json -show_format -show_streams input.mp4
```

## 2. 格式转换

```bash
# MP4 转 AVI
ffmpeg -i input.mp4 output.avi

# MKV 转 MP4（不重新编码，仅换容器）
ffmpeg -i input.mkv -c copy output.mp4

# 批量转换（Bash）
for f in *.mkv; do ffmpeg -i "$f" -c copy "${f%.mkv}.mp4"; done
```

## 3. 视频编码

```bash
# H.264 编码（libx264）
ffmpeg -i input.mp4 -c:v libx264 -preset medium -crf 23 -c:a aac -b:a 128k output.mp4

# H.265/HEVC 编码（libx265）
ffmpeg -i input.mp4 -c:v libx265 -preset medium -crf 28 -c:a aac -b:a 128k output.mp4

# 硬件加速编码（NVIDIA NVENC）
ffmpeg -i input.mp4 -c:v h264_nvenc -preset p4 -cq 23 -c:a aac -b:a 128k output.mp4

# 硬件加速编码（Intel QSV）
ffmpeg -i input.mp4 -c:v h264_qsv -global_quality 23 -c:a aac -b:a 128k output.mp4

# 码率控制（固定码率）
ffmpeg -i input.mp4 -c:v libx264 -b:v 2M -maxrate 2M -bufsize 4M -c:a aac -b:a 128k output.mp4
```

## 4. 音频处理

```bash
# 提取音频
ffmpeg -i input.mp4 -vn -c:a copy output.aac

# 提取音频并转 MP3
ffmpeg -i input.mp4 -vn -c:a libmp3lame -q:a 2 output.mp3

# 合并音频和视频（用音频替换原音频）
ffmpeg -i video.mp4 -i audio.mp3 -c:v copy -c:a aac -map 0:v:0 -map 1:a:0 output.mp4

# 调整音量（2 倍）
ffmpeg -i input.mp4 -af "volume=2.0" output.mp4

# 去除音频（静音输出）
ffmpeg -i input.mp4 -an -c:v copy output.mp4
```

## 5. 剪辑与拼接

```bash
# 截取片段（-ss 起始时间，-t 持续时长）
ffmpeg -i input.mp4 -ss 00:01:30 -t 00:00:30 -c copy output.mp4

# 截取片段（-to 结束时间）
ffmpeg -i input.mp4 -ss 00:01:30 -to 00:02:00 -c copy output.mp4

# 精确剪辑（重新编码，避免关键帧问题）
ffmpeg -i input.mp4 -ss 00:01:30 -t 00:00:30 -c:v libx264 -c:a aac output.mp4

# 拼接多个视频（先转为 ts 再拼接）
ffmpeg -i part1.mp4 -c copy -bsf:v h264_mp4toannexb -f mpegts temp1.ts
ffmpeg -i part2.mp4 -c copy -bsf:v h264_mp4toannexb -f mpegts temp2.ts
ffmpeg -i "concat:temp1.ts|temp2.ts" -c copy -bsf:a aac_adtstoasc output.mp4

# 使用 concat 文件列表拼接（无损）
# filelist.txt 内容：
# file 'part1.mp4'
# file 'part2.mp4'
ffmpeg -f concat -safe 0 -i filelist.txt -c copy output.mp4
```

## 6. 分辨率与画面

```bash
# 缩放（指定宽高）
ffmpeg -i input.mp4 -vf "scale=1280:720" -c:a copy output.mp4

# 缩放（指定宽度，高度按比例）
ffmpeg -i input.mp4 -vf "scale=1280:-1" -c:a copy output.mp4

# 缩放（指定高度，宽度按比例）
ffmpeg -i input.mp4 -vf "scale=-1:720" -c:a copy output.mp4

# 裁剪（宽:高:起始x:起始y）
ffmpeg -i input.mp4 -vf "crop=640:480:100:50" -c:a copy output.mp4

# 旋转（90度）
ffmpeg -i input.mp4 -vf "transpose=1" -c:a copy output.mp4

# 画面翻转（水平）
ffmpeg -i input.mp4 -vf "hflip" -c:a copy output.mp4

# 画面翻转（垂直）
ffmpeg -i input.mp4 -vf "vflip" -c:a copy output.mp4
```

## 7. 帧率处理

```bash
# 修改帧率（强制输出 30fps）
ffmpeg -i input.mp4 -r 30 -c:v libx264 -c:a copy output.mp4

# 帧率减半（删除每 2 帧中的 1 帧）
ffmpeg -i input.mp4 -vf "fps=15" -c:v libx264 -c:a copy output.mp4
```

## 8. 关键帧/截图

```bash
# 指定时间点截图
ffmpeg -i input.mp4 -ss 00:00:05 -vframes 1 output.jpg

# 每隔 N 秒截一张图
ffmpeg -i input.mp4 -vf "fps=1/10" img_%04d.png    # 每 10 秒一张

# 生成缩略图网格（3x3，总共 9 帧）
ffmpeg -i input.mp4 -vf "fps=1/10,scale=320:-1,tile=3x3" -frames:v 1 thumbnail.jpg
```

## 9. 水印与字幕

```bash
# 添加图片水印（左上角）
ffmpeg -i input.mp4 -i logo.png -filter_complex "overlay=10:10" output.mp4

# 添加图片水印（右下角）
ffmpeg -i input.mp4 -i logo.png -filter_complex "overlay=W-w-10:H-h-10" output.mp4

# 添加文字水印
ffmpeg -i input.mp4 -vf "drawtext=text='MyWatermark':fontsize=24:fontcolor=white:x=10:y=10" output.mp4

# 烧录硬字幕到视频
ffmpeg -i input.mp4 -vf "subtitles=subtitle.srt" output.mp4

# 提取内挂软字幕
ffmpeg -i input.mkv -map 0:s:0 subtitle.srt
```

## 10. 流操作

```bash
# 提取视频流（去除音频）
ffmpeg -i input.mp4 -an -c:v copy output.mp4

# 提取指定流
ffmpeg -i input.mp4 -map 0:v -c:v copy video.mp4 -map 0:a -c:a copy audio.aac

# 移除指定流（如移除第 2 条字幕流）
ffmpeg -i input.mkv -map 0 -map -0:s:1 -c copy output.mkv
```

## 11. 直播与推流

```bash
# 推流 RTMP
ffmpeg -re -i input.mp4 -c:v libx264 -preset veryfast -b:v 2M -c:a aac -f flv rtmp://server/live/streamkey

# 从摄像头推流（Linux V4L2）
ffmpeg -f v4l2 -i /dev/video0 -c:v libx264 -f flv rtmp://server/live/streamkey

# 录直播流
ffmpeg -i "rtmp://server/live/streamkey" -c copy output.mp4
```

## 12. 视频转 GIF

```bash
# 高质量 GIF
ffmpeg -i input.mp4 -vf "fps=10,scale=480:-1:flags=lanczos,split[s0][s1];[s0]palettegen[p];[s1][p]paletteuse" output.gif

# 简单 GIF
ffmpeg -i input.mp4 -ss 00:00:02 -t 3 -vf "fps=10,scale=320:-1" output.gif
```

## 13. 常用参数备忘

| 参数 | 含义 |
|------|------|
| `-i` | 输入文件 |
| `-c:v` / `-vcodec` | 视频编码器 |
| `-c:a` / `-acodec` | 音频编码器 |
| `-c copy` | 不重新编码，直接复制流 |
| `-vn` | 不要视频 |
| `-an` | 不要音频 |
| `-sn` | 不要字幕 |
| `-ss` | 起始时间 |
| `-t` | 持续时间 |
| `-to` | 结束时间 |
| `-r` | 帧率 |
| `-b:v` | 视频码率 |
| `-b:a` | 音频码率 |
| `-vf` | 视频滤镜 |
| `-af` | 音频滤镜 |
| `-filter_complex` | 复合滤镜 |
| `-map` | 选择流 |
| `-preset` | x264/x265 预设 (ultrafast/fast/medium/slow/veryslow) |
| `-crf` | x264/x265 质量 (0-51, 越小质量越高, 默认 23) |
| `-f` | 强制指定格式 |
| `-re` | 实时读取输入（用于推流） |
| `-y` | 覆盖输出文件不询问 |
| `-hide_banner` | 隐藏版本信息 |
