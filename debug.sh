#  debug.sh
#  
#  Copyright 2013 JS-OS <js@duck-squirell>
#  
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#  
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#  
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
#  MA 02110-1301, USA.
#  
#  

#!/bin/bash

#~ harddisk_image="harddisk.img"
floppy_image="floppy.img"
storage_image="storage.img"
#~ storage_image="test.img"

printf "\n--------------------------GDB------[in new terminal window]----"
printf "\ntarget remote localhost:1234"
printf "\nsymbol-file kernel.sym"
printf "\nbreak <filename.c> :<function or line to break at>"
printf "\nUse 'continue' instead of 'run' in gdb\n"

#checks if xclip is installed
command -v xclip >/dev/null 2>&1 || { printf '\e[0;31m\n' ; echo >&2 "I require xclip, but it's not installed.  It is best to install it."; printf '\e[0m\n';

objcopy --only-keep-debug kernel kernel.sym;
qemu-system-x86_64 -m 64M -vga std -soundhw pcspk -s -S -fda "$floppy_image" -fdb "$storage_image" ; exit 1;}

printf "\nBy default, the initial gdb commands are copied to the clipboard\n\n"

echo "target remote localhost:1234
symbol-file kernel.sym" |  xclip -selection clipboard

objcopy --only-keep-debug kernel kernel.sym

qemu-system-x86_64 -m 64M -vga std -soundhw pcspk -s -S -fda "$floppy_image" -fdb "$storage_image"


