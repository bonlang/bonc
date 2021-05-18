#!/usr/bin/python3

import sys, shlex
from subprocess import call

diags = {}

try:
    in_file = open(sys.argv[1], 'r')  

    line = in_file.readline()
    while line:
        sections = shlex.split(line)
        diags[sections[0].strip()] = (sections[1], list(map(str.strip, sections[2:])))
        line = in_file.readline()

finally:
    in_file.close()

enum_dec = 'typedef enum {\n'
union_dec = 'union DiagData {\n'
prototype_dec = ''

for idx, (k, v) in enumerate(diags.items()): 
    arg_list = ['SourcePosition range']
    ender = ',\n'
    if idx == len(diags) - 1:
        ender = '\n'
    enum_dec += 'DIAG_' + k.upper() + ender
    type_cnt = 1
    pos_cnt = 1
    temp_struct = 'struct {\n'
    has_members = False
    for format_spec in v[1]:
        if format_spec.strip() == '%t':
            arg_list += ['Type *type' + str(type_cnt)]
            temp_struct += 'Type *type' + str(type_cnt) + ';\n'
            has_members = True
            type_cnt += 1
        if format_spec.strip() == '%p':
            arg_list += ['SourcePosition pos' + str(pos_cnt)]
            temp_struct += 'SourcePosition pos' + str(pos_cnt) + ';\n'
            has_members = True
            pos_cnt += 1

    prototype_dec += 'void log_' + k.strip() + '(' + ','.join(arg_list) + ');\n'
    temp_struct += '} ' + k.strip() + ';\n'
    if has_members:
        union_dec += temp_struct


enum_dec += '} DiagKind;\n'
union_dec += '};\n' 

header_f = open(sys.argv[2], 'w')
header_f.write('#include "helper.h"\n#include "type.h"\n')
header_f.write(enum_dec)
header_f.write(union_dec)
header_f.write('typedef struct {DiagKind t; union DiagData data;} Diag; ')
header_f.write(prototype_dec)
