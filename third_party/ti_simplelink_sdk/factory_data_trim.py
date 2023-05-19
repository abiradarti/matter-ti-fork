#!/usr/bin/env python

# Copyright (c) 2022 Project CHIP Authors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Script to extract the factory data content from the matter application + factory data image 

This script will trim the factory data content out of the matter application image. It will also get the length of the factory data from the matter application map file. The output is the factory data content.

Run with:
    python factory_data_trim.py <matter image> <matter image map file> <factory data image> 
"""

import sys
import intelhex
import itertools
import re

matter_app_file = sys.argv[1]
matter_app_map_file = sys.argv[2]
factory_data_hex = sys.argv[3]

# extract factory data length from map file 
with open(matter_app_map_file, "r") as map_file:
    pattern = ".factory_data   0x00000000000ac000"
    for line in map_file:
        if re.search(pattern, line):
            factory_data_num_bytes = line
            break

# this is the length of the factory data in hexadecmial form
factory_data_num_bytes = factory_data_num_bytes[40:]

# convert hex image to dictionary 
matter_image = intelhex.IntelHex()
matter_image.fromfile(matter_app_file, format='hex')
matter_image_dict = matter_image.todict()

# 704512 is 0xAC000 - start of factory data
start_index = list(matter_image_dict.keys()).index(704512)
# convert length of factory data into a decimal value 
end_index = start_index + int(factory_data_num_bytes, 16)

# slice dictionary to only have factory data elements 
matter_image_dict = dict(itertools.islice(matter_image_dict.items(), start_index, end_index))

# convert sliced dictionary to back to hex 
factory_data = intelhex.IntelHex()
factory_data.fromdict(matter_image_dict)

factory_data.tofile(factory_data_hex, format='hex')