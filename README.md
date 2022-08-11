# Startup-Monitor

Linux kernel module to monitor created processes

<h2>Install:</h2>

`make` for build packages

`sudo make load` for insert module into system.

`sudo make unload` for remove module from system.

<h2>Usage:</h2>

Reading from device in `/dev/startupmon` will return information about the created process.

`cat /dev/startupmon`
