#!/usr/bin/python -u

LUPDATE  = "@_qt5_linguisttools_install_prefix@/bin/lupdate"
LRELEASE = "@_qt5_linguisttools_install_prefix@/bin/lrelease"
BASEPATH = "@PROJECT_SOURCE_DIR@"
LANGS    = ["fr", "es", "de", "tl", "it", "tr", "ja", "zh"]

import os
import subprocess
import lxml

PROJECTS = {
    "asebaplayground" : {
        "dirs"    : ["aseba/targets/playground"],
        "trdir"   : "aseba/targets/playground",
        "qrcfile" : "aseba/targets/playground/asebaplayground.qrc"
    },
    "thymiownetconfig" : {
        "dirs"   : ["aseba/clients/thymiownetconfig"],
        "trdir"  : "aseba/clients/thymiownetconfig",
        "qrcfile" : "aseba/clients/thymiownetconfig/thymiownetconfig.qrc"
    },
    "thymioupgrader" : {
        "dirs"    : ["aseba/clients/thymioupgrader"],
        "trdir"   : "aseba/clients/thymioupgrader",
        "qrcfile" : "aseba/clients/thymioupgrader/thymioupgrader.qrc"
    },
    "asebastudio" : {
        "dirs"    : ["aseba/clients/qtcommon", "aseba/clients/studio", "aseba/clients/vpl"],
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

def update(qrcfile, prefix, files):
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

if __name__ == "__main__":
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
            update(os.path.join(BASEPATH, settings['qrcfile']), settings['qrcprefix'] if 'qrcprefix' in settings else "", files)