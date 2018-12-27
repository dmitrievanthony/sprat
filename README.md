# Sprat [![Build Status](https://travis-ci.org/dmitrievanthony/sprat.svg?branch=master)](https://travis-ci.org/dmitrievanthony/sprat) #

*Sprat* is a lightweight container and allows to run your code in isolated environment. It's simple:

```bash
ubuntu@ip-172-31-31-112:~/sprat$ sudo ./sprat base_image.img bash
Supervisor started
Container 8ddb51c4-b0f7-4db0-ac83-4b8b39e7dcd0 started (base image base_image.img)
bash-4.3# ps -ef
UID        PID  PPID  C STIME TTY          TIME CMD
0            1     0  0 20:20 ?        00:00:00 ./sprat base_image.img bash
0            2     1  0 20:20 ?        00:00:00 sh -c bash
0            3     2  0 20:20 ?        00:00:00 bash
0            4     3  0 20:20 ?        00:00:00 ps -ef
bash-4.3# df -h
Filesystem      Size  Used Avail Use% Mounted on
/dev/loop0      240M  148M   76M  67% /
none            496M     0  496M   0% /dev
none             64M     0   64M   0% /dev/shm
bash-4.3# ls -la
total 23
drwxr-xr-x  11 0 0  1024 Dec 12 20:20 .
drwxr-xr-x  11 0 0  1024 Dec 12 20:20 ..
drwx------   2 0 0  1024 Dec 12 20:20 .root
drwxr-xr-x   2 0 0  5120 Dec 10 13:44 bin
drwxr-xr-x   5 0 0   180 Dec 12 20:20 dev
drwx------   2 0 0  1024 Dec 12 20:20 etc
drwxr-xr-x  22 0 0  1024 Dec 10 13:44 lib
drwxr-xr-x   2 0 0  1024 Dec 10 13:44 lib64
drwx------   2 0 0 12288 Dec 10 13:44 lost+found
dr-xr-xr-x 139 0 0     0 Dec 12 20:20 proc
dr-xr-xr-x  13 0 0     0 Dec 12 20:20 sys
bash-4.3# exit
exit
Container 8ddb51c4-b0f7-4db0-ac83-4b8b39e7dcd0 finished
Supervisor finished
```
