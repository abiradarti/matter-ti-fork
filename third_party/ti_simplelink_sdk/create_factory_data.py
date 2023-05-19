# we are taking in 9 parameters - vendor id, product id, vendor name,
# product name, hw_ver_str, dac cert, dac private key, dac public key, and pai cert

import sys
import argparse 
import intelhex
from io import StringIO
from cryptography import x509

def create_hex_file(args):
    # create empty factory data file
    factory_data = intelhex.IntelHex()
    factory_data_struct = intelhex.IntelHex()

    factory_data_struct.start_addr = {'EIP': 0x000ac000}
    factory_data.start_addr = {'EIP': 0x000ac048}


    # there are 9 elements, each element will need 8 bytes in the struct  
    # 4 for length of the element, and 4 for the pointer to the element
    # factory data starts at 0xAC000, so the elements will start 72 bytes
    # after the start address
    value_address = 0xAC048

    factory_data_list = list(args.__dict__.items())
    for arg,value in factory_data_list[:-1]:
        if arg == 'vendor_id' or arg == 'product_id':
            final_value = bytearray(hex(value))
        if arg == 'vendor_name' or arg == 'product_name' or arg == 'hw_ver_str':
            final_value = [ord(l) for l in value]
        if arg == 'dac_cert':
            with open(value, "rb") as f:
                dac_cert = x509.load_der_x509_certificate(f.read())
                print(dac_cert)
        if arg == 'pai_cert':
            with open(value, "rb") as f:
                pai_cert = x509.load_der_x509_certificate(f.read())
                print(pai_cert)

    # factory_data_list = list(args.__dict__.items())
    # print(factory_data_list)
    # # get an element
    # for arg,value in factory_data_list[:-1]:
    #     # get its size in bytes

    #     #value_bytearray = bytearray(value, 'utf-8')
    #     value_arr = ctypes.Array[ctypes.c_uint8] = ctypes.create_string_buffer(value)
    #     value_len = len(value_arr)
    #     print("arg: " + arg + " value: ")
    #     print(value_arr)
    #     print("arg_len: " + str(value_len))
    #     print("value_address: " + str(value_address))

    #     value_len_buf = StringIO()
    #     value_len_buf.write(str(bytearray(value_len)))
        
    #     value_address_buf = StringIO()
    #     value_address_buf.write(str(bytearray((value_address))))

    #     value_buf = StringIO()
    #     value_buf.write(str(value_arr))

    #     # add to the struct and factory data hex files 
    #     factory_data_struct.tofile(value_len_buf, format='hex')
    #     factory_data_struct.tofile(value_address_buf, format='hex')
    #     value_len_buf.close()
    #     value_address_buf.close()

    #     factory_data.tofile(value_buf, format='hex')
    #     value_buf.close()
    #     value_address += 8
    
    #factory_data_struct.merge(factory_data, overlap='error')
    

def main():
    parser = argparse.ArgumentParser(description="TI Factory Data hex file creator")

    parser.add_argument('-vid', '--vendor_id', required=True, type=int)
    parser.add_argument('-pid', '--product_id', required=True, type=int)
    parser.add_argument('-vendor_name', required=True, type=str)
    parser.add_argument('-product_name', required=True, type=str) 
    parser.add_argument('-hw_ver_str', required=True, type=str)
    parser.add_argument('-dac_priv_key', required=True)
    parser.add_argument('-dac_pub_key', required=True)
    parser.add_argument('-dac_cert', required=True)
    parser.add_argument('-pai_cert', required=True)
    parser.add_argument('-o', required=True)

    args = parser.parse_args()
    #print(args)
    create_hex_file(args)

if __name__ == "__main__":
    main()