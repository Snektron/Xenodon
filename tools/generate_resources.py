#!python

import os
import argparse

header_template = """\
#ifndef {guard}
#define {guard}

#include <string_view>

namespace {namespace} {{
    namespace externals {{
{externals}
    }}

    constexpr std::string_view open(const std::string_view& path) {{
{cases}
        {on_error}
    }}
}}

#endif
"""

case_template = """\
if (path == "{alias}") {{
{indent}    return std::string_view(externals::{symbol}, {size});
"""

extern_template = '{indent}extern "C" const char {symbol}[];'

asm_template = """\
# {alias}
    .global {symbol}
    .align 8
{symbol}:
    .incbin "{file}"
"""

parser = argparse.ArgumentParser(description = 'Generate asm file and header from resources')
parser.add_argument('-s', '--asm-output', metavar = '<assembly output>', help = 'Output generated assembly files')
parser.add_argument('-i', '--header-output', metavar = '<header output>', help = 'Output generated header files')
parser.add_argument('-f', '--file', dest = 'resources', metavar = ('<path>', '<alias>'), nargs = 2, action = 'append', help = 'Add a file to the resources')
args = parser.parse_args()

def mangle(name):
    return '_' + name.replace('/', '_').replace('.', '_')

def generate_header(f):
    indent = "        "
    cases = ""
    externals = ""

    first = True
    for (path, alias) in args.resources:
        cases += indent
        symbol = mangle(alias)
        if first:
            first = False
        else:
            cases += '} else '
            externals += '\n'
        cases += case_template.format(
            alias = alias,
            indent = indent,
            symbol = symbol,
            size = os.path.getsize(path)
        )

        externals += extern_template.format(
            indent = indent,
            symbol = symbol
        )

    if len(args.resources) > 0:
        cases += indent + '}'

    header = header_template.format(
        guard = '_RESOURCES_H',
        namespace = 'resources',
        externals = externals,
        cases = cases,
        on_error = 'return std::string_view();'
    )

    f.write(header.encode())

def generate_asm(f):
    asm = '\n'.join([asm_template.format(alias = alias, symbol = mangle(alias), file = path) for (path, alias) in args.resources])
    f.write(asm.encode())

if args.resources is None:
    args.resources = []

if args.asm_output is not None:
    with open(args.asm_output, 'wb') as f:
        generate_asm(f)

if args.header_output is not None:
    with open(args.header_output, 'wb') as f:
        generate_header(f);
