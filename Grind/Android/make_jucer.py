#!/usr/bin/env python3
"""Writes MidnightGrind.jucer for the Android build (JUCE's CMake can't target Android).

    python3 make_jucer.py <path-to-JUCE-relative-to-this-folder>     (default: JUCE)
    Projucer --resave MidnightGrind.jucer
    cd Builds/Android && ./gradlew assembleDebug

The Debug configuration is built with full optimisation, so the debug-signed APK
it produces is installable straight away and fast enough for real-time audio.
"""

import hashlib
import os
import sys
from xml.sax.saxutils import quoteattr

HERE = os.path.dirname(os.path.abspath(__file__))
GRIND = os.path.dirname(HERE)
JUCE = sys.argv[1] if len(sys.argv) > 1 else "JUCE"

MODULES = ["juce_audio_basics", "juce_audio_devices", "juce_audio_formats", "juce_audio_plugin_client",
           "juce_audio_processors", "juce_audio_utils", "juce_core", "juce_data_structures", "juce_events",
           "juce_graphics", "juce_gui_basics", "juce_gui_extra"]


def uid(text):
    return hashlib.md5(text.encode()).hexdigest()[:6]


def file_entry(rel_from_grind):
    path = "../" + rel_from_grind.replace(os.sep, "/")
    name = os.path.basename(rel_from_grind)
    compile_flag = "1" if name.endswith((".cpp", ".cc")) else "0"
    return '      <FILE id="%s" name=%s compile="%s" resource="0" file=%s/>' % (
        uid(path), quoteattr(name), compile_flag, quoteattr(path))


def collect(folder, exts):
    out = []
    for root, _, files in os.walk(os.path.join(GRIND, folder)):
        for f in sorted(files):
            if f.endswith(exts):
                out.append(os.path.relpath(os.path.join(root, f), GRIND))
    return sorted(out)


source_files = collect("Source", (".cpp", ".h"))
mutable_files = collect("ThirdParty/mutable", (".cc", ".h"))

lines = []
lines.append('<?xml version="1.0" encoding="UTF-8"?>')
lines.append('<JUCERPROJECT id="MdGrnd" name="MidnightGrind" projectType="audioplug" useAppConfig="0"')
lines.append('              addUsingNamespaceToJuceHeader="0" jucerFormatVersion="1" version="1.0.0"')
lines.append('              companyName="Midnight Audio" bundleIdentifier="com.midnightaudio.midnightgrind"')
lines.append('              pluginFormats="buildStandalone" pluginCharacteristicsValue="pluginIsSynth,pluginWantsMidiIn"')
lines.append('              pluginName="Midnight Grind" pluginDesc="Midnight Grind" pluginManufacturer="Midnight Audio"')
lines.append('              pluginManufacturerCode="Mdnt" pluginCode="MdGr" displaySplashScreen="0"')
lines.append('              cppLanguageStandard="20" defines="TEST=1">')
lines.append('  <MAINGROUP id="%s" name="MidnightGrind">' % uid("main"))
lines.append('    <GROUP id="%s" name="Source">' % uid("Source"))
lines += [file_entry(f) for f in source_files]
lines.append('    </GROUP>')
lines.append('    <GROUP id="%s" name="Mutable">' % uid("Mutable"))
lines += [file_entry(f) for f in mutable_files]
lines.append('    </GROUP>')
lines.append('  </MAINGROUP>')

lines.append('  <MODULES>')
for m in MODULES:
    lines.append('    <MODULE id="%s" showAllCode="1" useLocalCopy="0" useGlobalPath="0"/>' % m)
lines.append('  </MODULES>')

# Header search paths are relative to each exporter's target folder (Builds/<name>)
header_paths = "../../../ThirdParty/mutable;../../../Source"


def module_paths(indent):
    return "\n".join('%s<MODULEPATH id="%s" path=%s/>' % (indent, m, quoteattr(JUCE + "/modules")) for m in MODULES)


lines.append('  <EXPORTFORMATS>')
lines.append('    <ANDROIDSTUDIO targetFolder="Builds/Android" androidMinimumSDK="26" androidTargetSDK="34"')
lines.append('                   androidScreenOrientation="landscape" androidExtraAssetsFolder=""')
lines.append('                   androidAllowBackup="0" app_name="Midnight Grind" androidBluetoothNeeded="0"')
lines.append('                   androidInternetNeeded="0" microphonePermissionNeeded="0">')
lines.append('      <CONFIGURATIONS>')
lines.append('        <CONFIGURATION isDebug="1" name="Debug" optimisation="3" headerPath=%s/>' % quoteattr(header_paths))
lines.append('        <CONFIGURATION isDebug="0" name="Release" optimisation="3" headerPath=%s/>' % quoteattr(header_paths))
lines.append('      </CONFIGURATIONS>')
lines.append('      <MODULEPATHS>')
lines.append(module_paths('        '))
lines.append('      </MODULEPATHS>')
lines.append('    </ANDROIDSTUDIO>')
lines.append('    <LINUX_MAKE targetFolder="Builds/LinuxMakefile">')
lines.append('      <CONFIGURATIONS>')
lines.append('        <CONFIGURATION isDebug="0" name="Release" optimisation="3" headerPath=%s/>' % quoteattr(header_paths))
lines.append('      </CONFIGURATIONS>')
lines.append('      <MODULEPATHS>')
lines.append(module_paths('        '))
lines.append('      </MODULEPATHS>')
lines.append('    </LINUX_MAKE>')
lines.append('  </EXPORTFORMATS>')
lines.append('  <JUCEOPTIONS JUCE_WEB_BROWSER="0" JUCE_USE_CURL="0" JUCE_VST3_CAN_REPLACE_VST2="0"'
             ' JUCE_USE_ANDROID_OBOE="1"/>')
lines.append('</JUCERPROJECT>')

with open(os.path.join(HERE, "MidnightGrind.jucer"), "w") as f:
    f.write("\n".join(lines) + "\n")
print("wrote MidnightGrind.jucer (%d files)" % (len(source_files) + len(mutable_files)))
