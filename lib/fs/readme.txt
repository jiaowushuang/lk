memfs usage:

1. bio memcreate memdev 0x100000
2. fs mount /memdev memfs memdev
/memdev 可以换成别的路径，但不要是/
3. mkfile a.txt
4. fs write /memdev/a.txt <string> <offset>
5. cd /memdev
6. ls pwd rm stat cat df
7. fs unmount /memdev
