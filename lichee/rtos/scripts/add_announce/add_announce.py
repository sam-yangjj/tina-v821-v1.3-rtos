# -*- coding: utf-8 -*-

import os
import locale
import subprocess
import argparse
import re

# 要忽略的目录,以 --input 指定的目录为根目录
# rtos
ignore_dir = [
    './scripts',
    './kernel/FreeRTOS/Source',
    './kernel/FreeRTOS-orig/Source',
    './kernel/FreeRTOS-orig/Source/include',
    './include/FreeRTOS_POSIX/',
    './kernel/Posix/include',
    './kernel/Posix/source',
    './tools',
    './components/aw/bluetooth/zephyr',
    './components/thirdparty',
    './projects',
]

# thirdparty libraries
thirdparty = []

parser = argparse.ArgumentParser()
parser.add_argument("note", help="need add anotation")
parser.add_argument("--input", help="input directory")
parser.add_argument("--show", "-s", action='store_true', help="show update dir", default=False)
parser.add_argument("--debug", "-d", action='store_true', help="show update file,but don't update", default=False)
args = parser.parse_args()
debug = args.debug
path = ''
if args.input is not None:
    path = args.input
else:
    print('need --input arg')
    exit(1)

notes_file = os.path.abspath(args.note)

def find_files(path = '.'):
    cmd = 'find -L {} -name \'*.[ch]\''.format(path)
    s = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE)
    out = s.communicate()[0].decode()
    l = out.split()
    return l

def show_dir(path = '.'):
    cmd = 'find -L {} -name *.[ch]'.format(path)
    s = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE)
    out = s.communicate()[0].decode()
    l = out.split()
    ll = []
    print('搜索到的目录:')
    for item in l:
        dirname = os.path.dirname(item)
        if dirname not in ll:
            ll.append(dirname)
    for item in ll:
        print(item)

def read_new_notes(path):
    with open(path, 'r') as f:
        context = f.read()

    new_notes = context.strip()
    new_notes += '\n'
    # print('要插入的注释:\n')
    # print(new_notes)
    # print('\n')
    return new_notes


def read_file_contents(path):
    encodes = ['utf-8', 'gbk', 'gb18030', 'latin-1', 'ansi']
    for item in encodes:
        try:
            f = open(path, 'r', encoding=item)
            context = f.read()
            f.close()
            return context
        except:
            continue
    return None

def match_anotation(paths):
    global notes_file, debug, ignore_dir, thirdparty
    new_notes = read_new_notes(notes_file)
    pattern = re.compile('//.*$|/\*[\s\S\r\n]*?\*/')

    # 进行预处理,剔除不需要更新的目录
    match_dir = []
    nomatch_dir = []
    for item in paths:
        m = False
        item = os.path.relpath(item)
        for ignore in ignore_dir:
            if item.startswith(ignore):
                nomatch_dir.append(item)
                m = True
                break
        if m == False:
            match_dir.append(item)

    if debug == True:
        print('不需要更新的文件:')
        for item in nomatch_dir:
            print(item)
        print('\n更新文件的文件')
        for item in match_dir:
            print(item)
        return

    i = 0
    total = len(match_dir)
    print('\n更新文件...')
    for filename in match_dir:
        i += 1
        name = os.path.basename(filename)
        dirname = os.path.dirname(filename)
        print('正在处理 {: <50} ({: <10} | {: <10}个)'.format(name, i, total), end='\r')

        context = read_file_contents(filename)
        if context == None:
            print('Undown file encodes type:%s\n' % filename)
            exit(1)

        # 检查是否 存在copyright
        if 'copyright (c)' in context.lower():
            thirdparty.append(filename)
            continue
        f = open(filename, 'w')
        match = pattern.search(context)
        if match:
            notes = match.group(0)
            contexts = context.split(notes)
            before = contexts[0].strip()
            after = ''
            for i in range(1, len(contexts)):
                after += contexts[i].strip() + '\n'
                if (i + 1) != len(contexts):
                    after += notes + '\n'

            if (len(before) > 0):
                # 前面没有注释 直接添加新注释即可
                f.write(new_notes)
                f.write(context)
            else:
                # 匹配到的是第一个注释,需要进行删除
                f.write(new_notes)
                f.write(after)
        else:
            # 前面没有注释 直接添加新注释即可
            f.write(new_notes)
            f.write(context)

        f.close()
    print('\n更新完成\n')
    print('跳过文件:')
    for item in thirdparty:
        print(item)

pwd = os.getcwd()

print('chdir %s -> %s' % (pwd, path))
os.chdir(path)

print('ignore dir:')
for i in range(len(ignore_dir)):
    ignore = os.path.relpath(ignore_dir[i])
    ignore_dir[i] = ignore
    print('\t%s' % ignore_dir[i])

if args.show:
    show_dir()
files = find_files()
match_anotation(files)
os.chdir(pwd)
