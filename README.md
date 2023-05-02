A linux kernel module that creates a readable and writable file /proc/test_task.
You can test it with cat and echo, for example "echo '1' > /proc/test_task" "cat /proc/test_task".
For testing with large data, type "cat large_file.txt > /proc/test_task", then "diff large_file.txt /proc/test_task", this command will display mistakes in copying if any exists. 

How to use:
	1. Enter "make" in root directory of this project to compile.
	2. Type "sudo insmod proc_temp_mem.ko" to load kernel module.
	3. Type "sudo rmmod proc_temp_mem" to remove module.
Maximum single entry size is 512 chars. Maximum file size is unlimited.	
