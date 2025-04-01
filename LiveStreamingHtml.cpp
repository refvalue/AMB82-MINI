#include "Resources.hpp"

constexpr char liveStreamingHtml[] = R"AAAAA(<!DOCTYPE html>
<html lang="en">

<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Live Streaming</title>
    <link rel="stylesheet" href="styles.css">
    <style>
        #video-box {
            width: 640px;
            height: 480px;
            margin: auto;
        }
    </style>
</head>

<body>
    <div class="container">
        <button onclick="window.location.href='/system-info.html'">System Info</button>
        <h1>Live Streaming</h1>
        <video id="video-box" autoplay></video>
    </div>

    <script type="text/javascript" src="jmuxer.js"></script>

    <script>
        function createVideoPlayer() {
            const jmuxer = new JMuxer({
                node: 'video-box',
                mode: 'video',
                fps: 25,
                flushingTime: 0,
                onError: data => console.log('Buffer error encountered', data),
                onMissingVideoFrames: data => console.log('Video frames missing', data),
                onMissingAudioFrames: data => console.log('Audio frames missing', data),
            });

            const socket = new WebSocket(`ws://${location.hostname}:8080/live`);

            socket.binaryType = "arraybuffer";
            socket.addEventListener('open', () => console.log('Connected to the video server.'));
            socket.addEventListener('error', e => console.log(e));
            socket.addEventListener('message', async e => {
                jmuxer.feed({
                    video: new Uint8Array(e.data),
                    duration: 40,
                });
            });
        }

        window.addEventListener('load', createVideoPlayer);
    </script>
</body>

</html>)AAAAA";
