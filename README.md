# Sprat

*Sprat* is a lightweight container and allows to run your code in isolated environment. It's simple:

```
ubuntu@ip-172-31-31-112:~/sprat$ sudo ./sprat base_image.img "ls -la"
Supervisor started
Container 56cc93dc-ba1c-4b14-98c7-45086543304d started (base image base_image.img)
total 24
drwxr-xr-x  9 0 0  1024 Dec 10 13:44 .
drwxr-xr-x  9 0 0  1024 Dec 10 13:44 ..
drwxr-xr-x  2 0 0  5120 Dec 10 13:44 bin
drwxr-xr-x  2 0 0  1024 Dec 10 13:44 dev
drwxr-xr-x 22 0 0  1024 Dec 10 13:44 lib
drwxr-xr-x  2 0 0  1024 Dec 10 13:44 lib64
drwx------  2 0 0 12288 Dec 10 13:44 lost+found
drwxr-xr-x  2 0 0  1024 Dec 10 13:44 proc
drwxr-xr-x  2 0 0  1024 Dec 10 13:44 sys
Container 56cc93dc-ba1c-4b14-98c7-45086543304d finished
Supervisor finished
```
