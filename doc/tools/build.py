#!/usr/bin/python3
# coding: utf-8

# Builds Voxel Tools API docs
#
# Requires Python 3.4+
# Run with --help for usage
#
# Configure these variables for your system or specify on the command line

import sys

if sys.version_info < (3, 4):
    print("Please upgrade python to version 3.4 or higher.")
    print("Your version: %s\n" % sys.version_info)
    sys.exit(1)

import xml_to_markdown
import subprocess
import getopt
import glob
import os
import platform
import shutil
from pathlib import Path

SOURCES = "source"


def update_classes_xml(custom_godot_path, project_root, xml_path, verbose=False):
    """运行 Godot doctool 更新类 XML，仅保留本模块（Voxel*）的类文档。

    doctool 会把引擎全部模块的类文档按 modules/<m>/doc_classes 写到 --doctool
    目标目录下，故用项目内临时目录承接输出，跑完仅把本模块的类 XML 合并回
    classes_dir，其余全部丢弃。避免在项目根生成 modules/、platform/ 等残留目录，
    也保证模块内类 XML 的 schema 相对路径（../../../doc/class.xsd）可稳定改写。
    """
    godot_executable = custom_godot_path
    if godot_executable is None or godot_executable == "":
        bindir = project_root / 'bin'
        godot_executable = find_godot(bindir)
        if godot_executable is None:
            print("Godot executable not found")
            return

    if verbose:
        print("Found Godot at: %s" % godot_executable)

    # --doctool 必须作为独立参数（无前后空格），否则被当作位置参数而静默失败
    scratch = project_root / ".doctool_tmp"
    shutil.rmtree(scratch, ignore_errors=True)
    scratch.mkdir(parents=True)

    try:
        args = [str(godot_executable), "--doctool", str(scratch)]
        if verbose:
            print("Running: ", args)
        result = subprocess.run(args, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                universal_newlines=True)
        if verbose:
            print(result.stdout)
            print("Disregard Godot's errors about files unless they are about Voxel*.")

        # 从临时索引中仅保留本模块类 XML 并合并进 classes_dir
        index_dir = scratch / "doc" / "classes"
        if index_dir.is_dir():
            _prune_to_doc_classes(index_dir, project_root)
            xml_path.mkdir(parents=True, exist_ok=True)
            for xml_path_file in index_dir.glob("*.xml"):
                shutil.copy2(xml_path_file, xml_path / xml_path_file.name)
    finally:
        shutil.rmtree(scratch, ignore_errors=True)


def _prune_to_doc_classes(index_dir, project_root):
    """仅保留本模块的类 XML，删除 doctool 倾泻到临时目录的其它引擎类。[br]
    [param index_dir] doctool 类 XML 临时目录[br]
    [param project_root] 模块项目根（用于读取 config.py 的类清单）。
    """
    doc_classes = _get_doc_classes(project_root)
    if not doc_classes:
        return
    keep_set = set(doc_classes)
    for xml_file in index_dir.glob("*.xml"):
        if xml_file.stem not in keep_set:
            xml_file.unlink()


def _get_doc_classes(project_root):
    """从模块根 config.py 的 get_doc_classes() 静态解析本模块类清单（AST，不执行代码）。[br]
    [param project_root] 模块项目根[br]
    [return] 类名列表，未解析到返回空列表。
    """
    cfg_path = project_root / "config.py"
    if not cfg_path.is_file():
        return []
    import ast
    tree = ast.parse(cfg_path.read_text(encoding="utf-8"))
    for node in ast.walk(tree):
        if isinstance(node, ast.FunctionDef) and node.name == "get_doc_classes":
            for stmt in node.body:
                if isinstance(stmt, ast.Return) and isinstance(stmt.value, (ast.List, ast.Tuple)):
                    names = [e.value for e in stmt.value.elts
                             if isinstance(e, ast.Constant) and isinstance(e.value, str)]
                    if names:
                        return names
    return []


def rewrite_class_schema(xml_path):
    # doctool 每次都会把 schemaLocation 写成 ../../../doc/class.xsd，
    # 该相对路径面向“模块位于引擎源码 modules/ 下”的布局；
    # 本模块位于 custom_modules/voxel，上溯三级越界无法解析，故统一改写为项目内副本。
    for xml_file in xml_path.glob("*.xml"):
        text = xml_file.read_text(encoding="utf-8")
        fixed = text.replace(
            'xsi:noNamespaceSchemaLocation="../../../doc/class.xsd"',
            'xsi:noNamespaceSchemaLocation="../class.xsd"')
        if fixed != text:
            xml_file.write_text(fixed, encoding="utf-8")


def find_godot(bindir): # bindir: Path
    # Match a filename like these
    # godot.windows.editor.dev.x86_64.exe
    # godot.linuxbsd.editor.dev.x86_64
    #regex = r"godot\.(windows|macos|linuxbsd)\.editor(\.dev)?\.(x86_32|x86_64|arm64|rv64)(\.exe)?"
    prefix = "godot"
    os_prefix = ""
    suffix = ""
    if sys.platform == "win32" or sys.platform == "cygwin":
        os_prefix = ".windows"
        suffix = ".exe"
    elif sys.platform == "darwin":
        os_prefix = ".macos"
    else:
        os_prefix = ".linuxbsd"

    arch = ".x86_64"
    if platform.machine().lower() == "arm64":
        arch = ".arm64"
    if platform.machine().lower() == "riscv64":
        arch = ".rv64"

    # Names to try
    names = [
        prefix + os_prefix + ".editor.dev" + arch + suffix,
        prefix + os_prefix + ".editor" + arch + suffix,
        prefix + os_prefix + ".editor.dev.double" + arch + suffix,
    ]

    # Of all that we find, pick the most recent

    binaries = []

    for name in names:
        path = bindir / name
        if path.is_file():
            mtime = os.path.getmtime(path)
            binaries.append((path, mtime))
    
    if len(binaries) == 0:
        print("Error: Godot binary not specified and none suitable found in %s" % bindir)
        return None
    
    binaries = sorted(binaries, key=lambda tup: tup[1])
    most_recent = binaries[-1][0]
    return most_recent


def update_mkdocs_file(mkdocs_config_fpath, md_classes_dir):
    absolute_paths = md_classes_dir.glob("*.md")
    class_files = []
    docs_folder = mkdocs_config_fpath.parents[0] / SOURCES
    for absolute_path in absolute_paths:
        class_files.append(str(absolute_path.relative_to(docs_folder)).replace('\\', '/'))
    class_files.sort()

    # Put special item at the beginning
    for i in range(0, len(class_files)):
        fname = class_files[i]
        if "all_classes" in fname:
            class_files.pop(i)
            class_files.insert(0, fname)
            break

    with open(mkdocs_config_fpath, 'r', encoding='utf-8') as f:
        lines = f.readlines()
    
    processed_lines = []
    in_generated_section = False

    for line in lines:
        if in_generated_section:
            if "</generated_class_list>" in line:
                in_generated_section = False
            else:
                continue
        if "<generated_class_list>" in line:
            in_generated_section = True
            indent = line[:line.find('#')]
            processed_lines.append(line)
            for class_file in class_files:
                processed_lines.append(indent + "- " + class_file + "\n")
        else:
            processed_lines.append(line)

    yml = "".join(processed_lines)
    with open(mkdocs_config_fpath, 'w', encoding='utf-8') as f:
        f.write(yml)


def print_usage():
    print("\nUsage: ", sys.argv[0],
          "[-d] [-a] [-m] [-h] [-v] [-g path_to_godot]")
    print()
    print("\t-d -> Execute Godot doctool to update XML class data")
    print("\t-a -> Update Markdown API files from XML class data")
    print("\t-m -> Update mkdocs config file with generated content such as API files")
    print("\t-h, --help -> Prints help")
    print("\t-v -> Verbose. Print more details when running.")
    print("\t-g -> Specify custom path to Godot. Otherwise will use compiled executable under the repo's bin directory.")
    print()


###########################
# Main()

def main():
    # Default paths are determined from the location of this script
    my_path = Path(os.path.realpath(__file__))

    # Default parameters
    verbose = False
    must_run_doctool = False
    must_update_md_from_xml = False
    must_update_mkdocs_config = False
    godot_executable = ""

    godot_repo_root = my_path.parents[4]
    voxel_root = my_path.parents[2]
    md_path = my_path.parents[1] / SOURCES / 'api'
    xml_path = my_path.parents[1] / 'classes'
    mkdocs_config_path = my_path.parents[1] / 'mkdocs.yml'

    # Parse command line arguments
    try:
        opts, args = getopt.getopt(sys.argv[1:], "damhvg:", "help")
    except getopt.error as msg:
        print("Error: ", msg)
        print_usage()
        return

    did_something = False

    for opt, arg in opts:
        if opt == '-d':
            must_run_doctool = True
        if opt == '-a':
            must_update_md_from_xml = True
        if opt == '-m':
            must_update_mkdocs_config = True
        if opt in ('-h', '--help'):
            print_usage()
            did_something = True
        if opt == '-v':
            verbose = True
        if opt == '-g':
            godot_executable = arg

    if must_run_doctool:
        update_classes_xml(godot_executable, voxel_root, xml_path, verbose)
        rewrite_class_schema(xml_path)
        did_something = True
    
    if must_update_mkdocs_config:
        update_mkdocs_file(mkdocs_config_path, md_path)
        did_something = True

    if must_update_md_from_xml:
        xml_to_markdown.process_xml_folder(xml_path, md_path, verbose)
        did_something = True

    if not did_something:
        print("No operation specified.")
        print_usage()


# If called from command line
if __name__ == "__main__":
    main()
