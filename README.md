# SIYI A8 mini camera manager

Camera Manager for SIYI A8 mini for RPi based on MAVSDK.

## Prerequisites

This tutorial is using a Raspberry Pi 4 with [Raspberry Pi OS](https://www.raspberrypi.com/software/) 64 bits (based on Debian Bullseye)
accessible over WiFi using ssh.

If you haven't already, I suggest flashing the RPi's SD card using the RPi flasher. It allows you to set up login passwords, WiFi passwords and ssh support right there before flashing. Connect it to your WiFi, find the IP using your router's web UI, or [look for open ssh ports to find the IP](https://serverfault.com/a/376895). If you have a hard time finding it, consider doing an nmap call before connecting it, and after and then diffing the output.

## Connect to SIYI camera

## Set up static IP on Ethernet

Connect the SIYI camera using the Ethernet adapter cable and an Ethernet cable into the RPi.

We need to set up a static IP for Ethernet in order to talk to SIYI:

Edit the file `/etc/dhcpcd.conf`:

```
/etc/dhcpcd.conf
```

To enable it, just unplug Ethernet and plug it back in.

You can check the IP used:

```
ip addr
```

```
...
2: eth0: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc mq state UP group default qlen 1000
    link/ether xx:xx:xx:xx:xx:xx brd ff:ff:ff:ff:ff:ff
    inet 192.168.144.20/24 brd 192.168.144.255 scope global noprefixroute eth0
       valid_lft forever preferred_lft forever
    inet6 fe80::2c28:c725:4499:e123/64 scope link
       valid_lft forever preferred_lft forever
...
```

And you should be able to ping the camera:
```
ping 192.168.144.25
```
And see something like
```
PING 192.168.144.25 (192.168.144.25) 56(84) bytes of data.
64 bytes from 192.168.144.25: icmp_seq=1 ttl=64 time=0.368 ms
64 bytes from 192.168.144.25: icmp_seq=2 ttl=64 time=0.710 ms
64 bytes from 192.168.144.25: icmp_seq=3 ttl=64 time=0.175 ms
```

## RTSP re-broadcasting

In order to access the video stream from WiFi, we need to subscribe to the camera's RTSP server and open another RTSP server on the RPi.

This can be done using the [rtsp-rebroadcast](rtsp-rebroadcast/rtsp_rebroadcast.cpp) application which is using gstreamer.

### Build

In order to run the program, you can build it on device for prototyping (or use cross-compilation, or whatever deployment tool you use).

Install the gstreamer dependencies:

```
sudo apt install libgstreamer1.0-dev libgstrtspserver-1.0-dev build-essential cmake
```

Get the source code, which is part of this repository, either using `scp`, or via a `git clone`.

Then build it on the RPi:

```
cd rtsp-rebroadcast
cmake -Bbuild -S.
cmake --build build
```

### Run

Then run it:

```
build/rtsp_rebroadcast
```
(by default there is no output)

To test the RTSP server, try to connect to it from another computer connected to the same network.

Either using gstreamer:

```
sudo apt install gstreamer1.0-plugins-base-apps

gst-play rtsp://192.168.x.y:8554/live
```

Or ffmpeg:

```
sudo apt install ffmpeg

ffplay rtsp://192.168.x.y:8554/live
```

Or open the URL in VLC.

Note that all these tools add quite a bit of buffering by default, so a delay of 1-3 seconds is quite normal.


## MAVLink camera server

In order to have QGroundControl auto-connect to the video feed and show camera settings, we can use a MAVLink camera server.

The camera manager is implemented using A small application on top of MAVSDK the RTSP URL, as well as camera settings.

### Build

Clone or copy this repo on to the the RPi:

Then build:
```
cd camera-manager
cmake -Bbuild -S.
cmake --build build
```

### Run

And run it:
```
build/camera_manager
```

TODO: add instructions and changes how to connect to a Pixhawk.
