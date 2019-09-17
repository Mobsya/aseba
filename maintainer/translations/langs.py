#!/usr/bin/python -u

import os
import subprocess
import lxml
import sys
from string import Template
import re
import subprocess


LUPDATE  = "@_qt5_linguisttools_install_prefix@/bin/lupdate"
LRELEASE = "@_qt5_linguisttools_install_prefix@/bin/lrelease"
BASEPATH = "@PROJECT_SOURCE_DIR@"
LANGS    = ["fr", "es", "de", "el", "it", "pl", "tr", "ja", "zh"]


# files used when updating translations for the compiler
# errors_cpp -> errors_code_h -> compiler_ts_cpp

COMPILER_BASEPATH = os.path.join(BASEPATH, "aseba", "compiler")
errors_cpp = os.path.join(COMPILER_BASEPATH, 'errors.cpp')
errors_code_h = os.path.join(COMPILER_BASEPATH, 'errors_code.h')
compiler_ts_cpp = os.path.join(BASEPATH, "aseba/clients/qtcommon/translations/", 'CompilerTranslator.cpp')


PROJECTS = {
    "asebaplayground" : {
        "dirs"    : ["aseba/targets/playground"],
        "trdir"   : "aseba/targets/playground",
        "qrcfile" : "aseba/targets/playground/asebaplayground.qrc"
    },
    "asebastudio" : {
        "dirs"    : ["aseba/clients/qtcommon/common", "aseba/clients/studio", "aseba/clients/vpl"],
        "trdir"   : "aseba/clients/qtcommon/translations",
        "qrcfile" : "aseba/clients/qtcommon/asebastudio.qrc",
        "qrcprefix": "translations/"
    },
    "compiler" : {
        "dirs"     : ["aseba/clients/qtcommon/translations"],
        "trdir"    : "aseba/clients/qtcommon/translations",
        "qrcfile"  : "aseba/clients/qtcommon/asebastudio.qrc",
        "qrcprefix": "translations/"
    },
    "qtabout" : {
        "dirs"    : ["aseba/common/about"],
        "trdir"   : "aseba/common/about",
        "qrcfile" : "aseba/common/about/asebaqtabout.qrc"
    },
    "launcher" : {
        "dirs"  : ["aseba/launcher"],
        "trdir" : "aseba/launcher/src/translations",
        "qrcfile" : "aseba/launcher/src/translations.qrc",
        "qrcprefix": "translations/"
    }
}

def update_qrc(qrcfile, prefix, files):
    from lxml import etree
    parser = etree.XMLParser(remove_blank_text=True)
    if not os.path.isfile(qrcfile):
        r = etree.fromstring("""<!DOCTYPE RCC><RCC version="1.0"><qresource></qresource></RCC>""", parser)

    else:
        r = etree.parse(qrcfile, parser).getroot()

    qresource = r.find("qresource")

    existing = set(c.text for c in qresource)
    files = [prefix + os.path.basename(f) for f in files]
    for f in filter(lambda f: f not in existing, files):
        n = etree.SubElement(qresource, 'file')
        n.text = f

    with open(qrcfile, "w") as file:
        file.write(etree.tostring(r, pretty_print=True, encoding="utf-8").decode())


def update_compiler_trads():
    comment_regexp = re.compile(r'\A\s*// (.*)')
    error_regexp = re.compile(r'error_map\[(.*?)\]\s*=\s*L"(.*?)";')

    output_code_file = """
    #ifndef __ERRORS_H
    #define __ERRORS_H

    /*
    This file was automatically generated. Do not modify it,
    or your changes will be lost!
    You can manually run the script 'sync_translation.py'
    to generate a new version, based on errors.cpp
    */

    // define error codes
    namespace Aseba
    {{
        enum ErrorCode
        {{
            // {}
            {} = 0,
            {}
            ERROR_END
        }};
    }};
    #endif // __ERRORS_H
    """
    output_qt_file = """
    #include "CompilerTranslator.h"
    #include "compiler/errors_code.h"

    namespace Aseba
    {{
        CompilerTranslator::CompilerTranslator()
        {{

        }}

        const std::wstring CompilerTranslator::translate(ErrorCode error)
        {{
            QString msg;

            switch (error)
            {{
                {}
                default:
                    msg = tr("Unknown error");
            }}

            return msg.toStdWString();
        }}
    }};
    """

    output_qt_element = """
            case {}:
                msg = tr("{}");
                break;"""


    # process input file
    case_vector = ""
    count = 0
    id_0 = None
    start_comment = ""
    id_others = ""

    print("Reading " + errors_cpp)
    with open(errors_cpp, 'r') as fh:
        print("Processing...")
        for line in fh.readlines():
            # a comment?
            match = comment_regexp.search(line)
            if match:
                # add it to the error codes file
                if not id_0:
                    start_comment = match.group(1)
                else:
                    id_others += "		// " + match.group(1) + "\n"
                continue

            match = error_regexp.search(line)
            if match:
                count = count + 1
                code = match.group(1)
                msg = match.group(2)
                # error codes file
                if not id_0:
                    id_0 = code
                else:
                    id_others += "		" + code + ",\n"
                case_vector += output_qt_element.format(code, msg)
    print( "Found " + str(count) + " + string(s)")

    # writing errors_code.h
    result = output_code_file.format(start_comment, id_0, id_others)
    print("Writing to " + errors_code_h)
    print(result)
    with open(errors_code_h, 'w') as fh:
        fh.write(result)

    print("Writing to " + compiler_ts_cpp)
    with open(compiler_ts_cpp, 'w') as fh:
        result = output_qt_file.format(case_vector)
        fh.write(result)

if __name__ == "__main__":
    print("Update compiler trads")
    update_compiler_trads()
    for project, settings in PROJECTS.items():
        print("Updating translations of {}:".format(project))
        files = []
        for lang in LANGS:
            out_dir = os.path.join(BASEPATH, settings['trdir'])
            os.makedirs(out_dir, exist_ok=True)
            basename = os.path.join(out_dir, "{}_{}".format(project, lang))
            files.append(basename + ".qm")
            args = [LUPDATE, '-source-language', 'en', '-target-language', lang]
            args = args + [os.path.join(BASEPATH, d) for d in settings["dirs"] ]
            args = args + ['-ts', basename + ".ts"]
            subprocess.run(args)
        if 'qrcfile' in settings:
            update_qrc(os.path.join(BASEPATH, settings['qrcfile']), settings['qrcprefix'] if 'qrcprefix' in settings else "", files)

