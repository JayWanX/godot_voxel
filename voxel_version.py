
# voxel 工程的版本信息。
# 注意：不能命名为 "version.py"，否则 `import version` 会误导入 Godot 自带的同名模块。

MAJOR = 1
MINOR = 7
PATCH = 2
# dev、release
STATUS = "release"

import os


def generate_version_header(is_module):
    # 生成 constants/version.gen.h 头文件。
    # 该头文件被 C++ 侧引用，用于在运行期暴露模块版本信息。
    git_hash = get_git_commit_hash()

    info = {
        "major": MAJOR,
        "minor": MINOR,
        "patch": PATCH,
        "status": STATUS,
        "edition": "Module" if is_module else "GDExtension",
        "git_hash": git_hash
    }

    f = open("constants/version.gen.h", "w")

    f.write(
        """/* THIS FILE IS GENERATED DO NOT EDIT */
#ifndef VOXEL_VERSION_GEN_H
#define VOXEL_VERSION_GEN_H

#define VOXEL_VERSION_MAJOR {major}
#define VOXEL_VERSION_MINOR {minor}
#define VOXEL_VERSION_PATCH {patch}
#define VOXEL_VERSION_STATUS "{status}"
#define VOXEL_VERSION_EDITION "{edition}"
#define VOXEL_VERSION_GIT_HASH "{git_hash}"

#endif // VOXEL_VERSION_GENERATED_GEN_H
""".format(**info))

    f.close()


def get_git_commit_hash():
    # 解析当前所在的 Git 仓库的提交哈希。
    # 若不在 Git 仓库中则返回空字符串，逻辑参考 Godot methods.py。

    githash = ""
    gitfolder = ".git"

    if os.path.isfile(".git"):
        # 仓库根目录以 .git 文件代替目录的情况，读取真实路径。
        module_folder = open(".git", "r").readline().strip()
        if module_folder.startswith("gitdir: "):
            gitfolder = module_folder[8:]

    head_path = os.path.join(gitfolder, "HEAD")

    if os.path.isfile(head_path):
        head = open(head_path, "r", encoding="utf8").readline().strip()
        if head.startswith("ref: "):
            ref = head[5:]
            # 若是 Git worktree 而非根克隆，需回退到 worktree 所在的主仓库目录。
            parts = gitfolder.split("/")
            if len(parts) > 2 and parts[-2] == "worktrees":
                gitfolder = "/".join(parts[0:-2])
            head = os.path.join(gitfolder, ref)
            packedrefs = os.path.join(gitfolder, "packed-refs")
            if os.path.isfile(head):
                githash = open(head, "r").readline().strip()
            elif os.path.isfile(packedrefs):
                # refs 可能被打包进单一文件，此处从 .git/packed-refs 中匹配当前 ref 的哈希。
                # https://mirrors.edge.kernel.org/pub/software/scm/git/docs/git-pack-refs.html
                for line in open(packedrefs, "r").read().splitlines():
                    if line.startswith("#"):
                        continue
                    (line_hash, line_ref) = line.split(" ")
                    if ref == line_ref:
                        githash = line_hash
                        break
        else:
            # HEAD 直接指向提交而不是引用，哈希即为其内容。
            githash = head

    return githash
