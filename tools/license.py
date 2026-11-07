import subprocess
import re
import sys
from pathlib import Path
from typing import List, Optional
from urllib.parse import urlparse

PROJECT_ROOT = Path(__file__).resolve().parents[1]
LIB_DIR = PROJECT_ROOT / "lib"
OUTPUT_FILE = PROJECT_ROOT / "third_party/THIRD_PARTY_LICENSES.md"
LICENSE_FILE_CANDIDATES: List[str] = [
    "LICENSE",
    "LICENSE.txt",
    "LICENSE.md",
    "COPYING",
    "COPYING.txt",
]
SEPARATOR = "-" * 32


def find_license_file(lib_path: Path) -> Optional[Path]:
    for candidate in LICENSE_FILE_CANDIDATES:
        candidate_path = lib_path / candidate
        if candidate_path.is_file():
            return candidate_path
    return None


def get_git_remote_url(lib_path: Path) -> Optional[str]:
    try:
        result = subprocess.run(
            [
                "git",
                "-C",
                str(lib_path),
                "config",
                "--get",
                "remote.origin.url",
            ],
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            text=True,
            check=False,
        )
        url = result.stdout.strip()
        return url if url else None
    except Exception:
        return None


def normalize_to_https(url: Optional[str]) -> Optional[str]:
    if url is None:
        return None

    url = url.strip()

    # すでに https
    if url.startswith("https://"):
        return url

    # ssh://git@github.com/user/repo.git
    if url.startswith("ssh://"):
        return re.sub(r"^ssh://git@", "https://", url)

    # git@github.com:user/repo.git
    scp_like = re.match(r"git@([^:]+):(.+)", url)
    if scp_like:
        host = scp_like.group(1)
        path = scp_like.group(2)
        return f"https://{host}/{path}"

    # その他(未知形式)はそのまま返す
    return url


def extract_repo_name(remote_url: Optional[str]) -> Optional[str]:
    if remote_url is None:
        return None

    try:
        parsed = urlparse(remote_url)

        # パスを分解（例: /owner/repo）
        path_parts = [p for p in parsed.path.split("/") if p]

        if not path_parts:
            return None

        # 最後の要素がリポジトリ名
        return path_parts[-1]

    except Exception:
        return None


def collect_licenses() -> None:
    if not LIB_DIR.exists():
        raise FileNotFoundError(f"{LIB_DIR} does not exist")

    OUTPUT_FILE.parent.mkdir(parents=True, exist_ok=True)

    sections: List[str] = []
    toc_entries: List[str] = []

    for lib_path in sorted(LIB_DIR.iterdir()):
        if not lib_path.is_dir():
            continue

        license_file = find_license_file(lib_path)
        if license_file is None:
            print(f"[ERROR] License not found: {lib_path.name}")
            sys.exit(1)

        # URL取得と正規化
        remote_url = get_git_remote_url(lib_path)
        remote_url = normalize_to_https(remote_url)
        remote_url = (
            remote_url[:-4] if remote_url.endswith(".git") else remote_url
        )

        lib_name = extract_repo_name(remote_url) or lib_path.name

        # Markdownアンカー用
        anchor = lib_name.lower().replace(" ", "-")

        # --- 目次 ---
        toc_entries.append(f"- [{lib_name}](#{anchor})")

        # --- セクション構築 ---
        section_lines: List[str] = []

        section_lines.append(SEPARATOR)
        section_lines.append("")
        section_lines.append(f"## {lib_name}")
        section_lines.append("")

        if remote_url:
            section_lines.append(remote_url)
        else:
            section_lines.append("(URL not available)")

        section_lines.append("")

        # ライセンス本文
        try:
            with license_file.open(
                "r", encoding="utf-8", errors="ignore"
            ) as lf:
                section_lines.append("```")
                section_lines.append(lf.read())
                section_lines.append("```")
        except Exception as e:
            print(f"[ERROR] Failed to read {license_file}: {e}")
            sys.exit(1)

        section_lines.append("")
        section_lines.append("")

        sections.append("\n".join(section_lines))

    with OUTPUT_FILE.open("w", encoding="utf-8") as out:
        out.write("# THIRD PARTY LICENSES\n\n")
        out.write(
            "This file lists third-party libraries used in this project and their licenses.\n"
            "All licenses are reproduced as required by their respective terms.",
        )

        # 目次
        out.write("## Table of Contents\n\n")
        out.write("\n".join(toc_entries))
        out.write("\n\n")

        # 本文
        out.write("\n".join(sections))

    print(f"Generated: {OUTPUT_FILE}")


if __name__ == "__main__":
    collect_licenses()
