#!/usr/bin/env python3
# coding: utf-8
"""更新 Mkdocs 配置中的生成类列表。

从 <doc>/source/api 下生成的类 Markdown 文件，刷新 <doc>/mkdocs.yml 中
<generated_class_list>…</generated_class_list> 标记区间的导航条目。

替代原 build.py 的 -m 分支（XML→Markdown 已改由共享工具 godot_docs_gen/build.py 完成）。
路径相对本脚本所在目录（doc/tools/）推导：上一级即 doc，源码目录为 doc/source/api。
"""

import os
import sys
from pathlib import Path

SOURCES = "source"


def update_mkdocs_file(mkdocs_config_fpath, md_classes_dir):
    absolute_paths = md_classes_dir.glob("*.md")
    class_files = []
    docs_folder = mkdocs_config_fpath.parents[0] / SOURCES
    for absolute_path in absolute_paths:
        class_files.append(str(absolute_path.relative_to(docs_folder)).replace("\\", "/"))
    class_files.sort()

    # 将特殊页放到最前
    for i in range(0, len(class_files)):
        fname = class_files[i]
        if "all_classes" in fname:
            class_files.pop(i)
            class_files.insert(0, fname)
            break

    with open(mkdocs_config_fpath, "r", encoding="utf-8") as f:
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
            indent = line[:line.find("#")]
            processed_lines.append(line)
            for class_file in class_files:
                processed_lines.append(indent + "- " + class_file + "\n")
        else:
            processed_lines.append(line)

    yml = "".join(processed_lines)
    with open(mkdocs_config_fpath, "w", encoding="utf-8") as f:
        f.write(yml)


def main():
    my_path = Path(os.path.realpath(__file__))
    doc_dir = my_path.parents[1]
    md_classes_dir = doc_dir / SOURCES / "api"
    mkdocs_config_path = doc_dir / "mkdocs.yml"
    update_mkdocs_file(mkdocs_config_path, md_classes_dir)


if __name__ == "__main__":
    sys.exit(main())