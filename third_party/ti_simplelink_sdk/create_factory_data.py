import argparse 
import intelhex
import json
from jsonschema import validate
import subprocess

def create_hex_file(args):
    # create empty factory data file
    factory_data_intelhex = intelhex.IntelHex()
    factory_data_struct_intelhex = intelhex.IntelHex()

    # there are 16 elements, each element will need 8 bytes in the struct  
    # 4 for length of the element, and 4 for the pointer to the element
    # factory data starts at 0xAC000, so the elements will start 128 bytes
    # after the start address
    factory_data_dict = json.load(args.factory_data_json_file[0])
    factory_data_schema = json.load(args.factory_data_schema[0])
   
    validate(factory_data_dict, factory_data_schema)
    factory_data = factory_data_dict['elements']

    struct_idx = 0
    values_idx = 0
    value_address = 0xAC080

    for element in factory_data:
        #get the length in hex and write to first hex file
        len_integer = element['len']
        
        factory_data_struct_intelhex[struct_idx + 3]     = (len_integer & 0xFF000000) >> 24
        factory_data_struct_intelhex[struct_idx + 2]     = (len_integer & 0x00FF0000) >> 16
        factory_data_struct_intelhex[struct_idx + 1]     = (len_integer & 0x0000FF00) >> 8
        factory_data_struct_intelhex[struct_idx]         = (len_integer & 0x000000FF) 
        
        struct_idx += 4

        #write the address to the file and increment by the size of the element
        factory_data_struct_intelhex[struct_idx + 3]     = (value_address & 0xFF000000) >> 24
        factory_data_struct_intelhex[struct_idx + 2]     = (value_address & 0x00FF0000) >> 16
        factory_data_struct_intelhex[struct_idx + 1]     = (value_address & 0x0000FF00) >> 8
        factory_data_struct_intelhex[struct_idx]         = (value_address & 0x000000FF) 
        
        struct_idx += 4
        value_address += len_integer

        #convert the value to hex and write to the second file
        key = list(element.keys())[0]
        if type(element[key]) == str:
            list_value = list(element[key].strip(" "))
            hex_check = ''.join(list_value[0:4])
            if hex_check == "hex:":
                list_value = list_value[4:]
                idx = 0
                list_len = len(list_value)
                while idx < list_len:
                    hex_list = []
                    hex_str_1 = list_value[idx]
                    
                    hex_list.append(hex_str_1)
                    hex_str_2 = list_value[idx+1]
                    
                    hex_list.append(hex_str_2)
                    final_hex_str = ''.join(hex_list)
                    
                    factory_data_intelhex[values_idx] = int(final_hex_str, 16)
                    values_idx += 1
                    idx += 2
            else:
                for ele in list_value:
                    factory_data_intelhex[values_idx] = ord(ele)
                    values_idx += 1 
        else:    
            if key != "spake2_it":
                factory_data_intelhex[values_idx]     = (element[key] & 0xFF00) >> 8
                factory_data_intelhex[values_idx + 1] = (element[key] & 0x00FF)
                
                values_idx += 2
            else:
                factory_data_intelhex[values_idx]     = (element[key] & 0xFF0000) >> 16
                factory_data_intelhex[values_idx + 1] = (element[key] & 0x00FF00) >> 8
                factory_data_intelhex[values_idx + 2] = (element[key] & 0x0000FF)
                
                values_idx += 3

    #merge both hex files
    idx = 0
    while idx < values_idx:
        factory_data_struct_intelhex[struct_idx] = factory_data_intelhex[idx]
        idx = idx + 1
        struct_idx = struct_idx + 1

    #output to hex file
    factory_data_struct_intelhex.tofile(args.factory_data_hex_file, format = 'hex')

    #get hex file in a format that can be merged with the app and BIM in a later step
    subprocess.call(['objcopy', args.factory_data_hex_file, '--input-target', 'ihex', '--output-target', 'binary', 'temp.bin'])
    subprocess.call(['objcopy', 'temp.bin','--input-target','binary','--output-target', 'ihex', args.factory_data_hex_file, '--change-addresses=0xac000'])
    subprocess.call(['rm', 'temp.bin'])

def main():
    parser = argparse.ArgumentParser(description="TI Factory Data hex file creator")

    parser.add_argument('-factory_data', '--factory_data_json_file', required=True,  nargs=1, help="JSON file of factory data", type=argparse.FileType('r'))
    parser.add_argument('-schema', '--factory_data_schema', required=True, nargs=1, help="Factory Data Schema", type=argparse.FileType('r'))
    parser.add_argument('-o', '--factory_data_hex_file', required=True)
    
    args = parser.parse_args()
    create_hex_file(args)

if __name__ == "__main__":
    main()