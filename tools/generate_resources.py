#!python

import os
import argparse

header_template = """\
#ifndef {guard}
#define {guard}

namespace {namespace} {{
    namespace detail {{
        // Constexpr variant of strcmp
        constexpr bool str_eq(const char* a, const char* b) {{
            while (*a && *b) {{
                if (*a != *b)
                    return false;
                ++a;
                ++b;
            }}

            return *a == *b;
        }}
    }}

    constexpr const char* open(const char* path) {{
{cases}
        {on_error}
    }}
}}

#endif
"""

case_template = """\
if (detail::str_eq(path, {resource})) {{
            return {symbol};
"""

parser = argparse.ArgumentParser(description = 'Generate asm file and header from resources')
parser.add_argument('-s', '--asm-output', metavar = '<assembly output>', help = 'Output generated assembly files')
parser.add_argument('-i', '--header-output', metavar = '<header output>', help = 'Output generated header files')
parser.add_argument('-f', '--file', dest = 'resources', metavar = ('<path>', '<alias>'), nargs = 2, action = 'append', help = 'Add a file to the resources')
args = parser.parse_args()

def mangle(name):
    return '_' + name.replace('/', '_').replace('.', '_')

def generate_header(f):
    cases = ""
    first = True
    for (path, alias) in args.resources:
        cases += "        "
        if first:
            first = False
        else:
            cases += "} else "
        cases += case_template.format(
            resource = alias,
            symbol = mangle(alias)
        )

    if len(args.resources) > 0:
        cases += "        }"

    header = header_template.format(
        guard = '_RESOURCES_H',
        namespace = 'resources',
        cases = cases,
        on_error = 'return nullptr;'
    )

    f.write(header.encode())

if args.asm_output is not None:
    with open(args.asm_output, 'wb') as f:
        f.write(b'#asm output')

if args.header_output is not None:
    with open(args.header_output, 'wb') as f:
        generate_header(f);
