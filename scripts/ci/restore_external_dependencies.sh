#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$repo_root"

ensure_repo() {
  local path="$1"
  local url="$2"
  local branch="${3:-}"

  if [[ ! -d "$path" ]] || [[ -z "$(find "$path" -mindepth 1 -maxdepth 1 -print -quit 2>/dev/null)" ]]; then
    echo "Restoring missing dependency: $path"
    rm -rf "$path"
    mkdir -p "$(dirname "$path")"

    if [[ -n "$branch" ]]; then
      git clone --depth=1 --branch "$branch" "$url" "$path"
    else
      git clone --depth=1 "$url" "$path"
    fi
  fi

  if [[ -d "$path/.git" ]] && [[ -f "$path/.gitmodules" ]]; then
    echo "Syncing nested submodules for: $path"
    git -C "$path" submodule update --init --recursive --depth 1
  fi
}

if [[ ! -f .gitmodules ]]; then
  echo "No .gitmodules found, nothing to restore."
else
  git config -f .gitmodules --get-regexp '^submodule\..*\.path$' | while read -r key path; do
    url_key="${key%.path}.url"
    branch_key="${key%.path}.branch"
    url="$(git config -f .gitmodules --get "$url_key")"
    branch="$(git config -f .gitmodules --get "$branch_key" || true)"

    ensure_repo "$path" "$url" "$branch"
  done
fi

ensure_repo "Natives/external/UnzipKit" "https://github.com/abbeycode/UnzipKit.git"

"$repo_root/scripts/ci/apply_external_patches.sh"
