#!/usr/bin/env python3
"""跨平台：重新生成 compile_commands.json（供 VSCode/CLion 智能感知）到脚本所在目录。"""
import argparse
import shutil
import subprocess
import sys
from pathlib import Path

# 脚本位于模块的 misc 目录，据此推导相关根目录
SCRIPT_DIR = Path(__file__).resolve().parent
MODULE_ROOT = SCRIPT_DIR.parent


def default_engine_root() -> Path:
    """从模块位置推断引擎根（sources_stable 下第一个 godot-* 目录）。"""
    repo_root = MODULE_ROOT.parent.parent
    sources = repo_root / "sources_stable"
    matches = sorted(sources.glob("godot-*")) if sources.exists() else []
    return matches[0] if matches else sources


def invoke_scons(engine_root: Path, scons_args: list) -> None:
    """在引擎根以编译数据库模式运行 SCons（只生成不编译）。"""
    base = ["compiledb=yes", "compiledb_gen_only=yes", *scons_args]
    # 优先使用 PATH 上的 scons，其次退回 python -m SCons，兼顾不同安装方式
    cmd = ["scons", *base]
    if shutil.which("scons") is None:
        cmd = [sys.executable, "-m", "SCons", *base]
    subprocess.run(cmd, cwd=str(engine_root), check=True)


def main() -> None:
    parser = argparse.ArgumentParser(description="重新生成 compile_commands.json 到脚本所在目录")
    parser.add_argument("-e", "--engine-root", type=Path, default=default_engine_root(),
                        help="引擎根目录（默认自动探测）")
    parser.add_argument("scons_args", nargs="*",
                        default=["target=editor", "platform=windows", "arch=x86_64", "precision=double"],
                        help="额外 SCons 构建参数")
    args = parser.parse_args()

    if not args.engine_root.is_dir():
        sys.exit(f"引擎根目录不存在: {args.engine_root}")

    invoke_scons(args.engine_root, args.scons_args)

    dest = SCRIPT_DIR / "compile_commands.json"
    shutil.copy2(args.engine_root / "compile_commands.json", dest)
    print(f"已生成: {dest}")


if __name__ == "__main__":
    main()