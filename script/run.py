#!/usr/bin/python

import sys, os, subprocess, re, glob, argparse
import numpy as np
import matplotlib.pyplot as plt
import threading
import multiprocessing
import random
import struct
import time
import xlwings as xw

from colorama import init,Fore

init(autoreset=True)

PROGRAM_NAME = "main"
INPUT_DIR_PATH = "D:\\0_Desktop\\raw"

if os.name == 'nt':
    PROGRAM_NAME = "main.exe"

bold="\033[1m"
green="\033[0;32m"
red="\033[0;31m"
normal="\033[0m"

_type = {"our_A" : 0, "our_V" : 1, "song_A" : 2, "song_V": 3}
stat_data = [[] for i in range(4)]
stat_data_final = [[] for i in range(4)]

def get_idx(s):
    pre = int(s[:-1:])
    now = ord(s[-1])
    if now >= 88:
        return pre * 10 + now - 88
    elif  now >= 78:
        return pre * 10 + now - 78
    elif  now >= 68:
        return pre * 10 + now - 68
    elif  now >= 58:
        return pre * 10 + now - 58
    else:
        return pre * 10 + now - 48

def get_status(s):
    now = ord(s[-1])
    if now >= 88:
        return 4
    elif  now >= 78:
        return 3
    elif  now >= 68:
        return 2
    elif  now >= 58:
        return 1
    else:
        return 0

def check_dir(_path):
    if not os.path.isdir(_path):
        os.makedirs(_path)

def get_slice_files(_path, _slices, _size):
    check_dir(_path)
    for i in range(len(_slices)):
        _slice = _slices[i]
        if len(_slice) != _size:
            break
        now_path = f"{_path}/{i}_from_{i * _size}_to_{(i + 1) * _size}.txt"
        print(f"gen file -> {now_path}")
        with open(now_path, "wb") as fi, open(f"{now_path}.raw", 'w') as fi_raw:
            for unit in _slice:
                unit[1] = 511 if unit[1] > 511 else unit[1]
                fi.write(struct.pack("<ii", int(unit[0]), int(unit[1])))
                fi_raw.write(f"{int(unit[0])} {int(unit[1])}\n")

def get_data():
    our_A_set = []
    our_V_set = []
    song_A_set = []
    song_V_set = []
    # print(os.getcwd())
    __path = os.path.normpath(os.path.join(os.getcwd(), INPUT_DIR_PATH))
    # print(__path)
    _path = os.listdir(__path)
    start_time = time.time()
    for path in _path:
        if not path.endswith(".txt"): continue
        with open(f"{INPUT_DIR_PATH}/{path}", 'r') as fi:
                print(f"acquire data from -> {INPUT_DIR_PATH}/{path}")
                lines = fi.readlines()
                for i in range(10):
                    if lines[i][:11] == 'name = iptA':
                        lines = lines[i::]
                        break
                lines = lines[:(len(lines) - len(lines) % 3):]
                iptA = iptV = dac = stat = idx = []
                x = [
                    (
                        int(v1.split()[-1]),
                        int(v2.split()[-1]),
                        int(v3.split()[-1]),
                        get_idx(v3.split()[2]),
                        get_status(v3.split()[2])
                    ) 
                        for v1, v2, v3 in zip(lines[::3], lines[1::3], lines[2::3])
                ]
                iptA, iptV, dac, idx, stat = np.matrix(x).T.tolist()
                if path.startswith("Our"):
                    our_A_set.extend([[stat[i], iptA[i]] for i in range(len(stat))])
                    our_V_set.extend([[stat[i], iptV[i]] for i in range(len(stat))])
                elif path.startswith("Song"):
                    song_A_set.extend([[stat[i], iptA[i]] for i in range(len(stat))])
                    song_V_set.extend([[stat[i], iptV[i]] for i in range(len(stat))])
    print(f"Total reading time is {(time.time() - start_time):.3f}s.")
    start_time = time.time()
    pro_dir = time.strftime("%Y-%m-%d_%H_%M_%S", time.localtime())
    gen_data_dir_path = os.path.normpath(os.path.join(os.getcwd(), f"input/inputs/{pro_dir}"))
    check_dir(gen_data_dir_path)
    slice_size = [25, 30, 35, 40, 45, 50]
    for _size in slice_size:
        real_size = _size * 10000
        our_A_slices  = [our_A_set[i:i+real_size:]  for i in range(len(our_A_set)//real_size)]
        our_V_slices  = [our_V_set[i:i+real_size:]  for i in range(len(our_V_set)//real_size)]
        song_A_slices = [song_A_set[i:i+real_size:] for i in range(len(song_A_set)//real_size)]
        song_V_slices = [song_V_set[i:i+real_size:] for i in range(len(song_V_set)//real_size)]

        our_A_dir_path  = f"{gen_data_dir_path}/iptA/our/{_size}"
        our_V_dir_path  = f"{gen_data_dir_path}/iptV/our/{_size}"
        song_A_dir_path = f"{gen_data_dir_path}/iptA/song/{_size}"
        song_V_dir_path = f"{gen_data_dir_path}/iptV/song/{_size}"

        get_slice_files(our_A_dir_path,  our_A_slices,  real_size)
        get_slice_files(our_V_dir_path,  our_V_slices,  real_size)
        get_slice_files(song_A_dir_path, song_A_slices, real_size)
        get_slice_files(song_V_dir_path, song_V_slices, real_size)
    print(f"Total slicing time is {(time.time() - start_time):.3f}s.")

def run(_bit, _file):
    global PROGRAM_NAME
    # f"D:\\0_Desktop\\Pro\\FD_\\main.exe"
    proc = subprocess.Popen([PROGRAM_NAME, f"{_bit}", _file], 
                            stdin=subprocess.PIPE, 
                            stdout=subprocess.PIPE, 
                            stderr=subprocess.PIPE)
    (r, r_err) = proc.communicate()
    # print(r)

    return r, r_err

def filter_out(run_out):
    regex = re.compile("^(H1)|(H2).*$")
    out = []
    for l in run_out:
        if regex.match(l.decode()):
          out.append(l.decode())
    # print(out)
    return out

# D:\0_Desktop\Pro\FD_\input\inputs/2020-11-15_13_39_56\iptA\our\25\0_from_0_to_250000.txt
def get_info_from_file(_file):
    _size = float(_file.split('\\')[-2])
    if _file.find('iptA') != -1 and _file.find('our') != -1:
        return _size, 0
    elif _file.find('iptV') != -1 and _file.find('our') != -1:
        return _size, 1
    elif _file.find('iptA') != -1 and _file.find('song') != -1:
        return _size, 2
    else:  
        return _size, 3

def main():
    global stat_data
    start_time = time.time()
    abs_path = os.path.normpath(os.path.join(os.getcwd(), "input/inputs/"))
    _dir = os.listdir(abs_path)
    _dir.sort(reverse=True)
    pro_dir = _dir[0]

    all_inputs = glob.glob(f"{abs_path}/{pro_dir}/*/*/*/*.txt")
    all_bit    = [5,6,7,8,9]
    global PROGRAM_NAME
    PROGRAM_NAME = os.path.normpath(os.path.join(os.getcwd(), f"{PROGRAM_NAME}"))

    # # print(f"default inputs \n all_inputs")
    # print(f"default all_bit\n all_bit")

    parser = argparse.ArgumentParser(description="Process some files")
    parser.add_argument("--inputs", nargs="*", default=all_inputs, help="inputs file path")
    parser.add_argument("--bit",    nargs="*", default=all_bit,    help="quantize bits")
    parser = parser.parse_args()

    # print(parser.bit)
    # print(parser.inputs)

    error_sum = 0
    result_path = os.path.normpath(os.path.join(os.getcwd(), "result"))
    check_dir(result_path)
    now_time_ = time.strftime("%Y_%m_%d_%H_%M_%S", time.localtime())
    with open(f"{result_path}/{now_time_}.txt", 'w') as fi, open(f"{result_path}/{now_time_}_short.txt", 'w') as fi_short:
        for _bit in parser.bit:
            for _file in parser.inputs:
                if not os.path.exists(_file):
                    print(red + f"ERROR -- input file {_file} not found: " + normal)
                    continue
                fi.write('-'*100+'\n')
                fi.write(f"bit = {_bit}\nfile name -> {_file}\n\n")
                fi_short.write('-'*100+'\n')
                fi_short.write(f"bit = {_bit}\nfile name -> {_file}\n\n")
                print(bold + "Testing: " + normal + f"{_bit}  {_file}" + normal)
                run_out, run_out_err = run(_bit, _file)

                assert len(run_out_err) == 0, f"your main has error, as follows:\n{run_out_err}"

                run_out = run_out.split("\n".encode())
                short_run_out = filter_out(run_out)
                _size, _type = get_info_from_file(_file)
                _val = [_bit, _size] + [float(val.split()[-1]) for val in short_run_out]
                stat_data[_type].append(_val)
                for _ in run_out:
                    fi.write(_.decode())
                for _ in short_run_out:
                    fi_short.write(_)
                fi.write('\n' + '-' * 100 + '\n\n')
                fi_short.write('\n' + '-' * 100 + '\n\n')
                # raise
            # break
    print(f"Total gen file time is {(time.time() - start_time):.3f}s.")
    
    for i in range(4):
        stat_data[i].append([-1]*8)
        new_val, cnt, _len, pre_val = np.array(stat_data[i][0]), 0, len(stat_data[i]), stat_data[i][0][1]
        for j in range(1,_len):
            if stat_data[i][j][1] != pre_val:
                stat_data_final[i].append((new_val/(np.array([cnt + 1]*8))).tolist())
                new_val = np.array(stat_data[i][j])
                cnt, pre_val = 0, new_val[1]
            else:
                cnt += 1
                new_val += np.array(stat_data[i][j])

def table_vir():
    global stat_data, stat_data_final

    start_time = time.time()
    now_time_ = time.strftime("%Y_%m_%d_%H_%M_%S", time.localtime())
    app = xw.App()
    wb = app.books.add()
    for sub_type in _type.keys():
        sht = wb.sheets.add(sub_type)
        sht.range('A2:H2').value = ['Bit', 'Size', 'MS', 'DS', 'ALL', 'MS', 'DS', 'ALL', '']
        sht.range('A3:H3').value = stat_data[_type[sub_type]]
        sht.autofit()
    result_path = os.path.normpath(os.path.join(os.getcwd(), "result"))
    wb.save(f"{result_path}/{now_time_}__result.xlsx")

    app = xw.App()
    wb = app.books.add()
    for sub_type in _type.keys():
        sht = wb.sheets.add(sub_type)
        sht.range('A2:H2').value = ['Bit', 'Size', 'MS', 'DS', 'ALL', 'MS', 'DS', 'ALL', '']
        sht.range('A3:H3').value = stat_data_final[_type[sub_type]]
        sht.autofit()
    wb.save(f"{result_path}/{now_time_}__result_avg_final.xlsx")
    print(f"Total make table time is {(time.time() - start_time):.3f}s.")

if __name__ == "__main__":
    get_data()
    main()
    table_vir()
