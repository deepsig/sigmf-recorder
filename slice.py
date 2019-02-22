import sys
import numpy as np

def sample_convert(in_path_name, num_samps):
    samples = np.memmap(in_path_name, mode = "r", dtype = np.int16)
    cut_samples = samples[0:(num_samps*2)]
    my_bytes = cut_samples.tobytes()
    in_file_name = in_path_name[(in_path_name.rfind('/')+1):]
    out_file_name = "{}_{}".format(num_samps, in_file_name)
    out_path_name = "{}{}".format(in_path_name[:(in_path_name.rfind('/')+1)], out_file_name)
    file_object = open(out_path_name, "wb")
    file_object.write(my_bytes)
    file_object.close()

if __name__ == "__main__":
    sample_convert(sys.argv[1], int(sys.argv[2])