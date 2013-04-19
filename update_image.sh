#  update_image.sh
#  
#  Copyright 2013 JS <js@duck-squirell>
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

####**********Takes the kernel image and the initrd.img and builds the
####**********floppy.img

#!/bin/bash

sudo /sbin/losetup /dev/loop1 floppy.img
sudo mount /dev/loop1 /mnt
sudo cp kernel /mnt/kernel
sudo cp ./initrd/initrd.img /mnt/initrd
sudo umount /dev/loop1
sudo /sbin/losetup -d /dev/loop1
