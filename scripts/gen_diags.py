#!/usr/bin/python3

import sys, shlex
from subprocess import call

diags = {}

try:
    in_file = open(sys.argv[2], 'r')  

    line = in_file.readline()
    while line:
        sections = shlex.split(line)
        diags[sections[0].strip()] = (sections[1], list(map(str.strip, sections[2:])))
        line = in_file.readline()

finally:
    in_file.close()

if sys.argv[1] == 'h':
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
        char_cnt = 1
        temp_struct = 'struct {\n'
        has_members = False
        for format_spec in v[1]: 
            if format_spec.strip() == '%t':
                arg_list += ['Type *type' + str(type_cnt)]
                temp_struct += 'Type *type' + str(type_cnt) + ';\n'
                has_members = True
                type_cnt += 1
            elif format_spec.strip() == '%p':
                arg_list += ['SourcePosition pos' + str(pos_cnt)]
                temp_struct += 'SourcePosition pos' + str(pos_cnt) + ';\n'
                has_members = True
                pos_cnt += 1
            elif format_spec.strip() == '%c':
                arg_list += ['uint8_t c' + str(char_cnt)]
                temp_struct += 'uint8_t c' + str(char_cnt) + ';\n'
                has_members = True
                char_cnt += 1

        prototype_dec += 'void log_' + k.strip() + '(' + ','.join(arg_list) + ');\n'
        temp_struct += '} ' + k.strip() + ';\n'
        if has_members:
            union_dec += temp_struct


    enum_dec += '} DiagKind;\n'
    union_dec += '};\n' 

    header_f = open(sys.argv[3], 'w')
    header_f.write('#ifndef DIAGS_H\n#define DIAGS_H\n#include <stdio.h>\n#include "helper.h"\n#include "type.h"\n')
    header_f.write(enum_dec)
    header_f.write(union_dec)
    header_f.write('typedef struct {DiagKind t; union DiagData data; SourcePosition range;} Diag; ')
    header_f.write(prototype_dec)
    header_f.write("void diag_output(Diag *  diag, FILE* file);\n#endif\n")

elif sys.argv[1] == 'c':
    source_f = open(sys.argv[3], 'w')
    source_f.write('#include <diags.h>\n' \
                    'void diag_output(Diag * diag, FILE* file) {\n'\
                    '(void) diag; fprintf(file, "error\\n");\n'\
                    '}\n')
    for idx, (k, v) in enumerate(diags.items()): 
        arg_list = ['SourcePosition range']
        type_cnt = 1
        pos_cnt = 1
        char_cnt = 1
        for format_spec in v[1]:
            if format_spec.strip() == '%t':
                arg_list += ['Type *type' + str(type_cnt)]
                type_cnt += 1
            if format_spec.strip() == '%p':
                arg_list += ['SourcePosition pos' + str(pos_cnt)]
                pos_cnt += 1
            if format_spec.strip() == '%c':
                arg_list += ['uint8_t c' + str(char_cnt)]
                char_cnt += 1
        fun_def = 'void log_' + k.strip() + '(' + ','.join(arg_list) + ') {\nDiag ret;\nret.range = range;\n'
        type_cnt = 1
        pos_cnt = 1
        char_cnt = 1
        for format_spec in v[1]:
            if format_spec.strip() == '%t':
                fun_def += 'ret.data.' + k + '.type' + str(type_cnt) + ' = type' + str(type_cnt) + ';\n'

                type_cnt += 1
            if format_spec.strip() == '%p':
                fun_def += 'ret.data.' + k + '.pos' + str(pos_cnt) + ' = pos' + str(pos_cnt) + ';\n'
                pos_cnt += 1
            if format_spec.strip() == '%c':
                fun_def += 'ret.data.' + k + '.c' + str(char_cnt) + ' = c' + str(char_cnt) + ';\n'
                char_cnt += 1
        fun_def += 'errors_log(ret);\n}\n'
        source_f.write(fun_def)
